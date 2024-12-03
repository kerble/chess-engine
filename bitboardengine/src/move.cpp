#include "move.hpp"

/*
Bits [0-5]: fromSquare
Bits [6-11]: toSquare
Bits [12-15]: Castling or en passant.
*/
uint16_t encodeMove(int fromSquare, int toSquare, int special) {
    return (fromSquare & 0x3F) | ((toSquare & 0x3F) << 6) | (special << 12);
}

void decodeMove(uint16_t move, int& fromSquare, int& toSquare, int& special) {
    fromSquare = move & 0x3F;       // Extract bits [0-5]
    toSquare = (move >> 6) & 0x3F;  // Extract bits [6-11]
    special = (move >> 12) & 0xF;   // Extract bits [12-15]
}

// Convert uint16_t move to string representation
std::string moveToString(uint16_t move) {
    int fromSquare, toSquare;
    int special;
    decodeMove(move, fromSquare, toSquare, special);

    std::ostringstream moveString;
    moveString << algebraicFromSquare(fromSquare) << algebraicFromSquare(toSquare);

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
        case CASTLING_KINGSIDE:
            moveString.str("O-O");
            break;
        case CASTLING_QUEENSIDE:
            moveString.str("O-O-O");
            break;
        case EN_PASSANT:
            moveString << "e.p.";
            break;
        default:
            break;  // No special move
    }

    return moveString.str();
}

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

    // If no match is found, this should not happen in normal circumstances
    throw std::runtime_error("Square mask does not match any piece bitboard");
}

void applyMove(BoardState& board, uint16_t move) {
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

    // Handle promotions
    if (special >= PROMOTION_QUEEN && special <= PROMOTION_BISHOP) {
        int promotionType = getPromotedPieceType(special, isWhite);
        uint64_t promotionBitboard = board.getBitboard(promotionType) | destMask;
        board.updateBitboard(promotionType, promotionBitboard);
        // Remove the pawn from the board
        fromPieceBitboard ^= sourceMask;  // remove the pawn
        board.updateBitboard(pieceType, fromPieceBitboard);
    } else {
        // Move the piece
        fromPieceBitboard ^= sourceMask;  // Remove from source
        fromPieceBitboard |= destMask;    // Add to destination
        board.updateBitboard(pieceType, fromPieceBitboard);
    }
    // Handle captures
    if (special == EN_PASSANT) {
        int captureSquare = toSquare + (isWhite ? -8 : 8);
        uint64_t enemyPawnMask = (1ULL << captureSquare);
        int enemyPawnType = isWhite ? BLACK_PAWNS : WHITE_PAWNS;
        uint64_t enemyPawnBitboard = board.getBitboard(enemyPawnType) ^ enemyPawnMask;
        board.updateBitboard(enemyPawnType, enemyPawnBitboard);
        capture = true;
    } else if (board.getOccupancy(!isWhite) & destMask) {
        int capturedType = findPieceType(board, destMask, !isWhite);
        uint64_t capturedBitboard = board.getBitboard(capturedType);
        capturedBitboard ^= destMask;  // remove the bit
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

        rookBitboard ^= rookFromMask;  // Remove rook from its initial position
        rookBitboard |= rookToMask;    // Place rook in its new position
        board.updateBitboard(isWhite ? WHITE_ROOKS : BLACK_ROOKS, rookBitboard);
    }

    // Handle castling rights
    if (pieceType == WHITE_KINGS || pieceType == BLACK_KINGS) {
        // King moves, clear both castling rights for this side
        board.revokeAllCastlingRights(board, isWhite);
    }

    if (pieceType == WHITE_ROOKS || pieceType == BLACK_ROOKS) {
        // Rook moves, clear corresponding castling right
        if (fromSquare == (isWhite ? 0 : 56)) {  // Queenside rook
            board.revokeQueensideCastlingRights(board, isWhite);
        } else if (fromSquare == (isWhite ? 7 : 63)) {  // Kingside rook
            board.revokeKingsideCastlingRights(board, isWhite);
        }
    }

    // Update en passant state
    if (special == DOUBLE_PAWN_PUSH) {
        int enemy_pawn_index = isWhite ? BLACK_PAWNS : WHITE_PAWNS;
        uint64_t enemyPawnBitboard = board.getBitboard(enemy_pawn_index);
        int epSquare = isWhite ? toSquare - 8 : toSquare + 8;
        uint64_t adjacentMask = 0;

        // Not on the left edge
        if (epSquare % 8 != 0) adjacentMask |= (1ULL << (epSquare - 1));

        // Not on the right edge
        if (epSquare % 8 != 7) adjacentMask |= (1ULL << (epSquare + 1));
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

    if (pieceType == WHITE_PAWNS || pieceType == BLACK_PAWNS) pawn_move = true;
    // Update counters
    int halfmove = board.getHalfmoveClock();
    halfmove = capture || pawn_move || revoked_castling_rights ? 0 : halfmove + 1;
    int fullmove = board.getFullmoveNumber();
    if (!isWhite) fullmove += 1;

    board.setMoveCounters(halfmove, fullmove);
    board.flipTurn();
}


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
    undoState.moveCounter = board.getMoveCounter();
    undoState.move = move;
    return undoState;
}

void undoMove(BoardState& board, const MoveUndo& undoState) {
    // Swap the turn back
    board.flipTurn();

    // Restore moved piece
    board.updateBitboard(undoState.from_piece_type, undoState.fromBitboard);

    // Restore captured piece if there was a capture
    if (undoState.capture) {
        uint64_t capturedBitboard = undoState.capturedBitboard;
        board.updateBitboard(undoState.captured_piece_type, capturedBitboard);
    }

    int fromSquare, toSquare, special;
    decodeMove(undoState.move, fromSquare, toSquare, special);

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
    }

    // Undo promotions
    if (special >= PROMOTION_QUEEN && special <= PROMOTION_BISHOP) {
        // Remove the promoted piece
        board.updateBitboard(undoState.promotedPieceType, undoState.promotedBitboard);
    }

    // Restore metadata
    board.setEnPassant(undoState.enPassantState);
    board.setCastlingRights(undoState.castlingRights);
    board.setMoveCounters(undoState.halfMoveClock, undoState.moveCounter);
}
