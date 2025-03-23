#include "search.hpp"
// Assumed to be white's turn, but they can't move, so black wins
bool blackCheckmate(const BoardState& board, const std::vector<uint16_t>& legalMoves) {
    return legalMoves.empty() && is_in_check(board);  // False -> Black
}

// Assumed to be black's turn, but they can't move, so white wins
bool whiteCheckmate(const BoardState& board, const std::vector<uint16_t>& legalMoves) {
    return legalMoves.empty() && is_in_check(board);  // True -> White
}

/**
 * Determines if the position has insufficient material to checkmate.
 * A position is considered insufficient if checkmate is impossible for either side.
 * This includes:
 * - King vs King
 * - King + minor piece (bishop/knight) vs King
 * - King + Bishop vs King + Bishop when both bishops are on the same color squares
 *
 * @param board The current board state.
 * @return True if the position is a draw due to insufficient material, false otherwise.
 */
bool insufficientMaterial(const BoardState& board) {
    uint64_t whitePieces = board.getOccupancy(true);
    uint64_t blackPieces = board.getOccupancy(false);

    // Remove kings from occupancy
    uint64_t nonKingWhite = whitePieces & ~board.getBitboard(WHITE_KINGS);
    uint64_t nonKingBlack = blackPieces & ~board.getBitboard(BLACK_KINGS);

    // Check for King vs King
    if (nonKingWhite == 0 && nonKingBlack == 0) {
        return true;
    }

    // Check for King + Minor vs King (no other pieces)
    uint64_t allPieces = nonKingWhite | nonKingBlack;
    uint64_t minors = board.getBitboard(WHITE_BISHOPS) | board.getBitboard(WHITE_KNIGHTS) |
                      board.getBitboard(BLACK_BISHOPS) | board.getBitboard(BLACK_KNIGHTS);
    uint64_t others = allPieces & ~minors;  // Exclude minors to check for pawns, rooks, or queens

    if (others == 0 && __builtin_popcountll(allPieces) == 1) {
        return true;
    }

    // Check for King + Bishop vs King + Bishop on same color squares
    uint64_t whiteBishops = board.getBitboard(WHITE_BISHOPS);
    uint64_t blackBishops = board.getBitboard(BLACK_BISHOPS);

    // Check for King + Bishop vs King + Bishop on same color squares
    if (nonKingWhite == whiteBishops && nonKingBlack == blackBishops) {
        // Determine the color of squares each bishop occupies
        uint64_t lightSquareMask = 0x55AA55AA55AA55AA;  // Bitmask for light squares
        uint64_t darkSquareMask = ~lightSquareMask;     // Bitmask for dark squares

        bool whiteAllLight = !(whiteBishops & darkSquareMask);
        bool whiteAllDark = !(whiteBishops & lightSquareMask);
        bool blackAllLight = !(blackBishops & darkSquareMask);
        bool blackAllDark = !(blackBishops & lightSquareMask);

        if ((whiteAllLight && blackAllLight) || (whiteAllDark && blackAllDark)) {
            return true;
        }
    }

    // Insufficient material check failed
    return false;
}

/**
 * Checks whether the fifty-move rule applies.
 * The game is drawn if no pawn moves or captures have occurred in the last 50 full moves (100
 * half-moves).
 *
 * @param board The current board state.
 * @return True if the fifty-move rule applies, false otherwise.
 */
bool fiftyMoveRule(const BoardState& board) { 
    return board.getHalfmoveClock() >= 100;
}

/**
 * Determines if the current position is a stalemate.
 * A position is a stalemate if the player to move has no legal moves and is not in check.
 *
 * @param board The current board state.
 * @param legalMoves A list of legal moves for the current position.
 * @return True if the position is a stalemate, false otherwise.
 */
bool stalemate(const BoardState& board, const std::vector<uint16_t>& legalMoves) {
    return legalMoves.empty() && !is_in_check(board);
}

/**
 * Determines if the current position has occurred three times, leading to a draw by threefold
 * repetition. This function checks the transposition table for the number of times the current
 * position's Zobrist hash has been encountered.
 *
 * @param table The transposition table storing previous positions.
 * @param current_pos_hash The Zobrist hash of the current position.
 * @return True if the position has occurred at least three times, false otherwise.
 */
bool threefold(const TranspositionTable& table, uint64_t current_pos_hash) {
    auto it = table.find(current_pos_hash);
    if (it != table.end()) {
        // If the hash exists in the table, check its visitCount
        return it->second.visitCount >= 3;
    }
    return false;  // If the hash is not found, it's not a repetition
}

/**
 * Determines the game result based on the current board state.
 * This function checks for checkmate, stalemate, the fifty-move rule,
 * insufficient material, and threefold repetition using the transposition table.
 *
 * @param board The current board state.
 * @param legalMoves A list of legal moves available in the current position.
 * @param table The transposition table, used for threefold repetition checks.
 * @return The game result: WHITE_WINS, BLACK_WINS, DRAW (various types), or ONGOING.
 */
GameResult gameOver(const BoardState& board, const std::vector<uint16_t>& legalMoves, TranspositionTable& table) {
    if(board.getTurn()){ //White's turn and no legal moves
        if (blackCheckmate(board, legalMoves)) return WHITE_WINS;
    }
    else{ //Black's turn and no legal moves
        if (whiteCheckmate(board, legalMoves)) return BLACK_WINS;
    }
    if (stalemate(board, legalMoves)) return DRAW_STALEMATE;
    if (fiftyMoveRule(board)) return DRAW_50_MOVE_RULE;
    if (insufficientMaterial(board)) return DRAW_INSUFFICIENT_MATERIAL;
    if (threefold(table, board.getZobristHash())) return DRAW_THREEFOLD_REPETITION;
    return ONGOING;
}

/**
 * @brief Performs Static Exchange Evaluation (SEE).
 *
 * SEE determines whether a capture sequence on a given square results in a material gain or loss.
 * It evaluates piece exchanges recursively, considering defenders and attackers.
 *
 * This function is useful for:
 * - Move ordering (prioritizing better captures)
 * - Quiescence Search (avoiding unnecessary pruning of good captures)
 *
 * @param board The current board state.
 * @param toSq The destination square where the exchange occurs.
 * @param target The piece type being captured.
 * @param frSq The square of the attacking piece.
 * @param aPiece The piece type initiating the capture.
 * @return The net material gain (positive for a favorable capture, negative for a loss).
 */
int see(const BoardState& board, int toSq, int target, int frSq, int aPiece);

/**
 * Orders moves based on a heuristic evaluation to improve move ordering for alpha-beta pruning.
 * Moves are scored based on various factors:
 * - Captures are evaluated using Static Exchange Evaluation (SEE).
 * - Checks, castling, and promotions are given priority.
 * - Pawns and kings are slightly deprioritized to encourage deeper searches first.
 *
 * @param board The current board state.
 * @param moves A vector of legal moves.
 * @return A vector of moves ordered from highest to lowest priority.
 */
std::vector<uint16_t> orderMoves(BoardState& board, const std::vector<uint16_t>& moves) {
    std::vector<std::pair<int, uint16_t>> scoredMoves;

    for (const auto& move : moves) {
        int score = 0;
        int fromSquare, toSquare, special;
        decodeMove(move, fromSquare, toSquare, special);

        uint64_t fromSquareMask = (1ULL << fromSquare);
        uint64_t toSquareMask = (1ULL << toSquare);
        
        int fromPieceType = findPieceType(board, fromSquareMask, board.getTurn());
        uint64_t threatMask = generateThreatMask(fromPieceType, toSquare, board.getAllOccupancy());
        int enemyKingIndex = board.getTurn() ? BLACK_KINGS : WHITE_KINGS;

        
        bool isCheck = threatMask & board.getBitboard(enemyKingIndex);
        bool isCapture = toSquareMask & board.getOccupancy(!board.getTurn());
        
        // Compare the value of the capturing piece with the captured piece
        if (isCapture){
            // Do a static exchange evalaution on the capture.
            int capturedPieceType = findPieceType(board, toSquareMask, !board.getTurn());
            score = see(board, toSquare, capturedPieceType, fromSquare, fromPieceType);
        }

        if (isCheck) score += 5;
        if (special == PROMOTION_QUEEN) score += 9;
        else if (special == PROMOTION_KNIGHT) score += 2;
        else if (special == PROMOTION_ROOK) score -= 4;
        else if (special == PROMOTION_BISHOP) score -= 3;
        else if (special == CASTLING_KINGSIDE || special == CASTLING_QUEENSIDE) score += 4;

        if(fromPieceType == WHITE_PAWNS || fromPieceType == BLACK_PAWNS) score -= 1;
        else if(fromPieceType == WHITE_KINGS || fromPieceType == BLACK_KINGS) score -= 1;

        scoredMoves.emplace_back(score, move);
    }

    // Sort moves by descending score
    std::sort(scoredMoves.begin(), scoredMoves.end(), std::greater<>());

    // Extract sorted moves
    std::vector<uint16_t> orderedMoves;
    for (const auto& scoredMove : scoredMoves) {
        orderedMoves.push_back(scoredMove.second);
    }

    return orderedMoves;
}

/**
 * @brief Determines which pieces attack a given square.
 *
 * This function checks for all possible attacks on the given square,
 * considering all piece types (kings, knights, pawns, bishops, rooks, and queens).
 *
 * @param board The current board state.
 * @param occupancy The bitboard representing occupied squares.
 * @param toSquare The target square to check for attacks.
 * @return A bitboard representing all pieces that attack the given square.
 */
uint64_t attacksTo(const BoardState& board, const uint64_t occupancy, int toSquare) {
    uint64_t kings   = board.getBitboard(WHITE_KINGS)   | board.getBitboard(BLACK_KINGS);
    uint64_t knights = board.getBitboard(WHITE_KNIGHTS) | board.getBitboard(BLACK_KNIGHTS);
    uint64_t bishops = board.getBitboard(WHITE_BISHOPS) | board.getBitboard(BLACK_BISHOPS);
    uint64_t queens  = board.getBitboard(WHITE_QUEENS)  | board.getBitboard(BLACK_QUEENS);
    uint64_t rooks   = board.getBitboard(WHITE_ROOKS)   | board.getBitboard(BLACK_ROOKS);
    uint64_t wpawns  = board.getBitboard(WHITE_PAWNS);
    uint64_t bpawns  = board.getBitboard(BLACK_PAWNS);

    uint64_t attackers = 0;
    // King attacks
    attackers |= king_threats_table[toSquare] & kings;

    // Knight attacks
    attackers |= knight_threats_table[toSquare] & knights;

    // Pawn attacks
    attackers |= wpawn_threats_table[toSquare] & bpawns;
    attackers |= bpawn_threats_table[toSquare] & wpawns;

    // Bishop and Queen attacks (diagonal)
    attackers |= Bmagic(toSquare, occupancy) & (bishops | queens);

    // Rook and Queen attacks (orthogonal)
    attackers |= Rmagic(toSquare, occupancy) & (rooks | queens);
    return attackers;
}

/**
 * @brief Finds the least valuable piece attacking or defending a square.
 *
 * This function scans through all pieces in the given bitboard (`attadef`),
 * determining the one with the lowest material value.
 *
 * @param board The current board state.
 * @param attadef A bitboard of attacking/defending pieces.
 * @param isWhite Whether we are searching for white pieces (true) or black pieces (false).
 * @return A bitboard with a single bit set, representing the least valuable piece.
 */
uint64_t getLeastValuablePiece(const BoardState& board, uint64_t attadef, bool isWhite) {
    uint64_t leastValuablePiece = 0;
    int minValue = 9999;  // Start with a high initial value
    attadef = attadef & board.getOccupancy(isWhite);
    while (attadef) {
        // Isolate the least significant bit (one piece in attadef)
        uint64_t squareMask = attadef & -attadef;
        // Find the piece type at the square
        int pieceType = findPieceType(board, squareMask, isWhite);

        // Get the absolute material score for the piece
        int pieceValue = std::abs(MATERIAL_SCORES[pieceType]);

        // Update the least valuable piece if this one has a smaller value
        if (pieceValue < minValue) {
            minValue = pieceValue;
            leastValuablePiece = squareMask;
        }

        // Clear the least significant bit to move to the next piece
        attadef &= attadef - 1;
    }

    return leastValuablePiece;  // Return the bitboard of the least valuable piece
}

/**
 * @brief Determines x-ray attacks on a given square.
 *
 * This function identifies pieces (bishops, rooks, and queens) that indirectly attack
 * a square through another piece, leveraging x-ray vision techniques.
 *
 * @param board The current board state.
 * @param occ The bitboard representing occupied squares.
 * @param toSquare The target square where x-ray attacks are considered.
 * @return A bitboard representing all pieces exerting x-ray attacks on the square.
 */
uint64_t considerXrays(const BoardState& board, uint64_t occ, int toSquare) {
    uint64_t xrayAttackers = 0;

    // Bishop and Queen diagonal x-rays
    uint64_t bishopXrays = Bmagic(toSquare, occ) &
                           (board.getBitboard(WHITE_BISHOPS) | board.getBitboard(BLACK_BISHOPS) |
                            board.getBitboard(WHITE_QUEENS) | board.getBitboard(BLACK_QUEENS));
    xrayAttackers |= bishopXrays;

    // Rook and Queen orthogonal x-rays
    uint64_t rookXrays =
        Rmagic(toSquare, occ) & (board.getBitboard(WHITE_ROOKS) | board.getBitboard(BLACK_ROOKS) |
                                 board.getBitboard(WHITE_QUEENS) | board.getBitboard(BLACK_QUEENS));
    xrayAttackers |= rookXrays;
    xrayAttackers &= occ;
    return xrayAttackers;
}

/**
 * @brief Performs Static Exchange Evaluation (SEE).
 *
 * SEE determines whether a capture sequence on a given square results in a material gain or loss.
 * It evaluates piece exchanges recursively, considering defenders and attackers.
 *
 * This function is useful for:
 * - Move ordering (prioritizing better captures)
 * - Quiescence Search (avoiding unnecessary pruning of good captures)
 *
 * @param board The current board state.
 * @param toSq The destination square where the exchange occurs.
 * @param target The piece type being captured.
 * @param frSq The square of the attacking piece.
 * @param aPiece The piece type initiating the capture.
 * @return The net material gain (positive for a favorable capture, negative for a loss).
 */
int see(const BoardState& board, int toSq, int target, int frSq, int aPiece) {
    int gain[32];
    int d = 0;
    
    uint64_t bishops = board.getBitboard(WHITE_BISHOPS) | board.getBitboard(BLACK_BISHOPS);
    uint64_t queens  = board.getBitboard(WHITE_QUEENS)  | board.getBitboard(BLACK_QUEENS);
    uint64_t rooks   = board.getBitboard(WHITE_ROOKS)   | board.getBitboard(BLACK_ROOKS);
    uint64_t wpawns  = board.getBitboard(WHITE_PAWNS);
    uint64_t bpawns  = board.getBitboard(BLACK_PAWNS);
    uint64_t mayXray = wpawns | bpawns | bishops | rooks | queens;
    uint64_t fromBoard = 1ULL << frSq;
    uint64_t occ = board.getAllOccupancy();
    uint64_t attadef = attacksTo(board, occ, toSq);
    gain[d] = std::abs(MATERIAL_SCORES[target]);
    bool isWhite = board.getTurn();
    do {
        d++;                                    // next depth and side
        isWhite = !isWhite;
        gain[d] = std::abs(MATERIAL_SCORES[aPiece]) - gain[d - 1];  // speculative store, if defended
        attadef ^= fromBoard;                   // reset bit in set to traverse
        occ ^= fromBoard;                       // reset bit in temporary occupancy (for x-Rays)
        if (fromBoard & mayXray){
            attadef |= considerXrays(board, occ, toSq);
        }
        fromBoard = getLeastValuablePiece(board, attadef, isWhite);
        if (fromBoard){
            aPiece = findPieceType(board, fromBoard, isWhite);
        }
    } while (fromBoard);
    while (--d) gain[d - 1] = -std::max(-gain[d - 1], gain[d]);
    return gain[0];
}

/**
 * @brief Filters a list of legal moves to include only favorable captures, promotions, and checks.
 *
 * This function refines a list of legal moves by selecting:
 * - Captures that result in a net material gain (using Static Exchange Evaluation).
 * - Queen promotions.
 * - Moves that deliver check.
 *
 * This is used in Quiescence Search to extend search depth on unstable positions.
 *
 * @param board The current board state.
 * @param moves A list of all legal moves.
 * @return A filtered list of favorable captures, promotions, and checks.
 */
std::vector<uint16_t> goodCaptureOrChecks(BoardState& board, const std::vector<uint16_t>& moves) {
    std::vector<uint16_t> result;
    for (uint16_t move : moves) {
        // Check if the position is in check after the move
        MoveUndo undoState = applyMove(board, move);
        bool check = is_in_check(board);
        undoMove(board, undoState);
        if (check) {
            result.push_back(move);
            continue;
        }

        int toSq, frSq, special;
        decodeMove(move, frSq, toSq, special);
        if (special == PROMOTION_QUEEN) {
            result.push_back(move);
            continue;
        }

        //If the tosquare is occupied by a piece already, it's a capture.
        if ((1ULL << toSq) & board.getAllOccupancy()) {
            // Use SEE to evaluate the capture
            int movingPiece = findPieceType(board, (1ULL << frSq), board.getTurn());
            int capturedPiece = findPieceType(board, (1ULL << toSq), !board.getTurn());

            if (see(board, toSq, capturedPiece, frSq, movingPiece) > 0) {
                result.push_back(move);
            }
        }
    }

    return result;
}

/**
 * @brief Constructor for the Search class.
 *
 * Initializes the search parameters, including the board state, transposition table,
 * time limit, and search control variables.
 *
 * @param boardParam The board state to search on.
 * @param tableParam The transposition table used for storing results.
 * @param timeLimitParam The time limit in milliseconds for the search.
 */
Search::Search(BoardState& boardParam, TranspositionTable& tableParam, int timeLimitParam) {
    board = boardParam;
    table = tableParam;
    timeLimitMs = timeLimitParam;
    maxDepth = 12;
    bestMoveSoFar = 0;
    bestEvalSoFar = -99999;
    searchInterrupted = false;
}

/**
 * @brief Checks whether the search should be stopped due to time constraints.
 *
 * This function monitors the elapsed search time and stops the search if the time limit
 * is exceeded. If the search is interrupted, the `searchInterrupted` flag is set.
 *
 * @return True if the search should stop, false otherwise.
 */
bool Search::shouldStopSearch() {
    auto now = std::chrono::steady_clock::now();
    // std::cout << "now: " << now << std::endl;
    // std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime)
                    //  .count() << " >= " << timeLimitMs << std::endl;
    if(std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count() >=
        timeLimitMs){
            searchInterrupted = true;
            return true;
        }
    return false;
}

/**
 * @brief Performs Quiescence Search to refine evaluation in tactical positions.
 *
 * Quiescence Search extends the search at leaf nodes by considering only captures,
 * promotions, and checks to avoid the horizon effect. It ensures that unstable positions
 * are evaluated more accurately.
 *
 * The function follows these steps:
 * 1. Checks if the search should stop due to time constraints.
 * 2. Evaluates the current position (stand pat).
 * 3. If stand pat exceeds beta, pruning occurs.
 * 4. Otherwise, captures, promotions, and checks are explored recursively.
 * 5. The best found score is returned.
 *
 * @param alpha The alpha bound (best guaranteed score for the maximizing player).
 * @param beta The beta bound (best guaranteed score for the minimizing player).
 * @return The evaluation score for the position.
 */
int Search::QSearch(int alpha, int beta) {
    if (shouldStopSearch()) {
        return 0;
    }
    int standPat = evaluate(board);
    if (standPat >= beta) return beta;
    if (standPat > alpha) alpha = standPat;
    
    std::vector<uint16_t> allMoves = allLegalMoves(board);
    std::vector<uint16_t> checksAndCaptures = goodCaptureOrChecks(board, allMoves);
    for (uint16_t move : checksAndCaptures) {
        MoveUndo undoState = applyMove(board, move);
        int score = -QSearch(-beta, -alpha);
        undoMove(board, undoState);

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

    return alpha;
}


bool debugnm = false;
bool debuggbm = false;

/**
 * Implements the Negamax search algorithm with alpha-beta pruning.
 *
 * - Uses a transposition table to avoid redundant calculations.
 * - Detects threefold repetition and game-ending conditions early.
 * - Incorporates move ordering for better pruning efficiency.
 * - Calls Quiescence Search (QSearch) when reaching depth 0.
 * - Implements a visit count mechanism to handle threefold repetition correctly.
 *
 * @param depth  The current search depth.
 * @param alpha  The lower bound of the best score found so far.
 * @param beta   The upper bound of the best score found so far.
 * @return       The evaluated score of the position.
 */
int Search::negamax(int depth, int alpha, int beta) {
    if (shouldStopSearch()) {
        return 0; // Stop searching if time is up
    }

    uint64_t zobristHash = board.getZobristHash();

    // Track position visits for threefold repetition detection
    incrementVisitCount(table, zobristHash);

    if(threefold(table, zobristHash)) {
        // Threefold repetition detected → Draw (score = 100)
        // Viewed from the otherside, the three-fold is -100, meaning
        // we will try to avoid three-folds unless we are in a worse position
        // than -100.
        updateTranspositionTable(table, zobristHash, 0, 100, depth, EXACT_SCORE);
        decrementVisitCount(table, zobristHash);
        return 100;
    }
    
    // Check if the position is already stored in the transposition table
    auto it = table.find(zobristHash);
    if (it != table.end()) {

        const TranspositionTableEntry& entry = it->second;

        // If depth is sufficient, use stored evaluation
        if (entry.depth >= depth) {
            if (debugnm) {
                std::cout << "using ttable on already-found-position\n" << board << "\n";
                std::cout << "already searched on depth " << entry.depth
                          << " which is better than current depth " << depth << "\n";
                std::cout << "returning " << entry.evaluation << "\n";
            }
            decrementVisitCount(table, zobristHash);
            // Return stored evaluation based on the type of bound
            if (entry.eval_type == EXACT_SCORE) return entry.evaluation;
            if (entry.eval_type == UPPERBOUND_SCORE && entry.evaluation <= alpha)
                return entry.evaluation;
            if (entry.eval_type == LOWERBOUND_SCORE && entry.evaluation >= beta)
                return entry.evaluation;
        }
    }

    // Generate all legal moves in the current position. Used for determining if the game is over
    std::vector<uint16_t> legalMoves = allLegalMoves(board);

    // Base case: if the depth is 0 or game over, return the evaluation of the position
    GameResult result = gameOver(board, legalMoves, table);

    int moveIndex = 0;
    if (result != ONGOING) {
        if (result == WHITE_WINS || result == BLACK_WINS) {
            if (debugnm) std::cout << "Game over at depth " << depth << ": ";
            if (debugnm)
                std::cout << ((result == WHITE_WINS) ? "White wins" : "Black wins") << "\n";
            if (debugnm) std::cout << board << "\n";
            int mateScore = -99999 - depth;
            updateTranspositionTable(table, zobristHash, 0, mateScore, depth, EXACT_SCORE);
            decrementVisitCount(table, zobristHash);
            return mateScore;
        }

        //The game is a draw for any of four reasons
        if (debugnm){
            std::cout << result << "\n";
            std::cout << "Game drawn at depth " << depth << "due to \n";
            switch (result)
            {
            case DRAW_STALEMATE:
                std::cout << "STALEMATE\n";
                break;
            case DRAW_INSUFFICIENT_MATERIAL:
                std::cout << "DRAW_INSUFFICIENT_MATERIAL\n";
                break;
            case DRAW_50_MOVE_RULE:
                std::cout << "50MOVERULE\n";
                break;
            case DRAW_THREEFOLD_REPETITION:
                std::cout << "THREEFOLD\n";
                break;
            default:
                break;
            }
        }
        updateTranspositionTable(table, zobristHash, 0, 100, depth, EXACT_SCORE);
        decrementVisitCount(table, zobristHash);
        return 100;
    }
    if (depth == 0) {
        int eval = QSearch(alpha, beta);
        updateTranspositionTable(table, zobristHash, 0, eval, depth, EXACT_SCORE);
        if (debugnm) std::cout << "Evaluating leaf node at depth 0: eval = " << eval << "\n";
        if (debugnm) std::cout << board << "\n";
        decrementVisitCount(table, zobristHash);
        return eval;
    }

    // Now that we know we will iterate through the legal moves its worthwhile to go through the
    // effort of ordering them
    legalMoves = orderMoves(board, legalMoves);
    int bestScore = -999999;
    uint16_t bestMoveNM = 0;
    int alpha_original = alpha;
    for (const uint16_t& move : legalMoves) {
        MoveUndo undoData = applyMove(board, move);
        if (debugnm)
        std::cout << "Depth " << depth << ", Move " << moveIndex << ": " << moveToString(move)
        << "\n";
        int score = -negamax(depth - 1, -beta, -alpha);
        undoMove(board, undoData);
        
        if (score > bestScore) {
            bestScore = score;
            if (score > alpha) {
                alpha = score;
            }
            bestMoveNM = move;
        }
        if (alpha >= beta) {
            break;  // Beta-cutoff
        }
    }
    int flag = 0;
    if (bestScore <= alpha_original) {
        flag = UPPERBOUND_SCORE;  // No move improved alpha
    } else if (bestScore >= beta) {
        flag = LOWERBOUND_SCORE;  // Beta cutoff happened
    } else {
        flag = EXACT_SCORE;  // Best move was found without beta cutoff
    }

    // Update transposition table with the correct score type
    updateTranspositionTable(table, zobristHash, bestMoveNM, bestScore, depth, flag);

    // Decrement visit count after recursion completes
    decrementVisitCount(table, zobristHash);

    return bestScore;
}

/**
 * Determines the best move for the current position using the negamax search algorithm.
 *
 * - Iterates through `orderedLegalMoves` to evaluate each move.
 * - Uses `negamax` to recursively determine the evaluation of each move.
 * - Keeps track of the best move found so far, updating `bestMoveSoFar` and `bestEvalSoFar`.
 * - Uses alpha-beta pruning to improve efficiency.
 * - Stores move evaluations in `scoredMoves` for potential reordering in iterative deepening.
 * - Stops searching if `shouldStopSearch()` is triggered.
 *
 * @param depth The depth to search for the best move.
 */
void Search::getBestMove(int depth) {
    int alpha = -999999;
    int beta = 999999;

    if (debuggbm) std::cout << "Evaluating moves at depth " << depth << "\n";
    int moveIndex = 0; //remember to delete, only useflu for logs
    for (const uint16_t& move : orderedLegalMoves) {
        if (shouldStopSearch()) {
            return;
        }
        
        MoveUndo undoData = applyMove(board, move);
        if (debuggbm) std::cout << "Testing move " << moveIndex << ": " << moveToString(move) << "\n";
        // Determine if we're maximizing for white (positive) or black (negative)
        int eval = -negamax(depth - 1, -beta, -alpha);
        undoMove(board, undoData);
        scoredMoves.emplace_back(eval, move);
        if (debuggbm) std::cout << "Move " << moveToString(move) << " -> gbm eval = " << eval
                  << ", bestEval = " << bestEvalSoFar << "\n";

        if (move == bestMoveSoFar && eval != bestEvalSoFar) {
            bestEvalSoFar = eval;
        }

        if (eval > bestEvalSoFar && !searchInterrupted) {
            bestEvalSoFar = eval;
            bestMoveSoFar = move;
            if (debuggbm) std::cout << "New best move: " << moveToString(bestMoveSoFar)
                      << " with score = " << bestEvalSoFar << "\n";
        }
        alpha = std::max(alpha, eval);

        moveIndex++;
        if (debuggbm) std::cout << "\n\n";
    }
    if (debuggbm) std::cout << "Best move at depth " << depth << ": " << moveToString(bestMoveSoFar)
              << " with score = " << bestEvalSoFar << "\n";
    return;
}

/**
 * Performs Iterative Deepening Search (IDS) to find the best move.
 *
 * - Starts at depth 1 and incrementally increases the search depth.
 * - Uses `shouldStopSearch()` to respect time constraints.
 * - Calls `getBestMove(depth)` at each iteration to perform a full-depth search.
 * - Uses move ordering to improve search efficiency in subsequent iterations.
 * - Stores the best move found at the deepest completed depth.
 *
 * @return The best move found during the search.
 */
uint16_t Search::iterativeDeepening() {
    startTime = std::chrono::steady_clock::now();
    orderedLegalMoves = orderMoves(board, allLegalMoves(board));
    for (int depth = 1; depth <= maxDepth; ++depth) {
        if (shouldStopSearch()) {
            break;
        }

        // Perform depth-first search at the current depth.
        getBestMove(depth);

        // Sort the vector
        std::sort(scoredMoves.begin(), scoredMoves.end(), std::greater<>());

        // Extract the moves in sorted order
        std::vector<uint16_t> orderedMoves;
        for (const auto& scoredMove : scoredMoves) {
            orderedMoves.push_back(scoredMove.second);
        }
        orderedLegalMoves = orderedMoves;
        scoredMoves.clear();
    }

    return bestMoveSoFar;
}

/**
 * Searches for the best move to a fixed depth.
 *
 * - Begins with move ordering to improve search efficiency.
 * - Calls `getBestMove(depth)` to perform a depth-limited search.
 * - Used for debugging or testing fixed-depth searches without iterative deepening.
 *
 * @param depth The fixed depth to search.
 * @return The best move found at the given depth.
 */
uint16_t Search::searchToDepth(int depth) {
    startTime = std::chrono::steady_clock::now();
    orderedLegalMoves = orderMoves(board, allLegalMoves(board));
    std::cout << moveToString(orderedLegalMoves[0]) << std::endl;
    getBestMove(depth);
    return bestMoveSoFar;
}
