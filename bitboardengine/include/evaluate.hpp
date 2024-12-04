#ifndef EVALUATE_HPP
#define EVALUATE_HPP

#include "movegen.hpp"
// Material scores
constexpr double MATERIAL_SCORES[] = {
    1.0,   // White Pawn
    3.2,   // White Knight
    3.3,   // White Bishop
    5.0,   // White Rook
    9.0,   // White Queen
    200,   // White King
    -1.0,  // Black Pawn
    -3.2,  // Black Knight
    -3.3,  // Black Bishop
    -5.0,  // Black Rook
    -9.0,  // Black Queen
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
double evaluate(const BoardState& board);
int materialCount(const BoardState& board);
int positionalScore(const BoardState& board);

// Game-ending conditions
GameResult gameOver(const BoardState& board, const std::vector<uint16_t>& legalMoves);
bool insufficientMaterial(const BoardState& board);
bool whiteCheckmate(const BoardState& board, const std::vector<uint16_t>& legalMoves);
bool blackCheckmate(const BoardState& board, const std::vector<uint16_t>& legalMoves);
bool fiftyMoveRule(const BoardState& board);
bool stalemate(const BoardState& board, const std::vector<uint16_t>& legalMoves);
#endif // EVALUATE_HPP
