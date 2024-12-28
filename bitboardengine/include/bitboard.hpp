// bitboard.hpp
#ifndef BITBOARD_HPP
#define BITBOARD_HPP
#include <random>
#include <array>
#include <cstdint>
#include <string>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <unordered_map>

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

// Used for FEN/zobrist hashing
constexpr int NO_EN_PASSANT = 64;
constexpr int UNKNOWN_EVAL = 1234.0;
// Declare Zobrist tables
extern uint64_t zobristTable[12][64];
extern uint64_t zobristCastling[16];
extern uint64_t zobristEnPassant[8];
extern uint64_t zobristSideToMove;

struct TranspositionTableEntry {
    int visitCount = 0;       // Default visit count
    uint16_t bestMove = 0;    // Default best move (0 indicates unknown)
    double evaluation = UNKNOWN_EVAL;  // Default evaluation
    int depth = -1;           // Default depth (-1 indicates uninitialized depth)
};

using TranspositionTable = std::unordered_map<uint64_t, TranspositionTableEntry>;


// Function to convert algebraic notation (e.g., "e3") to square index (0-63)
int algebraicToSquare(const std::string& algebraic);

std::string squareToAlgebraic(int square);

std::string bitboardToBinaryString(uint64_t bitboard);

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

    void setOccupancy(uint64_t white, uint64_t black);
    uint64_t getOccupancy(bool isWhite) const;
    uint64_t getAllOccupancy() const;
    void updateOccupancy();

    void setCastlingRights(uint8_t rights);
    void revokeKingsideCastlingRights(bool isWhite);
    void revokeQueensideCastlingRights(bool isWhite);
    void revokeAllCastlingRights(bool isWhite);
    uint8_t getCastlingRights() const;

    bool canCastleKingside(bool isWhite) const;
    bool canCastleQueenside(bool isWhite) const;

    void setEnPassant(uint8_t square);
    uint8_t getEnPassant() const;

    void flipTurn();
    void setTurn(bool isWhiteTurn);
    bool getTurn() const;


    void setMoveCounters(int halfmove, int fullmove);
    int getHalfmoveClock() const;
    int getFullmoveNumber() const;

    void setZobristHash(uint64_t zobrist);
    uint64_t getZobristHash() const;

    friend bool operator==(const BoardState& lhs, const BoardState& rhs);
    // Overload the << operator to visualize the board state
    friend std::ostream& operator<<(std::ostream& os, const BoardState& board);
};

// FEN utility

int charToPieceIndex(char piece);

// Function to parse castling rights from FEN string (e.g., "KQkq") to uint64_t
int parseCastlingRights(const std::string& rights);

BoardState parseFEN(const std::string& fen);




// Function to initialize the Zobrist tables
void initializeZobrist();
uint64_t computeZobristHash(const BoardState& board);

void updateTranspositionTable(TranspositionTable& table, uint64_t hash, uint16_t bestMove = 0,
                              double evaluation = UNKNOWN_EVAL, int depth = -1);

bool getTranspositionTableEntry(const TranspositionTable& table, uint64_t hash,
                                TranspositionTableEntry& entry);
                          
void decrementVisitCount(TranspositionTable& table, uint64_t hash);

#endif // BITBOARD_HPP
