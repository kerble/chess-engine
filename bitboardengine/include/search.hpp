#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <evaluate.hpp>
#include <string>
#include <algorithm>

// Search functions
int minimax(BoardState& board, int depth, bool isMaximizing);
uint16_t getBestMove(BoardState& board, int depth);
// std::string alpha_beta(const uint64_t bitboards[12], bool is_white, int depth, int alpha, int beta);
std::vector<uint16_t> orderMoves(BoardState& board, const std::vector<uint16_t>& moves);

// Game-ending conditions
GameResult gameOver(const BoardState& board, const std::vector<uint16_t>& legalMoves);
bool insufficientMaterial(const BoardState& board);
bool whiteCheckmate(const BoardState& board, const std::vector<uint16_t>& legalMoves);
bool blackCheckmate(const BoardState& board, const std::vector<uint16_t>& legalMoves);
bool fiftyMoveRule(const BoardState& board);
bool stalemate(const BoardState& board, const std::vector<uint16_t>& legalMoves);

// Utility
// int quiescence_search(const uint64_t bitboards[12], int alpha, int beta, bool is_white);

#endif // SEARCH_HPP
