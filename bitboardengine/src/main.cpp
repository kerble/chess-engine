#include <iostream>
#include <sstream>
#include <iomanip>

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

void printTranspositionTable(const TranspositionTable& table) {
    std::cout << "Transposition Table Contents:" << std::endl;
    std::cout << std::setw(20) << "Zobrist Hash"
              << std::setw(15) << "Visit Count"
              << std::setw(15) << "Best Move"
              << std::setw(15) << "Evaluation"
              << std::setw(10) << "Depth" << std::endl;
    std::cout << std::string(75, '-') << std::endl;

    for (const auto& [key, entry] : table) {
        std::cout << std::setw(20) << key
                  << std::setw(15) << entry.visitCount
                  << std::setw(15) << moveToString(entry.bestMove)
                  << std::setw(15) << entry.evaluation
                  << std::setw(10) << entry.depth << std::endl;
    }
}

void handlePosition(const std::string& args, BoardState& board, TranspositionTable& table) {
    std::istringstream iss(args);
    std::string token;

    if (!(iss >> token)) {
        std::cerr << "Error: Missing argument for position command." << std::endl;
        return;
    }

    if (token == "startpos") {
        BoardState startpos;
        board = startpos;
        if (iss >> token && token == "moves") {
            std::string move;
            while (iss >> move) {
                applyMove(board, encodeUCIMove(board, move));
                updateTranspositionTable(table, board.getZobristHash());
                incrementVisitCount(table, board.getZobristHash());
            }
        }
    } else if (token == "fen") {
        std::string fen, fenPart;
        for (int i = 0; i < 6; ++i) {  // FEN strings have 6 parts.
            if (!(iss >> fenPart)) {
                std::cerr << "Error: Incomplete FEN string." << std::endl;
                return;
            }
            fen += (i > 0 ? " " : "") + fenPart;
        }
        board = parseFEN(fen);
        if (iss >> token && token == "moves") {
            std::string move;
            while (iss >> move) {
                applyMove(board, encodeUCIMove(board, move));
                updateTranspositionTable(table, board.getZobristHash());
                incrementVisitCount(table, board.getZobristHash());
            }
        }
    } else {
        std::cerr << "Error: Invalid argument for position command: " << token << std::endl;
    }

    if(allLegalMoves(board).size() == 0){
        cout << "no moves" << endl;
    }
    cout << "all moves for this position " << endl;
    for (const uint16_t& move : allLegalMoves(board)) {
        cout << moveToString(move) << endl;
    }
}

void handleGo(const std::string& args, BoardState& board, TranspositionTable& table) {
    int wtime = -1, btime = -1, movestogo = 30, movetime = -1;
    bool infinite = false;

    std::istringstream iss(args);
    std::string token;
    while (iss >> token) {
        if (token == "wtime") {
            iss >> wtime;
        } else if (token == "btime") {
            iss >> btime;
        } else if (token == "movestogo") {
            iss >> movestogo;
        } else if (token == "movetime") {
            iss >> movetime;
        } else if (token == "infinite") {
            infinite = true;
        }
    }

    // Determine available time for this move
    int timeLimitMs = -1;
    if (movetime != -1) {
        timeLimitMs = movetime;
    } else if (board.getTurn()) {  // White to move
        if (wtime > 0) {
            timeLimitMs = (wtime / movestogo);
        }
    } else {  // Black to move
        if (btime > 0) {
            timeLimitMs = (btime / movestogo);
        }
    }

    if (timeLimitMs == -1 && !infinite) {
        std::cerr << "Invalid time configuration in 'go' command" << std::endl;
        return;
    }

    // Call iterative deepening with the calculated time limit
    // std::vector<uint16_t> legalMoves = generateLegalMoves(board);
    Search search(board, table, timeLimitMs);
    uint16_t bestMove = search.iterativeDeepening();
    // uint16_t bestMove = iterativeDeepening(board, table, timeLimitMs);

    // Print the best move in UCI format
    std::cout << "bestmove " << moveToString(bestMove) << std::endl;
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

    /* Import UCI moves from command line rather than from cin
    // Example UCI moves
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
            incrementVisitCount(table, board.getZobristHash());
            // cout << "adding " << board.getZobristHash() << " to table after " << moveToString(encodedMove) << "\n";
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
    // cout << board << endl;
    */

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
        cout << "fen is: " << fen << endl;
        board = parseFEN(fen);
        cout << "Parsed board state:\n" << board << endl;
    } catch (const exception& e) {
        // std::cout << "fen is" << std::endl;
        // std::cout << fen << std::endl;
        cerr << "Failed to parse FEN: " << fen << e.what() << endl;
        return 1;
    }
    // double materialGain = see(board, squareFromAlgebraic("e5"), BLACK_PAWNS, squareFromAlgebraic("d3"), WHITE_KNIGHTS);
    // cout << materialGain << endl;
    */
    // /*


    std::string input;
    while (std::getline(std::cin, input)) {
        std::istringstream iss(input);
        std::string command;
        iss >> command;

        if (command == "uci") {
            std::cout << "id name ColbysBot\n";
            std::cout << "id author Colby Smith\n";
            std::cout << "uciok" << std::endl;
        } else if (command == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (command == "ucinewgame") {
            BoardState newBoard;
            board = newBoard;
        } else if (command == "position") {
            std::string args;
            std::getline(iss, args);
            handlePosition(args, board, table);
        } else if (command == "go") {
            std::string args;
            std::getline(iss, args);
            handleGo(args, board, table);
        } else if (command == "stop") {
            std::cout << "Stopping search." << std::endl;
            // Logic to stop search would go here.
        } else if (command == "quit") {
            std::cout << "Quitting engine." << std::endl;
            break;
        } else {
            std::cout << "Unknown command: " << command << std::endl;
        }
    }

    return 0;


    // */

    // auto start = std::chrono::high_resolution_clock::now();
    // uint16_t bestMove = getBestMove(board, table, 5);
    // uint16_t bestMove = searchToDepth(3);




    // int timeLimitMs = 100000;
    // Search search(board, table, timeLimitMs);
    // uint16_t bestMove = search.searchToDepth(3);

    // cout << moveToString(bestMove) << endl;
    // int timeLimitMs = 1000;
    // uint16_t bestMove = iterativeDeepening(board, table, timeLimitMs);
    // printTranspositionTable(table);
    // auto end = std::chrono::high_resolution_clock::now();
    // std::chrono::duration<double> elapsed = end - start;
    // std::cout << "Time taken: " << elapsed.count() << " seconds" << std::endl;
    // std::cout << "Transpositon table entries: " << table.size() << endl;
    // cout << moveToString(bestMove);
    // return 0;
}