#include "bitboard.hpp"


uint64_t zobristTable[12][64];
uint64_t zobristCastling[16];
uint64_t zobristEnPassant[8];
uint64_t zobristSideToMove;

// Function to convert algebraic notation to square index (0-63)
int algebraicToSquare(const std::string& algebraic) {
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
std::string squareToAlgebraic(int square) {
    if (square < 0 || square > 63) {
        throw std::out_of_range("Square must be between 0 and 63");
    }

    // Calculate file and rank
    char file = 'a' + (square % 8);  // File: 0 maps to 'a', 7 maps to 'h'
    char rank = '1' + (square / 8);  // Rank: 0 maps to '1', 7 maps to '8'

    return std::string(1, file) + rank;
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
      en_passant_square(NO_EN_PASSANT),// No en passant square at start
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

    uint64_t oldBitboard = bitboards[pieceType];  // Store the old bitboard
    bitboards[pieceType] = newBitboard;           // Update the piece's bitboard

    // Update the occupancy bitboards based on the difference in bitboards
    if (pieceType < 6) {                                     // White piece
        white_occupancy ^= oldBitboard;  // Remove old
        white_occupancy |= newBitboard;  // Add new
    } else {                                                 // Black piece
        black_occupancy ^= oldBitboard;  // Remove old
        black_occupancy |= newBitboard;  // Add new
    }
    updateOccupancy();
}

// Get the bitboard for a specific piece type
uint64_t BoardState::getBitboard(int pieceType) const {
    if (pieceType < 0 || pieceType >= 12) {
        throw std::invalid_argument("Invalid pieceType in getBitboard.");
        return 0;
    }
    return bitboards[pieceType];
}

// Set the occupancy bitboards for white and black
void BoardState::setOccupancy(uint64_t white, uint64_t black) {
    white_occupancy = white;
    black_occupancy = black;
    all_occupancy = white | black;  // All occupied squares (white + black pieces)
}

uint64_t BoardState::getOccupancy(bool isWhite) const {
    return isWhite ? white_occupancy : black_occupancy;
}

uint64_t BoardState::getAllOccupancy() const { return all_occupancy; }

// Update the all_occupancy bitboard (white + black pieces combined)
void BoardState::updateOccupancy() {
    // Reset the occupancy bitboards
    white_occupancy = 0;
    black_occupancy = 0;

    // Add up all white pieces' bitboards
    for (int i = WHITE_PAWNS; i <= WHITE_KINGS; ++i) {
        white_occupancy |= bitboards[i];
    }

    // Add up all black pieces' bitboards
    for (int i = BLACK_PAWNS; i <= BLACK_KINGS; ++i) {
        black_occupancy |= bitboards[i];
    }

    // Combine white and black occupancies
    all_occupancy = white_occupancy | black_occupancy;
}

// Set castling rights using a bitmask
void BoardState::setCastlingRights(uint8_t rights) { castling_rights = rights; }

void BoardState::revokeKingsideCastlingRights(bool isWhite) {
    castling_rights &= ~(isWhite ? 0b01 : 0b0100);
}

void BoardState::revokeQueensideCastlingRights(bool isWhite) {
    castling_rights &= ~(isWhite ? 0b10 : 0b1000);
}

void BoardState::revokeAllCastlingRights(bool isWhite) {
    castling_rights &= ~(isWhite ? 0b11 : 0b1100);
}
uint8_t BoardState::getCastlingRights() const { return castling_rights; }

bool BoardState::canCastleKingside(bool isWhite) const {
    if (isWhite) {
        return castling_rights & (1ULL << 0);  // Check bit 0 (White kingside)
    } else {
        return castling_rights & (1ULL << 2);  // Check bit 2 (Black kingside)
    }
}

bool BoardState::canCastleQueenside(bool isWhite) const {
    if (isWhite) {
        return castling_rights & (1ULL << 1);  // Check bit 1 (White queenside)
    } else {
        return castling_rights & (1ULL << 3);  // Check bit 3 (Black queenside)
    }
}

void BoardState::setEnPassant(uint8_t square){ en_passant_square = square; }
// Get the en passant square (returns the square where en passant is possible, 0-63)
// NO_EN_PASSANT if not possible
uint8_t BoardState::getEnPassant() const { return en_passant_square; }

void BoardState::flipTurn(){ is_white_turn = !is_white_turn;}
void BoardState::setTurn(bool isWhiteTurn) { is_white_turn = isWhiteTurn; }
bool BoardState::getTurn() const { return is_white_turn; }
void BoardState::setMoveCounters(int halfmove, int fullmove) {
    halfmove_clock = halfmove;
    fullmove_number = fullmove;
}

// Get halfmove clock (the number of halfmoves since the last pawn move or capture)
int BoardState::getHalfmoveClock() const { return halfmove_clock; }

// Get fullmove number (the number of moves in the game, counting both players' moves)
int BoardState::getFullmoveNumber() const { return fullmove_number; }

uint64_t BoardState::getZobristHash() const { return zobrist_hash; }
void BoardState::setZobristHash(uint64_t zobrist){ zobrist_hash = zobrist; }

bool operator==(const BoardState& lhs, const BoardState& rhs) {
    return lhs.bitboards == rhs.bitboards &&
           lhs.white_occupancy == rhs.white_occupancy &&
           lhs.black_occupancy == rhs.black_occupancy &&
           lhs.all_occupancy == rhs.all_occupancy &&
           lhs.en_passant_square == rhs.en_passant_square &&
           lhs.castling_rights == rhs.castling_rights &&
           lhs.is_white_turn == rhs.is_white_turn &&
           lhs.halfmove_clock == rhs.halfmove_clock &&
           lhs.fullmove_number == rhs.fullmove_number &&
           lhs.zobrist_hash == rhs.zobrist_hash;
}

std::ostream& operator<<(std::ostream& os, const BoardState& board) {
    // Display the board
    for (int rank = 7; rank >= 0; --rank) {
        for (int file = 0; file < 8; ++file) {
            int square = rank * 8 + file;
            char pieceChar = '.';  // Default empty square

            // Check each piece bitboard to determine what is on this square
            for (int i = 0; i < 12; ++i) {
                if (board.getBitboard(i) & (1ULL << square)) {
                    switch (i % 6) {
                        case 0:
                            pieceChar = 'P';
                            break;  // Pawns
                        case 1:
                            pieceChar = 'N';
                            break;  // Knights
                        case 2:
                            pieceChar = 'B';
                            break;  // Bishops
                        case 3:
                            pieceChar = 'R';
                            break;  // Rooks
                        case 4:
                            pieceChar = 'Q';
                            break;  // Queens
                        case 5:
                            pieceChar = 'K';
                            break;  // Kings
                    }
                    if (i >= 6) pieceChar = tolower(pieceChar);  // Black pieces
                    break;
                }
            }

            os << pieceChar;
        }
        os << '\n';
    }
    // Display turn
    if(board.getTurn()) os << "White to play\n";
    else{ os << "Black to play\n";}
    // Display castling rights
    int castlingRights = board.getCastlingRights();
    os << "Castling rights: ";
    if (castlingRights & 0x1) os << 'K';     // White kingside
    if (castlingRights & 0x2) os << 'Q';     // White queenside
    if (castlingRights & 0x4) os << 'k';     // Black kingside
    if (castlingRights & 0x8) os << 'q';     // Black queenside
    if (!(castlingRights & 0xF)) os << '-';  // No castling rights
    os << '\n';

    // Display halfmove counter
    os << "Halfmove counter: " << board.getHalfmoveClock() << '\n';

    // Display fullmove counter
    os << "Fullmove counter: " << board.getFullmoveNumber() << '\n';

    // Display en passant target square
    int epSquare = board.getEnPassant();
    os << "En passant target square: ";
    if (epSquare != NO_EN_PASSANT) {
        os << squareToAlgebraic(epSquare);
    } else {
        os << '-';
    }
    os << '\n';

    return os;
}

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
            std::string piecestr = "";
            piecestr += piece;
            std::string error = "Invalid FEN character for piece. " + piecestr;
            throw std::invalid_argument(error);
    }
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

BoardState parseFEN(const std::string& fen) {
    BoardState board;
    std::istringstream fenStream(fen);
    std::string piecePlacement, sideToMove, castlingRights, enPassant;
    int halfmoveClock, fullmoveNumber;

    // Parse the FEN components
    fenStream >> piecePlacement >> sideToMove >> castlingRights >> enPassant >> halfmoveClock >>
        fullmoveNumber;

    // Initialize bitboards for all pieces
    for (int i = 0; i < 12; ++i) {
        board.updateBitboard(i, 0);
    }

    int square = 56;  // Start from top-left (a8), which is bit 56
    for (char ch : piecePlacement) {
        if (std::isdigit(ch)) {
            square += (ch - '0');  // Skip empty squares
        } else if (ch == '/') {
            square -= 16;  // Move to the next rank (up one row in FEN, down in bitboard)
        } else {
            int pieceType = charToPieceIndex(ch);  // Get piece index
            uint64_t currentBitboard = board.getBitboard(pieceType);

            // Add the piece to the piece bitboard
            uint64_t newBitboard = (1ULL << square);
            newBitboard |= currentBitboard;
            board.updateBitboard(pieceType, newBitboard);
            square++;
        }
    }

    // Parse side to move
    board.setTurn(sideToMove == "w");

    // Parse castling rights
    uint64_t castling = parseCastlingRights(castlingRights);
    board.setCastlingRights(castling);

    // Parse en passant square
    int enPassantSq = (enPassant == "-") ? NO_EN_PASSANT : algebraicToSquare(enPassant);
    board.setEnPassant(enPassantSq);

    // Parse move counters
    board.setMoveCounters(halfmoveClock, fullmoveNumber);

    // Calculate and set Zobrist hash
    uint64_t zobristHash = computeZobristHash(board);
    board.setZobristHash(zobristHash);
    return board;
}

void initializeZobrist() {
    std::mt19937_64 rng(1234567);  // Seed for reproducibility
    std::uniform_int_distribution<uint64_t> dist;

    // Randomize piece positions
    for (int piece = 0; piece < 12; ++piece) {
        for (int square = 0; square < 64; ++square) {
            zobristTable[piece][square] = dist(rng);
        }
    }

    // Randomize castling rights
    for (int i = 0; i < 16; ++i) {
        zobristCastling[i] = dist(rng);
    }

    // Randomize en passant files
    for (int i = 0; i < 8; ++i) {
        zobristEnPassant[i] = dist(rng);
    }

    // Randomize side to move
    zobristSideToMove = dist(rng);
}

uint64_t computeZobristHash(const BoardState& board) {
    uint64_t hash = 0;

    // Add pieces
    for (int piece = 0; piece < 12; ++piece) {
        uint64_t bitboard = board.getBitboard(piece);
        while (bitboard) {
            int square = __builtin_ctzll(bitboard);  // f least significant bit
            hash ^= zobristTable[piece][square];
            bitboard &= bitboard - 1;  // Clear the least significant bit
        }
    }

    // Add castling rights
    hash ^= zobristCastling[board.getCastlingRights()];

    // Add en passant
    int enPassantSquare = board.getEnPassant();
    if (enPassantSquare != NO_EN_PASSANT) {
        int enPassantFile = enPassantSquare % 8;
        hash ^= zobristEnPassant[enPassantFile];
    }

    // Add side to move
    if (!board.getTurn()) {
        hash ^= zobristSideToMove;
    }

    return hash;
}

void updateTranspositionTable(TranspositionTable& table, uint64_t hash, uint16_t bestMove,
                              double evaluation, int depth) {
    auto& entry = table[hash];  // Access or create the entry
    // entry.visitCount++;
    if (depth > entry.depth) {  // Only update if depth is greater
        entry.bestMove = bestMove;
        entry.evaluation = evaluation;
        entry.depth = depth;
    }
}

bool getTranspositionTableEntry(const TranspositionTable& table, uint64_t hash,
                                TranspositionTableEntry& entry) {
    auto it = table.find(hash);
    if (it != table.end()) {
        entry = it->second;
        return true;
    }
    return false;
}
void incrementVisitCount(TranspositionTable& table, uint64_t hash) {
    auto it = table.find(hash);
    if (it != table.end()) {
        it->second.visitCount++;
    }
}
void decrementVisitCount(TranspositionTable& table, uint64_t hash) {
    auto it = table.find(hash);
    if (it != table.end() && it->second.visitCount > 0) {
        it->second.visitCount--;
    }
}