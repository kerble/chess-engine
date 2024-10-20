import argparse
import chess
import chess.pgn

def uci_to_pgn(uci_moves):
    game = chess.pgn.Game()
    board = chess.Board()

    node = game
    for move in uci_moves:
        node = node.add_variation(board.push_san(board.san(chess.Move.from_uci(move))))

    return game

def main():
    # Set up argument parser
    parser = argparse.ArgumentParser(description='Convert UCI moves to PGN')
    parser.add_argument('uci_moves', nargs='+', help='List of UCI moves (e.g. e2e4 e7e5 g1f3)')
    parser.add_argument('--output', '-o', type=str, help='Output PGN file path (optional)', default=None)
    
    args = parser.parse_args()

    # Convert UCI moves to PGN
    pgn_game = uci_to_pgn(args.uci_moves)

    if args.output:
        # Write to file if output is specified
        with open(args.output, 'w') as f:
            f.write(str(pgn_game))
        print(f"PGN written to {args.output}")
    else:
        # Print PGN to console
        print(pgn_game)

if __name__ == '__main__':
    main()
