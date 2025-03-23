#include "move.hpp"

/**
 * Converts a uint16_t move into its string representation.
 *
 * - Decodes the move into its components.
 * - Converts the from and to squares into algebraic notation.
 * - Appends promotion piece notation if applicable.
 *
 * @param move The encoded move.
 * @return The UCI string representation of the move.
 */
std::string moveToString(uint16_t move) {
    int fromSquare, toSquare;
    int special;
    decodeMove(move, fromSquare, toSquare, special);

    std::ostringstream moveString;
    moveString << squareToAlgebraic(fromSquare) << squareToAlgebraic(toSquare);

    // Append promotion character or special move indicator if applicable
    switch (special) {
        case PROMOTION_QUEEN:
            moveString << "q";
            break;
        case PROMOTION_KNIGHT:
            moveString << "n";
            break;
        case PROMOTION_ROOK:
            moveString << "r";
            break;
        case PROMOTION_BISHOP:
            moveString << "b";
            break;
        case EN_PASSANT:
            moveString << "e.p.";
            break;
        default:
            break;  // No special move
    }

    return moveString.str();
}

/**
 * Stores the necessary board state data before making a move.
 *
 * - Captures castling rights, en passant square, move counters, and captured piece.
 * - Used for move undo functionality.
 *
 * @param board The current board state.
 * @param move The move being applied.
 * @return A MoveUndo object storing relevant pre-move state data.
 */
MoveUndo storeUndoData(const BoardState& board, uint16_t move) {
    MoveUndo undoState;
    // Decode the move
    int fromSquare, toSquare, special;
    decodeMove(move, fromSquare, toSquare, special);

    // Retrieve relevant bitboards
    uint64_t sourceMask = (1ULL << fromSquare);
    uint64_t destMask = (1ULL << toSquare);
    bool isWhite = board.getTurn();

    // Find the source piece type
    undoState.from_piece_type = findPieceType(board, sourceMask, isWhite);

    // Save the source piece's bitboard
    undoState.fromBitboard = board.getBitboard(undoState.from_piece_type);

    // Save captured piece details if a capture is happening
    uint64_t enemyOccupancy = board.getOccupancy(!isWhite);
    undoState.capture = enemyOccupancy & destMask;

    // En passant
    if (special == EN_PASSANT) {
        int enemy_pawn_index = isWhite ? BLACK_PAWNS : WHITE_PAWNS;
        uint64_t enemy_pawn_bitboard = board.getBitboard(enemy_pawn_index);

        undoState.captured_piece_type = enemy_pawn_index;
        undoState.capturedBitboard = enemy_pawn_bitboard;
        undoState.capture = true;

    } else if (undoState.capture) {
        // Find the captured piece type
        undoState.captured_piece_type = findPieceType(board, destMask, !isWhite);

        // Save the captured piece's bitboard
        undoState.capturedBitboard = board.getBitboard(undoState.captured_piece_type);
    } else {
        undoState.captured_piece_type = -1;
        undoState.capturedBitboard = 0;
    }

    // Save promoted-piece information
    if (special >= PROMOTION_QUEEN && special <= PROMOTION_BISHOP) {
        undoState.promotedPieceType = getPromotedPieceType(special, isWhite);
        undoState.promotedBitboard = board.getBitboard(undoState.promotedPieceType);
    }

    // Store additional state
    undoState.enPassantState = board.getEnPassant();
    undoState.castlingRights = board.getCastlingRights();
    undoState.halfMoveClock = board.getHalfmoveClock();
    undoState.moveCounter = board.getFullmoveNumber();
    undoState.move = move;
    return undoState;
}

/**
 * Applies a move to the board and updates necessary game state data.
 *
 * - Decodes and executes the move.
 * - Updates piece bitboards, occupancy bitboards, and Zobrist hash.
 * - Handles special moves: captures, promotions, castling, and en passant.
 * - Updates move counters and flips the turn.
 *
 * @param board The board state to modify.
 * @param move The move to apply.
 * @return A MoveUndo object containing data to revert the move if needed.
 */
MoveUndo applyMove(BoardState& board, uint16_t move) {
    MoveUndo moveData = storeUndoData(board, move);
    uint64_t zobristHash = board.getZobristHash();
    // Decode the move
    int fromSquare, toSquare, special;
    decodeMove(move, fromSquare, toSquare, special);

    uint64_t sourceMask = (1ULL << fromSquare);
    uint64_t destMask = (1ULL << toSquare);
    bool isWhite = board.getTurn();
    bool capture = false;
    bool pawn_move = false;
    bool revoked_castling_rights = false;

    // Identify the moving piece and its bitboard
    int pieceType = findPieceType(board, sourceMask, isWhite);

    uint64_t fromPieceBitboard = findBitboard(board, fromSquare, isWhite);
    int enemyRooks = isWhite ? BLACK_ROOKS : WHITE_ROOKS;
    int enemyKingSideCorner = isWhite ? 63 : 7;
    int enemyQueenSideCorner = isWhite ? 56 : 0;
    //If we're capturing a rook in the corner
    if(destMask & board.getBitboard(enemyRooks)){
        if(toSquare == enemyKingSideCorner){
            board.revokeKingsideCastlingRights(!isWhite);
        }
        else if (toSquare == enemyQueenSideCorner){
            board.revokeQueensideCastlingRights(!isWhite);
        }
    }

    // Remove the piece from the old square and add it to the new one
    // If promoting, remove the pawn and add the promoted piece.
    if (special >= PROMOTION_QUEEN && special <= PROMOTION_BISHOP) { 
        int promotionType = getPromotedPieceType(special, isWhite);
        uint64_t promotionBitboard = board.getBitboard(promotionType) | destMask;
        zobristHash ^= zobristTable[promotionType][toSquare];
        board.updateBitboard(promotionType, promotionBitboard);

        // Remove the pawn from the board
        zobristHash ^= zobristTable[pieceType][fromSquare];
        fromPieceBitboard ^= sourceMask;  // remove the pawn
        board.updateBitboard(pieceType, fromPieceBitboard);
    }
    else {
        // Move the piece
        fromPieceBitboard ^= sourceMask;  // Remove from source
        fromPieceBitboard |= destMask;    // Add to destination

        zobristHash ^= zobristTable[pieceType][fromSquare];
        zobristHash ^= zobristTable[pieceType][toSquare];

        board.updateBitboard(pieceType, fromPieceBitboard);
    }
    // Handle captures
    if (special == EN_PASSANT) {
        int captureSquare = toSquare + (isWhite ? -8 : 8);
        uint64_t enemyPawnMask = (1ULL << captureSquare);
        int enemyPawnType = isWhite ? BLACK_PAWNS : WHITE_PAWNS;
        uint64_t enemyPawnBitboard = board.getBitboard(enemyPawnType) ^ enemyPawnMask;

        zobristHash ^= zobristTable[enemyPawnType][toSquare];
        board.updateBitboard(enemyPawnType, enemyPawnBitboard);
        capture = true;
    } else if (board.getOccupancy(!isWhite) & destMask) {
        int capturedType = findPieceType(board, destMask, !isWhite);
        uint64_t capturedBitboard = board.getBitboard(capturedType);
        capturedBitboard ^= destMask;  // remove the bit
        zobristHash ^= zobristTable[capturedType][toSquare];
        board.updateBitboard(capturedType, capturedBitboard);
        capture = true;
    }

    // Handle castling
    if (special == CASTLING_KINGSIDE || special == CASTLING_QUEENSIDE) {
        int rookFromSquare, rookToSquare;
        if (special == CASTLING_KINGSIDE) {
            rookFromSquare = isWhite ? 7 : 63;
            rookToSquare = toSquare - 1;
        } else {
            rookFromSquare = isWhite ? 0 : 56;
            rookToSquare = toSquare + 1;
        }
        uint64_t rookFromMask = (1ULL << rookFromSquare);
        uint64_t rookToMask = (1ULL << rookToSquare);
        uint64_t rookBitboard = board.getBitboard(isWhite ? WHITE_ROOKS : BLACK_ROOKS);

        zobristHash ^= zobristTable[isWhite ? WHITE_ROOKS : BLACK_ROOKS][rookFromSquare];
        zobristHash ^= zobristTable[isWhite ? WHITE_ROOKS : BLACK_ROOKS][rookToSquare];

        rookBitboard ^= rookFromMask;  // Remove rook from its initial position
        rookBitboard |= rookToMask;    // Place rook in its new position
        board.updateBitboard(isWhite ? WHITE_ROOKS : BLACK_ROOKS, rookBitboard);
    }

    uint64_t oldCastlingRights = board.getCastlingRights();
    zobristHash ^= zobristCastling[oldCastlingRights];

    // Handle castling rights
    if (pieceType == WHITE_KINGS || pieceType == BLACK_KINGS) {
        // King moves, clear both castling rights for this side
        board.revokeAllCastlingRights(isWhite);
    }

    if (pieceType == WHITE_ROOKS || pieceType == BLACK_ROOKS) {
        // Rook moves, clear corresponding castling right
        if (fromSquare == (isWhite ? 0 : 56)) {  // Queenside rook
            board.revokeQueensideCastlingRights(isWhite);
        } else if (fromSquare == (isWhite ? 7 : 63)) {  // Kingside rook
            board.revokeKingsideCastlingRights(isWhite);
        }
    }

    uint64_t newCastlingRights = board.getCastlingRights();
    zobristHash ^= zobristCastling[newCastlingRights];

    int oldEnPassantSquare = board.getEnPassant();
    if (oldEnPassantSquare != NO_EN_PASSANT) {
        int oldEnPassantFile = oldEnPassantSquare % 8;
        zobristHash ^= zobristEnPassant[oldEnPassantFile];
    }

    // Update en passant state
    if (special == DOUBLE_PAWN_PUSH) {
        int enemy_pawn_index = isWhite ? BLACK_PAWNS : WHITE_PAWNS;
        uint64_t enemyPawnBitboard = board.getBitboard(enemy_pawn_index);
        int epSquare = isWhite ? toSquare - 8 : toSquare + 8;
        uint64_t adjacentMask = 0;

        // Not on the left edge
        if (epSquare % 8 != 0) adjacentMask |= (1ULL << (toSquare - 1));

        // Not on the right edge
        if (epSquare % 8 != 7) adjacentMask |= (1ULL << (toSquare + 1));
        // If there's at least one enemy pawn adjacent, set the en passant square
        if (adjacentMask & enemyPawnBitboard) {
            board.setEnPassant(epSquare);
        } else {
            board.setEnPassant(NO_EN_PASSANT);
        }
    } else {
        // Clear en passant state for all other moves
        board.setEnPassant(NO_EN_PASSANT);
    }
    
    int newEnPassantSquare = board.getEnPassant();
    if (newEnPassantSquare != NO_EN_PASSANT) {
        int newEnPassantFile = newEnPassantSquare % 8;
        zobristHash ^= zobristEnPassant[newEnPassantFile];
    }

    if (pieceType == WHITE_PAWNS || pieceType == BLACK_PAWNS) pawn_move = true;
    // Update counters
    int halfmove = board.getHalfmoveClock();
    halfmove = capture || pawn_move || revoked_castling_rights ? 0 : halfmove + 1;
    int fullmove = board.getFullmoveNumber();
    if (!isWhite) fullmove += 1;

    board.setMoveCounters(halfmove, fullmove);
    zobristHash ^= zobristSideToMove;

    board.flipTurn();
    board.setZobristHash(zobristHash);

    return moveData;
}

/**
 * Restores the board to its previous state by undoing a move.
 *
 * - Restores captured pieces, castling rights, en passant state, and move counters.
 * - Reverts piece positions and bitboards.
 * - Updates Zobrist hash accordingly.
 * - Flips the turn back to the previous player.
 *
 * @param board The board state to modify.
 * @param undoState The stored state used to revert the move.
 */
void undoMove(BoardState& board, const MoveUndo& undoState) {
    // Retrieve the current Zobrist hash
    uint64_t zobristHash = board.getZobristHash();

    // Flip the turn back
    board.flipTurn();
    zobristHash ^= zobristSideToMove;  // Reverse turn flip in Zobrist hash

    // Restore moved piece to its original square
    int fromSquare, toSquare, special;
    decodeMove(undoState.move, fromSquare, toSquare, special);

    // Restore captured piece if there was a capture
    if (undoState.capture) {
        zobristHash ^= zobristTable[undoState.captured_piece_type][toSquare];  // Add captured piece back
        board.updateBitboard(undoState.captured_piece_type, undoState.capturedBitboard);
    }

    // Handle special logic for castling
    if (special == CASTLING_KINGSIDE || special == CASTLING_QUEENSIDE) {
        int rookFrom, rookTo;
        if (special == CASTLING_KINGSIDE) {
            rookFrom = (board.getTurn() ? 7 : 63);
            rookTo = (board.getTurn() ? 5 : 61);
        } else {
            rookFrom = (board.getTurn() ? 0 : 56);
            rookTo = (board.getTurn() ? 3 : 59);
        }

        // Move the rook back to its original square
        int alliedRookIndex = board.getTurn() ? WHITE_ROOKS : BLACK_ROOKS;
        uint64_t rookBitboard = board.getBitboard(alliedRookIndex);
        rookBitboard ^= (1ULL << rookTo);    // Remove rook from castled-to square
        rookBitboard |= (1ULL << rookFrom);  // Place rook back at original square
        board.updateBitboard(alliedRookIndex, rookBitboard);

        // Update Zobrist hash for the rook's movement
        zobristHash ^= zobristTable[alliedRookIndex][rookTo];  // Remove rook from castled-to square
        zobristHash ^= zobristTable[alliedRookIndex][rookFrom];  // Add rook back to original square
    }

    // Undo promotions
    // Add back to source   
    zobristHash ^= zobristTable[undoState.from_piece_type][fromSquare];
    if (special >= PROMOTION_QUEEN && special <= PROMOTION_BISHOP) {
        // Remove promoted piece
        zobristHash ^= zobristTable[undoState.promotedPieceType][toSquare];
        board.updateBitboard(undoState.promotedPieceType, undoState.promotedBitboard);
        int pawnType = board.getTurn() ? WHITE_PAWNS : BLACK_PAWNS;
        uint64_t pawnBitboard = board.getBitboard(pawnType);
        pawnBitboard |= (1ULL << fromSquare);
        board.updateBitboard(pawnType, pawnBitboard);
    }
    else{
        zobristHash ^= zobristTable[undoState.from_piece_type][toSquare];    // Remove from destination
        board.updateBitboard(undoState.from_piece_type, undoState.fromBitboard);
    }
    // Restore metadata
    if (undoState.enPassantState != NO_EN_PASSANT) {
        int enPassantFile = undoState.enPassantState % 8;
        zobristHash ^= zobristEnPassant[enPassantFile];  // Add en passant square back
    }
    int currentEnPassant = board.getEnPassant();
    if (currentEnPassant != NO_EN_PASSANT) {
        int enPassantFile = currentEnPassant % 8;
        zobristHash ^= zobristEnPassant[enPassantFile];  // Remove current en passant square
    }
    board.setEnPassant(undoState.enPassantState);

    // Restore castling rights
    zobristHash ^= zobristCastling[board.getCastlingRights()];  // Remove current castling rights
    zobristHash ^= zobristCastling[undoState.castlingRights];   // Add back old castling rights
    board.setCastlingRights(undoState.castlingRights);

    // Restore move counters
    board.setMoveCounters(undoState.halfMoveClock, undoState.moveCounter);

    // Update the Zobrist hash
    board.setZobristHash(zobristHash);
}

/**
 * Decodes a uint16_t move into its individual components.
 *
 * - Extracts source square, destination square, and special move flags.
 * - Bits [0-5]: fromSquare
 * - Bits [6-11]: toSquare
 * - Bits [12-15]: Castling or en passant.
 * 
 * @param move The encoded move.
 * @param fromSquare Output parameter for the from-square.
 * @param toSquare Output parameter for the to-square.
 * @param special Output parameter for the special move flag.
 */
uint16_t encodeMove(int fromSquare, int toSquare, int special) {
    return (fromSquare & 0x3F) | ((toSquare & 0x3F) << 6) | (special << 12);
}

/**
 * Decodes a uint16_t move into its individual components.
 *
 * - Extracts source square, destination square, and special move flags.
 *
 * @param move The encoded move.
 * @param fromSquare Output parameter for the from-square.
 * @param toSquare Output parameter for the to-square.
 * @param special Output parameter for the special move flag.
 */
void decodeMove(uint16_t move, int& fromSquare, int& toSquare, int& special) {
    fromSquare = move & 0x3F;       // Extract bits [0-5]
    toSquare = (move >> 6) & 0x3F;  // Extract bits [6-11]
    special = (move >> 12) & 0xF;   // Extract bits [12-15]
}

/**
 * Encodes a UCI move string into uint16_t move format.
 *
 * - Parses the UCI move string (e.g., "e2e4").
 * - Converts square coordinates to indices.
 * - Handles special move encoding (e.g., castling, promotions).
 *
 * @param board The current board state.
 * @param uciMove The UCI move string.
 * @return The encoded uint16_t move.
 */
uint16_t encodeUCIMove(BoardState& board, const std::string& uciMove) {
    if (uciMove.length() < 4 || uciMove.length() > 5) {
        throw std::invalid_argument("Invalid UCI move format: " + uciMove);
    }

    int fromSquare = algebraicToSquare(uciMove.substr(0, 2));  // e.g., "e2"
    int toSquare = algebraicToSquare(uciMove.substr(2, 2));    // e.g., "e4"

    int special = SPECIAL_NONE;

    // Handle special cases
    if (uciMove.length() == 5) {  // Promotion move, e.g., e7e8q
        char promotionPiece = uciMove[4];
        switch (promotionPiece) {
            case 'q':
                special = PROMOTION_QUEEN;
                break;
            case 'n':
                special = PROMOTION_KNIGHT;
                break;
            case 'r':
                special = PROMOTION_ROOK;
                break;
            case 'b':
                special = PROMOTION_BISHOP;
                break;
            default:
                throw std::invalid_argument("Invalid promotion piece: " +
                                            std::string(1, promotionPiece));
        }
    } else {
        // Detect other special moves based on the board state
        if (isCastlingMove(fromSquare, toSquare, board)) {
            special = isKingsideCastling(fromSquare, toSquare, board) ? CASTLING_KINGSIDE
                                                                      : CASTLING_QUEENSIDE;
        } else if (isEnPassantMove(board, fromSquare, toSquare)) {
            special = EN_PASSANT;
        } else if (isDoublePawnPush(board, fromSquare, toSquare)) {
            special = DOUBLE_PAWN_PUSH;
        }
    }

    return encodeMove(fromSquare, toSquare, special);
}

/**
 * Determines if a given move is a kingside castling move.
 *
 * - Validates if the move is a castling move.
 * - Checks if the destination square corresponds to kingside castling.
 *
 * @param fromSquare The square from which the move originates.
 * @param toSquare The square to which the move is made.
 * @param board The board state for validation.
 * @return True if the move is kingside castling, otherwise false.
 */
bool isKingsideCastling(int fromSquare, int toSquare, const BoardState& board) {
    // Use isCastlingMove to validate it's a castling move
    if (!isCastlingMove(fromSquare, toSquare, board)) {
        return false;
    }

    // Check if the destination square is g1 or g8
    return toSquare == 6 || toSquare == 62;
}

/**
 * Determines if a given move is a castling move.
 *
 * - Validates if the move follows standard castling patterns.
 * - Ensures the moving piece is a king.
 *
 * @param fromSquare The square from which the move originates.
 * @param toSquare The square to which the move is made.
 * @param board The board state for validation.
 * @return True if the move is a castling move, otherwise false.
 */
bool isCastlingMove(int fromSquare, int toSquare, const BoardState& board) {
    // Check if the move is one of the standard castling patterns
    if (!((fromSquare == 4 && (toSquare == 6 || toSquare == 2)) ||
          (fromSquare == 60 && (toSquare == 62 || toSquare == 58)))) {
        return false;
    }

    // Verify that the piece at fromSquare is a king
    uint64_t fromMask = (1ULL << fromSquare);
    int pieceType = findPieceType(board, fromMask, board.getTurn());
    return pieceType == WHITE_KINGS || pieceType == BLACK_KINGS;
}

/**
 * Determines if a move is a double pawn push.
 *
 * - Ensures the moving piece is a pawn.
 * - Checks if the move advances exactly two squares forward.
 *
 * @param board The current board state.
 * @param fromSquare The square from which the move originates.
 * @param toSquare The square to which the move is made.
 * @return True if the move is a double pawn push, otherwise false.
 */
bool isDoublePawnPush(const BoardState& board, int fromSquare, int toSquare) {
    // Ensure the moving piece is a pawn
    uint64_t fromMask = (1ULL << fromSquare);
    int pieceType = findPieceType(board, fromMask, board.getTurn());
    if (pieceType != WHITE_PAWNS && pieceType != BLACK_PAWNS) {
        return false;
    }

    // Check if the move is a two-square forward push
    int rankDifference = toSquare / 8 - fromSquare / 8;
    return (rankDifference == 2 || rankDifference == -2);
}

/**
 * Determines if a move is an en passant capture.
 *
 * - Ensures the moving piece is a pawn.
 * - Checks if the move targets the en passant square.
 *
 * @param board The current board state.
 * @param fromSquare The square from which the move originates.
 * @param toSquare The square to which the move is made.
 * @return True if the move is an en passant capture, otherwise false.
 */
bool isEnPassantMove(const BoardState& board, int fromSquare, int toSquare) {
    // Ensure the moving piece is a pawn
    uint64_t fromMask = (1ULL << fromSquare);
    int pieceType = findPieceType(board, fromMask, board.getTurn());
    if (pieceType != WHITE_PAWNS && pieceType != BLACK_PAWNS) {
        return false;
    }

    // Check if the move targets the en passant square
    return toSquare == board.getEnPassant();
}

/**
 * Identifies the type of piece occupying a given square.
 *
 * - Checks the bitboards of all piece types for the current player.
 * - Returns the corresponding piece index if found.
 * - If no matching piece is found, the function terminates the program.
 *
 * @param board The current board state.
 * @param squareMask A bitmask representing the target square.
 * @param isWhite True if checking for a white piece, false for black.
 * @return The index of the piece occupying the square.
 */
int findPieceType(const BoardState& board, uint64_t squareMask, bool isWhite) {
    // Piece indices for the board's bitboards
    const int pieceOffset = isWhite ? 0 : 6;  // White: 0-5, Black: 6-11

    // Loop through the bitboards for all piece types
    for (int i = 0; i < 6; ++i) {
        int pieceIndex = pieceOffset + i;
        if (board.getBitboard(pieceIndex) & squareMask) {
            return pieceIndex;  // Return the index of the matching piece
        }
    }
    std::cout << "crashing inside of findPieceType on this board:\n";
    std::cout << board << std::endl;
    std::cout << bitboardToBinaryString(squareMask) << std::endl;
    std::string side = isWhite ? "white" : "black";
    std::cout << "Looking for " << side << " pieces\n";
    // If no match is found, terminate. This should not happen in normal circumstances
    std::terminate();
    throw std::runtime_error("Square mask does not match any piece bitboard");
}

/**
 * Finds the bitboard corresponding to the piece on a given square.
 *
 * - Iterates through the bitboards of all pieces for the specified color.
 * - Identifies the bitboard that contains the given square.
 * - Throws an exception if no piece is found on the square.
 *
 * @param board The current board state.
 * @param square The square index to check.
 * @param isWhite True if checking for a white piece, false for black.
 * @return The bitboard containing the piece on the specified square.
 * @throws std::runtime_error If no piece is found on the given square.
 */
uint64_t findBitboard(const BoardState& board, int square, bool isWhite) {
    uint64_t targetBit = (1ULL << square);

    int start = isWhite ? WHITE_PAWNS : BLACK_PAWNS;
    int end = isWhite ? WHITE_KINGS : BLACK_KINGS;

    for (int pieceType = start; pieceType <= end; ++pieceType) {
        uint64_t bitboard = board.getBitboard(pieceType);
        if (bitboard & targetBit) {
            return bitboard;  // Found the piece
        }
    }

    throw std::runtime_error("No piece found on the specified square!");
}

/**
 * Determines the piece type for a pawn promotion.
 *
 * - Converts the special move flag into the corresponding piece index.
 * - Supports queen, knight, bishop, and rook promotions.
 * - Throws an exception if an invalid promotion type is provided.
 *
 * @param special The special move flag indicating the promotion type.
 * @param isWhite True if the promotion applies to a white piece, false for black.
 * @return The piece index for the promoted piece.
 * @throws std::runtime_error If an invalid promotion type is specified.
 */
int getPromotedPieceType(int special, bool isWhite) {
    switch (special) {
        case PROMOTION_QUEEN:
            return isWhite ? WHITE_QUEENS : BLACK_QUEENS;
        case PROMOTION_KNIGHT:
            return isWhite ? WHITE_KNIGHTS : BLACK_KNIGHTS;
        case PROMOTION_BISHOP:
            return isWhite ? WHITE_BISHOPS : BLACK_BISHOPS;
        case PROMOTION_ROOK:
            return isWhite ? WHITE_ROOKS : BLACK_ROOKS;
    }
    throw std::runtime_error("Invalid special when trying to get the promoted piece type");
}