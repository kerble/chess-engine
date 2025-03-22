#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <evaluate.hpp>
#include <string>
#include <algorithm>
#include <chrono>

class Search {
   public:
    // Constructor
    Search(BoardState& board, TranspositionTable& table, int timeLimitMs);

    // Main entry point for search
    uint16_t iterativeDeepening();
    uint16_t searchToDepth(int depth);

   private:
    // Search parameters
    int timeLimitMs;
    int maxDepth;

    // Search state
    uint16_t bestMoveSoFar;
    int bestEvalSoFar;
    int completedDepth;
    bool searchInterrupted;
    std::chrono::steady_clock::time_point startTime;

    // Board and table as member variables
    BoardState board;
    TranspositionTable table;

    std::vector<std::pair<int, uint16_t>> scoredMoves;
    std::vector<uint16_t> orderedLegalMoves;

    // Helper functions
    bool shouldStopSearch();
    void getBestMove(int depth);
    int negamax(int depth, int alpha, int beta);
    int QSearch(int alpha, int beta);
};

// Search functions
// uint16_t getBestMove(BoardState& board, TranspositionTable& table, int depth);
// void Search::getBestMove(int depth);

std::vector<uint16_t> orderMoves(BoardState& board, const std::vector<uint16_t>& moves);

// Game-ending conditions
// GameResult gameOver(const BoardState& board, const std::vector<uint16_t>& legalMoves, TranspositionTable& table);
// bool insufficientMaterial(const BoardState& board);
// bool whiteCheckmate(const BoardState& board, const std::vector<uint16_t>& legalMoves);
// bool blackCheckmate(const BoardState& board, const std::vector<uint16_t>& legalMoves);
// bool fiftyMoveRule(const BoardState& board);
// bool stalemate(const BoardState& board, const std::vector<uint16_t>& legalMoves);



#endif // SEARCH_HPP
