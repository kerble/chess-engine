#include <iostream>
#include <sstream>
#include <chrono>

// #include "bitboard.hpp"
// #include "movegen.hpp"
// #include "move.hpp"
// #include "evaluate.hpp"
#include "search.hpp"
using namespace std;

// Function to print usage instructions
void printUsage() {
    cout << "Usage: chess_engine <FEN_string>" << endl;
    cout << "Example: chess_engine \"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\"" << endl;
}


int main(int argc, char* argv[]) {
    // Check if the FEN string components are provided
    if (argc < 2) {
        cerr << "Error: FEN string components missing." << endl;
        printUsage();
        return 1;
    }

    // Join the FEN components into a single string
    string fen;
    for (int i = 1; i < argc; ++i) {
        if (i > 1) {
            fen += " ";  // Add a space between parts
        }
        fen += argv[i];
    }
    // evaluated_positions = 0;
    initKingThreatMasks();
    initKnightThreatMasks();
    initPawnThreatMasks();
    initmagicmoves();
    initializeZobrist();

    // Create a BoardState object
    BoardState board;

    // Parse the FEN string
    try {
        // cout << "fen is: " << fen << endl;
        board = parseFEN(fen);
        // cout << "Parsed board state:\n" << board << endl;
    } catch (const exception& e) {
        // std::cout << "fen is" << std::endl;
        // std::cout << fen << std::endl;
        cerr << "Failed to parse FEN: " << fen << e.what() << endl;
        return 1;
    }
    // uint16_t move = encodeMove(squareFromAlgebraic("e2"), squareFromAlgebraic("f1"), PROMOTION_QUEEN);
    // MoveUndo undostate = applyMove(board, move);
    // cout << board << endl;
    // cout << "\n\n\n\n\n";
    // cout << bitboardToBinaryString(board.getBitboard(WHITE_BISHOPS));
    // undoMove(board, undostate);
    // cout << board << endl;

    // cout << evaluate(board) << "\n";
    // return 0;
    /*
    // For just evaluating a user move
    vector<uint16_t> legalMoves = orderMoves(board, allLegalMoves(board));
    int i = 1;
    for(auto move : legalMoves){ 
        cout << i << ": " << moveToString(move) << endl;
        i++;
    }

    // Ask the user to pick a move
    int choice;
    // std::cout << "Enter the number of the move to play: ";
    std::cin >> choice;

    if (choice < 1 || choice > legalMoves.size()) {
    // std::cout << "Invalid choice.\n";
        return 1;
    }
     */

    // Apply the chosen move
    // MoveUndo undoState;
    // uint16_t move = legalMoves[choice - 1];
    // uint64_t originalHash = board.getZobristHash();
    // // std::cout << "Original Zobrist Hash: " << originalHash << std::endl;
    // undoState = applyMove(board, move);
    // std::cout << board << endl;
    // uint64_t afterMoveHash = board.getZobristHash();
    // std::cout << "Hash after applyMove: " << afterMoveHash << std::endl;
    // undoMove(board, undoState);
    // uint64_t afterUndoHash = board.getZobristHash();
    // std::cout << "Hash after undoMove: " << afterUndoHash << std::endl;
    
    // if (originalHash == afterUndoHash) {
    //     std::cout << "Test passed: Hashes match!" << std::endl;
    // } else {
    //     std::cerr << "Test failed: Hashes do not match!" << std::endl;
    // }
    // double score = evaluate(board);
    // bool insuffi = insufficientMaterial(board);
    // bool checkmate = false;
    // bool isstalemate = stalemate(board, legalMoves);
    // if ((board.getTurn() && whiteCheckmate(board, legalMoves)) ||
        // (!board.getTurn() && blackCheckmate(board, legalMoves))) {
            // checkmate = true;
    // }
    // if (isstalemate) cout << "stalemate" << endl;
    // if (checkmate) cout << "checkmate" << endl;
    // if (insuffi) cout << "insufficient" << endl;
    // std::cout << "score is: " << score << std::endl;
    // auto start = std::chrono::high_resolution_clock::now();
    // std::cout << insufficientMaterial(board) << "\n";
    // return 0;
    uint16_t bestMove = getBestMove(board, 6);
    // // allLegalMoves(board);
    // auto end = std::chrono::high_resolution_clock::now();
    // std::chrono::duration<double> elapsed = end - start;
    // std::cout << "Time taken: " << elapsed.count() << " seconds" << std::endl;
    cout << moveToString(bestMove);

    // cout << "looked at " << moves_looked_at << " moves\n";
    // cout << "evaluated " << evaluated_positions << " positoins\n";
    
    // vector<uint16_t> kingMoves = generateKingMoves(board);
    // int i = 1;
    // for (auto move : kingMoves) {
    //     cout << i << ": " << moveToString(move) << endl;
    //     i++;
    // }

    return 0;
}