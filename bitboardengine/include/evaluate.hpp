#ifndef EVALUATE_HPP
#define EVALUATE_HPP

#include <cstdint>
#include "bitboard.hpp"

// Functions for evaluating positions
int evaluatePosition(const BoardState& board);
int materialCount(const BoardState& board);
int positionalScore(const BoardState& board);

// Game-ending conditions
bool gameOver(const BoardState& board);

#endif // EVALUATE_HPP
