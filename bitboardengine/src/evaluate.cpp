#include "evaluate.hpp"
// int evaluated_positions = 0;

std::array<uint64_t, 64> whitePassedPawnMask;
std::array<uint64_t, 64> blackPassedPawnMask;

/**
 * @brief Precomputes whitePassedPawnMask/blackPassedPawnMask for every square.
 *
 * For a pawn on square `sq`, its mask covers its own file and the two adjacent files,
 * restricted to the ranks strictly ahead of it (toward promotion, from that pawn's own
 * color's point of view). If none of the opponent's pawns occupy any square in that mask,
 * no enemy pawn can ever block or capture this pawn before it queens -- it's passed.
 *
 * Called once at startup, alongside the other threat-table init functions.
 */
void initPassedPawnMasks() {
    for (int square = 0; square < 64; ++square) {
        int file = square % 8;
        int rank = square / 8;

        // This square's file plus its left/right neighbors (clipped at the board edge).
        uint64_t threeFileMask = 0;
        for (int f = file - 1; f <= file + 1; ++f) {
            if (f < 0 || f > 7) continue;
            for (int r = 0; r < 8; ++r) {
                threeFileMask |= (1ULL << (r * 8 + f));
            }
        }

        uint64_t ranksAheadForWhite = 0;
        for (int r = rank + 1; r < 8; ++r) {
            ranksAheadForWhite |= (0xFFULL << (r * 8));
        }
        whitePassedPawnMask[square] = threeFileMask & ranksAheadForWhite;

        uint64_t ranksAheadForBlack = 0;
        for (int r = 0; r < rank; ++r) {
            ranksAheadForBlack |= (0xFFULL << (r * 8));
        }
        blackPassedPawnMask[square] = threeFileMask & ranksAheadForBlack;
    }
}

// The a-file (squares a1, a2, ... a8) shifted right by `file` places it over any file --
// a standard bitboard trick for "give me every square on file N".
inline uint64_t fileMask(int file) {
    constexpr uint64_t A_FILE_MASK = 0x0101010101010101ULL;
    return A_FILE_MASK << file;
}

/**
 * @brief Scores a position by pawn structure: passed, doubled, and isolated pawns.
 *
 * Passed pawns (see initPassedPawnMasks()) are rewarded with a bonus that grows sharply
 * the closer they are to promotion, since the opponent must actively spend a piece to stop
 * one. Doubled pawns (two-plus pawns of the same color sharing a file) and isolated pawns
 * (no friendly pawn on either adjacent file, on any rank, so nothing can ever defend it)
 * are both long-term structural weaknesses, and are penalized per pawn.
 *
 * @param board The current board state.
 * @return White's pawn structure score minus black's, in centipawns (positive favors white).
 */
int pawnStructureScore(const BoardState& board) {
    int score = 0;

    uint64_t whitePawns = board.getBitboard(WHITE_PAWNS);
    uint64_t blackPawns = board.getBitboard(BLACK_PAWNS);

    // -- Passed pawns (per pawn) --
    uint64_t bitboard = whitePawns;
    while (bitboard) {
        int square = __builtin_ctzll(bitboard);
        if ((blackPawns & whitePassedPawnMask[square]) == 0) {
            int rank = square / 8;
            score += PASSED_PAWN_BONUS[rank];
        }
        bitboard &= bitboard - 1;
    }

    bitboard = blackPawns;
    while (bitboard) {
        int square = __builtin_ctzll(bitboard);
        if ((whitePawns & blackPassedPawnMask[square]) == 0) {
            int rank = square / 8;
            score -= PASSED_PAWN_BONUS[7 - rank];
        }
        bitboard &= bitboard - 1;
    }

    // -- Doubled and isolated pawns (per file) --
    for (int file = 0; file < 8; ++file) {
        uint64_t thisFile = fileMask(file);
        uint64_t adjacentFiles = (file > 0 ? fileMask(file - 1) : 0) |
                                  (file < 7 ? fileMask(file + 1) : 0);

        int whiteOnFile = __builtin_popcountll(whitePawns & thisFile);
        int blackOnFile = __builtin_popcountll(blackPawns & thisFile);

        if (whiteOnFile > 1) score -= DOUBLED_PAWN_PENALTY * (whiteOnFile - 1);
        if (blackOnFile > 1) score += DOUBLED_PAWN_PENALTY * (blackOnFile - 1);

        if (whiteOnFile > 0 && (whitePawns & adjacentFiles) == 0) {
            score -= ISOLATED_PAWN_PENALTY * whiteOnFile;
        }
        if (blackOnFile > 0 && (blackPawns & adjacentFiles) == 0) {
            score += ISOLATED_PAWN_PENALTY * blackOnFile;
        }
    }

    return score;
}

/**
 * @brief Scores a position by piece mobility (a cheap stand-in for legal move count).
 *
 * For each piece, counts the squares it pseudo-legally attacks/can move to (via the same
 * generateThreatMask() used for check detection and move ordering), excluding squares
 * occupied by its own side. This ignores pins and whether the mover is in check, so it's
 * not exactly "legal moves" -- but it's a close, much cheaper proxy: computing true legal
 * moves for both sides on every evaluated leaf would multiply search cost, since that's
 * the most expensive operation the engine has.
 *
 * @param board The current board state.
 * @return White's mobility minus black's, in centipawns (positive favors white).
 */
int mobilityScore(const BoardState& board) {
    int score = 0;
    uint64_t occupancy = board.getAllOccupancy();

    for (int pieceType = 0; pieceType <= BLACK_KINGS; ++pieceType) {
        bool isWhite = pieceType <= WHITE_KINGS;
        uint64_t ownOccupancy = board.getOccupancy(isWhite);
        uint64_t bitboard = board.getBitboard(pieceType);

        while (bitboard) {
            int square = __builtin_ctzll(bitboard);
            uint64_t reachable = generateThreatMask(pieceType, square, occupancy) & ~ownOccupancy;
            int reachableCount = __builtin_popcountll(reachable);

            score += isWhite ? reachableCount * MOBILITY_WEIGHT : -reachableCount * MOBILITY_WEIGHT;

            bitboard &= bitboard - 1;  // clear this piece's bit, move to the next one
        }
    }

    return score;
}

/**
 * @brief Scores a position by control of the four central squares (d4, e4, d5, e5).
 *
 * Awards a bonus for a piece or pawn standing directly on a center square, plus a smaller
 * bonus per attack a piece throws into the center from elsewhere. This deliberately
 * layers on top of mobilityScore() rather than replacing it: mobility already gives
 * uniform credit for every reachable square, this adds extra weight specifically for the
 * four squares that matter most in the opening.
 *
 * @param board The current board state.
 * @return White's center control minus black's, in centipawns (positive favors white).
 */
int centerControlScore(const BoardState& board) {
    int score = 0;
    uint64_t occupancy = board.getAllOccupancy();

    for (int pieceType = 0; pieceType <= BLACK_KINGS; ++pieceType) {
        bool isWhite = pieceType <= WHITE_KINGS;
        uint64_t bitboard = board.getBitboard(pieceType);

        while (bitboard) {
            int square = __builtin_ctzll(bitboard);

            if (CENTER_SQUARES_MASK & (1ULL << square)) {
                score += isWhite ? CENTER_OCCUPATION_BONUS : -CENTER_OCCUPATION_BONUS;
            }

            uint64_t centerAttacks = generateThreatMask(pieceType, square, occupancy) & CENTER_SQUARES_MASK;
            int attackCount = __builtin_popcountll(centerAttacks);
            score += isWhite ? attackCount * CENTER_ATTACK_BONUS : -attackCount * CENTER_ATTACK_BONUS;

            bitboard &= bitboard - 1;
        }
    }

    return score;
}

/**
 * @brief Penalizes minor pieces (knights/bishops) still sitting on their starting square.
 *
 * A simple, explainable stand-in for a piece-square table's "develop your pieces" signal:
 * one boolean check per minor piece instead of a tuned 64-entry lookup table.
 *
 * @param board The current board state.
 * @return White's development score minus black's, in centipawns (positive favors white).
 */
int developmentScore(const BoardState& board) {
    int score = 0;

    auto stillHome = [&board](int pieceType, int homeSquare) {
        return (board.getBitboard(pieceType) & (1ULL << homeSquare)) != 0;
    };

    if (stillHome(WHITE_KNIGHTS, 1)) score -= UNDEVELOPED_MINOR_PENALTY;   // b1
    if (stillHome(WHITE_KNIGHTS, 6)) score -= UNDEVELOPED_MINOR_PENALTY;   // g1
    if (stillHome(WHITE_BISHOPS, 2)) score -= UNDEVELOPED_MINOR_PENALTY;   // c1
    if (stillHome(WHITE_BISHOPS, 5)) score -= UNDEVELOPED_MINOR_PENALTY;   // f1

    if (stillHome(BLACK_KNIGHTS, 57)) score += UNDEVELOPED_MINOR_PENALTY;  // b8
    if (stillHome(BLACK_KNIGHTS, 62)) score += UNDEVELOPED_MINOR_PENALTY;  // g8
    if (stillHome(BLACK_BISHOPS, 58)) score += UNDEVELOPED_MINOR_PENALTY;  // c8
    if (stillHome(BLACK_BISHOPS, 61)) score += UNDEVELOPED_MINOR_PENALTY;  // f8

    return score;
}

/**
 * @brief Scores a position by rooks on open/semi-open files.
 *
 * A file with no pawns at all is "open"; one with only enemy pawns is "semi-open" -- both
 * give a rook a clear or half-clear path to infiltrate, and are worth more than a file
 * jammed behind the rook's own pawn.
 *
 * @param board The current board state.
 * @return White's rook-file score minus black's, in centipawns (positive favors white).
 */
int rookFileScore(const BoardState& board) {
    int score = 0;
    uint64_t whitePawns = board.getBitboard(WHITE_PAWNS);
    uint64_t blackPawns = board.getBitboard(BLACK_PAWNS);

    uint64_t bitboard = board.getBitboard(WHITE_ROOKS);
    while (bitboard) {
        int square = __builtin_ctzll(bitboard);
        uint64_t file = fileMask(square % 8);
        bool ownPawnOnFile = whitePawns & file;
        bool enemyPawnOnFile = blackPawns & file;
        if (!ownPawnOnFile && !enemyPawnOnFile) score += ROOK_OPEN_FILE_BONUS;
        else if (!ownPawnOnFile) score += ROOK_SEMI_OPEN_FILE_BONUS;
        bitboard &= bitboard - 1;
    }

    bitboard = board.getBitboard(BLACK_ROOKS);
    while (bitboard) {
        int square = __builtin_ctzll(bitboard);
        uint64_t file = fileMask(square % 8);
        bool ownPawnOnFile = blackPawns & file;
        bool enemyPawnOnFile = whitePawns & file;
        if (!ownPawnOnFile && !enemyPawnOnFile) score -= ROOK_OPEN_FILE_BONUS;
        else if (!ownPawnOnFile) score -= ROOK_SEMI_OPEN_FILE_BONUS;
        bitboard &= bitboard - 1;
    }

    return score;
}

int evaluate(const BoardState& boardState) {
    int score = 0;

    for (int pieceType = 0; pieceType <= BLACK_KINGS; ++pieceType) {
        uint64_t bitboard = boardState.getBitboard(pieceType);

        // Count the number of pieces using popcount
        int pieceCount = __builtin_popcountll(bitboard);

        // Add material value
        score += pieceCount * MATERIAL_SCORES[pieceType];
    }

    score += mobilityScore(boardState);
    score += pawnStructureScore(boardState);
    score += centerControlScore(boardState);
    score += developmentScore(boardState);
    score += rookFileScore(boardState);

    // evaluated_positions++;
    if(!boardState.getTurn()) return -score;
    return score;
}


// int evaluate(const BoardState& boardState) {
//     int score = 0;
//     int mobilityScore = 0;

//     // Calculate material and mobility for the current player's pieces
//     for (int pieceType = WHITE_PAWNS; pieceType <= WHITE_KINGS; ++pieceType) {
//         uint64_t bitboard = boardState.getBitboard(pieceType);

//         // Count material
//         int pieceCount = __builtin_popcountll(bitboard);
//         score += pieceCount * MATERIAL_SCORES[pieceType];

//         // Mobility
//         while (bitboard) {
//             uint64_t squareMask = bitboard & -bitboard;  // Get LSB (a single piece)
//             int square = __builtin_ctzll(squareMask);    // Get the index of the LSB
//             switch (pieceType) {
//                 case WHITE_PAWNS:
//                     mobilityScore += __builtin_popcountll(wpawn_threats_table[square]);
//                     break;
//                 case WHITE_KNIGHTS:
//                     mobilityScore += __builtin_popcountll(knight_threats_table[square]);
//                     break;
//                 case WHITE_BISHOPS:
//                     mobilityScore +=
//                         __builtin_popcountll(Bmagic(square, boardState.getAllOccupancy()));
//                     break;
//                 case WHITE_ROOKS:
//                     mobilityScore +=
//                         __builtin_popcountll(Rmagic(square, boardState.getAllOccupancy()));
//                     break;
//                 case WHITE_QUEENS:
//                     mobilityScore +=
//                         __builtin_popcountll(Bmagic(square, boardState.getAllOccupancy())) +
//                         __builtin_popcountll(Rmagic(square, boardState.getAllOccupancy()));
//                     break;
//                 case WHITE_KINGS:
//                     mobilityScore += __builtin_popcountll(king_threats_table[square]);
//                     break;
//             }
//             bitboard ^= squareMask;  // Remove this piece from the bitboard
//         }
//     }

//     // Repeat for the opponent's pieces
//     int opponentMobilityScore = 0.0;
//     for (int pieceType = BLACK_PAWNS; pieceType <= BLACK_KINGS; ++pieceType) {
//         uint64_t bitboard = boardState.getBitboard(pieceType);

//         // Count material
//         int pieceCount = __builtin_popcountll(bitboard);
//         score += pieceCount * MATERIAL_SCORES[pieceType];

//         // Mobility
//         while (bitboard) {
//             uint64_t squareMask = bitboard & -bitboard;
//             int square = __builtin_ctzll(squareMask);
//             switch (pieceType) {
//                 case BLACK_PAWNS:
//                     opponentMobilityScore += __builtin_popcountll(bpawn_threats_table[square]);
//                     break;
//                 case BLACK_KNIGHTS:
//                     opponentMobilityScore += __builtin_popcountll(knight_threats_table[square]);
//                     break;
//                 case BLACK_BISHOPS:
//                     opponentMobilityScore +=
//                         __builtin_popcountll(Bmagic(square, boardState.getAllOccupancy()));
//                     break;
//                 case BLACK_ROOKS:
//                     opponentMobilityScore +=
//                         __builtin_popcountll(Rmagic(square, boardState.getAllOccupancy()));
//                     break;
//                 case BLACK_QUEENS:
//                     opponentMobilityScore +=
//                         __builtin_popcountll(Bmagic(square, boardState.getAllOccupancy())) +
//                         __builtin_popcountll(Rmagic(square, boardState.getAllOccupancy()));
//                     break;
//                 case BLACK_KINGS:
//                     opponentMobilityScore += __builtin_popcountll(king_threats_table[square]);
//                     break;
//             }
//             bitboard ^= squareMask;
//         }
//     }

//     // Adjust score based on mobility ratio
//     if (mobilityScore + opponentMobilityScore > 0) {
//         int mobilityRatio = mobilityScore / (mobilityScore + opponentMobilityScore);
//         score *= (1.0 + mobilityRatio - 0.5);  // Normalize to a multiplier centered at 1
//     }

//     // Flip the score if it's the opponent's turn
//     if (!boardState.getTurn()) {
//         return -score;
//     }
//     return score;
// }
