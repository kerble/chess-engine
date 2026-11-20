# ColbysBot: a bitboard chess engine in C++

ColbysBot is a chess engine written from scratch in C++17. It stores the board as bitboards, generates sliding-piece moves with magic bitboards, and searches with negamax, alpha-beta pruning and a transposition table. It speaks the **UCI protocol**, so you can load it into any UCI-compatible GUI (Arena, Cute Chess, BanksiaGUI and others) or connect it to Lichess through a bridge such as [lichess-bot](https://github.com/lichess-bot-devs/lichess-bot).

## Features

### Board representation
- 12 bitboards, one 64-bit integer for each piece type and color.
- Magic bitboards for bishop, rook and queen attacks (based on Pradyumna Kannan's `magicmoves`).
- Precomputed attack tables for kings, knights and pawns.
- Zobrist hashing for fast position identification.
- FEN parsing.
- Each move packs into 16 bits:

  | Bits  | Meaning                                                              |
  |-------|----------------------------------------------------------------------|
  | 0–5   | from square (0 = a1 … 63 = h8)                                       |
  | 6–11  | to square                                                            |
  | 12–15 | special flag (promotion piece, castling, double push, en passant)    |

### Move generation
- Fully legal moves: pins, checks, castling rights, en passant and promotions are all handled.
- Make/unmake moves with an undo record, so the search never has to copy the board.

### Search
- **Iterative deepening** up to depth 12 within the time budget. Each iteration reorders the root moves by the scores from the previous one.
- **Negamax with alpha-beta pruning.**
- **Transposition table** keyed by Zobrist hash. It stores exact, upper-bound and lower-bound scores and **persists across moves** for the whole game.
- **Quiescence search** at the leaves. It looks only at checks, queen promotions and captures that SEE judges favorable, which avoids the horizon effect.
- **Static Exchange Evaluation (SEE)**, including x-ray attackers, used for capture ordering and quiescence pruning.
- **Move ordering heuristics**: SEE score for captures, plus bonuses for checks, promotions and castling.
- **Draw detection**: stalemate, threefold repetition (tracked through visit counts in the transposition table), the fifty-move rule and insufficient material.
- **Time management**: a share of the remaining clock (`time / movestogo + increment`), capped by a safety buffer. Also supports `movetime`, `infinite` and `stop`.

### Evaluation
All scores are in centipawns. The evaluation is a hand-written sum of these terms:

| Term | Details |
|------|---------|
| Material | P = 100, N = 325, B = 325, R = 500, Q = 900 |
| Mobility | +2 per pseudo-legal square each piece can reach |
| Pawn structure | Passed-pawn bonus grows with rank (10 → 160); −15 per doubled pawn and per isolated pawn |
| Center control | +10 for occupying d4/e4/d5/e5, +5 per attack on them |
| Development | −15 per knight or bishop still on its starting square |
| Rook files | +20 for a rook on an open file, +10 on a semi-open file |

All the weights are `constexpr` constants in [`include/evaluate.hpp`](bitboardengine/include/evaluate.hpp), so you can tune them in one place.

## Project layout

```
.
├── bitboardengine/
│   ├── Makefile
│   ├── include/            # headers
│   └── src/
│       ├── main.cpp        # UCI loop and time management
│       ├── search.cpp      # iterative deepening, negamax, quiescence, SEE, move ordering
│       ├── evaluate.cpp    # static evaluation
│       ├── movegen.cpp     # legal move generation
│       ├── move.cpp        # move encoding, make/unmake, UCI move parsing
│       ├── bitboard.cpp    # BoardState, FEN parsing, Zobrist hashing, transposition table helpers
│       ├── MagicMoves.cpp  # magic bitboard sliding attacks
│       ├── tt_demo.cpp     # demo: transposition table persistence across searches
│       └── eval_demo.cpp   # demo: evaluation terms on sample positions
└── uci_to_pgn.py           # helper: convert a list of UCI moves into PGN
```

## Building

You need a C++17 compiler with GCC builtins (`g++`, `clang++` or MinGW on Windows) and `make`.

```bash
cd bitboardengine
make              # builds chess_engine.exe (the UCI engine)
make tt_demo      # optional: transposition table demo
make eval_demo    # optional: evaluation demo
make clean
```

The binaries always get a `.exe` extension, even on Linux, where they still run normally (`./chess_engine.exe`).

Without `make`:

```bash
g++ -std=c++17 -O2 -pthread -Iinclude src/main.cpp src/search.cpp src/evaluate.cpp \
    src/movegen.cpp src/move.cpp src/bitboard.cpp src/MagicMoves.cpp -o chess_engine
```

## Usage

### With a chess GUI
Add `chess_engine.exe` as a UCI engine in your GUI of choice. The engine identifies itself as `ColbysBot`.

### From the command line
The engine reads UCI commands on stdin:

```
uci
id name ColbysBot
id author Colby Smith
uciok
isready
readyok
position startpos moves e2e4 e7e5
go movetime 2000
bestmove g1f3
```

Supported commands:

| Command | Notes |
|---------|-------|
| `uci`, `isready`, `ucinewgame`, `quit` | Standard handshake and lifecycle |
| `position startpos [moves ...]` | |
| `position fen <fen> [moves ...]` | |
| `go wtime <ms> btime <ms> [winc <ms>] [binc <ms>] [movestogo <n>]` | `movestogo` defaults to 30 |
| `go movetime <ms>` | Search for a fixed time |
| `go infinite` | Search until `stop` |
| `stop` | Ends the search and prints `bestmove` |

The search runs on its own thread, so the engine still responds to `stop` and `quit` while it is thinking.

### Converting games to PGN
`uci_to_pgn.py` turns a sequence of UCI moves into a PGN file that you can paste into an analysis board. It needs [`python-chess`](https://pypi.org/project/chess/):

```bash
pip install chess
python uci_to_pgn.py e2e4 e7e5 g1f3 b8c6 -o game.pgn
```

## Possible future improvements
- Null-move pruning, late move reductions and principal variation search
- Killer-move and history heuristics for ordering quiet moves
- Tapered (middlegame/endgame) evaluation and king safety
- A fixed-size transposition table with a replacement scheme (the current one is an unbounded `std::unordered_map`)
- UCI `info` output (depth, score, PV) during the search

## Credits
- Magic move generation: `magicmoves` by Pradyumna Kannan (zlib-style license, see the header of [`MagicMoves.cpp`](bitboardengine/src/MagicMoves.cpp)).

## License
MIT © 2024 Colby Smith. See [LICENSE](LICENSE).
