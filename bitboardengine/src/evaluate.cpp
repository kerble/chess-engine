#include "evaluate.hpp"
// int evaluated_positions = 0;
double evaluate(const BoardState& boardState) {
    double score = 0.0;

    for (int pieceType = 0; pieceType <= BLACK_KINGS; ++pieceType) {
        uint64_t bitboard = boardState.getBitboard(pieceType);

        // Count the number of pieces using popcount
        int pieceCount = __builtin_popcountll(bitboard);

        // Add material value
        score += pieceCount * MATERIAL_SCORES[pieceType];
    }
    // evaluated_positions++;
    return score;
}