#include <iostream>
#include <sstream>
#include "main.hpp"
#include "bitboard.hpp"
#include "movegen.hpp"
using namespace std; 


// BoardState parseFEN(const string& fen) {
//     BoardState board;
//     istringstream fenStream(fen);
//     string piecePlacement, sideToMove, castlingRights, enPassant;
//     int halfmoveClock, fullmoveNumber;

//     fenStream >> piecePlacement >> sideToMove >> castlingRights >> enPassant >> halfmoveClock >> fullmoveNumber;

//     for (int i = 0; i < 12; ++i) {
//         board.updateBitboard(i, 0);
//     }

//     int square = 56; // Start from bottom-left (a1), which is bit 0
//     for (char ch : piecePlacement) {
//         if (isdigit(ch)) {
//             square += (ch - '0'); // Skip empty squares
//         } else if (ch == '/') {
//             square -= 16; // Move to the next rank (up one row in FEN, down in bitboard)
//         } else {
//             int pieceType = charToPieceIndex(ch);
//             uint64_t currentBitboard = board.getBitboard(pieceType);

//             // Debug: Print current state before adding piece
//             // std::cout << "Adding piece " << ch << " at square #" << square
//                     // << " AKA " << algebraicFromSquare(square) << '\n';
//             // std::cout << "Bitboard before:\n" << bitboardToBinaryString(currentBitboard) << '\n';

//             // Add the piece to the bitboard
//             uint64_t newBitboard = set_bit(currentBitboard, square);
//             board.updateBitboard(pieceType, newBitboard);

//             // Debug: Print new state after adding piece
//             // std::cout << "Bitboard after:\n" << bitboardToBinaryString(newBitboard) << '\n';

//             square++;
//         }
//     }



//     // Parse side to move
//     board.setTurn(sideToMove == "w");

//     // Parse castling rights
//     uint64_t castling = parseCastlingRights(castlingRights);
//     board.setCastlingRights(castling);

//     // Parse en passant square
//     uint64_t enPassantSq = (enPassant == "-") ? 0 : squareFromAlgebraic(enPassant);
//     board.setEnPassant(enPassantSq);

//     // Parse move counters
//     board.setMoveCounters(halfmoveClock, fullmoveNumber);
//     return board;
// }

BoardState parseFEN(const std::string& fen) {
    BoardState board;
    std::istringstream fenStream(fen);
    std::string piecePlacement, sideToMove, castlingRights, enPassant;
    int halfmoveClock, fullmoveNumber;

    // Parse the FEN components
    fenStream >> piecePlacement >> sideToMove >> castlingRights >> enPassant >> halfmoveClock >> fullmoveNumber;

    // Initialize bitboards for all pieces
    for (int i = 0; i < 12; ++i) {
        board.updateBitboard(i, 0);
    }

    // Initialize occupancy bitboards
    uint64_t whiteOccupancy = 0;
    uint64_t blackOccupancy = 0;

    int square = 56; // Start from bottom-left (a1), which is bit 0
    for (char ch : piecePlacement) {
        if (std::isdigit(ch)) {
            square += (ch - '0'); // Skip empty squares
        } else if (ch == '/') {
            square -= 16; // Move to the next rank (up one row in FEN, down in bitboard)
        } else {
            int pieceType = charToPieceIndex(ch); // Get piece index
            uint64_t currentBitboard = board.getBitboard(pieceType);

            // Add the piece to the piece bitboard
            uint64_t newBitboard = set_bit(currentBitboard, square);
            board.updateBitboard(pieceType, newBitboard);

            // Update the appropriate occupancy bitboard
            if (isupper(ch)) {
                whiteOccupancy |= (1ULL << square); // Uppercase letters are white
            } else {
                blackOccupancy |= (1ULL << square); // Lowercase letters are black
            }

            square++;
        }
    }

    // Parse side to move
    board.setTurn(sideToMove == "w");

    // Parse castling rights
    uint64_t castling = parseCastlingRights(castlingRights);
    board.setCastlingRights(castling);

    // Parse en passant square
    uint64_t enPassantSq = (enPassant == "-") ? 0 : squareFromAlgebraic(enPassant);
    board.setEnPassant(enPassantSq);

    // Parse move counters
    board.setMoveCounters(halfmoveClock, fullmoveNumber);

    // Set occupancy bitboards
    board.setOccupancy(whiteOccupancy, blackOccupancy);

    return board;
}


// Function to print usage instructions
void printUsage() {
    cout << "Usage: chess_engine <FEN_string>" << endl;
    cout << "Example: chess_engine \"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\"" << endl;
}

// int debugIndicesForSquare(int square, bool forRooks) {
//     const uint64_t* magics = forRooks ? magicmoves_r_magics : magicmoves_b_magics;
//     const uint64_t* masks = forRooks ? magicmoves_r_mask : magicmoves_b_mask;
//     const int* shifts = forRooks ? magicmoves_r_shift : magicmoves_b_shift;

//     uint64_t mask = masks[square];
//     uint64_t magic = magics[square];
//     int shift = shifts[square];

//     auto occupancies = generateOccupancies(mask);
//     std::unordered_set<uint64_t> uniqueIndices;

//     for (uint64_t occupancy : occupancies) {
//         uint64_t index = (occupancy * magic) >> (64 - shift);
//         cout << "index is" << index << endl;
//         uniqueIndices.insert(index);
//     }

//     std::cout << (forRooks ? "Rook" : "Bishop") 
//               << " square " << square << ": "
//               << uniqueIndices.size() << " unique indices\n";
//     return uniqueIndices.size();
// }
int main(int argc, char* argv[]) {
    // Check if the FEN string is provided as a command-line argument
    if (argc != 2) {
        cerr << "Error: Incorrect number of arguments." << endl;
        printUsage();
        return 1;
    }

    string fen = argv[1];

    // Create a BoardState object
    BoardState board;

    // Parse the FEN string
    try {
        board = parseFEN(fen);
        cout << "Parsed board state:\n" << board << endl;
    } catch (const exception& e) {
        cerr << "Failed to parse FEN: " << e.what() << endl;
        return 1;
    }
    initKingThreatMasks();
    initKnightThreatMasks();
    initPawnThreatMasks();
    initmagicmoves();
    // cout << bitboardToBinaryString(Bmagic(19, 0)) << endl;
    // cout << bitboardToBinaryString(Rmagic(19, 0)) << endl;

    // bool isWhiteInCheck = is_in_check(board, true);
    // bool isBlackInCheck = is_in_check(board, false);

    // std::cout << "White in check: " << (isWhiteInCheck ? "Yes" : "No") << '\n';
    // std::cout << "Black in check: " << (isBlackInCheck ? "Yes" : "No") << '\n';

    // Find attackers and their attack masks
    // bool isWhite = board.getTurn();
    // uint64_t attackers = 0;
    // uint64_t kingBB = board.getBitboard(WHITE_KINGS);
    // uint64_t allOccupancy = board.getAllOccupancy();
    // int attackerSquare = 0;
    // for (int pieceType = (isWhite ? BLACK_PAWNS : WHITE_PAWNS); 
    //     pieceType <= (isWhite ? BLACK_KINGS : WHITE_KINGS); 
    //     ++pieceType) {
    //     uint64_t pieceBB = board.getBitboard(pieceType);
    //     while (pieceBB) {
    //         attackerSquare = __builtin_ctzll(pieceBB);
    //         uint64_t attackMask = generateThreatMask(BLACK_QUEENS, 39, allOccupancy);
    //         if (attackMask & kingBB) {
    //             attackers |= (1ULL << attackerSquare);
    //         }
    //         pieceBB &= pieceBB - 1; // Clear the bit
    //     }
    // }
    // cout << "num attackers found: " << __builtin_popcountll(attackers) << endl;
    // if (__builtin_popcountll(attackers) == 1) {
        // attackerSquare = 39;
        // uint64_t rayMask = generateRayMask(whiteKingSquare, attackerSquare, allOccupancy);
        // cout << "ray mask: " << bitboardToBinaryString(rayMask);
    // }
    // cout << board.getTurn() << endl;
    // vector<pair<int, int>> moves = generateKingMoves(board);

    // uint16_t regularMove = encodeMove(12, 20);                // Regular move
    // uint16_t queenPromotion = encodeMove(52, 60, 1);         // d7d8q
    // uint16_t knightPromotion = encodeMove(52, 60, 4);        // d7d8n
    // uint16_t enPassant = encodeMove(27, 36, 8);              // En passant
    // uint16_t castling = encodeMove(4, 6, 9);                 // Castling

    // int origin = getOrigin(queenPromotion); // 52
    // int destination = getDestination(queenPromotion); // 60
    // int flags = getFlags(queenPromotion); // 1 (Queen promotion)

    // uint16_t move;
    // int fromSquare;
    // int toSquare;
    // char promotion;
    // decodeMove(move, fromSquare, toSquare, promotion);
    // cout << 


    // const int PROMO_QUEEN = 1; // 'q'
    // const int PROMO_ROOK = 2;  // 'r'
    // const int PROMO_BISHOP = 3; // 'b'
    // const int PROMO_KNIGHT = 4; // 'n'

    // uint16_t move1 = encodeMove(squareFromAlgebraic("e4"), squareFromAlgebraic("g7"));
    // uint16_t move2 = encodeMove(squareFromAlgebraic("g7"), squareFromAlgebraic("h8"), 'n');

    // std::cout << "Move 1: " << moveToString(move1) << std::endl; // Expected: e2e4
    // std::cout << "Move 2: " << moveToString(move2) << std::endl; // Expected: e7e8q
        // Load a BoardState from a FEN string (you'll need your FEN parsing function here)

    // Determine whether it's white to move
    // bool isWhite = board.getTurn();

    // // Get the pawn bitboard for the active player
    // uint64_t pawnBitboard = board.getBitboard(isWhite ? WHITE_PAWNS : BLACK_PAWNS);

    // // Get occupancy bitboards
    // uint64_t enemyOccupancy = isWhite ? board.getBlackOccupancy() : board.getWhiteOccupancy();
    // uint64_t allOccupancy = board.getAllOccupancy();

    // // Get the en passant square
    // int enPassantSquare = board.getEnPassant();

    // // Process each pawn on the bitboard
    // while (pawnBitboard) {
    //     // Get the least significant bit of the pawn bitboard
    //     int pawnSquare = __builtin_ctzll(pawnBitboard);

    //     // Generate moves for this pawn
    //     uint64_t pawnMoves = generatePawnMoves(pawnSquare, enemyOccupancy, allOccupancy, enPassantSquare, isWhite);

    //     // Print the results
    //     std::cout << "Pawn at: " << algebraicFromSquare(pawnSquare) << std::endl;
    //     std::cout << "Possible moves bitboard:" << std::endl;
    //     std::cout << bitboardToBinaryString(pawnMoves) << std::endl;

    //     // Clear the processed pawn from the bitboard
    //     pawnBitboard &= pawnBitboard - 1;
    // }
    // uint64_t pinMask = 0;
    // uint64_t pinMask2 = 0;
    // uint64_t pinMask3 = 0;

    // vector<uint64_t> pinMasks;
    // int kingSquare = __builtin_ctzll(board.getBitboard(WHITE_KINGS));
    // uint64_t enemyOccupancy = board.getBlackOccupancy();
    // uint64_t enemyQueens = board.getBitboard(BLACK_QUEENS);
    // uint64_t enemyBishops = board.getBitboard(BLACK_BISHOPS);
    // uint64_t enemyRooks = board.getBitboard(BLACK_ROOKS);
    // uint64_t alliedOccupancy = board.getWhiteOccupancy();
    // pinMasks = detectPinnedPieces(kingSquare, enemyOccupancy, enemyQueens, enemyBishops, enemyRooks, alliedOccupancy);
    // pinMask = pinMasks[0];
    // pinMask2 = pinMasks[1];
    // pinMask3 = pinMasks[2];

    // cout << pinMasks.size() << endl;
    // cout << bitboardToBinaryString(pinMask) << endl;
    // cout << bitboardToBinaryString(pinMask2) << endl;
    // cout << bitboardToBinaryString(pinMask3) << endl;
    vector<uint16_t> legalMoves = allLegalMoves(board);
    for(auto move : legalMoves){ 
        cout << moveToString(move) << endl;
    }
    return 0;


}