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
    uint64_t whitePieces = board.getWhiteOccupancy();
    uint64_t blackPieces = board.getBlackOccupancy();

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

GameResult gameOver(const BoardState& board, const std::vector<uint16_t>& legalMoves) {
    if (whiteCheckmate(board, legalMoves)) return BLACK_WINS;
    if (blackCheckmate(board, legalMoves)) return WHITE_WINS;
    if (stalemate(board, legalMoves)) return DRAW_STALEMATE;
    if (fiftyMoveRule(board)) return DRAW_50_MOVE_RULE;
    if (insufficientMaterial(board)) return DRAW_INSUFFICIENT_MATERIAL;
    return ONGOING;
}

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
            //Always treat the captured piece as if they're white and 
            //abs() the capturing piece so we are comparing positive piece
            //values with positive piece values, not negatives.
            int capturedPieceType = findPieceType(board, toSquareMask, !board.getTurn());
            int capturingPieceValue = abs(MATERIAL_SCORES[fromPieceType]);
            int capturedPieceValue = abs(MATERIAL_SCORES[capturedPieceType]);
            
            //
            score += std::max(capturedPieceValue - capturingPieceValue, 0);
        }

        if (isCheck) score += 50;
        if (special == PROMOTION_QUEEN) score += 90;
        else if (special == PROMOTION_KNIGHT) score += 50;
        else if (special == PROMOTION_ROOK) score += 40;
        else if (special == PROMOTION_BISHOP) score += 30;
        else if (special == CASTLING_KINGSIDE || special == CASTLING_QUEENSIDE) score += 40;

        if(fromPieceType == WHITE_PAWNS || fromPieceType == BLACK_PAWNS) score -= 10;
        else if(fromPieceType == WHITE_KINGS || fromPieceType == BLACK_KINGS) score -= 10;

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

    // if (is_in_check(board)) {
    //     // look a little further if we're in check.
    //     // wouldn't want to miss a checkmate on the
    //     // edge of our evaluation depth.
    //     depth = 1;
    // }
// Negamax with alpha-beta pruning
// double negamax(BoardState& board, int depth, double alpha, double beta, int color) {
//     std::vector<uint16_t> legalMoves = orderMoves(board, allLegalMoves(board));
//     double maxScore = -999999;

//     // Base case: if the depth is 0 or game over, return the evaluation of the position
//     GameResult result = gameOver(board, legalMoves);
//     if(result != ONGOING){
//         if (result == WHITE_WINS || result == BLACK_WINS) {
//             // std::cout << "found checkmate" << std::endl;
//             return (999999 - depth) * color;
//         }
//         if (result == DRAW_STALEMATE || result == DRAW_50_MOVE_RULE ||
//             result == DRAW_INSUFFICIENT_MATERIAL) {
//             return 0;
//         }
//     }
//     if (depth == 0) {
//         return color * evaluate(board);
//     }
//     int i = 0;
//     // std::cout << board;
//     for (const uint16_t& move : legalMoves) {
//         // Apply the move
//         BoardState test = board;
//         MoveUndo undoData = applyMove(test, move);

//         // Recursively calculate the negamax score
//         double score = -negamax(test, depth - 1, -beta, -alpha, -color);
//         // Undo the move
//         undoMove(test, undoData);

//         maxScore = std::max(maxScore, score);
//         alpha = std::max(alpha, score);

//         // // Alpha-beta pruning
//         if (alpha >= beta) {
//             break;
//         }
//     }

//     return maxScore;
// }
bool debugnm = false;
bool debuggbm = false;
double negamax(BoardState& board, int depth, double alpha, double beta) {
    std::vector<uint16_t> legalMoves = orderMoves(board, allLegalMoves(board));
    double maxScore = -999999;

    // Base case: if the depth is 0 or game over, return the evaluation of the position
    GameResult result = gameOver(board, legalMoves);
    if (result != ONGOING) {
        if (result == WHITE_WINS || result == BLACK_WINS) {
            if (debugnm) std::cout << "Game over at depth " << depth << ": ";
            if (debugnm) std::cout << ((result == WHITE_WINS) ? "White wins" : "Black wins") << "\n";
            return (-250 - depth);
        }
        if (result == DRAW_STALEMATE || result == DRAW_50_MOVE_RULE ||
            result == DRAW_INSUFFICIENT_MATERIAL) {
            if (debugnm) std::cout << result << "\n";
            if (debugnm) std::cout << "Game drawn at depth " << depth << "\n";
            return 0;
        }
    }
    if (depth == 0) {
        double eval = evaluate(board);
        if (debugnm) std::cout << "Evaluating leaf node at depth 0: eval = " << eval << "\n";
        return eval;
    }

    int moveIndex = 0;  // Track the move index for debugging
    for (const uint16_t& move : legalMoves) {
        // BoardState test = board;
        MoveUndo undoData = applyMove(board, move);

        if (debugnm) std::cout << "Depth " << depth << ", Move " << moveIndex << ": " << moveToString(move)
                  << "\n";
        double score = -negamax(board, depth - 1, -beta, -alpha);
        undoMove(board, undoData);

        if (debugnm) std::cout << "Depth " << depth << ", Move " << moveToString(move) << " -> nm score = " << score
                  << ", alpha = " << alpha << ", beta = " << beta << "\n";

        maxScore = std::max(maxScore, score);
        alpha = std::max(alpha, score);

        if (alpha >= beta) {
            if (debugnm) std::cout << "Pruned branch at move " << moveToString(move) << " (alpha >= beta)\n";
            break;
        }
        moveIndex++;
    }

    return maxScore;
}

uint16_t getBestMove(BoardState& board, int depth) {
    std::vector<uint16_t> legalMoves = orderMoves(board, allLegalMoves(board));

    uint16_t bestMove = 0;
    double bestScore = -INT_MAX;
    double alpha = -999999;
    double beta = 999999;

    if (debuggbm) std::cout << "Evaluating moves at depth " << depth << "\n";
    int moveIndex = 0;
    for (const uint16_t& move : legalMoves) {
        // BoardState test = board;
        MoveUndo undoData = applyMove(board, move);

        if (debuggbm) std::cout << "Testing move " << moveIndex << ": " << moveToString(move) << "\n";
        double score = -negamax(board, depth - 1, -beta, -alpha);
        undoMove(board, undoData);

        if (debuggbm) std::cout << "Move " << moveToString(move) << " -> gbm score = " << score
                  << ", bestScore = " << bestScore << "\n";

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
            if (debuggbm) std::cout << "New best move: " << moveToString(bestMove)
                      << " with score = " << bestScore << "\n";
        }
        alpha = std::max(alpha, score);

        moveIndex++;
        if (debuggbm) std::cout << "\n\n\n\n";
    }
    if (debuggbm) std::cout << "Best move at depth " << depth << ": " << moveToString(bestMove)
              << " with score = " << bestScore << "\n";

    return bestMove;
}

// Returns the best move for the current player based on the negamax algorithm
// uint16_t getBestMove(BoardState& board, int depth) {
//     std::vector<uint16_t> legalMoves = orderMoves(board, allLegalMoves(board));

//     uint16_t bestMove = 0;
//     double bestScore = -INT_MAX;
//     double alpha = -999999;
//     double beta = 999999;
//     int color = board.getTurn() ? 1 : -1;
//     int i = 0;
//     for (const uint16_t& move : legalMoves) {
//         // Apply the move
//         // MoveUndo undoData = storeUndoData(board, move);
//         BoardState test = board;
//         MoveUndo undoData = applyMove(test, move);

//         // Get the negamax score for the move
//         double score = -negamax(test, depth - 1, -beta, -alpha, -color);
//         undoMove(test, undoData);

//         // Update the best score and best move
//         if (score > bestScore) {
//             bestScore = score;
//             bestMove = move;
//         }
//         alpha = std::max(alpha, score);
//     }
//     return bestMove;
// }

