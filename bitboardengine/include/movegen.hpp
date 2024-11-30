#ifndef MOVEGEN_HPP
#define MOVEGEN_HPP

#include <vector>
#include <string>
#include <array>
#include <iostream>
#include <unordered_set>
#include "bitboard.hpp"
#include "MagicMoves.hpp"

// Declare the king threats table as extern
extern std::array<uint64_t, 64> king_threats_table;
extern std::array<uint64_t, 64> knight_threats_table;
extern std::array<uint64_t, 64> wpawn_threats_table;
extern std::array<uint64_t, 64> bpawn_threats_table;

void initKingThreatMasks();
void initKnightThreatMasks();
void initPawnThreatMasks();


std::string moveToString(uint16_t move);
std::vector<uint16_t> allLegalMoves(const BoardState& board);

#endif // MOVEGEN_HPP
