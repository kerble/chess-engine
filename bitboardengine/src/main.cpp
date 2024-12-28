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
    initKingThreatMasks();
    initKnightThreatMasks();
    initPawnThreatMasks();
    initmagicmoves();
    initializeZobrist();

    // Create a BoardState object
    BoardState board;  // Initialize board state
    TranspositionTable table;

    // /* Import UCI moves
    // Example UCI moves
    // Check if there are enough arguments (argc includes the program name)
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <moves>\n";
        return 1;
    }

    // Create an array to store UCI moves
    std::string uciMoves[argc - 1];

    // Loop through the command-line arguments (starting from 1, since argv[0] is the program name)
    for (int i = 1; i < argc; i++) {
        uciMoves[i - 1] = argv[i];  // Store each move in the array
    }
    for (const std::string& uciMove : uciMoves) {
        try {
            uint16_t encodedMove = encodeUCIMove(board, uciMove);
            // std::cout << "UCI Move: " << uciMove << " -> Encoded Move: " << moveToString(encodedMove)
            //           << std::endl;
            applyMove(board, encodedMove);  // Apply the move to the board
            updateTranspositionTable(table, board.getZobristHash());
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
    // cout << board << endl;
    // */

    /* FEN
    // Parse the FEN string
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
    // double materialGain = see(board, squareFromAlgebraic("e5"), BLACK_PAWNS, squareFromAlgebraic("d3"), WHITE_KNIGHTS);
    // cout << materialGain << endl;
    */
    // auto start = std::chrono::high_resolution_clock::now();
    uint16_t bestMove = getBestMove(board, table, 5);
    // auto end = std::chrono::high_resolution_clock::now();
    // std::chrono::duration<double> elapsed = end - start;
    // std::cout << "Time taken: " << elapsed.count() << " seconds" << std::endl;
    // std::cout << "Transpositon table entries: " << table.size() << endl;
    cout << moveToString(bestMove);
    return 0;
}