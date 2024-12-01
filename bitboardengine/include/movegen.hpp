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

// Move encoding constants
constexpr int SPECIAL_NONE = 0x0;        // 0000
constexpr int PROMOTION_QUEEN = 0x1;     // 0001
constexpr int PROMOTION_KNIGHT = 0x2;    // 0010
constexpr int PROMOTION_ROOK = 0x3;      // 0011
constexpr int PROMOTION_BISHOP = 0x4;    // 0100
constexpr int CASTLING_KINGSIDE = 0x5;   // 0101
constexpr int CASTLING_QUEENSIDE = 0x6;  // 0110
constexpr int DOUBLE_PAWN_PUSH = 0x7;    // 0111
constexpr int EN_PASSANT = 0x8;          // 1000, capturing e.p

void initKingThreatMasks();
void initKnightThreatMasks();
void initPawnThreatMasks();

uint16_t encodeMove(int fromSquare, int toSquare, int special);
void decodeMove(uint16_t move, int& fromSquare, int& toSquare, int& special);

std::string moveToString(uint16_t move);
std::vector<uint16_t> allLegalMoves(const BoardState& board);

#endif // MOVEGEN_HPP
