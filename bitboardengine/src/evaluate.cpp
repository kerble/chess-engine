#include "evaluate.hpp"

uint64_t findBitboard(BoardState& board, int square, bool isWhite) {
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

int findPieceType(BoardState& board, uint64_t squareMask, bool isWhite) {
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
MoveUndo applyMove(BoardState& board, uint16_t move) {
    MoveUndo undoState;
    undoState.move = move;

    // Decode the move
    int fromSquare, toSquare, special;
    decodeMove(move, fromSquare, toSquare, special);

    // Retrieve relevant bitboards
    uint64_t sourceMask = (1ULL << fromSquare);
    uint64_t destMask = (1ULL << toSquare);
    bool isWhite = board.getTurn();
    bool is_pawn_move = board.getBitboard(isWhite ? WHITE_PAWNS : BLACK_PAWNS) & sourceMask;
    bool revoked_castling_rights = false;
    uint64_t fromPieceBitboard = findBitboard(board, fromSquare, isWhite);
    undoState.fromBitboard = fromPieceBitboard;

    // Prepare undo state
    undoState.enPassantState = board.getEnPassant();
    undoState.castlingRights = board.getCastlingRights();
    undoState.halfMoveClock = board.getHalfmoveClock();
    undoState.moveCounter = board.getMoveCounter();
    undoState.capture = false;
    undoState.captured_piece_type = -1;  // Default: no capture

    int halfmove = board.getHalfmoveClock();
    int fullmove = board.getMoveCounter();
    // Handle capture
    uint64_t enemyOccupancy = isWhite ? board.getBlackOccupancy() : board.getWhiteOccupancy();

    // Grab enemy pawn bitboard, useful for capturing en passant and also setting en passant
    int enemy_pawn_index = isWhite ? BLACK_PAWNS : WHITE_PAWNS;
    uint64_t enemyPawnBitboard = board.getBitboard(enemy_pawn_index);

    // Handle castling rights
    if (sourceMask & board.getBitboard(isWhite ? WHITE_KINGS : BLACK_KINGS)) {
        revoked_castling_rights = true;
        // King moves, clear both castling rights for this side
        undoState.castlingRights = board.getCastlingRights();
        board.setCastlingRights(board.getCastlingRights() & ~(isWhite ? 0b11 : 0b1100));
    }

    if (sourceMask & board.getBitboard(isWhite ? WHITE_ROOKS : BLACK_ROOKS)) {
        // Rook moves, clear corresponding castling right
        if (fromSquare == (isWhite ? 0 : 56)) {  // Queenside rook
            board.setCastlingRights(board.getCastlingRights() & ~(isWhite ? 0b10 : 0b1000));
            revoked_castling_rights = true;
        } else if (fromSquare == (isWhite ? 7 : 63)) {  // Kingside rook
            board.setCastlingRights(board.getCastlingRights() & ~(isWhite ? 0b01 : 0b0100));
            revoked_castling_rights = true;
        }
    }

    if (special == EN_PASSANT) {
        int captureSquare = toSquare + (isWhite ? -8 : 8);  // En passant target square

        // Store the captured piece before modification
        undoState.capturedBitboard = enemyPawnBitboard;
        undoState.captured_piece_type = enemy_pawn_index;
        enemyPawnBitboard ^= (1ULL << captureSquare);  // Remove the captured pawn
        board.updateBitboard(enemy_pawn_index,
                             enemyPawnBitboard);  // Update the board with the new bitboard

        undoState.capture = true;  // Mark that there was a capture

    } else if (enemyOccupancy & destMask) {
        // Loop through all enemy pieces to find the captured piece
        for (int pieceType = (isWhite ? BLACK_PAWNS : WHITE_PAWNS);
             pieceType <= (isWhite ? BLACK_QUEENS : WHITE_QUEENS); ++pieceType) {
            uint64_t enemyBitboard = board.getBitboard(pieceType);
            if (enemyBitboard & destMask) {
                // Capture the piece: store its bitboard before removing it
                undoState.capturedBitboard = enemyBitboard;
                undoState.captured_piece_type = pieceType;       // Store captured piece type
                enemyBitboard ^= destMask;                       // Remove the captured piece
                board.updateBitboard(pieceType, enemyBitboard);  // Update the board

                undoState.capture = true;  // Mark that there was a capture
                break;
            }
        }
    }

    // Handle promotions
    if (special >= PROMOTION_QUEEN && special <= PROMOTION_BISHOP) {
        fromPieceBitboard^= sourceMask;  // Remove the pawn

        // Add the promoted piece to the to bitboard and
        // the BoardState's bitboards

        // grab the piece index of what we're promoting to
        int promotedPieceIndex;
        int knights = isWhite ? WHITE_KNIGHTS : BLACK_KNIGHTS;
        switch(special){
            case PROMOTION_KNIGHT:
                promotedPieceIndex = knights;
                break;
            case PROMOTION_BISHOP:
                promotedPieceIndex = knights + 1; //bishops
                break;
            case PROMOTION_ROOK:
                promotedPieceIndex = knights + 2; //rooks
                break;
            case PROMOTION_QUEEN:
                promotedPieceIndex = knights + 3; //queens
                break;
        }

        // Save the bitboard containing the pieces the pawn is promoting to
        uint64_t promotedPieceBitboard = board.getBitboard(promotedPieceIndex);
        undoState.toBitboard = promotedPieceBitboard;

        // Add the pawn that has now promoted to the actual board.
        promotedPieceBitboard |= destMask;
        board.updateBitboard(promotedPieceIndex, promotedPieceBitboard);
    } else {
        fromPieceBitboard^= sourceMask;  // Remove from source
        fromPieceBitboard|= destMask;    // Add to destination
    }

    //remove the moved piece from its original square
    int pieceType = findPieceType(board, sourceMask, isWhite);
    board.updateBitboard(pieceType, fromPieceBitboard);


    
    if (special == CASTLING_KINGSIDE) {
        // h1 (7) for white, h8 (63) for black
        uint64_t rookStartBitboard = isWhite ? (1ULL << 7) : (1ULL << 63);

        // f1 (5) for white, f8 (61) for black
        uint64_t rookEndBitboard = isWhite ? (1ULL << 5) : (1ULL << 61);

        // Save original rook bitboard
        undoState.toBitboard = board.getBitboard(isWhite ? WHITE_ROOKS : BLACK_ROOKS);
        
        // Remove from starting square
        uint64_t updatedRookBitboard = undoState.toBitboard ^ rookStartBitboard;

        // Add to destination square
        updatedRookBitboard |= rookEndBitboard;
        board.updateBitboard(isWhite ? WHITE_ROOKS : BLACK_ROOKS, updatedRookBitboard);
    }
    else if (special == CASTLING_QUEENSIDE) {
        // a1 (0) for white, a8 (56) for black
        uint64_t rookStartBitboard = isWhite ? (1ULL << 0) : (1ULL << 56);

        // d1 (3) for white, d8 (59) for black
        uint64_t rookEndBitboard = isWhite ? (1ULL << 3) : (1ULL << 59);

        // Save original rook bitboard
        undoState.toBitboard = board.getBitboard(isWhite ? WHITE_ROOKS : BLACK_ROOKS);

        // Remove from starting square
        uint64_t updatedRookBitboard = undoState.toBitboard ^ rookStartBitboard;

        // Add to destination square
        updatedRookBitboard |= rookEndBitboard;
        board.updateBitboard(isWhite ? WHITE_ROOKS : BLACK_ROOKS, updatedRookBitboard);
    }


    // Update en passant state
    if (special == DOUBLE_PAWN_PUSH) {
        // Determine the en passant square (8 squares behind the destination)
        int epSquare = isWhite ? toSquare - 8 : toSquare + 8;

        // Check if there are enemy pawns adjacent to the en passant square
        uint64_t adjacentMask = 0;
        if (epSquare % 8 != 0) {  // Not on the left edge
            adjacentMask |= (1ULL << (epSquare - 1));
        }
        if (epSquare % 8 != 7) {  // Not on the right edge
            adjacentMask |= (1ULL << (epSquare + 1));
        }

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


    // Update counters
    if (undoState.capture || is_pawn_move || revoked_castling_rights) {
        halfmove = 0;
    } else {
        halfmove += 1;
    }

    if (!isWhite) {
        fullmove += 1;
    }

    board.setMoveCounters(halfmove, fullmove);

    // Flip the turn
    board.setTurn(!isWhite);

    return undoState;
}


void undoMove(BoardState& board, const MoveUndo& undoData) {
}
