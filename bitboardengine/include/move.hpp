#ifndef MOVE_HPP
#define MOVE_HPP
#include "bitboard.hpp"


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

MoveUndo storeUndoData(const BoardState& board, uint16_t move);
MoveUndo applyMove(BoardState& board, uint16_t move);
void undoMove(BoardState& board, const MoveUndo& undoState);

uint16_t encodeMove(int fromSquare, int toSquare, int special = SPECIAL_NONE);
void decodeMove(uint16_t move, int& fromSquare, int& toSquare, int& special);
// extern int moves_looked_at;
// extern int evaluated_positions;
#endif  // MOVE_HPP
