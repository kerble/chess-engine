#include "evaluate.hpp"
// int evaluated_positions = 0;
// double evaluate(const BoardState& boardState) {
//     double score = 0.0;

//     for (int pieceType = 0; pieceType <= BLACK_KINGS; ++pieceType) {
//         uint64_t bitboard = boardState.getBitboard(pieceType);

//         // Count the number of pieces using popcount
//         int pieceCount = __builtin_popcountll(bitboard);

//         // Add material value
//         score += pieceCount * MATERIAL_SCORES[pieceType];
//     }
//     // evaluated_positions++;
//     if(!boardState.getTurn()) return -score;
//     return score;
// }

double evaluate(const BoardState& boardState) {
    double score = 0.0;
    double mobilityScore = 0.0;

    // Calculate material and mobility for the current player's pieces
    for (int pieceType = WHITE_PAWNS; pieceType <= WHITE_KINGS; ++pieceType) {
        uint64_t bitboard = boardState.getBitboard(pieceType);

        // Count material
        int pieceCount = __builtin_popcountll(bitboard);
        score += pieceCount * MATERIAL_SCORES[pieceType];

        // Mobility
        while (bitboard) {
            uint64_t squareMask = bitboard & -bitboard;  // Get LSB (a single piece)
            int square = __builtin_ctzll(squareMask);    // Get the index of the LSB
            switch (pieceType) {
                case WHITE_PAWNS:
                    mobilityScore += __builtin_popcountll(wpawn_threats_table[square]);
                    break;
                case WHITE_KNIGHTS:
                    mobilityScore += __builtin_popcountll(knight_threats_table[square]);
                    break;
                case WHITE_BISHOPS:
                    mobilityScore +=
                        __builtin_popcountll(Bmagic(square, boardState.getAllOccupancy()));
                    break;
                case WHITE_ROOKS:
                    mobilityScore +=
                        __builtin_popcountll(Rmagic(square, boardState.getAllOccupancy()));
                    break;
                case WHITE_QUEENS:
                    mobilityScore +=
                        __builtin_popcountll(Bmagic(square, boardState.getAllOccupancy())) +
                        __builtin_popcountll(Rmagic(square, boardState.getAllOccupancy()));
                    break;
                case WHITE_KINGS:
                    mobilityScore += __builtin_popcountll(king_threats_table[square]);
                    break;
            }
            bitboard ^= squareMask;  // Remove this piece from the bitboard
        }
    }

    // Repeat for the opponent's pieces
    double opponentMobilityScore = 0.0;
    for (int pieceType = BLACK_PAWNS; pieceType <= BLACK_KINGS; ++pieceType) {
        uint64_t bitboard = boardState.getBitboard(pieceType);

        // Count material
        int pieceCount = __builtin_popcountll(bitboard);
        score += pieceCount * MATERIAL_SCORES[pieceType];

        // Mobility
        while (bitboard) {
            uint64_t squareMask = bitboard & -bitboard;
            int square = __builtin_ctzll(squareMask);
            switch (pieceType) {
                case BLACK_PAWNS:
                    opponentMobilityScore += __builtin_popcountll(bpawn_threats_table[square]);
                    break;
                case BLACK_KNIGHTS:
                    opponentMobilityScore += __builtin_popcountll(knight_threats_table[square]);
                    break;
                case BLACK_BISHOPS:
                    opponentMobilityScore +=
                        __builtin_popcountll(Bmagic(square, boardState.getAllOccupancy()));
                    break;
                case BLACK_ROOKS:
                    opponentMobilityScore +=
                        __builtin_popcountll(Rmagic(square, boardState.getAllOccupancy()));
                    break;
                case BLACK_QUEENS:
                    opponentMobilityScore +=
                        __builtin_popcountll(Bmagic(square, boardState.getAllOccupancy())) +
                        __builtin_popcountll(Rmagic(square, boardState.getAllOccupancy()));
                    break;
                case BLACK_KINGS:
                    opponentMobilityScore += __builtin_popcountll(king_threats_table[square]);
                    break;
            }
            bitboard ^= squareMask;
        }
    }

    // Adjust score based on mobility ratio
    if (mobilityScore + opponentMobilityScore > 0) {
        double mobilityRatio = mobilityScore / (mobilityScore + opponentMobilityScore);
        score *= (1.0 + mobilityRatio - 0.5);  // Normalize to a multiplier centered at 1
    }

    // Flip the score if it's the opponent's turn
    if (!boardState.getTurn()) {
        return -score;
    }
    return score;
}
