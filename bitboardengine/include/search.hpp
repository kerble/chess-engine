#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <evaluate.hpp>
#include <string>
#include <algorithm>
#include <chrono>
#include <atomic>

class Search {
   public:
    // Constructor
    Search(BoardState& board, TranspositionTable& table, int timeLimitMs, std::atomic<bool>& stopFlag);

    // Main entry point for search
    uint16_t iterativeDeepening();
    uint16_t searchToDepth(int depth);

    // How many times this search returned a cached result instead of re-searching a subtree.
    // Only meaningful as a diagnostic/demo counter, not used by the search itself.
    int getTTHits() const { return ttHits; }

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
    int ttHits;

    // Board and table as member variables.
    // `table` is a REFERENCE: it binds to the caller's transposition table instead of copying
    // it, so entries this search writes are visible to (and persist for) whoever owns the table.
    BoardState board;
    TranspositionTable& table;

    // External stop signal, set by the UCI "stop" command from another thread.
    std::atomic<bool>& stopFlag;

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
