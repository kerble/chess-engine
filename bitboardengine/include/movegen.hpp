#ifndef MOVEGEN_HPP
#define MOVEGEN_HPP

#include <vector>
#include "move.hpp"
#include "MagicMoves.hpp"

// Declare the king threats table as extern
extern std::array<uint64_t, 64> king_threats_table;
extern std::array<uint64_t, 64> knight_threats_table;
extern std::array<uint64_t, 64> wpawn_threats_table;
extern std::array<uint64_t, 64> bpawn_threats_table;



void initKingThreatMasks();
void initKnightThreatMasks();
void initPawnThreatMasks();


std::vector<uint16_t> allLegalMoves(const BoardState& board);

#endif // MOVEGEN_HPP
