from os import environ
environ['PYGAME_HIDE_SUPPORT_PROMPT'] = '1'
import pygame
import subprocess
# import os
import copy
# import msvcrt
# import time

#TODO: add king "capturing" its own rook-looking syntax for castling
#TODO: don't highlight the square of the last moved piece's to_location if it is in the valid squares of a selected piece
# Define colors
WHITE = (240, 217, 181)
BLACK = (181, 136, 99)

def get_piece_image(piece):
    # Load piece image based on piece name (e.g., "Wp" for white pawn)
    king_aliases = ["K", "k", "S", "s", "U", "u", "W", "w"]
    pawn_aliases = ["P", "p", "E", "e"]
    if piece in king_aliases:
        piece = "k" if piece.islower() else "K"
    if piece in pawn_aliases:
        piece = "p" if piece.islower() else "P"

    if piece.islower():
        piece_key = "B"
    else:
        piece_key = "W"
    piece_key += piece[0].lower()
    image_path = "graphics/" + f"{piece_key}.png"
    return pygame.image.load(image_path)

def create_piece_from_char(char, position):
    if char == 'P':
        return Pawn("white", position)
    elif char == 'p':
        return Pawn("black", position)
    elif char == 'E':
        pawn = Pawn("white", position)
        pawn.en_passant_eligible = True
        return pawn
    elif char == 'e':
        pawn = Pawn("black", position)
        pawn.en_passant_eligible = True
        return pawn
    elif char == 'K': #K denotes a white king with no castling rights.
        king = King("white", position)
        king.has_queenside_castling_rights = False
        king.has_kingside_castling_rights = False
        return king
    elif char == 'k': #k denotes a black king with no castling rights.
        king = King("black", position)
        king.has_queenside_castling_rights = False
        king.has_kingside_castling_rights = False
        return king
    elif char == 'S': #S denotes a white king with only kingside castling rights.
        king = King("white", position)
        king.has_queenside_castling_rights = False
        king.has_kingside_castling_rights = True
        return king
    elif char == 's': #s denotes a black king with only kingside castling rights.
        king = King("black", position)
        king.has_queenside_castling_rights = False
        king.has_kingside_castling_rights = True
        return king
    elif char == 'U': #U denotes a white king with only queenside castling rights.
        king = King("white", position)
        king.has_queenside_castling_rights = True
        king.has_kingside_castling_rights = False
        return king
    elif char == 'u': #u denoets a black king with only queenside castling rights.
        king = King("black", position)
        king.has_queenside_castling_rights = True
        king.has_kingside_castling_rights = False
        return king
    elif char == 'W': #W denotes a white king with full castling rights.
        king = King("white", position)
        king.has_queenside_castling_rights = True
        king.has_kingside_castling_rights = True
        return king
    elif char == 'w': #w denoets a black king with full castling rights.
        king = King("black", position)
        king.has_queenside_castling_rights = True
        king.has_kingside_castling_rights = True
        return king
    elif char == 'R':
        return Rook("white", position)
    elif char == 'r':
        return Rook("black", position)
    elif char == 'N':
        return Knight("white", position)
    elif char == 'n':
        return Knight("black", position)
    elif char == 'B':
        return Bishop("white", position)
    elif char == 'b':
        return Bishop("black", position)
    elif char == 'Q':
        return Queen("white", position)
    elif char == 'q':
        return Queen("black", position)
    else:
        return None  # Return None for empty squares (e.g., '.')
    
# def move_piece(board, from_position, to_position):
#     row_from, col_from = from_position
#     row_to, col_to = to_position
#     king_aliases = ['S', 'W', 'U', 's', 'w', 'u']
#     piece = board[row_from][col_from]
    
#     # Check if the pawn is promoting (promotion coordinates are greater than 63)
#     if piece == 'P' or piece == 'p' and row_to > 7:
#         # Extract the promotion code before converting coordinates
#         code = col_to // 4
#         print(f"code is: {code}")
#         # Determine the promoted piece
#         promotion_pieces = ['q', 'n', 'r', 'b']
#         promo_piece = promotion_pieces[code]
#         print(f"piece is: {promo_piece}")
#         # Conditionally uppercase the piece if it's a white pawn
#         # if piece.isupper():
#         #     piece = promo_piece.upper()
#         piece = promo_piece
        
#         # Now convert the promotion coordinates to regular coordinates
#         print(f"giving {row_to}{col_to}")
#         to_position = promotion_coord_to_regular_coord((row_to, col_to))
#         row_to = to_position[0]
#         col_to = to_position[1]

#     # Revoke en passant eligibility on other pieces
#     for row in range(8):
#         for col in range(8):
#             if board[row][col] == 'E':
#                 board[row][col] = 'P'
#             elif board[row][col] == 'e':
#                 board[row][col] = 'p'

#     # Handle en passant eligibility and capture
#     if piece == 'P' or piece == 'p':
#         en_passant_eligible = False
#         if piece == 'P' and row_from - row_to == 2:  # White pawn moving two squares
#             en_passant_eligible = (
#                 (col_from > 0 and board[4][col_to - 1] == 'p') or
#                 (col_from < 7 and board[4][col_to + 1] == 'p')
#             )
#         elif piece == 'p' and row_from - row_to == -2:  # Black pawn moving two squares
#             en_passant_eligible = (
#                 (col_from > 0 and board[3][col_to - 1] == 'P') or
#                 (col_from < 7 and board[3][col_to + 1] == 'P')
#             )
        
#         if en_passant_eligible:
#             piece = 'E' if piece == 'P' else 'e'

#         # Handle en passant capture
#         if col_to != col_from and board[row_to][col_to] == '.':
#             if piece.isupper():
#                 board[row_to + 1][col_to] = '.'
#             elif piece.islower():
#                 board[row_to - 1][col_to] = '.'

#     # Handle castling rights revocation
#     if piece == 'R' or piece == 'r':
#         handle_castling_revoke(board, row_from, col_from)

#     # Handle king movement and castling
#     if piece in king_aliases:
#         handle_king_movement(board, row_from, col_from, row_to, col_to)
#         piece = 'K' if piece.isupper() else 'k'  # Revoke castling rights
    
#     # Update the board with the new piece position
#     board[row_from][col_from] = "."
#     board[row_to][col_to] = piece

#     return board

def move_piece(board, from_position, to_position):
    row_from, col_from = from_position
    row_to, col_to = to_position
    king_aliases = ['S', 'W', 'U', 's', 'w', 'u']
    #Revoke en passant eligibility on other pieces
    for row in range(8):
        for col in range(8):
            if board[row][col] == 'E':
                board[row][col] = 'P'
            elif board[row][col] == 'e':
                board[row][col] = 'p'

    # Check if there's a piece at the from_position
    piece = board[row_from][col_from]
    if piece:
        # First order of business is detecting if we're moving a pawn or not for some pawn-specific logic.
        if piece == 'P' or piece == 'p':
            # Determine if this pawn is eligible to be captured en passant
            en_passant_eligible = False
            if piece == 'P' and row_from-row_to == 2:  # White pawn
                en_passant_eligible = (
                    (col_from > 0 and board[4][col_to - 1] == 'p') or  # Check left for black pawn
                    (col_from < 7 and board[4][col_to + 1] == 'p')     # Check right for black pawn
                )
            elif piece == 'p' and row_from-row_to == -2:  # Black pawn
                en_passant_eligible = (
                    (col_from > 0 and board[3][col_to - 1] == 'P') or  # Check left for white pawn
                    (col_from < 7 and board[3][col_to + 1] == 'P')     # Check right for white pawn
                )
            
            # If the pawn moved forward two squares AND there's an enemy pawn able to capture it en passant
        
            if en_passant_eligible:
                # Make it eligible to be en passanted
                piece = 'E' if piece == 'P' else 'e'
            # If we're capturing en passant. Determine by checking if moving to a different column onto an empty square
            if col_to != col_from and board[row_to][col_to] == '.':
                if piece.isupper():
                    board[row_to + 1][col_to] = '.'
                elif piece.islower():
                    board[row_to - 1][col_to] = '.'

        #Second, we need to revoke castling rights if it is a rook
        elif piece == 'R':
            # If we're moving the a1 rook from its starting square
            # and there's a king with both sides castling rights on the starting square, revoke
            # this king's queenside castling rights.
            if (row_from, col_from) == (7, 0) and board[7][4] == 'W': #Both sides castling rights
                board[7][4] = 'S' #Convert to kingside only castling rights

            elif (row_from, col_from) == (7, 0) and board[7][4] == 'U': #Queenside only castling rights
                board[7][4] = 'K' #Convert to no castling rights
            
            elif(row_from, col_from) == (7, 7) and board[7][4] == 'W': #Both sides castling rights
                board[7][4] = 'U' #Queenside only castling rights
            
            elif(row_from, col_from) == (7, 7) and board[7][4] == 'S': #Kingside only castling rights
                board[7][4] = 'K' #convert to no castling rights

        elif piece == 'r':
            # If we're moving the a8 rook from its starting square
            # and there's a king with both sides castling rights on the starting square, revoke
            # this king's queenside castling rights.
            if (row_from, col_from) == (0, 0) and board[0][4] == 'w': #Both sides castling rights
                board[0][4] = 's' #Convert to kingside only castling rights

            elif (row_from, col_from) == (0, 0) and board[0][4] == 'u': #Queenside only castling rights
                board[0][4] = 'k' #Convert to no castling rights
            
            elif(row_from, col_from) == (0, 7) and board[0][4] == 'w': #Both sides castling rights
                board[0][4] = 'u' #Queenside only castling rights
            
            elif(row_from, col_from) == (0, 7) and board[0][4] == 's': #Kingside only castling rights
                board[0][4] = 'k' #convert to no castling rights
        elif piece in king_aliases: #We're moving a king.
            piece = 'K' if piece.isupper() else 'k' #Revoke all castling rights.
            if(abs(col_from - col_to) == 2): #We are castling
                if(col_to == 2): #Queenside
                    if piece.isupper(): #White
                        board[7][0] = '.'
                        board[7][3] = 'R'
                    else: #Black
                        board[0][0] = '.'
                        board[0][3] = 'r'
                else: #Kingside
                    if piece.isupper(): #White
                        board[7][7] = '.'
                        board[7][5] = 'R'
                    else: #Black
                        board[0][7] = '.'
                        board[0][5] = 'r'
        # Update the board state
        board[row_from][col_from] = "."
        board[row_to][col_to] = piece

    else:
        raise ValueError("Tried to move a piece that doesn't exist.")
    
#Returns the coordinates of the kings 
def find_kings(board):
    white_king_aliases = ["K", "W", "U", "S"]
    black_king_aliases = ["k", "w", "u", "s"]

    kings = []
    for row in range(8):
        for col in range(8):
            if board[row][col] in white_king_aliases:
                kings.append((row, col))
    for row in range(8):
        for col in range(8):
            if board[row][col] in black_king_aliases:
                kings.append((row, col))
    return kings

#Look to see if the king is in check.
def look_for_check(board):
    # Find the king's positions
    king_positions = find_kings(board)
    white_in_check = False
    black_in_check = False
    for row in range(8):
        for col in range(8):
            piece = board[row][col]
            for king_position in king_positions:
                king_row = king_position[0]
                king_col = king_position[1]
                isWhite = board[king_row][king_col].isupper()
                if piece.islower() and isWhite: #If a black piece on white's turn:
                    enemy_piece = create_piece_from_char(piece, (row, col))
                    if king_position in enemy_piece.in_vision_positions(board):
                        white_in_check = True
                elif piece.isupper() and not isWhite: #If a white piece on black's turn
                    enemy_piece = create_piece_from_char(piece, (row, col))
                    if king_position in enemy_piece.in_vision_positions(board):
                        black_in_check = True
    if(white_in_check and black_in_check):
        return "both_in_check"
    elif white_in_check:
        return "white_in_check"
    elif black_in_check:
        return "black_in_check"

    return "none_in_check"

def simulate_move(board, from_pos, to_position):
    new_board = copy.deepcopy(board)
    move_piece(new_board, from_pos, to_position)

    return new_board

#Returns the position of a pawn on the back rank, if there is one. If there is not, it returns (-1, -1)
def pawn_on_back_rank(board) -> tuple:
    for col in range(8):
        if board[0][col] == 'P':
            return (0, col)
        elif board[7][col] == 'p':
            return (7, col)
    return (-1, -1)

def promote(board):
    for col in range(8):
        if board[0][col] == 'P':
            board[0][col] = 'Q'
        elif board[7][col] == 'p':
            board[0][col] = 'q'
def print_board(board):
    for row in board:
        print(" ".join(row))

def set_board_from_string(input_board):
    board_as_list = [[input_board[i * 8 + j] for j in range(8)] for i in range(8)]
    return board_as_list

def coordinate_to_algebraic(coordinate):
    """
    Convert coordinates (0, 0) to algebraic chess notation, e.g., (0, 0) becomes 'a8'.
    """
    if 0 <= coordinate[0] <= 7 and 0 <= coordinate[1] <= 7:
        column = chr(ord('a') + coordinate[1])
        row = str(8 - coordinate[0])
        return f"{column}{row}"
    else:
        raise ValueError("Invalid coordinate")

def algebraic_to_coordinate(algebraic):
    row = 0
    col = 0

    # Validate the basic algebraic notation (e.g., g7, a8)
    if len(algebraic) < 2 or not ('a' <= algebraic[0] <= 'h') or not ('1' <= algebraic[1] <= '8'):
        raise ValueError("Invalid algebraic notation")

    # Convert rank ('1'-'8') to row index (0-7)
    row = 7 - (int(algebraic[1]) - 1)
    # Convert file ('a'-'h') to column index (0-7)
    col = ord(algebraic[0]) - ord('a')

    # Return early for standard moves
    if len(algebraic) == 2:
        return (row, col)

    # Handle promotions with a third character (e.g., g7q, e8n)
    if len(algebraic) == 3 and algebraic[2].lower() in 'qrnb':
        piece_code = 0
        if algebraic[2].lower() == 'n':
            piece_code = 1
        elif algebraic[2].lower() == 'r':
            piece_code = 2
        elif algebraic[2].lower() == 'b':
            piece_code = 3

        # Calculate new row and column for the promoted piece
        if row == 0:
            coordinate = 64 + col * 4 + piece_code
        elif row == 7:
            coordinate = 96 + col * 4 + piece_code
        else:
            raise ValueError("Promotion can only occur on the last rank")

        row = coordinate // 8
        col = coordinate % 8

        return (row, col)

    # If the input length is not valid, raise an error
    print(len(algebraic))
    print(f"\"{algebraic}\"")
    raise ValueError("Invalid algebraic notation")


def split_algebraic(algebraic):
    # Strip any trailing newline or whitespace characters
    algebraic = algebraic.strip()
    if len(algebraic) == 4 or len(algebraic) == 5:
        first_part = algebraic[:2]  # Extract the first two characters
        second_part = algebraic[2:4]  # Extract the next two characters
    else:
        raise ValueError("Invalid algebraic notation")
    first_coordinate = algebraic_to_coordinate(first_part)
    second_coordinate = algebraic_to_coordinate(second_part)
    return (first_coordinate, second_coordinate)
    
def board_to_fen(board):
    fen = ''
    for row in board:
        empty_count = 0
        for cell in row:
            if cell == '.':
                empty_count += 1
            else:
                cell = 'K' if cell in ['W', 'S', 'U'] else cell
                cell = 'k' if cell in ['w', 's', 'u'] else cell

                cell = 'P' if cell == 'E' else cell
                cell = 'p' if cell == 'e' else cell

                if empty_count > 0:
                    fen += str(empty_count)
                    empty_count = 0
                fen += cell
        if empty_count > 0:
            fen += str(empty_count)
        fen += '/'
    fen = fen[:-1]  # Remove the trailing slash
    return fen

def generate_fen(board, active_color='w', castling='KQkq', en_passant='-', halfmove=0, fullmove=1):
    fen_board = board_to_fen(board)
    return f"{fen_board} {active_color} {castling} {en_passant} {halfmove} {fullmove}"

def generate_castling_rights(board):
    castling = ''

    # Flatten the 2D list to make searching easier
    flattened_board = [cell for row in board for cell in row]

    # Look for white castling rights
    if 'W' in flattened_board:
        castling += 'KQ'
    elif 'S' in flattened_board:
        castling += 'K'
    elif 'U' in flattened_board:
        castling += 'Q'

    # Look for black castling rights
    if 'w' in flattened_board:
        castling += 'kq'
    elif 's' in flattened_board:
        castling += 'k'
    elif 'u' in flattened_board:
        castling += 'q'

    # If no castling rights are found
    if not castling:
        return '-'

    return castling

def get_algebraic_notation(row, col):
    files = 'abcdefgh'  # a-h for the file (column)
    ranks = '87654321'  # 8-1 for the rank (row)
    return files[col] + ranks[row]

def find_en_passant_square(board):
    for row in range(len(board)):
        for col in range(len(board[row])):
            if board[row][col] == 'E':  # White en passant pawn
                return get_algebraic_notation(row + 1, col)  # En passant square is below the white pawn
            elif board[row][col] == 'e':  # Black en passant pawn
                return get_algebraic_notation(row - 1, col)  # En passant square is above the black pawn

    return '-'  # No en passant square

def consult_engine(fen) -> str:
    # Run the compiled C++ program and capture the output
    run_process = subprocess.run(["./engine"] + fen.split(), capture_output=True, text=True)
    # run_process = subprocess.run(["./hello"], capture_output=True, text=True)

    if run_process.returncode == 0:
        return run_process.stdout

class Screen:
    def __init__(self, screen_width, screen_height):
        self.screen_width = screen_width
        self.screen_height = screen_height
        self.square_size = screen_width // 8  # Size of each square
        self.board = "" #TODO: fix later
        self.board_as_list = [] #TODO: Make this work off of string boards instead of list boards.
        self.waiting_for_promotion = False
        pygame.init()

        # Create a Pygame window
        self.screen = pygame.display.set_mode((screen_width, screen_height))
        pygame.display.set_caption("Chessboard")

        self.buffer = pygame.Surface((screen_width, screen_height))
        self.buffer.fill((0, 0, 0))  # Fill the buffer with a black background
        # Main game loop flag
        self.running = True

        # Create a variable to track the currently selected piece
        self.selected_piece = None
        self.selected_piece_position = None #Position of the piece that's being dragged
        self.selected_piece_original_position = None # Store the original position of the selected piece
        self.shouldDrag = False
        self.selected_piece_valid_moves = []
        self.last_moved_piece_current_position = None
        self.last_moved_piece_original_position = None

        self.is_white_turn = True

        self.black_is_in_check = False
        self.white_is_in_check = False

        self.selected_promotion_piece = (-1, -1)

        self.board_state = "ongoing" #Can be stalemate or checkmate

        self.madeMoveThisIter = False

    def run(self):
        # Hardcoded player side (replace with actual UI choice later)
        player_side = 'white'  # 'white' or 'black'
        square_to_move_to = (-1, -1)
        fullmove = 1  # Incremented after black moves
        halfmove = 0  # Half moves since a capture/pawn move (for fifty-move rule)

        while self.running:
            # Handle events
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.running = False
                elif event.type == pygame.MOUSEBUTTONDOWN:
                    if event.button == 1:
                        square_to_move_to = self.handle_mouse_button_down(event)
                elif event.type == pygame.MOUSEMOTION:
                    self.handle_mouse_motion(event)
                elif event.type == pygame.MOUSEBUTTONUP:
                    if event.button == 1:
                        square_to_move_to = self.handle_mouse_button_up(event)

            # Clear and redraw the board and pieces
            self.clear_buffer()
            self.draw_board(self.buffer)
            self.highlight_squares(self.buffer)
            self.draw_pieces(self.buffer)
            self.blit_selected_piece()

            # Now either
            # 1. Wait for the player's move
            # 2. Play the player's move
            # 3. Play the engine's move

            # Handle player's move (if it's the player's turn)
            if self.selected_piece_original_position and square_to_move_to != (-1, -1) and self.is_white_turn == (player_side == 'white'):
                capture, pawn_move = self.handle_move(square_to_move_to)

                # Reset halfmove counter on capture or pawn move
                halfmove = 0 if capture or pawn_move else halfmove + 1

                # Play the move, check for promotion
                self.board_state = self.process_move(square_to_move_to, pawn_move)
                # Update FEN after player's move
                if self.madeMoveThisIter:
                    fen = self.generate_fen_string(halfmove, fullmove)
                    self.madeMoveThisIter = False

            # Handle engine's move (after player's move and if it's engine's turn)
            if self.board_state == "ongoing" and self.is_white_turn != (player_side == 'white'):
                # print('test')
                fen = self.generate_fen_string(halfmove, fullmove)
                response = consult_engine(fen)
                if response:
                    from_coord, to_coord = split_algebraic(response)
                    row = from_coord[0]
                    col = from_coord[1]

                    promotion_piece = ''
                    if(len(response) == 5):
                        promotion_piece = response[4]
                        print(response)
                        self.board_as_list[row][col] = promotion_piece
                    self.board_state = self.play_move(from_coord, to_coord)

            # Check game state
            if self.board_state != "ongoing":
                self.running = False

            # Update display
            self.update_display()

    def handle_move(self, square_to_move_to):
        """Handle user's move, check if it's a capture or pawn move."""
        row, col = square_to_move_to
        frow, fcol = self.selected_piece_original_position
        capture = self.board_as_list[row][col] != '.'
        pawn_move = self.board_as_list[frow][fcol].lower() == 'p'

        return capture, pawn_move

    def process_move(self, square_to_move_to, pawn_move):
        """Process the player's move, handling promotions if necessary."""
        frow, fcol = self.selected_piece_original_position
        trow = square_to_move_to[0]

        # Check for pawn promotion
        if pawn_move and (trow == 0 or trow == 7):
            promotion_piece = self.show_promotion_interface(square_to_move_to)

            # If no promotion piece has been selected yet, return current board state
            if promotion_piece == '':
                return self.board_state  # Return the unchanged board state

            # Update the piece if promotion was selected
            self.board_as_list[frow][fcol] = promotion_piece

        # Proceed with the move
        return self.play_move(self.selected_piece_original_position, square_to_move_to)



    def generate_fen_string(self, halfmove, fullmove):
        """Generate FEN string after player's move."""
        color = 'w' if self.is_white_turn else 'b'
        castling_string = generate_castling_rights(self.board_as_list)
        en_passant_square = find_en_passant_square(self.board_as_list)

        if not self.is_white_turn:
            fullmove += 1

        return generate_fen(self.board_as_list, active_color=color, castling=castling_string, en_passant=en_passant_square, halfmove=halfmove, fullmove=fullmove)

    def handle_mouse_button_down(self, event):
        mouse_x, mouse_y = pygame.mouse.get_pos()
        clicked_row = mouse_y // self.square_size
        clicked_col = mouse_x // self.square_size
        clicked_square = (clicked_row, clicked_col)
        piece = self.board_as_list[clicked_row][clicked_col]

        if(self.selected_piece != None): #Piece is selected
            if clicked_square in self.selected_piece_valid_moves:
                return clicked_square
            else:
                self.deselect_piece()
        else:
            # print("no piece selected, looking to pick up piece")
            if piece != ".":
                if(self.is_white_turn and piece.islower()):
                    # print("invalid piece selection. picked black piece on white's turn. do nothing")
                    return (-1, -1)
                elif(not self.is_white_turn and piece.isupper()):
                    # print("invalid piece selection. picked white piece on black's turn. do nothing")
                    return (-1, -1)
                # print("this piece isn't an empty square, it is of the right color, and we don't already have a piece selected. select piece. and drag it.")
                self.select_piece(piece, clicked_square)
                self.shouldDrag = True
        return (-1, -1)

    def handle_mouse_motion(self, event):
        if self.selected_piece:
            mouse_x, mouse_y = pygame.mouse.get_pos()
            self.selected_piece_position = (mouse_y // self.square_size, mouse_x // self.square_size)

    def handle_mouse_button_up(self, event):
        if self.selected_piece:
            # Place the piece back to its original position
            mouse_x, mouse_y = pygame.mouse.get_pos()
            clicked_row = mouse_y // self.square_size
            clicked_col = mouse_x // self.square_size
            clicked_square = (clicked_row, clicked_col)
            if clicked_square != self.selected_piece_original_position:
                if clicked_square not in self.selected_piece_valid_moves:
                    self.deselect_piece()
                    self.shouldDrag = False
                else:
                    # self.board_state = self.play_move(self.selected_piece_original_position, clicked_square)
                    return clicked_square
            else:
                self.shouldDrag = False
        return (-1, -1)
    def show_promotion_interface(self, pawn_position):
        # Define colors
        orange_outer = (207, 95, 35)
        orange_inner = (188, 148, 124)
        # Create a semi-transparent overlay
        overlay = pygame.Surface((self.screen_width, self.screen_height), pygame.SRCALPHA)
        overlay.fill((0, 0, 0, 128))  # Fills the overlay with semi-transparent black

        # Draw the overlay on the screen
        self.buffer.blit(overlay, (0, 0))
        promotion_squares = []
        if pawn_position[0] == 0: #white promotion at top of board
            # Assuming pawn_position is the position of the promoting pawn (e.g., (6, 3) for a white pawn on the 7th rank)
            promotion_squares = [pawn_position, #Should print a queen icon here
                                (pawn_position[0] + 1, pawn_position[1]),  # Should print a knight icon here
                                (pawn_position[0] + 2, pawn_position[1]),  # Should print a rook icon here
                                (pawn_position[0] + 3, pawn_position[1])]  # Should print a bishop icon here
        if pawn_position[0] == 7: #black promotion at bottom of board
            promotion_squares = [pawn_position, #Should print a queen icon here
                                (pawn_position[0] - 1, pawn_position[1]),  # Should print a knight icon here
                                (pawn_position[0] - 2, pawn_position[1]),  # Should print a rook icon here
                                (pawn_position[0] - 3, pawn_position[1])]  # Should print a bishop icon here


        # Assuming pawn_position is the position of the promoting pawn (e.g., (6, 3) for a white pawn on the 7th rank)
        promotion_pieces = ['Q', 'N', 'R', 'B'] if pawn_position[0] == 0 else ['q', 'n', 'r', 'b']
        # Create a gradient surface for the outer rectangle
        gradient_outer_surface = pygame.Surface((self.square_size, self.square_size), 255)
        for y in range(self.square_size):
            alpha = int(255 * (1 - abs(y - self.square_size // 2) / (self.square_size // 2)))
            pygame.draw.rect(gradient_outer_surface, (orange_outer[0], orange_outer[1], orange_outer[2], alpha),
                            (0, y, self.square_size, 1))

        # Blit the gradient surface for the outer rectangle onto the buffer
        x, y = pawn_position[1] * self.square_size, pawn_position[0] * self.square_size
        # self.buffer.blit(gradient_outer_surface, (x, y))
        
        # Create a solid surface for the inner circle
        inner_circle_surface = pygame.Surface((self.square_size, self.square_size), pygame.SRCALPHA)
        pygame.draw.circle(inner_circle_surface, orange_inner, (self.square_size // 2, self.square_size // 2),
                        self.square_size // 2)

        # Draw circles and piece images on promotion squares
        for i, square in enumerate(promotion_squares):
            x, y = square[1] * self.square_size, square[0] * self.square_size
            center = (x + self.square_size // 2, y + self.square_size // 2)
            pygame.draw.circle(self.buffer, (173, 173, 173), center, self.square_size // 2)

            # Get the piece image based on the promotion piece character
            piece_image = get_piece_image(promotion_pieces[i])

            # Calculate the position to blit the piece image
            piece_rect = piece_image.get_rect()
            piece_rect.center = center

            x, y = square[1] * self.square_size, square[0] * self.square_size
            rect = pygame.Rect(x, y, self.square_size, self.square_size)

            # Check if mouse is hovering over the square
            mouse_x, mouse_y = pygame.mouse.get_pos()

            if rect.collidepoint(mouse_x, mouse_y):  # Replace mouse_x and mouse_y with actual mouse coordinates
                # Blit the gradient surface
                self.buffer.blit(gradient_outer_surface, (x, y))
                self.buffer.blit(inner_circle_surface, (x, y))
            # Blit the piece image onto the buffer
            self.buffer.blit(piece_image, piece_rect)
        
        for event in pygame.event.get():
            mouse_x, mouse_y = pygame.mouse.get_pos()
            hovered_square_coord = (mouse_y // self.square_size, mouse_x // self.square_size)
            if event.type == pygame.MOUSEBUTTONDOWN:
                self.selected_promotion_piece = hovered_square_coord if hovered_square_coord in promotion_squares else (-1, -1)
            elif event.type == pygame.MOUSEBUTTONUP: #I think the issue I'm having is here. It's that there needs to be a cooldown.
                if self.selected_promotion_piece != (-1, -1):
                    self.shouldShowPromotionInterface = False
                    promotion_map = {
                        0: 'Q', 1: 'N', 2: 'R', 3: 'B',
                        7: 'q', 6: 'n', 5: 'r', 4: 'b'
                    }
                    return promotion_map.get(self.selected_promotion_piece[0], None)
        self.shouldShowPromotionInterface = True
        return '' #Code for continue showing the interface


    def play_move(self, from_pos, to_pos):
        move_piece(self.board_as_list, from_pos, to_pos)

        
        self.is_white_turn = not self.is_white_turn
        self.white_is_in_check = True if("white_in_check" == look_for_check(self.board_as_list)) else False
        self.black_is_in_check = True if("black_in_check" == look_for_check(self.board_as_list)) else False

        self.madeMoveThisIter = True
        # print(f"is white's turn? {self.is_white_turn}")
        # print(f"is black in check? {self.black_is_in_check}")
        # print(f"no legal moves? {self.no_legal_moves(self.board_as_list)}")
        # print_board(self.board_as_list)
        no_legal_moves = self.no_legal_moves(self.board_as_list)
        if self.is_white_turn and self.white_is_in_check and no_legal_moves:
            print("Checkmate. Black wins.")
            return "Checkmate."
        elif not self.is_white_turn and self.black_is_in_check and no_legal_moves:
            print("Checkmate. White wins.")
            return "Checkmate."
        if self.is_white_turn and not self.white_is_in_check and no_legal_moves:
            print("Stalemate. Draw.")
            return "Stalemate."
        elif not self.is_white_turn and not self.black_is_in_check and no_legal_moves:
            print("Stalemate. Draw.")
            return "Stalemate."
        
        #Write down the squares we moved from and to before we clear it so we can highlight them.
        self.last_moved_piece_original_position = from_pos
        self.last_moved_piece_current_position = to_pos
        self.deselect_piece()
        return "ongoing"

    def no_legal_moves(self, board):
        for row in range(8):
            for col in range(8):
                if self.is_white_turn and board[row][col].isupper():
                    piece = board[row][col]
                    piece_as_class = create_piece_from_char(piece, (row, col))
                    if piece_as_class.valid_moves(board):  # Check if there are any valid moves
                        return False
                elif not self.is_white_turn and board[row][col].islower():
                    piece = board[row][col]
                    piece_as_class = create_piece_from_char(piece, (row, col))
                    if piece_as_class.valid_moves(board):  # Check if there are any valid moves
                        # for move in piece_as_class.valid_moves(board):
                        #     print(f"{coordinate_to_algebraic(piece_as_class.position)}{coordinate_to_algebraic(move)}")
                        return False
        return True

    def select_piece(self, piece, position):
        # print("shouldn't this function only be called once")
        self.selected_piece = piece
        self.selected_piece_original_position = position
        piece_class = create_piece_from_char(piece, position)
        self.selected_piece_valid_moves = piece_class.valid_moves(self.board_as_list)
    def deselect_piece(self):
        self.selected_piece = None
        self.selected_piece_position = None #Position of the piece that's being dragged
        self.selected_piece_original_position = None # Store the original position of the selected piece
        self.selected_piece_valid_moves = []

    def clear_buffer(self):
        # Clear the buffer (not the screen)
        self.buffer.fill((0, 0, 0))

    def blit_selected_piece(self):
        if self.selected_piece and self.shouldDrag:
            mouse_x, mouse_y = pygame.mouse.get_pos()
            selected_piece_image = get_piece_image(self.selected_piece)
            self.buffer.blit(selected_piece_image, (mouse_x - 30, mouse_y - 30))

    def update_display(self):
        # Blit the buffer to the screen
        self.screen.blit(self.buffer, (0, 0))
        pygame.display.update()

    # Draw the chessboard
    def draw_board(self, surface=None):
        if surface is None:
            surface = self.screen  # Default to drawing on the screen
        # Labels for rows and columns
        row_labels = "87654321"
        column_labels = "abcdefgh"
        for row in range(8):
            for col in range(8):
                square_color = WHITE if (row + col) % 2 == 0 else BLACK
                pygame.draw.rect(surface, square_color, (col * self.square_size, row * self.square_size, self.square_size, self.square_size))

                # Determine the color for the labels based on the square's color
                label_color = BLACK if square_color == WHITE else WHITE  # Black text for white squares, white text for black squares
                font = pygame.font.Font(None, 14)

                # Draw row numbers in the top right corner of each square
                row_label = row_labels[row]
                text = font.render(row_label, True, label_color)
                text_rect = text.get_rect()
                text_rect.topright = ((8 - 0.05) * self.square_size, (row + 0.05) * self.square_size)
                surface.blit(text, text_rect)

                # Draw column letters in the bottom left corner of each square along the bottom
                col_label = column_labels[col]
                text = font.render(col_label, True, label_color)
                text_rect = text.get_rect()
                text_rect.bottomleft = ((col + 0.05) * self.square_size, (8 - 0.05) * self.square_size)
                surface.blit(text, text_rect)
    def highlight_squares(self, surface=None):
        if surface is None:
            surface = self.screen  # Default to drawing on the screen
        # Highlight the selected square (if any)
        squares_to_highlight = []
        if self.selected_piece != '.' and self.selected_piece != None:
            row = self.selected_piece_original_position[0]
            col = self.selected_piece_original_position[1]
            squares_to_highlight = self.selected_piece_valid_moves
        highlight_color_light = (130, 150, 105)  # Light square highlight color
        highlight_color_dark = (100, 111, 64)   # Dark square highlight color
        # Define red gradient color for check highlight
        # Check if any king is in check
        check_status = look_for_check(self.board_as_list)
        king_positions = find_kings(self.board_as_list)  # [(white_king_row, white_king_col), (black_king_row, black_king_col)]
        check_gradient_color = (255, 0, 0, 128)  # Semi-transparent red
        last_move_highlight_color_dark = (170, 162, 58)
        last_move_highlight_color_light = (205, 210, 106)
        for row in range(8):
            for col in range(8):
                square_color = WHITE if (row + col) % 2 == 0 else BLACK
                highlight_color = highlight_color_light if square_color == WHITE else highlight_color_dark
                last_move_highlight_color = last_move_highlight_color_light if square_color == WHITE else last_move_highlight_color_dark
                if self.last_moved_piece_original_position == (row, col) or self.last_moved_piece_current_position == (row, col):
                    pygame.draw.rect(surface, last_move_highlight_color, (col * self.square_size, row * self.square_size, self.square_size, self.square_size))
                #Highlight the entire square that the selected piece sits on.
                if self.selected_piece_original_position == (row, col):
                    pygame.draw.rect(surface, highlight_color, (col * self.square_size, row * self.square_size, self.square_size, self.square_size))
                
                #Highlight the squares that the piece can move to.
                if (row, col) in squares_to_highlight:
                    center_x = (col + 0.5) * self.square_size
                    center_y = (row + 0.5) * self.square_size

                    # Draw a small circle or marker at the center
                    if(self.board_as_list[row][col] == '.'):
                        pygame.draw.circle(surface, highlight_color, (int(center_x), int(center_y)), 7, 7)
                    # Draw triangles around the corner of square that the piece can capture to.
                    else:
                        square_rect = pygame.Rect(col * self.square_size, row * self.square_size, self.square_size, self.square_size)
                        top_left = (square_rect.left, square_rect.top)
                        top_right = (square_rect.right-1, square_rect.top)
                        bottom_left = (square_rect.left, square_rect.bottom-1)
                        bottom_right = (square_rect.right-1, square_rect.bottom-1)


                        # Define triangle points
                        triangle_size = 12
                        triangle_points_top_left = [top_left, (top_left[0], top_left[1] + triangle_size), (top_left[0] + triangle_size, top_left[1])]
                        triangle_points_top_right = [top_right, (top_right[0], top_right[1] + triangle_size), (top_right[0] - triangle_size, top_right[1])]

                        triangle_points_bottom_right = [bottom_right, (bottom_right[0], bottom_right[1] - triangle_size), (bottom_right[0] - triangle_size, bottom_right[1])]
                        triangle_points_bottom_left = [bottom_left, (bottom_left[0], bottom_left[1] - triangle_size), (bottom_left[0] + triangle_size, bottom_left[1])]


                        # Draw triangles at the corners
                        pygame.draw.polygon(surface, highlight_color, triangle_points_top_left)
                        pygame.draw.polygon(surface, highlight_color, triangle_points_top_right)
                        pygame.draw.polygon(surface, highlight_color, triangle_points_bottom_left)
                        pygame.draw.polygon(surface, highlight_color, triangle_points_bottom_right)
                    #Highlight the full square of valid moves that the cursor is also hovering.
                    mouse_x, mouse_y = pygame.mouse.get_pos()
                    cursor_board_pos_y = mouse_y // self.square_size
                    cursor_board_pos_x = mouse_x // self.square_size
                    if (cursor_board_pos_y, cursor_board_pos_x) == (row, col) and (row, col) in squares_to_highlight:
                        pygame.draw.rect(surface, highlight_color, (col * self.square_size, row * self.square_size, self.square_size, self.square_size))
            # Add red gradient circle behind the king in check
        if "white_in_check" in check_status:
            white_king_row, white_king_col = king_positions[0]
            center_x = (white_king_col + 0.5) * self.square_size
            center_y = (white_king_row + 0.5) * self.square_size
            pygame.draw.circle(surface, check_gradient_color, (int(center_x), int(center_y)), self.square_size // 2, 0)

        if "black_in_check" in check_status:
            black_king_row, black_king_col = king_positions[1]
            center_x = (black_king_col + 0.5) * self.square_size
            center_y = (black_king_row + 0.5) * self.square_size
            pygame.draw.circle(surface, check_gradient_color, (int(center_x), int(center_y)), self.square_size // 2, 0)
    def draw_pieces(self, surface=None):
        if surface is None:
            surface = self.screen  # Default to drawing on the screen
        # Draw the pieces based on self.board_as_list
        for row in range(8):
            for col in range(8):
                piece = self.board_as_list[row][col]

                if piece != ".":
                    image = get_piece_image(piece)
                    if (row, col) == self.selected_piece_original_position and self.shouldDrag:
                        image.convert_alpha()
                        image.set_alpha(128)
                    if image:
                        surface.blit(image, (col * self.square_size, row * self.square_size))




class Board:
    def __init__(self, screen_width, screen_height):
        self.screen_width = screen_width
        self.screen_height = screen_height
        self.square_size = screen_width // 8  # Size of each square
        self.board_as_string = "................................................................" #defaulted to full empty board
        self.board_as_list = [] #Board as a list of chars. This is useful because lists are mutable.
        


        # Initialize Pygame
        pygame.init()

        # Create a Pygame window
        self.screen = pygame.display.set_mode((screen_width, screen_height))
        pygame.display.set_caption("Chessboard")

        self.buffer = pygame.Surface((screen_width, screen_height))
        self.buffer.fill((0, 0, 0))  # Fill the buffer with a black background
        # Main game loop flag
        self.running = True

class Piece:
    def __init__(self, color, position):
        self.color = color  # Color of the piece ("white" or "black")
        self.position = position  # Current position of the piece (e.g., "e2")
        
    def valid_moves(self, board):
        valid_moves = []
        in_vision_positions = self.in_vision_positions(board)
        for new_position in in_vision_positions:
            
            #Check if this move puts us in check, or doesn't resolve the check we're in.
            hypothetical_board = simulate_move(board, self.position, new_position)
            check_status = look_for_check(hypothetical_board)
            # if self.position == (0, 3):
            #     print(f"check status: {check_status}")
            #     print_board(hypothetical_board)
            if(self.color == "white" and (check_status == "white_in_check" or check_status == "both_in_check")):
                #if so, don't add it to the list of valid moves.
                continue
            elif(self.color == "black" and (check_status == "black_in_check" or check_status == "both_in_check")):
                continue

            new_row = new_position[0]
            new_col = new_position[1]
            if board[new_row][new_col] == '.': #If it's empty, add it.
                valid_moves.append(new_position)
            elif board[new_row][new_col].isupper() and self.color == "black": #If it's enemy, add it.
                valid_moves.append(new_position)
            elif board[new_row][new_col].islower() and self.color == "white": #If it's enemy, add it.
                valid_moves.append(new_position)

        return valid_moves

    def move(self, new_position):
        """
        Move the piece to a new position on the board.
        """
        self.position = new_position

    def in_vision_positions(self, board):
        """
        Returns a list of squares that are occupied by allies
        that are defended by this piece. Used for checking legal
        king moves.
        """
        raise NotImplementedError("Subclasses of Piece must implement in_vision_positions method")
    
class Pawn(Piece):

    def __init__(self, color, position):
        super().__init__(color, position)  # Call the parent class's constructor
        self.en_passant_eligible = False  # Class-level variable

    def valid_moves(self, board):
        valid_moves = []

        # Determine the current position's coordinates
        row, col = self.position[0], self.position[1]

        # Define the direction based on the pawn's color
        direction = -1 if self.color == "white" else 1

        # Check one square forward
        new_row = row + direction
        if 0 <= new_row <= 7 and board[new_row][col] == ".":
            #Add queening logic here. 
            #We are queening and need to have the coordinate reflect the queening data.
            # if new_row == 0:
            #     for i in range(4):
            #         to_pos_int = (64 + (col * 4) + i)
            #         valid_moves.append((to_pos_int/8, col))  # All promotions go to row 0
            # elif new_row == 7:
            #     for i in range(4):
            #         to_pos_int = (96 + (col * 4) + i)
            #         valid_moves.append((to_pos_int/8, col))  # All promotions go to row 7
            # else:
            valid_moves.append((new_row, col))

        # Check two squares forward (only on the first move)
        if (row == (6 if self.color == "white" else 1)
            and board[row + direction][col] == "."
            and board[row + direction * 2][col] == "."):
            valid_moves.append((row + direction * 2, col))

        # Check diagonal captures
        for position in self.in_vision_positions(board):
            new_row = position[0]
            new_col = position[1]
            piece_exists = board[new_row][new_col] != "."
            is_enemy_piece = (
                (board[new_row][new_col].islower() and self.color == "white") or
                (board[new_row][new_col].isupper() and self.color == "black")
            )

            if (piece_exists and is_enemy_piece):
                valid_moves.append(position)
            
        # Define en passant eligible markers for each color
        en_passant_markers = {"white": 'e', "black": 'E'}

        # Check for en passant
        for offset in [-1, 1]:
            new_col = col + offset
            is_in_bounds = 0 <= new_col <= 7 and 0 <= row <= 7
            if is_in_bounds and board[row][new_col] == en_passant_markers.get(self.color):
                valid_moves.append((new_row, new_col))

        new_valid_moves = [] #This variable is used to keep track of the valid moves that do not put us in check
        for new_position in valid_moves:
            hypothetical_board = simulate_move(board, self.position, new_position)
            check_status = look_for_check(hypothetical_board)
            if(self.color == "white" and (check_status == "white_in_check" or check_status == "both_in_check")):
                #if so, don't add it to the list of valid moves.
                continue
            elif(self.color == "black" and (check_status == "black_in_check" or check_status == "both_in_check")):
                continue
            else:
                new_valid_moves.append(new_position)

        valid_moves = new_valid_moves
        return valid_moves
    def in_vision_positions(self, board):
        defended_positions = []
        row, col = self.position[0], self.position[1]

        # Determine the direction based on the pawn's color
        direction = -1 if self.color == "white" else 1

        # Check diagonal captures
        for offset in [-1, 1]:
            new_col = col + offset
            new_row = row + direction
            if 0 <= new_row < 8 and 0 <= new_col < 8:
                defended_positions.append((new_row, new_col))
        return defended_positions
    
class Rook(Piece):

    def __init__(self, color, position):
        super().__init__(color, position)  # Call the parent class's constructor
    
    def in_vision_positions(self, board):
        in_vision_positions = []
        row, col = self.position[0], self.position[1]
        directions = [(1, 0), (-1, 0), (0, 1), (0, -1)]

        for dr, dc in directions:
            new_row, new_col = row + dr, col + dc
            while 0 <= new_row < 8 and 0 <= new_col < 8:
                piece = board[new_row][new_col]
                in_vision_positions.append((new_row, new_col))
                if piece != ".":
                    in_vision_positions.append((new_row, new_col))
                    break
                new_row, new_col = new_row + dr, new_col + dc
        return in_vision_positions

class Knight(Piece):

    def __init__(self, color, position):
        super().__init__(color, position)  # Call the parent class's constructor

    def in_vision_positions(self, board):
        in_vision_positions = []
        row, col = self.position[0], self.position[1]

        # Define the possible knight moves
        knight_moves = [(-2, -1), (-2, 1), (-1, -2), (-1, 2), (1, -2), (1, 2), (2, -1), (2, 1)]

        for dr, dc in knight_moves:
            new_row, new_col = row + dr, col + dc
            is_in_bounds = 0 <= new_row < 8 and 0 <= new_col < 8
            if is_in_bounds:
                in_vision_positions.append((new_row, new_col))
        return in_vision_positions
class Bishop(Piece):

    def __init__(self, color, position):
        super().__init__(color, position)  # Call the parent class's constructor

    def in_vision_positions(self, board):
        in_vision_positions = []
        row, col = self.position[0], self.position[1]

        # Define the four diagonal directions
        directions = [(-1, -1), (-1, 1), (1, -1), (1, 1)]
        for dr, dc in directions:
            new_row, new_col = row + dr, col + dc
            while 0 <= new_row < 8 and 0 <= new_col < 8:
                piece = board[new_row][new_col]
                in_vision_positions.append((new_row, new_col))
                if piece != ".":
                    break
                new_row, new_col = new_row + dr, new_col + dc

        return in_vision_positions

class Queen(Piece):
    def __init__(self, color, position):
        super().__init__(color, position)  # Call the parent class's constructor

    def in_vision_positions(self, board):
        in_vision_positions = []
        row, col = self.position[0], self.position[1]

        # Define the eight directions (vertical, horizontal, and diagonal)
        directions = [(1, 0), (-1, 0), (0, 1), (0, -1), (-1, -1), (-1, 1), (1, -1), (1, 1)]

        for dr, dc in directions:
            new_row, new_col = row + dr, col + dc
            while 0 <= new_row < 8 and 0 <= new_col < 8:
                piece = board[new_row][new_col]
                in_vision_positions.append((new_row, new_col))
                if piece != ".":
                    break
                new_row, new_col = new_row + dr, new_col + dc
        return in_vision_positions

    
class King(Piece):
    def __init__(self, color, position):
        super().__init__(color, position)  # Call the parent class's constructor
        self.has_queenside_castling_rights = True
        self.has_kingside_castling_rights = True

    def in_vision_positions(self, board) -> list:
        defended_positions = []
        row, col = self.position[0], self.position[1]

        # Define the eight directions the king can move (vertical, horizontal, and diagonal)
        directions = [(1, 0), (-1, 0), (0, 1), (0, -1), (-1, -1), (-1, 1), (1, -1), (1, 1)]

        for dr, dc in directions:
            new_row, new_col = row + dr, col + dc
            if 0 <= new_row < 8 and 0 <= new_col < 8:
                defended_positions.append((new_row, new_col))
        return defended_positions
    
    def valid_moves(self, board):
        valid_moves = []
        default_row = 7 if self.color == "white" else 0
        enemy_vision = []
        for row in range(8):
            for col in range(8):
                if self.color == "white" and board[row][col].islower():
                    piece = board[row][col]
                    piece_as_class = create_piece_from_char(piece, (row, col))
                    enemy_vision.extend(piece_as_class.in_vision_positions(board))
                elif self.color == "black" and board[row][col].isupper():
                    piece = board[row][col]
                    piece_as_class = create_piece_from_char(piece, (row, col))
                    enemy_vision.extend(piece_as_class.in_vision_positions(board))

        rook_row = 7 if self.color == "white" else 0
        #Check kingside castling
        path_is_empty = board[default_row][5] == '.' and board[default_row][6] == '.'
        path_is_safe = board[default_row][4] not in enemy_vision and board[default_row][5] not in enemy_vision and board[default_row][6] not in enemy_vision
        rook_char = 'R' if self.color == "white" else 'r'
        rook_is_there = True if board[rook_row][7] == rook_char else False
        check = look_for_check(board)
        in_check = True if (check == "white_in_check" and self.color == "white") or (check == "black_in_check" and self.color == "black") else False
        if self.has_kingside_castling_rights and path_is_empty and path_is_safe and rook_is_there and not in_check:
            valid_moves.append((default_row, 6))
        
        #Check queenside castling
        path_is_empty = board[default_row][1] == '.' and board[default_row][2] == '.' and board[default_row][3] == '.'
        path_is_safe = board[default_row][2] not in enemy_vision and board[default_row][3] not in enemy_vision and board[default_row][4] not in enemy_vision
        rook_char = 'R' if self.color == "white" else 'r'
        rook_is_there = True if board[rook_row][0] == rook_char else False
        if self.has_kingside_castling_rights and path_is_empty and path_is_safe and rook_is_there:
            valid_moves.append((default_row, 2))

        for new_position in self.in_vision_positions(board):
            if new_position in enemy_vision:
                continue
            new_row = new_position[0]
            new_col = new_position[1]
            #if the move is onto an ally piece, skip it
            if board[new_row][new_col].islower() and self.color == "black":
                continue
            if board[new_row][new_col].isupper() and self.color == "white":
                continue
            #then simulate the move to check if puts us in check
            hypothetical_board = simulate_move(board, self.position, new_position)
            check_status = look_for_check(hypothetical_board)
            if(self.color == "white" and (check_status == "white_in_check" or check_status == "both_in_check")):
                #if so, don't add it to the list of valid moves.
                continue
            elif(self.color == "black" and (check_status == "black_in_check" or check_status == "both_in_check")):
                continue
            else:
                valid_moves.append(new_position)
        return valid_moves

screen = Screen(480, 480)
starting_position = "rnbqwbnrpppppppp................................PPPPPPPPRNBQWBNR"
# starting_position = "r...k..rppp.pppp..n......Bp...........b.BP..qN..P.PP..PPRN..K..R"
# starting_position = "r....rk.pp...ppp.pp.......B..............P..PPR.PBP....P..K....."
# starting_position =  "r...w.......r...............................................W..R"
# starting_position = "k.........KP...................................................."
# starting_position =  "k.........K............................................p........"
# starting_position = "...r...kppp....p..b..p.....p.......Q.pR.P.B......PP..PPP....R.K."
# starting_position = "k............................................................rKR"
# starting_position = "................................................................"
# starting_position = "k..q............................................PPP.....R...U..."
# starting_position =   ".r.r.k...ppbqp.Qp.n.p......pP.Np...P...PP.....R..PP..PP..K.R...."

screen.board_as_list = set_board_from_string(starting_position)
screen.run()
