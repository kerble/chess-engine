#ifndef EVALUATE_HPP
#define EVALUATE_HPP

#include <cstdint>
#include "movegen.hpp"

struct MoveUndo {
    uint16_t move;
    uint64_t fromBitboard; //The bit
    uint64_t toBitboard;
    uint64_t capturedBitboard; 
    bool capture;
    int captured_piece_type;

    uint8_t enPassantState;  // Stores the en passant square
    uint8_t castlingRights;  // Store castling rights
    int halfMoveClock;   // Tracks half-move clock for 50-move rule
    int moveCounter;     // Full move counter
};
// Functions for evaluating positions
int evaluatePosition(const BoardState& board);
int materialCount(const BoardState& board);
int positionalScore(const BoardState& board);

// Game-ending conditions
bool gameOver(const BoardState& board);
MoveUndo applyMove(BoardState& board, uint16_t move);
void undoMove(BoardState& board, const MoveUndo& undoData);

#endif // EVALUATE_HPP
