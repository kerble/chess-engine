#include "evaluate.hpp"

double evaluate(const BoardState& boardState) {
    double score = 0.0;

    for (int pieceType = 0; pieceType <= BLACK_KINGS; ++pieceType) {
        uint64_t bitboard = boardState.getBitboard(pieceType);

        // Count the number of pieces using popcount
        int pieceCount = __builtin_popcountll(bitboard);

        // Add material value
        score += pieceCount * MATERIAL_SCORES[pieceType];
        std::cout << "adding " << pieceCount * MATERIAL_SCORES[pieceType] << " for piece: " << pieceType << std::endl;
    }

    return score;
}

bool blackCheckmate(const BoardState& board, const std::vector<Move>& legalMoves) {
    return legalMoves.empty() && board.isKingInCheck(false);  // False -> Black
}

bool whiteCheckmate(const BoardState& board, const std::vector<Move>& legalMoves) {
    return legalMoves.empty() && board.isKingInCheck(true);  // True -> White
}
