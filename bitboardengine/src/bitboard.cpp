#include "bitboard.hpp"


uint64_t zobristTable[12][64];
uint64_t zobristCastling[16];
uint64_t zobristEnPassant[8];
uint64_t zobristSideToMove;

/**
 * Converts an algebraic notation square (e.g., "e4") into a 0-63 board index.
 *
 * - Files ('a' to 'h') are mapped to indices (0 to 7).
 * - Ranks ('1' to '8') are mapped to indices (0 to 7).
 * - The board is indexed from 0 (a1) to 63 (h8).
 * - Returns -1 if the input is invalid.
 *
 * @param algebraic A two-character string representing the square in algebraic notation.
 * @return The corresponding board index (0-63) or -1 if invalid.
 */
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

/**
 * Converts a 0-63 board index into algebraic notation (e.g., 36 → "e4").
 *
 * - Files (0 to 7) map to ('a' to 'h').
 * - Ranks (0 to 7) map to ('1' to '8').
 * - Throws an exception if the input index is out of bounds.
 *
 * @param square The board index (0-63).
 * @return A two-character string representing the square in algebraic notation.
 */
std::string squareToAlgebraic(int square) {
    if (square < 0 || square > 63) {
        throw std::out_of_range("Square must be between 0 and 63");
    }

    // Calculate file and rank
    char file = 'a' + (square % 8);  // File: 0 maps to 'a', 7 maps to 'h'
    char rank = '1' + (square / 8);  // Rank: 0 maps to '1', 7 maps to '8'

    return std::string(1, file) + rank;
}

/**
 * Converts a 64-bit bitboard into a human-readable binary string representation.
 *
 * - The output is an 8x8 grid where each bit (0 or 1) represents an occupied square.
 * - The highest rank (rank 8) is printed first, and the lowest rank (rank 1) is printed last.
 * - Files are displayed from left to right (a to h).
 *
 * @param bitboard A 64-bit integer representing a chess position.
 * @return A formatted string showing the bitboard as an 8x8 grid.
 */
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

/**
 * Updates the bitboard for a specific piece type.
 *
 * - Replaces the old bitboard with the new one.
 * - Updates the white and black occupancy bitboards accordingly.
 * - Calls `updateOccupancy()` to refresh the overall occupancy bitboard.
 * - Throws an exception if `pieceType` is out of the valid range (0-11).
 *
 * @param pieceType The index of the piece type (0-5 for white, 6-11 for black).
 * @param newBitboard The updated bitboard for the given piece type.
 */
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

/**
 * Retrieves the bitboard for a specific piece type.
 *
 * - Each piece type (0-11) corresponds to a specific bitboard.
 * - Throws an exception if `pieceType` is out of range.
 *
 * @param pieceType The index of the piece type (0-11).
 * @return The bitboard representing the given piece type.
 */
uint64_t BoardState::getBitboard(int pieceType) const {
    if (pieceType < 0 || pieceType >= 12) {
        throw std::invalid_argument("Invalid pieceType in getBitboard.");
        return 0;
    }
    return bitboards[pieceType];
}

/**
 * Sets the occupancy bitboards for white and black pieces.
 *
 * - Updates the `white_occupancy` and `black_occupancy` bitboards.
 * - Computes `all_occupancy`, which represents all occupied squares.
 *
 * @param white The bitboard representing white piece occupancy.
 * @param black The bitboard representing black piece occupancy.
 */
void BoardState::setOccupancy(uint64_t white, uint64_t black) {
    white_occupancy = white;
    black_occupancy = black;
    all_occupancy = white | black;  // All occupied squares (white + black pieces)
}

/**
 * Retrieves the occupancy bitboard for either white or black pieces.
 *
 * - Returns the bitboard of occupied squares for the specified color.
 *
 * @param isWhite A boolean indicating which occupancy to return (`true` for white, `false` for
 * black).
 * @return The bitboard representing occupied squares of the selected color.
 */
uint64_t BoardState::getOccupancy(bool isWhite) const {
    return isWhite ? white_occupancy : black_occupancy;
}

/**
 * Retrieves the bitboard of all occupied squares.
 *
 * - This bitboard represents the union of `white_occupancy` and `black_occupancy`.
 *
 * @return The bitboard containing all occupied squares on the board.
 */
uint64_t BoardState::getAllOccupancy() const { return all_occupancy; }

/**
 * Updates the occupancy bitboards for white, black, and all pieces.
 *
 * - Resets `white_occupancy` and `black_occupancy`.
 * - Aggregates all individual piece bitboards to compute white and black occupancy.
 * - Combines white and black occupancy bitboards into `all_occupancy`.
 */
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

/**
 * Sets the castling rights using a bitmask.
 *
 * - The bitmask represents castling rights for both sides:
 *   - Bit 0: White kingside
 *   - Bit 1: White queenside
 *   - Bit 2: Black kingside
 *   - Bit 3: Black queenside
 *
 * @param rights The bitmask representing castling rights.
 */
void BoardState::setCastlingRights(uint8_t rights) { castling_rights = rights; }

/**
 * Revokes kingside castling rights for the specified color.
 *
 * - Clears the bit corresponding to kingside castling.
 *
 * @param isWhite `true` to revoke white's kingside castling, `false` for black.
 */
void BoardState::revokeKingsideCastlingRights(bool isWhite) {
    castling_rights &= ~(isWhite ? 0b01 : 0b0100);
}

/**
 * Revokes queenside castling rights for the specified color.
 *
 * - Clears the bit corresponding to queenside castling.
 *
 * @param isWhite `true` to revoke white's queenside castling, `false` for black.
 */
void BoardState::revokeQueensideCastlingRights(bool isWhite) {
    castling_rights &= ~(isWhite ? 0b10 : 0b1000);
}

/**
 * Revokes all castling rights for the specified color.
 *
 * - Clears both kingside and queenside castling rights.
 *
 * @param isWhite `true` to revoke white's castling rights, `false` for black.
 */
void BoardState::revokeAllCastlingRights(bool isWhite) {
    castling_rights &= ~(isWhite ? 0b11 : 0b1100);
}

/**
 * Retrieves the current castling rights bitmask.
 *
 * - The bitmask contains castling rights for both white and black.
 *
 * @return The bitmask representing castling rights.
 */
uint8_t BoardState::getCastlingRights() const { return castling_rights; }

/**
 * Checks whether a player can castle kingside.
 *
 * - Determines if the kingside castling bit is set for the given color.
 *
 * @param isWhite `true` to check white's kingside rights, `false` for black.
 * @return `true` if the player can castle kingside, `false` otherwise.
 */
bool BoardState::canCastleKingside(bool isWhite) const {
    if (isWhite) {
        return castling_rights & (1ULL << 0);  // Check bit 0 (White kingside)
    } else {
        return castling_rights & (1ULL << 2);  // Check bit 2 (Black kingside)
    }
}

/**
 * Checks whether a player can castle queenside.
 *
 * - Determines if the queenside castling bit is set for the given color.
 *
 * @param isWhite `true` to check white's queenside rights, `false` for black.
 * @return `true` if the player can castle queenside, `false` otherwise.
 */
bool BoardState::canCastleQueenside(bool isWhite) const {
    if (isWhite) {
        return castling_rights & (1ULL << 1);  // Check bit 1 (White queenside)
    } else {
        return castling_rights & (1ULL << 3);  // Check bit 3 (Black queenside)
    }
}

/**
 * Sets the en passant square.
 *
 * - The en passant square is the target square where an en passant capture is possible.
 * - If no en passant capture is possible, use `NO_EN_PASSANT`.
 *
 * @param square The square index (0-63) where en passant is possible.
 */
void BoardState::setEnPassant(uint8_t square){ en_passant_square = square; }

/**
 * Retrieves the en passant square.
 *
 * - If en passant is possible, returns the target square index (0-63).
 * - If en passant is not possible, returns `NO_EN_PASSANT`.
 *
 * @return The en passant target square or `NO_EN_PASSANT`.
 */
uint8_t BoardState::getEnPassant() const { return en_passant_square; }

/**
 * Flips the turn between white and black.
 *
 * - Toggles `is_white_turn` to switch the active player.
 */
void BoardState::flipTurn(){ is_white_turn = !is_white_turn;}

/**
 * Sets the turn to the specified player.
 *
 * @param isWhiteTurn `true` if it's white's turn, `false` if it's black's turn.
 */
void BoardState::setTurn(bool isWhiteTurn) { is_white_turn = isWhiteTurn; }

/**
 * Retrieves the current turn.
 *
 * @return `true` if it's white's turn, `false` if it's black's turn.
 */
bool BoardState::getTurn() const { return is_white_turn; }

/**
 * Sets the halfmove clock and fullmove number.
 *
 * - The halfmove clock tracks the number of halfmoves since the last pawn move or capture.
 * - The fullmove number counts the total moves in the game, incrementing after black's move.
 *
 * @param halfmove The number of halfmoves since the last pawn move or capture.
 * @param fullmove The current fullmove number in the game.
 */
void BoardState::setMoveCounters(int halfmove, int fullmove) {
    halfmove_clock = halfmove;
    fullmove_number = fullmove;
}

/**
 * Retrieves the halfmove clock.
 *
 * - The halfmove clock tracks the number of halfmoves since the last pawn move or capture.
 *
 * @return The current halfmove clock value.
 */
int BoardState::getHalfmoveClock() const { return halfmove_clock; }

/**
 * Retrieves the fullmove number.
 *
 * - The fullmove number counts the total moves in the game, incrementing after black's move.
 *
 * @return The current fullmove number.
 */
int BoardState::getFullmoveNumber() const { return fullmove_number; }

/**
 * Retrieves the Zobrist hash of the current board position.
 *
 * - The Zobrist hash uniquely represents the board state for efficient lookup in transposition
 * tables.
 *
 * @return The Zobrist hash value.
 */
uint64_t BoardState::getZobristHash() const { return zobrist_hash; }

/**
 * Sets the Zobrist hash for the current board position.
 *
 * @param zobrist The new Zobrist hash value.
 */
void BoardState::setZobristHash(uint64_t zobrist){ zobrist_hash = zobrist; }

/**
 * Equality operator for `BoardState`.
 *
 * - Compares two `BoardState` objects to determine if they represent the same position.
 *
 * @param lhs The left-hand side `BoardState` object.
 * @param rhs The right-hand side `BoardState` object.
 * @return `true` if the board states are identical, `false` otherwise.
 */
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

/**
 * Overloads the `<<` operator for `BoardState` to print the board representation.
 *
 * - Outputs the current board position in a human-readable format.
 *
 * @param os The output stream.
 * @param board The `BoardState` object to print.
 * @return The modified output stream.
 */
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

/**
 * Converts a FEN character to the corresponding piece index.
 *
 * - White pieces: 'P', 'N', 'B', 'R', 'Q', 'K'
 * - Black pieces: 'p', 'n', 'b', 'r', 'q', 'k'
 *
 * @param piece The FEN character representing a piece.
 * @return The corresponding piece index in the bitboard array.
 * @throws std::invalid_argument If the character is not a valid FEN piece.
 */
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

/**
 * Parses castling rights from a FEN string into an integer bitmask.
 *
 * - 'K' (White kingside) → bit 0
 * - 'Q' (White queenside) → bit 1
 * - 'k' (Black kingside) → bit 2
 * - 'q' (Black queenside) → bit 3
 *
 * @param rights A string containing castling rights (e.g., "KQkq", "-").
 * @return A bitmask representing the available castling rights.
 */
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

/**
 * Parses a FEN string and initializes a `BoardState` object.
 *
 * - Sets up piece positions, castling rights, en passant target, and move counters.
 * - Updates the Zobrist hash after parsing.
 *
 * @param fen The FEN string representing the board state.
 * @return A `BoardState` object initialized from the FEN.
 * @throws std::invalid_argument If the FEN string is invalid.
 */
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

/**
 * Initializes the Zobrist hashing tables with random values.
 *
 * - Generates random hashes for piece-square positions.
 * - Generates random hashes for castling rights, en passant files, and side to move.
 * - Uses a fixed seed for reproducibility.
 */
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

/**
 * Computes the Zobrist hash for the given board state.
 *
 * - Uses precomputed Zobrist tables to generate a unique hash.
 * - Includes piece positions, castling rights, en passant targets, and side to move.
 *
 * @param board The `BoardState` to compute the hash for.
 * @return A 64-bit Zobrist hash representing the board position.
 */
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

/**
 * Updates the transposition table with a new entry.
 *
 * - Only updates if the new depth is greater than the stored depth.
 * - Stores the best move, evaluation, depth, and evaluation type.
 *
 * @param table The transposition table to update.
 * @param hash The Zobrist hash of the board position.
 * @param bestMove The best move found for this position.
 * @param evaluation The evaluation score of the position.
 * @param depth The depth at which this evaluation was obtained.
 * @param eval_type The type of evaluation (exact, lower bound, upper bound).
 */
void updateTranspositionTable(TranspositionTable& table, uint64_t hash, uint16_t bestMove,
                              double evaluation, int depth, int eval_type) {
    auto& entry = table[hash];  // Access or create the entry
    // entry.visitCount++;
    if (depth > entry.depth) {  // Only update if depth is greater
        entry.bestMove = bestMove;
        entry.evaluation = evaluation;
        entry.depth = depth;
        entry.eval_type = eval_type;
    }
}

/**
 * Retrieves an entry from the transposition table.
 *
 * - Searches for the given hash in the table.
 * - If found, copies the entry and returns true.
 * - If not found, returns false.
 *
 * @param table The transposition table.
 * @param hash The Zobrist hash of the board position.
 * @param entry Output parameter to store the found entry.
 * @return True if the entry was found, false otherwise.
 */
bool getTranspositionTableEntry(const TranspositionTable& table, uint64_t hash,
                                TranspositionTableEntry& entry) {
    auto it = table.find(hash);
    if (it != table.end()) {
        entry = it->second;
        return true;
    }
    return false;
}

/**
 * Increments the visit count of a transposition table entry.
 *
 * - If the hash exists in the table, increments its visit count.
 * - This is used for three-fold repetition detection.
 * 
 * @param table The transposition table.
 * @param hash The Zobrist hash of the board position.
 */
void incrementVisitCount(TranspositionTable& table, uint64_t hash) {
    auto it = table.find(hash);
    if (it != table.end()) {
        it->second.visitCount++;
    }
}

/**
 * Decrements the visit count of a transposition table entry.
 *
 * - If the hash exists and the visit count is greater than 0, decrements it.
 *
 * @param table The transposition table.
 * @param hash The Zobrist hash of the board position.
 */
void decrementVisitCount(TranspositionTable& table, uint64_t hash) {
    auto it = table.find(hash);
    if (it != table.end() && it->second.visitCount > 0) {
        it->second.visitCount--;
    }
}