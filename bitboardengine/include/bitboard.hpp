// bitboard.hpp
#ifndef BITBOARD_HPP
#define BITBOARD_HPP
#include <random>
#include <array>
#include <cstdint>
#include <string>
#include <sstream>
#include <stdexcept>
#include <iostream> //remove later

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

// Declare Zobrist tables
extern uint64_t zobristTable[12][64];
extern uint64_t zobristCastling[16];
extern uint64_t zobristEnPassant[8];
extern uint64_t zobristSideToMove;


// Bit manipulation functions
uint64_t set_bit(uint64_t bitboard, int square);

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
    uint8_t en_passant_square;
    uint8_t castling_rights; // maybe bitfield for each castling right
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
    void revokeKingsideCastlingRights(BoardState& board, bool isWhite);
    void revokeQueensideCastlingRights(BoardState& board, bool isWhite);
    void revokeAllCastlingRights(BoardState& board, bool isWhite);
    void setCastlingRights(uint8_t rights);
    uint8_t getCastlingRights() const;
    bool canCastleKingside(bool isWhite) const;
    bool canCastleQueenside(bool isWhite) const;
    int getMoveCounter() const;
    void setEnPassant(uint8_t square);
    uint8_t getEnPassant() const;
    void flipTurn();
    void setTurn(bool isWhiteTurn);
    bool getTurn() const;

    void setOccupancy(uint64_t white, uint64_t black);
    uint64_t getOccupancy(bool isWhite) const;
    uint64_t getWhiteOccupancy() const;
    uint64_t getBlackOccupancy() const;
    uint64_t getAllOccupancy() const;
    void updateOccupancy();

    void setMoveCounters(int halfmove, int fullmove);
    int getHalfmoveClock() const;
    int getFullmoveNumber() const;

    void setZobristHash(uint64_t zobrist);
    uint64_t getZobristHash() const;
    friend bool operator==(const BoardState& lhs, const BoardState& rhs);
};

// Overload the << operator to visualize the board state
std::ostream& operator<<(std::ostream& os, const BoardState& board);
int findPieceType(const BoardState& board, uint64_t squareMask, bool isWhite);
uint64_t findBitboard(const BoardState& board, int square, bool isWhite);
int getPromotedPieceType(int special, bool isWhite);
BoardState parseFEN(const std::string& fen);

// Function to initialize the Zobrist tables
void initializeZobrist();
uint64_t computeZobristHash(const BoardState& board);

#endif // BITBOARD_HPP
