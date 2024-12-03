#ifndef MOVE_HPP
#define MOVE_HPP
#include "bitboard.hpp"

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
constexpr int NO_EN_PASSANT = 64;

struct MoveUndo {
    uint16_t move;

    uint64_t fromBitboard;
    int from_piece_type;

    uint64_t promotedBitboard;  // Only used in queening
    int promotedPieceType;

    uint64_t capturedBitboard;  // Only used when capturing
    int captured_piece_type;

    bool capture;

    uint8_t enPassantState;  // Stores the en passant square
    uint8_t castlingRights;  // Store castling rights
    int halfMoveClock;       // Tracks half-move clock for 50-move rule
    int moveCounter;         // Full move counter
};

std::string moveToString(uint16_t move);

void applyMove(BoardState& board, uint16_t move);
void undoMove(BoardState& board, const MoveUndo& undoState);

uint16_t encodeMove(int fromSquare, int toSquare, int special = SPECIAL_NONE);
void decodeMove(uint16_t move, int& fromSquare, int& toSquare, int& special);

#endif  // MOVE_HPP
