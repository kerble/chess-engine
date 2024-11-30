#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <string>

// Search functions
std::string minimax(const uint64_t bitboards[12], bool is_white, int depth);
std::string alpha_beta(const uint64_t bitboards[12], bool is_white, int depth, int alpha, int beta);

// Utility
int quiescence_search(const uint64_t bitboards[12], int alpha, int beta, bool is_white);

#endif // SEARCH_HPP
