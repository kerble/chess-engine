#include "bitboard.hpp"

uint64_t set_bit(uint64_t bitboard, int square) { return bitboard | (1ULL << square); }

uint64_t clear_bit(uint64_t bitboard, int square) { return bitboard & ~(1ULL << square); }

bool get_bit(uint64_t bitboard, int square) { return bitboard & (1ULL << square); }

int charToPieceIndex(char piece) {
    switch (piece) {
        case 'P':
            return WHITE_PAWNS;
        case 'N':
            return WHITE_KNIGHTS;
        case 'B':
            return WHITE_BISHOPS;
        case 'R':
            return WHITE_ROOKS;
        case 'Q':
            return WHITE_QUEENS;
        case 'K':
            return WHITE_KINGS;
        case 'p':
            return BLACK_PAWNS;
        case 'n':
            return BLACK_KNIGHTS;
        case 'b':
            return BLACK_BISHOPS;
        case 'r':
            return BLACK_ROOKS;
        case 'q':
            return BLACK_QUEENS;
        case 'k':
            return BLACK_KINGS;
        default:
            throw std::invalid_argument("Invalid FEN character for piece");
    }
}

char pieceIndexToChar(int pieceIndex) {
    switch (pieceIndex) {
        case WHITE_PAWNS:
            return 'P';
        case WHITE_KNIGHTS:
            return 'N';
        case WHITE_BISHOPS:
            return 'B';
        case WHITE_ROOKS:
            return 'R';
        case WHITE_QUEENS:
            return 'Q';
        case WHITE_KINGS:
            return 'K';
        case BLACK_PAWNS:
            return 'p';
        case BLACK_KNIGHTS:
            return 'n';
        case BLACK_BISHOPS:
            return 'b';
        case BLACK_ROOKS:
            return 'r';
        case BLACK_QUEENS:
            return 'q';
        case BLACK_KINGS:
            return 'k';
        default:
            return '.';  // Empty square
    }
}

// Function to convert algebraic notation to square index (0-63)
int squareFromAlgebraic(const std::string& algebraic) {
    if (algebraic.size() != 2) {
        return -1;  // Invalid input (not a valid square)
    }

    char file = algebraic[0];  // e.g., 'a' to 'h'
    char rank = algebraic[1];  // e.g., '1' to '8'

    // Convert file (a-h) to index (0-7)
    int fileIndex = file - 'a';
    // Convert rank (1-8) to index (0-7)
    int rankIndex = rank - '1';

    if (fileIndex < 0 || fileIndex > 7 || rankIndex < 0 || rankIndex > 7) {
        return -1;  // Invalid square
    }

    // Return the square index (0-63) with 0 = a1 and 63 = h8
    return rankIndex * 8 + fileIndex;
}

// Function to convert square index (0-63) to algebraic notation
std::string algebraicFromSquare(int square) {
    if (square < 0 || square > 63) {
        throw std::out_of_range("Square must be between 0 and 63");
    }

    // Calculate file and rank
    char file = 'a' + (square % 8);  // File: 0 maps to 'a', 7 maps to 'h'
    char rank = '1' + (square / 8);  // Rank: 0 maps to '1', 7 maps to '8'

    return std::string(1, file) + rank;
}

// Function to parse castling rights from FEN string to int
int parseCastlingRights(const std::string& rights) {
    int result = 0;
    for (char c : rights) {
        switch (c) {
            case 'K':
                result |= (1ULL << 0);
                break;  // White kingside
            case 'Q':
                result |= (1ULL << 1);
                break;  // White queenside
            case 'k':
                result |= (1ULL << 2);
                break;  // Black kingside
            case 'q':
                result |= (1ULL << 3);
                break;  // Black queenside
            default:
                break;  // Ignore invalid characters
        }
    }
    return result;
}

std::ostream& operator<<(std::ostream& os, const BoardState& board) {
    // Initialize an empty 2D array to represent the board
    char boardVisual[8][8] = {
        {'.', '.', '.', '.', '.', '.', '.', '.'}, {'.', '.', '.', '.', '.', '.', '.', '.'},
        {'.', '.', '.', '.', '.', '.', '.', '.'}, {'.', '.', '.', '.', '.', '.', '.', '.'},
        {'.', '.', '.', '.', '.', '.', '.', '.'}, {'.', '.', '.', '.', '.', '.', '.', '.'},
        {'.', '.', '.', '.', '.', '.', '.', '.'}, {'.', '.', '.', '.', '.', '.', '.', '.'}};

    // Loop through each piece type and populate the boardVisual
    for (int i = 0; i < 12; ++i) {
        uint64_t bitboard = board.getBitboard(i);
        for (int square = 0; square < 64; ++square) {
            if (bitboard & (1ULL << square)) {
                // Determine the piece character for the current bitboard index
                char pieceChar;
                switch (i) {
                    case WHITE_PAWNS:
                        pieceChar = 'P';
                        break;
                    case WHITE_KNIGHTS:
                        pieceChar = 'N';
                        break;
                    case WHITE_BISHOPS:
                        pieceChar = 'B';
                        break;
                    case WHITE_ROOKS:
                        pieceChar = 'R';
                        break;
                    case WHITE_QUEENS:
                        pieceChar = 'Q';
                        break;
                    case WHITE_KINGS:
                        pieceChar = 'K';
                        break;
                    case BLACK_PAWNS:
                        pieceChar = 'p';
                        break;
                    case BLACK_KNIGHTS:
                        pieceChar = 'n';
                        break;
                    case BLACK_BISHOPS:
                        pieceChar = 'b';
                        break;
                    case BLACK_ROOKS:
                        pieceChar = 'r';
                        break;
                    case BLACK_QUEENS:
                        pieceChar = 'q';
                        break;
                    case BLACK_KINGS:
                        pieceChar = 'k';
                        break;
                    default:
                        pieceChar = '.';
                        break;  // Default case (should not occur)
                }

                // Map square to the correct visual board position
                int row = square / 8;  // No inversion here
                int col = square % 8;
                boardVisual[row][col] = pieceChar;
            }
        }
    }

    // Output the visual representation of the board (print rows from A8 to H1)
    for (int row = 7; row >= 0; --row) {  // Reverse row order for display
        for (int col = 0; col < 8; ++col) {
            os << boardVisual[row][col];
        }
        os << std::endl;  // End each rank
    }

    return os;
}

// Convert a uint64_t bitboard into a human-readable binary string
std::string bitboardToBinaryString(uint64_t bitboard) {
    std::string result = "\n";
    for (int rank = 7; rank >= 0; --rank) {     // Start from rank 8 down to rank 1
        for (int file = 0; file < 8; ++file) {  // Left to right: a to h
            int square = rank * 8 + file;
            result += (bitboard & (1ULL << square)) ? '1' : '0';
        }
        result += '\n';  // Newline after each rank
    }
    return result;
}

// Constructor for BoardState
BoardState::BoardState()
    : bitboards{},  // Initialize all bitboards to 0
      white_occupancy(0),
      black_occupancy(0),
      all_occupancy(0),
      en_passant_square(0),     // No en passant square at start
      castling_rights(0b1111),  // All castling rights enabled (KQkq)
      is_white_turn(true),      // White moves first
      halfmove_clock(0),        // No halfmoves at the start
      fullmove_number(1)        // First move of the game
{
    // Initialize the starting positions of pieces using bitboards
    bitboards[WHITE_PAWNS] = 0x000000000000FF00;
    bitboards[WHITE_KNIGHTS] = 0x0000000000000042;
    bitboards[WHITE_BISHOPS] = 0x0000000000000024;
    bitboards[WHITE_ROOKS] = 0x0000000000000081;
    bitboards[WHITE_QUEENS] = 0x0000000000000008;
    bitboards[WHITE_KINGS] = 0x0000000000000010;

    bitboards[BLACK_PAWNS] = 0x00FF000000000000;
    bitboards[BLACK_KNIGHTS] = 0x4200000000000000;
    bitboards[BLACK_BISHOPS] = 0x2400000000000000;
    bitboards[BLACK_ROOKS] = 0x8100000000000000;
    bitboards[BLACK_QUEENS] = 0x0800000000000000;
    bitboards[BLACK_KINGS] = 0x1000000000000000;

    // Calculate occupancies
    updateOccupancy();
}

// Update the bitboard for a specific piece type
void BoardState::updateBitboard(int pieceType, uint64_t newBitboard) {
    if (pieceType < 0 || pieceType >= 12) {
        throw std::invalid_argument("Invalid pieceType in updateBitboard");
        return;
    }
    bitboards[pieceType] = newBitboard;
}

// Get the bitboard for a specific piece type
uint64_t BoardState::getBitboard(int pieceType) const {
    if (pieceType < 0 || pieceType >= 12) {
        throw std::invalid_argument("Invalid pieceType in updateBitboard");
        return 0;
    }
    return bitboards[pieceType];
}

// Set castling rights using a bitmask
void BoardState::setCastlingRights(uint64_t rights) { castling_rights = rights; }

// Check kingside Castling
bool BoardState::canCastleKingside(bool isWhite) const {
    if (isWhite) {
        return castling_rights & (1ULL << 0);  // Check bit 0 (White kingside)
    } else {
        return castling_rights & (1ULL << 2);  // Check bit 2 (Black kingside)
    }
}

// Check if the queenside castling
bool BoardState::canCastleQueenside(bool isWhite) const {
    if (isWhite) {
        return castling_rights & (1ULL << 1);  // Check bit 1 (White queenside)
    } else {
        return castling_rights & (1ULL << 3);  // Check bit 3 (Black queenside)
    }
}

// Set the en passant square (only one square can be en passant at a time)
void BoardState::setEnPassant(uint64_t square) {
    en_passant_square = square;  // Directly set the en passant square (0-63)
}

// Get the en passant square (returns the square where en passant is possible, 0-63)
uint64_t BoardState::getEnPassant() const { return en_passant_square; }

// Set the turn (true = white's turn, false = black's turn)
void BoardState::setTurn(bool isWhiteTurn) { is_white_turn = isWhiteTurn; }

// Get the current turn (true = white's turn, false = black's turn)
bool BoardState::getTurn() const { return is_white_turn; }

// Set the occupancy bitboards for white and black
void BoardState::setOccupancy(uint64_t white, uint64_t black) {
    white_occupancy = white;
    black_occupancy = black;
    all_occupancy = white | black;  // All occupied squares (white + black pieces)
}

// Get the occupancy of white pieces (bitboard)
uint64_t BoardState::getWhiteOccupancy() const { return white_occupancy; }

// Get the occupancy of black pieces (bitboard)
uint64_t BoardState::getBlackOccupancy() const { return black_occupancy; }

uint64_t BoardState::getAllOccupancy() const { return all_occupancy; }

// Update the all_occupancy bitboard (white + black pieces combined)
void BoardState::updateOccupancy() { all_occupancy = white_occupancy | black_occupancy; }

// Set move counters (halfmove clock and fullmove number)
void BoardState::setMoveCounters(int halfmove, int fullmove) {
    halfmove_clock = halfmove;
    fullmove_number = fullmove;
}

// Get halfmove clock (the number of halfmoves since the last pawn move or capture)
int BoardState::getHalfmoveClock() const { return halfmove_clock; }

// Get fullmove number (the number of moves in the game, counting both players' moves)
int BoardState::getFullmoveNumber() const { return fullmove_number; }
