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

// Functions for evaluating positions
int evaluate(const BoardState& board);
int materialCount(const BoardState& board);
int positionalScore(const BoardState& board);


// extern int evaluated_positions;
#endif // EVALUATE_HPP
