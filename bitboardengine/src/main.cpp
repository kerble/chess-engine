#include <iostream>
#include <sstream>
#include <chrono>

#include "main.hpp"
#include "bitboard.hpp"
#include "movegen.hpp"
#include "move.hpp"
#include "evaluate.hpp"
#include "search.hpp"
using namespace std;

// Function to print usage instructions
void printUsage() {
    cout << "Usage: chess_engine <FEN_string>" << endl;
    cout << "Example: chess_engine \"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\"" << endl;
}

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
    // vector<uint16_t> legalMoves = allLegalMoves(board);
    // int i = 1;
    // for(auto move : legalMoves){ 
    //     cout << i << ": " << moveToString(move) << endl;
    //     i++;
    // }

    // // Ask the user to pick a move
    // int choice;
    // std::cout << "Enter the number of the move to play: ";
    // std::cin >> choice;

    // if (choice < 1 || choice > legalMoves.size()) {
    //     std::cout << "Invalid choice.\n";
    //     return 1;
    // }

    // Apply the chosen move
    // MoveUndo undoState;
    // uint16_t move = legalMoves[choice - 1];
    // undoState = storeUndoData(board, move);
    // BoardState copyboard = board;
    // applyMove(board, move);
    // undoMove(board, undoState);
    // if(copyboard == board){
    //     cout << "done and undid perfectly" << endl;
    // }
    // else{
    //     cout << "investigate" << endl;
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
    auto start = std::chrono::high_resolution_clock::now();
    uint16_t bestMove = getBestMove(board, 5);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time taken: " << elapsed.count() << " seconds" << std::endl;
    cout << moveToString(bestMove) << endl;
    return 0;
}