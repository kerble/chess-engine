# Chess Engine

A chess engine implemented in C++ with a custom evaluation function and a Minimax algorithm. The engine can evaluate positions, determine checkmates, and handle special rules like castling, en passant, and promotion.

## Features
- Minimax algorithm with basic depth search.
- Custom evaluation function with positional piece values.
- Support for special chess rules (castling, promotion, en passant).
- Plays as black against a human player via a Python GUI (Pygame interface).
- Stalemate and checkmate detection.

## Getting Started

### Prerequisites
- C++11 or higher
- Python 3.x (for the GUI)
- Pygame (for the chess board visualization)
  
Install Pygame with:
```bash
pip install pygame
