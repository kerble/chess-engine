#ifndef EVALUATE_HPP
#define EVALUATE_HPP

#include "movegen.hpp"
// Material scores
constexpr int MATERIAL_SCORES[] = {
    100,   // White Pawn
    325,   // White Knight
    325,   // White Bishop
    500,   // White Rook
    900,   // White Queen
    200,   // White King
    -100,  // Black Pawn
    -325,  // Black Knight
    -325,  // Black Bishop
    -500,  // Black Rook
    -900,  // Black Queen
    -200   // Black King
};

enum GameResult {
    ONGOING,
    WHITE_WINS,
    BLACK_WINS,
    DRAW_STALEMATE,
    DRAW_50_MOVE_RULE,
    DRAW_THREEFOLD_REPETITION,
    DRAW_INSUFFICIENT_MATERIAL
};

// Centipawns awarded per pseudo-legal square a piece can reach. Small on purpose -- this
// should nudge development/activity, not compete with material.
constexpr int MOBILITY_WEIGHT = 2;

// whitePassedPawnMask[sq] / blackPassedPawnMask[sq]: the squares on sq's file and the two
// adjacent files, ahead of sq (toward promotion, from that color's perspective). A pawn is
// passed if none of the opponent's pawns occupy any square in its own mask -- there is no
// enemy pawn left that could ever block or capture it on the way to promotion.
extern std::array<uint64_t, 64> whitePassedPawnMask;
extern std::array<uint64_t, 64> blackPassedPawnMask;
void initPassedPawnMasks();

// Bonus (centipawns) for a passed pawn, indexed by its rank (0 = rank 1 ... 7 = rank 8).
// A pawn can never actually sit on rank 0 or rank 7 (it would have already promoted), so
// those two entries are unused placeholders. Black pawns mirror this with index (7 - rank),
// since they advance toward rank 0 instead of rank 7. Grows sharply near promotion, since a
// passed pawn on the 7th rank is close to unstoppable.
constexpr int PASSED_PAWN_BONUS[8] = {0, 10, 20, 35, 60, 100, 160, 0};

// Penalty (centipawns) charged per doubled pawn (each pawn beyond the first a side has on
// one file) and per isolated pawn (a pawn with no friendly pawn on either adjacent file, on
// any rank -- meaning no pawn can ever defend it).
constexpr int DOUBLED_PAWN_PENALTY = 15;
constexpr int ISOLATED_PAWN_PENALTY = 15;

// The four central squares (d4, e4, d5, e5) -- classical "control the center" territory.
constexpr uint64_t CENTER_SQUARES_MASK = (1ULL << 27) | (1ULL << 28) | (1ULL << 35) | (1ULL << 36);
constexpr int CENTER_OCCUPATION_BONUS = 10;  // per piece/pawn actually standing on a center square
constexpr int CENTER_ATTACK_BONUS = 5;       // per attack a piece throws into the center from elsewhere

// Penalty (centipawns) for each knight/bishop still sitting on its own starting square.
// Simple stand-in for "you should develop your pieces" without a full piece-square table.
constexpr int UNDEVELOPED_MINOR_PENALTY = 15;

// Bonus for a rook on a file with no pawns at all (open) or none of its own (semi-open) --
// such a rook has a clear (or half-clear) path to infiltrate.
constexpr int ROOK_OPEN_FILE_BONUS = 20;
constexpr int ROOK_SEMI_OPEN_FILE_BONUS = 10;

// Functions for evaluating positions
int evaluate(const BoardState& board);
int materialCount(const BoardState& board);
int mobilityScore(const BoardState& board);
int pawnStructureScore(const BoardState& board);
int centerControlScore(const BoardState& board);
int developmentScore(const BoardState& board);
int rookFileScore(const BoardState& board);
int positionalScore(const BoardState& board);


// extern int evaluated_positions;
#endif // EVALUATE_HPP
