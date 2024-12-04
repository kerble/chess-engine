#include "search.hpp"

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

            score += capturedPieceValue - capturingPieceValue;
        }

        if (isCheck) score += 50;
        if (special == PROMOTION_QUEEN) score += 90;
        if (special == PROMOTION_KNIGHT) score += 50;
        if (special == PROMOTION_ROOK) score += 40;
        if (special == PROMOTION_BISHOP) score += 30;


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

// Negamax with alpha-beta pruning
int negamax(BoardState& board, int depth, int alpha, int beta, int color) {
    std::vector<uint16_t> legalMoves = allLegalMoves(board);
    int maxScore = -INT_MAX;

    // Base case: if the depth is 0 or game over, return the evaluation of the position
    if (depth == 0 || gameOver(board, legalMoves)) {
        return color * evaluate(board);  // Multiply by color to adjust for perspective
    }


    for (const uint16_t& move : legalMoves) {
        // Apply the move
        MoveUndo undoData = storeUndoData(board, move);
        applyMove(board, move);

        // Recursively calculate the negamax score
        int score = -negamax(board, depth - 1, -beta, -alpha, -color);

        // Undo the move
        undoMove(board, undoData);

        maxScore = std::max(maxScore, score);
        alpha = std::max(alpha, score);

        // Alpha-beta pruning
        if (alpha >= beta) {
            break;
        }
    }

    return maxScore;
}

// Returns the best move for the current player based on the negamax algorithm
uint16_t getBestMove(BoardState& board, int depth) {
    // std::vector<uint16_t> legalMoves = allLegalMoves(board);
    std::vector<uint16_t> legalMoves = orderMoves(board, allLegalMoves(board));

    uint16_t bestMove = 0;
    int bestScore = -INT_MAX;
    int alpha = -INT_MAX;
    int beta = INT_MAX;

    for (const uint16_t& move : legalMoves) {
        // Apply the move
        MoveUndo undoData = storeUndoData(board, move);
        applyMove(board, move);

        // Get the negamax score for the move
        int score = -negamax(board, depth - 1, -beta, -alpha, -1);

        // Undo the move
        undoMove(board, undoData);

        // Update the best score and best move
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }

        alpha = std::max(alpha, score);
    }

    return bestMove;
}

