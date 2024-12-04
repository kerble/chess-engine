#include "evaluate.hpp"

double evaluate(const BoardState& boardState) {
    double score = 0.0;

    for (int pieceType = 0; pieceType <= BLACK_KINGS; ++pieceType) {
        uint64_t bitboard = boardState.getBitboard(pieceType);

        // Count the number of pieces using popcount
        int pieceCount = __builtin_popcountll(bitboard);

        // Add material value
        score += pieceCount * MATERIAL_SCORES[pieceType];
    }

    return score;
}

// Assumed to be white's turn, but they can't move, so black wins
bool blackCheckmate(const BoardState& board, const std::vector<uint16_t>& legalMoves) {
    return legalMoves.empty() && is_in_check(board);  // False -> Black
}
// Assumed to be black's turn, but they can't move, so white wins
bool whiteCheckmate(const BoardState& board, const std::vector<uint16_t>& legalMoves) {
    return legalMoves.empty() && is_in_check(board);  // True -> White
}

bool insufficientMaterial(const BoardState& board) {
    uint64_t whitePieces = board.getWhiteOccupancy();
    uint64_t blackPieces = board.getBlackOccupancy();

    // Remove kings from occupancy
    uint64_t nonKingWhite = whitePieces & ~board.getBitboard(WHITE_KINGS);
    uint64_t nonKingBlack = blackPieces & ~board.getBitboard(BLACK_KINGS);

    // Check for King vs King
    if (nonKingWhite == 0 && nonKingBlack == 0) {
        return true;
    }

    // Check for King + Minor vs King
    if (__builtin_popcountll(nonKingWhite | nonKingBlack) == 1) {
        uint64_t allPieces = nonKingWhite | nonKingBlack;
        if ((allPieces & board.getBitboard(WHITE_BISHOPS)) ||
            (allPieces & board.getBitboard(WHITE_KNIGHTS)) ||
            (allPieces & board.getBitboard(BLACK_BISHOPS)) ||
            (allPieces & board.getBitboard(BLACK_KNIGHTS))) {
            return true;
        }
    }

    // Check for King + Bishop vs King + Bishop on same color squares
    uint64_t whiteBishops = board.getBitboard(WHITE_BISHOPS);
    uint64_t blackBishops = board.getBitboard(BLACK_BISHOPS);

    if (whiteBishops && blackBishops) {
        // Determine the color of squares each bishop occupies
        uint64_t lightSquareMask = 0x55AA55AA55AA55AA;  // Bitmask for light squares
        uint64_t darkSquareMask = ~lightSquareMask;     // Bitmask for dark squares

        bool whiteAllLight = !(whiteBishops & darkSquareMask);
        bool whiteAllDark = !(whiteBishops & lightSquareMask);
        bool blackAllLight = !(blackBishops & darkSquareMask);
        bool blackAllDark = !(blackBishops & lightSquareMask);

        if ((whiteAllLight && blackAllLight) || (whiteAllDark && blackAllDark)) {
            return true;
        }
    }

    // Insufficient material check failed
    return false;
}

bool fiftyMoveRule(const BoardState& board) { return board.getHalfmoveClock() >= 100; }

bool stalemate(const BoardState& board, const std::vector<uint16_t>& legalMoves) {
    return legalMoves.empty() && !is_in_check(board);
}

GameResult gameOver(const BoardState& board, const std::vector<uint16_t>& legalMoves) {
    if (whiteCheckmate(board, legalMoves)) return BLACK_WINS;
    if (blackCheckmate(board, legalMoves)) return WHITE_WINS;
    if (stalemate(board, legalMoves)) return DRAW_STALEMATE;
    if (fiftyMoveRule(board)) return DRAW_50_MOVE_RULE;
    if (insufficientMaterial(board)) return DRAW_INSUFFICIENT_MATERIAL;

    return ONGOING;
}