// bitboard.hpp
#ifndef BITBOARD_HPP
#define BITBOARD_HPP

#include <array>
#include <cstdint>
#include <string>
#include <sstream>
#include <stdexcept>

enum PieceIndex {
    WHITE_PAWNS = 0,
    WHITE_KNIGHTS = 1,
    WHITE_BISHOPS = 2,
    WHITE_ROOKS = 3,
    WHITE_QUEENS = 4,
    WHITE_KINGS = 5,
    BLACK_PAWNS = 6,
    BLACK_KNIGHTS = 7,
    BLACK_BISHOPS = 8,
    BLACK_ROOKS = 9,
    BLACK_QUEENS = 10,
    BLACK_KINGS = 11
};

// Bit manipulation functions
uint64_t set_bit(uint64_t bitboard, int square);
uint64_t clear_bit(uint64_t bitboard, int square);
bool get_bit(uint64_t bitboard, int square);

// FEN utility
int charToPieceIndex(char piece);
char pieceIndexToChar(int pieceIndex);

std::string bitboardToBinaryString(uint64_t bitboard);

// Function to parse castling rights from FEN string (e.g., "KQkq") to uint64_t
int parseCastlingRights(const std::string& rights);

// Function to convert algebraic notation (e.g., "e3") to square index (0-63)
int squareFromAlgebraic(const std::string& algebraic);

std::string algebraicFromSquare(int square);

class BoardState {
private:
    // 12 bitboards, one for each piece type (6 for white, 6 for black)
    std::array<uint64_t, 12> bitboards;

    // Additional board state data
    uint64_t white_occupancy;
    uint64_t black_occupancy;
    uint64_t all_occupancy;
    uint64_t en_passant_square;
    uint64_t castling_rights; // maybe bitfield for each castling right
    bool is_white_turn;
    int halfmove_clock;
    int fullmove_number;
    uint64_t zobrist_hash;

public:
    // Constructor, getters, and setters
    BoardState();

    // Methods to manipulate bitboards
    void updateBitboard(int pieceType, uint64_t newBitboard);
    uint64_t getBitboard(int pieceType) const;

    // Methods to update and check game state (castling, en-passant, etc.)
    void setCastlingRights(uint64_t rights);
    bool canCastleKingside(bool isWhite) const;
    bool canCastleQueenside(bool isWhite) const;

    void setEnPassant(uint64_t square);
    uint64_t getEnPassant() const;

    void setTurn(bool isWhiteTurn);
    bool getTurn() const;

    void setOccupancy(uint64_t white, uint64_t black);
    uint64_t getWhiteOccupancy() const;
    uint64_t getBlackOccupancy() const;
    uint64_t getAllOccupancy() const;
    void updateOccupancy();

    void setMoveCounters(int halfmove, int fullmove);
    int getHalfmoveClock() const;
    int getFullmoveNumber() const;

    uint64_t getZobristHash() const;

    void movePiece(int pieceType, int fromSquare, int toSquare);

};

// Overload the << operator to visualize the board state
std::ostream& operator<<(std::ostream& os, const BoardState& board);

#endif // BITBOARD_HPP
