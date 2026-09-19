// tt_demo.cpp
//
// Standalone demo for the transposition table persistence fix.
//
// Before the fix, Search's `table` member was a *copy* (TranspositionTable table;) made
// with `table = tableParam;` in the constructor body. Every new Search object therefore
// started from a copy of whatever the caller had, and anything it learned during its own
// search died with it when the object went out of scope. In main.cpp that meant every UCI
// "go" call got its own disposable table -- nothing persisted from one move to the next.
//
// After the fix, `table` is a reference (TranspositionTable& table;) bound in the
// constructor's initializer list. Two Search objects that are handed the *same*
// TranspositionTable variable now genuinely share it, exactly like two successive "go"
// calls in main.cpp share main()'s `table`.
//
// This program builds a single persistent TranspositionTable (standing in for main()'s
// table across a whole game) and runs three searches against it, printing what happens
// to the table's size and the hit counter at each step.
#include <atomic>
#include <chrono>
#include <iostream>

#include "search.hpp"

// Print a labeled rule so the steps are easy to tell apart in the output.
void printHeader(const std::string& label) {
    std::cout << "\n=== " << label << " ===\n";
}

int main() {
    // Same one-time setup main.cpp does before it can search anything.
    initKingThreatMasks();
    initKnightThreatMasks();
    initPawnThreatMasks();
    initmagicmoves();
    initializeZobrist();

    // This table plays the role of main()'s `table` in main.cpp: one instance meant to
    // live for the whole game, handed to a new Search object on every move.
    TranspositionTable table;
    std::atomic<bool> stopFlag{false};

    const int depth = 5;              // fixed depth: deterministic, easy to reason about
    const int generousTimeLimit = 60000;  // large enough that the time check never fires

    BoardState board;  // starting position

    // --- Step 1: search the starting position -------------------------------------------
    printHeader("Step 1: first search from the starting position");
    std::cout << "table.size() before search: " << table.size() << "\n";

    Search search1(board, table, generousTimeLimit, stopFlag);
    uint16_t move1 = search1.searchToDepth(depth);

    std::cout << "best move found: " << moveToString(move1) << "\n";
    std::cout << "table.size() after search:  " << table.size() << "\n";
    std::cout << "cache hits during this search: " << search1.getTTHits() << "\n";

    // --- Step 2: re-run the EXACT SAME search, sharing the same table -------------------
    // If the reference fix works, this Search object sees every entry search1 just wrote.
    // Nearly the whole tree should already be cached at >= depth 5, so this should be
    // fast and should log a large number of cache hits relative to step 1.
    printHeader("Step 2: repeat the identical search on the same table");
    std::cout << "table.size() before search: " << table.size() << "\n";

    auto t0 = std::chrono::steady_clock::now();
    Search search2(board, table, generousTimeLimit, stopFlag);
    uint16_t move2 = search2.searchToDepth(depth);
    auto t1 = std::chrono::steady_clock::now();

    std::cout << "best move found: " << moveToString(move2) << "\n";
    std::cout << "table.size() after search:  " << table.size() << "\n";
    std::cout << "cache hits during this search: " << search2.getTTHits() << "\n";
    std::cout << "wall time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count()
              << " ms\n";

    // --- Step 3: play the move found in step 1, then search AGAIN on the new position ---
    // This is the scenario that was actually broken: in main.cpp, this is exactly what
    // happens between one "go" call and the next. table.size() here should start from
    // wherever step 2 left off, NOT reset to whatever it was before step 1 -- proving the
    // table now survives across a move, i.e. across separate Search objects.
    printHeader("Step 3: apply the best move, then search the resulting position");
    applyMove(board, move1);
    std::cout << "table.size() before search: " << table.size()
              << " (carried over from step 2, not reset)\n";

    Search search3(board, table, generousTimeLimit, stopFlag);
    uint16_t move3 = search3.searchToDepth(depth);

    std::cout << "best move found: " << moveToString(move3) << "\n";
    std::cout << "table.size() after search:  " << table.size() << "\n";
    std::cout << "cache hits during this search: " << search3.getTTHits() << "\n";

    return 0;
}
