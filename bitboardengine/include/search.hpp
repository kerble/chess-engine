#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <evaluate.hpp>
#include <string>
#include <algorithm>

// Search functions
uint16_t getBestMove(BoardState& board, TranspositionTable& table, int depth);
// std::string alpha_beta(const uint64_t bitboards[12], bool is_white, int depth, int alpha, int beta);
std::vector<uint16_t> orderMoves(BoardState& board, const std::vector<uint16_t>& moves);

// Game-ending conditions
GameResult gameOver(const BoardState& board, const std::vector<uint16_t>& legalMoves, TranspositionTable& table);
bool insufficientMaterial(const BoardState& board);
bool whiteCheckmate(const BoardState& board, const std::vector<uint16_t>& legalMoves);
bool blackCheckmate(const BoardState& board, const std::vector<uint16_t>& legalMoves);
bool fiftyMoveRule(const BoardState& board);
bool stalemate(const BoardState& board, const std::vector<uint16_t>& legalMoves);


double see(const BoardState& board, int toSq, int target, int frSq, int aPiece);
// Utility
// int quiescence_search(const uint64_t bitboards[12], int alpha, int beta, bool is_white);

#endif // SEARCH_HPP
