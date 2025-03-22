#include "search.hpp"
// Assumed to be white's turn, but they can't move, so black wins
bool blackCheckmate(const BoardState& board, const std::vector<uint16_t>& legalMoves) {
    return legalMoves.empty() && is_in_check(board);  // False -> Black
}
// Assumed to be black's turn, but they can't move, so white wins
bool whiteCheckmate(const BoardState& board, const std::vector<uint16_t>& legalMoves) {
    return legalMoves.empty() && is_in_check(board);  // True -> White
}

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


bool fiftyMoveRule(const BoardState& board) { return board.getHalfmoveClock() >= 100; }

bool stalemate(const BoardState& board, const std::vector<uint16_t>& legalMoves) {
    return legalMoves.empty() && !is_in_check(board);
}

bool threefold(const TranspositionTable& table, uint64_t current_pos_hash) {
    // return false;
    // std::cout << "checking if the game is a draw" << std::endl;
    auto it = table.find(current_pos_hash);
    if (it != table.end()) {
        // If the hash exists in the table, check its visitCount
        // std::cout << "hash found, its visit count is " << it->second.visitCount << std::endl;
        return it->second.visitCount >= 3;
    }
    return false;  // If the hash is not found, it's not a repetition
}

GameResult gameOver(const BoardState& board, const std::vector<uint16_t>& legalMoves, TranspositionTable& table) {
    if (whiteCheckmate(board, legalMoves)) return BLACK_WINS;
    if (blackCheckmate(board, legalMoves)) return WHITE_WINS;
    if (stalemate(board, legalMoves)) return DRAW_STALEMATE;
    if (fiftyMoveRule(board)) return DRAW_50_MOVE_RULE;
    if (insufficientMaterial(board)) return DRAW_INSUFFICIENT_MATERIAL;
    if (threefold(table, board.getZobristHash())) return DRAW_THREEFOLD_REPETITION;
    return ONGOING;
}

// forward declaration so this function works
int see(const BoardState& board, int toSq, int target, int frSq, int aPiece);

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

uint64_t getLeastValuablePiece(const BoardState& board, uint64_t attadef, bool isWhite) {
    uint64_t leastValuablePiece = 0;
    int minValue = 201;  // Start with a high initial value
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

/* Static Exchange Evaluation (SEE)
  Statically determine if a capture results in a material gain or loss.
  Used for move ordering for more efficient pruning and also for QSearch
*/
int see(const BoardState& board, int toSq, int target, int frSq, int aPiece) {
    int gain[32];
    int d = 0;
    uint64_t kings   = board.getBitboard(WHITE_KINGS)   | board.getBitboard(BLACK_KINGS);
    uint64_t knights = board.getBitboard(WHITE_KNIGHTS) | board.getBitboard(BLACK_KNIGHTS);
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

// Reduces a list of all legal moves to a list of favorable captures, promotions, and checks
// Used to continually search deeper on positions until they are stable.
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
// Constructor for BoardState
Search::Search(BoardState& boardParam, TranspositionTable& tableParam, int timeLimitParam) {
    board = boardParam;
    table = tableParam;
    timeLimitMs = timeLimitParam;
    maxDepth = 12;
    bestMoveSoFar = 0;
    bestEvalSoFar = -99999;
    searchInterrupted = false;
}

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

int Search::QSearch(int alpha, int beta) {
    // updateTranspositionTable(table, zobristHash, bestMoveNM, bestScore, depth);
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


int ttable_uses = 0;
bool debugnm = false;
bool debuggbm = false;
//check if we're out of time, increment ttable, check legal moves, check if game is over,
//then check ttable, then order moves, then search moves
int Search::negamax(int depth, int alpha, int beta) {
    if (shouldStopSearch()) {
        return 0;
    }
    uint64_t zobristHash = board.getZobristHash();
    // Just increment the visit count
    incrementVisitCount(table, zobristHash);
    if(threefold(table, zobristHash)) {
        updateTranspositionTable(table, zobristHash, 0, 100, depth, EXACT_SCORE);
        decrementVisitCount(table, zobristHash);
        return 100;
    }
    
    auto it = table.find(zobristHash);
    if (it != table.end()) {
        // If depth is sufficient, use stored evaluation
        const TranspositionTableEntry& entry = it->second;
        if (entry.depth >= depth) {
            ttable_uses++;
            if (debugnm) {
                std::cout << "using ttable on already-found-position\n" << board << "\n";
                std::cout << "already searched on depth " << entry.depth
                          << " which is better than current depth " << depth << "\n";
                std::cout << "returning " << entry.evaluation << "\n";
            }
            decrementVisitCount(table, zobristHash);
            // if (entry.evaluation <= alpha) return alpha;  // Fail-low
            // if (entry.evaluation >= beta) return beta;    // Fail-high
            // return entry.evaluation;                      // Exact score
            if (entry.eval_type == EXACT_SCORE) return entry.evaluation;

            if (entry.eval_type == UPPERBOUND_SCORE && entry.evaluation <= alpha)
                return entry.evaluation;

            if (entry.eval_type == LOWERBOUND_SCORE && entry.evaluation >= beta)
                return entry.evaluation;
        }
    }
    std::vector<uint16_t> legalMoves = allLegalMoves(board);
    // Base case: if the depth is 0 or game over, return the evaluation of the position
    GameResult result = gameOver(board, legalMoves, table);
    // std::cout << "just checked if the game is over, gameresult returned " << result << std::endl;
    int moveIndex = 0;
    if (result != ONGOING) {
        if (result == WHITE_WINS || result == BLACK_WINS) {
            if (debugnm) std::cout << "Game over at depth " << depth << ": ";
            if (debugnm)
                std::cout << ((result == WHITE_WINS) ? "White wins" : "Black wins") << "\n";
            if (debugnm) std::cout << board << "\n";
            int mateScore = -2500 - depth;
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
        // int eval = evaluate(board);
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

// Returns the best move for the current player based on the negamax algorithm
void Search::getBestMove(int depth) {
    // std::vector<uint16_t> legalMoves = orderMoves(board, allLegalMoves(board));

    // uint16_t bestMove = 0;
    // int bestScore = -INT_MAX;
    // bestEval
    int alpha = -999999;
    int beta = 999999;

    if (debuggbm) std::cout << "Evaluating moves at depth " << depth << "\n";
    int moveIndex = 0; //remember to delete, only useflu for logs
    // std::cout << "length of legal moves: " << legalMoves.size() << "\n";
    // std::cout << "time limit: " << timeLimitMs << "\n";
    for (const uint16_t& move : orderedLegalMoves) {
        // BoardState test = board;
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
    // bestEval = bestScore;
    // std::cout << "bestEvalSoFar: " << bestEvalSoFar << std::endl;
    return;
}

bool compareMoves(uint16_t a, uint16_t b, const std::unordered_map<uint16_t, int>& moveEvaluations) {
    int evalA = moveEvaluations.count(a) ? moveEvaluations.at(a) : 0.0;
    int evalB = moveEvaluations.count(b) ? moveEvaluations.at(b) : 0.0;
    return evalA > evalB; // Higher evaluations come first
}

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

uint16_t Search::searchToDepth(int depth) {
    startTime = std::chrono::steady_clock::now();
    orderedLegalMoves = orderMoves(board, allLegalMoves(board));
    std::cout << moveToString(orderedLegalMoves[0]) << std::endl;
    getBestMove(depth);
    return bestMoveSoFar;
}
