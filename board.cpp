#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>

#define _HUGE_ENUF 1e+300
#define INFINITY ((float)(_HUGE_ENUF * _HUGE_ENUF))
using namespace std;

// Forward declaration of class so that
// I can do a function declaration for create_piece_from_char so I can
// use it in hasKingsideCastlingRights
class Piece;
string coordinate_to_algebraic(int coordinate);
Piece* create_piece_from_char(char piece_char, int position);
struct BoardState {
    string board;
    char active_color;
    string castling_rights;
    string en_passant;
    int halfmove;
    int fullmove;
};

bool isSideInCheck(const string& board, bool isWhite);
int game_over(const BoardState& boardState);


int findKing(const string& board, bool isWhite) {
    string kingAliasesWhite = "KWUS";
    string kingAliasesBlack = "kwus";

    // Choose the appropriate king aliases based on the color
    const string& kingAliases = isWhite ? kingAliasesWhite : kingAliasesBlack;

    for (size_t i = 0; i < board.size(); ++i) {
        char piece = board[i];
        if (kingAliases.find(piece) != string::npos) {
            return static_cast<int>(i);
        }
    }

    // Return -1 if no king found
    return -1;
}
void print_board(const string& board) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            int index = row * 8 + col;
            cout << board[index] << " ";
        }
        cout << endl;
    }
}

// string movePiece(const string& board, int fromIndex, int toIndex) {
//     int fromRow = fromIndex / 8;
//     int fromCol = fromIndex % 8;
//     int toRow = toIndex / 8;
//     int toCol = toIndex % 8;
    
//     string kingAliases = "SWUswu"; // Kings and their castling rights

//     // Create a copy of the board to make changes
//     string newBoard = board;

//     // Revoke en passant eligibility on other pieces
//     for (int row = 0; row < 8; ++row) {
//         for (int col = 0; col < 8; ++col) {
//             char piece = newBoard[row * 8 + col];
//             if (piece == 'E') {
//                 newBoard[row * 8 + col] = 'P';
//             } else if (piece == 'e') {
//                 newBoard[row * 8 + col] = 'p';
//             }
//         }
//     }

//     // Check if there's a piece at the fromPosition
//     char piece = newBoard[fromRow * 8 + fromCol];
//     if (piece != '.') {
//         int white_king_pos = findKing(newBoard, true);
//         int black_king_pos = findKing(newBoard, false);
//         // First order of business is detecting if we're moving a pawn or not for some pawn-specific logic.
//         if (piece == 'P' || piece == 'p') {
//             if (abs(toRow - fromRow) == 2) { // if a pawn moved forward two squares
//                 piece = (piece == 'P') ? 'E' : 'e'; // make it eligible to be en passanted
//             }
//             else if (toCol != fromCol && newBoard[toRow * 8 + toCol] == '.') { // capturing en passant
//                 if (isupper(piece)) {
//                     newBoard[(toRow + 1) * 8 + toCol] = '.';
//                 } else {
//                     newBoard[(toRow - 1) * 8 + toCol] = '.';
//                 }
//             }
//             else if (toIndex > 63) { // Queening
//                 int piece_code = toIndex % 4;
//                 char piece_char = '.';
//                 switch(piece_code){
//                     case 0:
//                         // toIndex = 
//                         piece_char = isupper(piece) ? 'Q' : 'q';
//                         break;
//                     case 1:
//                         piece_char = isupper(piece) ? 'N' : 'n';
//                         break;
//                     case 2:
//                         piece_char = isupper(piece) ? 'R' : 'r';
//                         break;
//                     case 3:
//                         piece_char = isupper(piece) ? 'B' : 'b';
//                         break;
//                     default:
//                         cerr << "Can't promote to this piece." << endl;
//                         break;
//                 }
//                 switch (toIndex) {
//                     case 64: case 65: case 66: case 67:
//                         toIndex = 0;
//                         break;
//                     case 68: case 69: case 70: case 71:
//                         toIndex = 1;
//                         break;
//                     case 72: case 73: case 74: case 75:
//                         toIndex = 2;
//                         break;
//                     case 76: case 77: case 78: case 79:
//                         toIndex = 3;
//                         break;
//                     case 80: case 81: case 82: case 83:
//                         toIndex = 4;
//                         break;
//                     case 84: case 85: case 86: case 87:
//                         toIndex = 5;
//                         break;
//                     case 88: case 89: case 90: case 91:
//                         toIndex = 6;
//                         break;
//                     case 92: case 93: case 94: case 95:
//                         toIndex = 7;
//                         break;
//                     case 96: case 97: case 98: case 99:
//                         toIndex = 56;
//                         break;
//                     case 100: case 101: case 102: case 103:
//                         toIndex = 57;
//                         break;
//                     case 104: case 105: case 106: case 107:
//                         toIndex = 58;
//                         break;
//                     case 108: case 109: case 110: case 111:
//                         toIndex = 59;
//                         break;
//                     case 112: case 113: case 114: case 115:
//                         toIndex = 60;
//                         break;
//                     case 116: case 117: case 118: case 119:
//                         toIndex = 61;
//                         break;
//                     case 120: case 121: case 122: case 123:
//                         toIndex = 62;
//                         break;
//                     case 124: case 125: case 126: case 127:
//                         toIndex = 63;
//                         break;
//                     default:
//                         cerr << "Invalid promotion index." << endl;
//                         break;
//                 }
//                 piece = piece_char;
//             }
//         }
//         // 60 and 4 are the default positions of the kings.
//         // Second, we need to revoke castling rights if it is a rook
//         else if ((piece == 'r' && black_king_pos == 4) || (piece == 'R' && white_king_pos == 60)) {
//             bool isQueensideRook = (fromCol == 0);

//             // Determine the character representing the king's castling rights
//             char& kingRights = newBoard[isupper(piece) ? 60 : 4]; 

//             if (isQueensideRook) {
//                 if (kingRights == 'W' || kingRights == 'w') {
//                     kingRights = (isupper(piece)) ? 'S' : 's';
//                 } else if (kingRights == 'U' || kingRights == 'u') {
//                     kingRights = (isupper(piece)) ? 'K' : 'k';
//                 }
//             }
//             else {
//                 if (kingRights == 'W' || kingRights == 'w') {
//                     kingRights = (isupper(piece)) ? 'U' : 'u';
//                 } else if (kingRights == 'S' || kingRights == 's') {
//                     kingRights = (isupper(piece)) ? 'K' : 'k';
//                 }
//             }
//         }

//         // Third, we need to handle castling rights for the king.
//         else if (kingAliases.find(piece) != string::npos) { //If we're moving a king that has any sorts of castling rights
//             piece = (isupper(piece)) ? 'K' : 'k'; // Revoke all castling rights
//             if (abs(fromCol - toCol) == 2) { // If we are castling
//                 if (toCol == 2) { // Queenside castling
//                     if (isupper(piece)) { // White
//                         newBoard[7 * 8 + 0] = '.'; //Also move the rook
//                         newBoard[7 * 8 + 3] = 'R'; //Also move the rook
//                     } else { // Black
//                         newBoard[0 * 8 + 0] = '.'; //Also move the rook
//                         newBoard[0 * 8 + 3] = 'r'; //Also move the rook
//                     }
//                 } else { // Kingside castling
//                     if (isupper(piece)) { // White
//                         newBoard[7 * 8 + 7] = '.'; //Also move the rook
//                         newBoard[7 * 8 + 5] = 'R'; //Also move the rook
//                     } else { // Black
//                         newBoard[0 * 8 + 7] = '.'; //Also move the rook
//                         newBoard[0 * 8 + 5] = 'r'; //Also move the rook
//                     }
//                 }
//             }
//         }

//         toRow = toIndex / 8;
//         toCol = toIndex % 8;
//         // Update the board state
//         newBoard[fromRow * 8 + fromCol] = '.';
//         newBoard[toRow * 8 + toCol] = piece;

//         return newBoard;
//     } else {
//         cerr << "Tried to move a piece that doesn't exist." << endl;
//         return board; // Return the original board if no piece was found
//     }
// }
pair<int, char> unpackPromotionIndex(int toIndex) {
    char promotionPiece = '\0';  // Default to no promotion

    if (toIndex >= 64 && toIndex <= 127) {
        int col = (toIndex - 64) / 4;
        int pieceType = (toIndex - 64) % 4;

        // Convert index back to a normal in-bounds index
        toIndex = (toIndex < 96) ? col : 56 + col;  // Row 0 for white, row 7 for black

        // Set the promotion piece
        switch (pieceType) {
            case 0: promotionPiece = 'q'; break;
            case 1: promotionPiece = 'n'; break;
            case 2: promotionPiece = 'r'; break;
            case 3: promotionPiece = 'b'; break;
        }
    }

    return {toIndex, promotionPiece};
}

string movePiece(string board, int fromIndex, int toIndex, char promote_to = '\0') {
    int fromRow = fromIndex / 8;
    int fromCol = fromIndex % 8;
    int toRow = toIndex / 8;
    int toCol = toIndex % 8;

    string kingAliases = "SWUswu"; // Kings and their castling rights
    string newBoard = board; // Create a copy of the board to make changes

    // Revoke en passant eligibility on all pawns
    for (int i = 0; i < 64; ++i) {
        if (newBoard[i] == 'E') newBoard[i] = 'P';
        else if (newBoard[i] == 'e') newBoard[i] = 'p';
    }

    // Check if there's a piece at the fromPosition
    char piece = newBoard[fromIndex];
    if (piece == '.') {
        cerr << "Tried to move a piece that doesn't exist." << endl;
        return board; // No piece to move, return the original board
    }

    // Handle pawn-specific logic (promotion, en passant)
    if (piece == 'P' || piece == 'p') {
        // Check for pawn advancing two squares (for en passant)
        if (abs(toRow - fromRow) == 2) {
            piece = (piece == 'P') ? 'E' : 'e'; // Make it en passant eligible
        }
        // Handle en passant capture
        else if (toCol != fromCol && newBoard[toIndex] == '.') {
            int capturedPawnIndex = (piece == 'P') ? (toIndex + 8) : (toIndex - 8);
            newBoard[capturedPawnIndex] = '.'; // Remove captured pawn
        }
        // Handle pawn promotion
        else if ((toRow == 0 || toRow == 7) && promote_to != '\0') {
            piece = promote_to; // Promote to the specified piece
        }
    }

    // Handle rook moving (to revoke castling rights)
    else if ((piece == 'R' || piece == 'r') && (fromIndex == 0 || fromIndex == 7 || fromIndex == 56 || fromIndex == 63)) {
        int kingPos = isupper(piece) ? findKing(newBoard, true) : findKing(newBoard, false);
        if (kingPos == (isupper(piece) ? 60 : 4)) { // Revoke castling rights for the respective king
            bool isQueenside = (fromCol == 0);
            char& kingRights = newBoard[kingPos];
            if (isQueenside) {
                kingRights = (kingRights == 'W' || kingRights == 'w') ? (isupper(piece) ? 'S' : 's') : (isupper(piece) ? 'K' : 'k');
            } else {
                kingRights = (kingRights == 'W' || kingRights == 'w') ? (isupper(piece) ? 'U' : 'u') : (isupper(piece) ? 'K' : 'k');
            }
        }
    }

    // Handle king moving (castling rights and castling move)
    else if (kingAliases.find(piece) != string::npos) {
        piece = isupper(piece) ? 'K' : 'k'; // Revoke all castling rights
        // Handle castling
        if (abs(fromCol - toCol) == 2) { // If the king moved two squares (castling)
            int rookFrom, rookTo;
            if (toCol == 2) { // Queenside castling
                rookFrom = isupper(piece) ? 56 : 0;
                rookTo = isupper(piece) ? 59 : 3;
            } else { // Kingside castling
                rookFrom = isupper(piece) ? 63 : 7;
                rookTo = isupper(piece) ? 61 : 5;
            }
            newBoard[rookTo] = newBoard[rookFrom]; // Move the rook
            newBoard[rookFrom] = '.'; // Clear old rook position
        }
    }

    // Update the board with the move
    newBoard[fromIndex] = '.';
    newBoard[toIndex] = piece;

    return newBoard;
}


string simulateMove(const string& board, int fromPosition, int toPosition) {
    auto [realToIndex, promotionPiece] = unpackPromotionIndex(toPosition);
    string newBoard = movePiece(board, fromPosition, realToIndex, promotionPiece);
    return newBoard;
}


class Piece {
public:
    Piece(bool isWhite, int position) : isWhite(isWhite), position(position) {}

    // virtual set<int> validMoves(const string& board) const;

    virtual set<int> validMoves(const string& board) const {
        set<int> validMoves;
        set<int> inVisionPositions = this->inVisionPositions(board);
        
        for (int newPosition : inVisionPositions) {
            // Check if this move puts us in check
            string hypotheticalBoard = simulateMove(board, position, newPosition);
            
            if (isSideInCheck(hypotheticalBoard, isWhite))
            {
                // If so, don't add it to the list of valid moves.
                continue;
            }
            
            int newRow = newPosition / 8;
            int newCol = newPosition % 8;
            
            if (board[newRow * 8 + newCol] == '.') { // If it's empty, add it.
                validMoves.insert(newPosition);
            } else if (!isWhite && isupper(board[newRow * 8 + newCol])) { // If it's an enemy, add it.
                validMoves.insert(newPosition);
            } else if (isWhite && islower(board[newRow * 8 + newCol])) { // If it's an enemy, add it.
                validMoves.insert(newPosition);
            }
        }
        
        return validMoves;
    }

    int getPosition() const {
        return position;
    }

    bool getIsWhite() const {
        return isWhite;
    }

    virtual set<int> inVisionPositions(const string& board) const {
        // Returns a set of squares that are occupied by allies
        // that are defended by this piece. Used for checking legal
        // king moves.
        return set<int>();
    }

private:
    bool isWhite;
    int position;
};

class Rook : public Piece {
public:
    Rook(bool isWhite, int position) : Piece(isWhite, position) {}

    set<int> inVisionPositions(const string& board) const override {
        set<int> inVisionPositions;
        int row = getPosition() / 8;
        int col = getPosition() % 8;
        int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

        for (const auto& dir : directions) {
            int dr = dir[0];
            int dc = dir[1];
            int newRow = row + dr;
            int newCol = col + dc;

            while (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                int newPosition = newRow * 8 + newCol;
                char piece = board[newPosition];

                inVisionPositions.insert(newPosition);

                if (piece != '.') {
                    break; // Stop if there's a piece in the way
                }

                newRow += dr;
                newCol += dc;
            }
        }

        return inVisionPositions;
    }
};

class Pawn : public Piece {
public:
    // Constructor
    Pawn(bool isWhite, int position) : Piece(isWhite, position) {}

    // set<int> inVisionPositions(const string& board) const override;

    // Implementing the in_vision_positions function
    set<int> inVisionPositions(const string& board) const override{
        set<int> inVisionPositions;
        int row = getPosition() / 8;
        int col = getPosition() % 8;

        // Determine the direction based on the pawn's color
        int direction = (getIsWhite()) ? -1 : 1;

        // Check diagonal captures
        for (int offset : {-1, 1}) {
            int newCol = col + offset;
            int newRow = row + direction;
            if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                int newPosition = newRow * 8 + newCol;
                inVisionPositions.insert(newPosition);
            }
        }

        return inVisionPositions;
    }
    set<int> validMoves(const string& board) const override{
        set<int> validMoves;
        int row = getPosition() / 8;
        int col = getPosition() % 8;

        // Determine the direction based on the pawn's color
        int direction = (getIsWhite()) ? -1 : 1;

        // Check one square forward
        int newRow = row + direction;
        
        // Only add forward moves if the square is empty
        if (board[newRow * 8 + col] == '.') {
            //Checks for queening.
            //We don't need to know what color we are because only
            //white pawns can ever be on row 0 and black pawns on row 7
            switch (newRow){
                case 0:
                    for(int i = 0; i < 4; i++){
                        validMoves.insert(96 + (col*4) + i); //Add all 4 queening moves
                    }
                    break;
                case 7:
                    for(int i = 0; i < 4; i++){
                        validMoves.insert(64 + (col*4) + i); //Add all 4 queening moves
                    }
                    break;
                default:
                    validMoves.insert(newRow * 8 + col);
            }

            // Check two squares forward (only on the first move)
            if ((row == 6 && getIsWhite()) || (row == 1 && !getIsWhite())) {
                int newRow2 = row + 2 * direction;
                if (board[newRow2 * 8 + col] == '.') {
                    validMoves.insert(newRow2 * 8 + col);
                }
            }
        }
        // Check diagonal captures
        for (int offset : {-1, 1}) {
            int newCol = col + offset;
            if (0 <= newRow && newRow < 8 && 0 <= newCol && newCol < 8) { //If we're in bounds
                char piece = board[newRow * 8 + newCol];
                if ((getIsWhite() && islower(piece)) || (!getIsWhite() && isupper(piece))) { //If the piece is enemy
                    //If we can capture onto the end rank, then we must promote.
                    switch (newRow){
                        case 0:
                            for(int i = 0; i < 4; i++){
                                validMoves.insert(64 + (newCol*4) + i); //Add all 4 queening moves
                            }
                            break;
                        case 7:
                            for(int i = 0; i < 4; i++){
                                validMoves.insert(96 + (newCol*4) + i); //Add all 4 queening moves
                            }
                            break;
                        default: //The row is not a queening square
                            validMoves.insert(newRow * 8 + newCol);
                    }
                }
            }
        }

        // Check for en passant captures
        int enPassantRow = (getIsWhite()) ? 3 : 4; // Row where en passant captures are possible
        if (row == enPassantRow) {
            for (int offset : {-1, 1}) {
                int newCol = col + offset;
                if (0 <= newCol && newCol < 8) {
                    char piece = board[row * 8 + newCol];
                    if ((getIsWhite() && piece == 'e') || (!getIsWhite() && piece == 'E')) {
                        validMoves.insert((row + direction) * 8 + newCol);
                    }
                }
            }
        }
        // Make sure the move doesn't put us in check.
        set<int> legal_moves;
        for (int newPosition : validMoves) {
            string hypotheticalBoard = simulateMove(board, getPosition(), newPosition);
            if (isSideInCheck(hypotheticalBoard, getIsWhite()))
            {
                // If so, don't add it to the list of valid moves.
                continue;
            }
            legal_moves.insert(newPosition);
        }
        return legal_moves;
    }

    void setEnPassantEligibility(bool eligibility){
        this->enPassantEligible = eligibility;
    }
private:
    bool enPassantEligible = false;
};

class Knight : public Piece {
public:
    // Constructor
    Knight(bool isWhite, int position) : Piece(isWhite, position) {}

    // Implementing the in_vision_positions function
    set<int> inVisionPositions(const string& board) const override {
        set<int> inVisionPositions;
        int row = getPosition() / 8;
        int col = getPosition() % 8;

        // Define the possible knight moves
        vector<pair<int, int>> knightMoves = {
            {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
            {1, -2}, {1, 2}, {2, -1}, {2, 1}
        };

        for (const auto& move : knightMoves) {
            int dr = move.first;
            int dc = move.second;
            int newRow = row + dr;
            int newCol = col + dc;
            bool isInBounds = (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8);
            if (isInBounds) {
                int newPosition = newRow * 8 + newCol;
                inVisionPositions.insert(newPosition);
            }
        }

        return inVisionPositions;
    }
};

class Bishop : public Piece {
public:
    // Constructor
    Bishop(bool isWhite, int position) : Piece(isWhite, position) {}

    // Implementing the in_vision_positions function for Bishop
    set<int> inVisionPositions(const string& board) const override {
        set<int> inVisionPositions;
        int row = getPosition() / 8;
        int col = getPosition() % 8;

        // Define the four diagonal directions
        vector<pair<int, int>> directions = {
            {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
        };

        for (const auto& direction : directions) {
            int dr = direction.first;
            int dc = direction.second;
            int newRow = row + dr;
            int newCol = col + dc;

            while (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                int newPosition = newRow * 8 + newCol;
                char piece = board[newPosition];

                inVisionPositions.insert(newPosition);

                if (piece != '.') {
                    break; // Stop if there's a piece in the way
                }

                newRow += dr;
                newCol += dc;
            }
        }

        return inVisionPositions;
    }
    // Add any other member variables or functions you need
};

class Queen : public Piece {
public:
    // Constructor
    Queen(bool isWhite, int position) : Piece(isWhite, position) {}

    // Implementing the in_vision_positions function for Queen
    set<int> inVisionPositions(const string& board) const override {
        set<int> inVisionPositions;
        int row = getPosition() / 8;
        int col = getPosition() % 8;

        // Define the eight possible directions (horizontal, vertical, and diagonal)
        vector<pair<int, int>> directions = {
            {-1, -1}, {-1, 0}, {-1, 1},
            {0, -1},           {0, 1},
            {1, -1}, {1, 0}, {1, 1}
        };

        for (const auto& dir : directions) {
            int dr = dir.first;
            int dc = dir.second;
            int newRow = row + dr;
            int newCol = col + dc;
            while (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                int newPosition = newRow * 8 + newCol;
                char piece = board[newPosition];
                inVisionPositions.insert(newPosition);
                if (piece != '.') {
                    break;
                }
                newRow += dr;
                newCol += dc;
            }
        }

        return inVisionPositions;
    }

    // Implement the getPosition function if needed
    // int getPosition() const { return position; }

    // Add any other member variables or functions you need
};

class King : public Piece {
public:
    // Constructor
    King(bool isWhite, int position) : Piece(isWhite, position) {}

    // Implementing the in_vision_positions function for King
    set<int> inVisionPositions(const string& board) const override {
        set<int> inVisionPositions;
        int row = getPosition() / 8;
        int col = getPosition() % 8;

        // Define the possible king moves
        vector<pair<int, int>> kingMoves = {
            {-1, -1}, {-1, 0}, {-1, 1},
            {0, -1},           {0, 1},
            {1, -1}, {1, 0}, {1, 1}
        };

        for (const auto& move : kingMoves) {
            int dr = move.first;
            int dc = move.second;
            int newRow = row + dr;
            int newCol = col + dc;
            bool isInBounds = (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8);
            if (isInBounds) {
                int newPosition = newRow * 8 + newCol;
                inVisionPositions.insert(newPosition);
            }
        }

        return inVisionPositions;
    }

    void setKingsideCastlingRights(bool hasRights){
        this->hasKingsideCastlingRights = hasRights;
    }

    void setQueensideCastlingRights(bool hasRights){
        this->hasQueensideCastlingRights = hasRights;
    }

    bool getKingsideCastlingRights() const{
        return this->hasKingsideCastlingRights;
    }

    bool getQueensideCastlingRights() const{
        return this->hasQueensideCastlingRights;
    }
    // Check if kingside castling is valid
    bool canCastleKingside(const string& board) const{
        if (!getKingsideCastlingRights() || isSideInCheck(board, getIsWhite())) {
            return false;
        }
        //Check to make sure the rook is actually there, too
        int kingsideRookPosition = getIsWhite() ? 63 : 7;

        char rookChar = getIsWhite() ? 'R' : 'r';
        if(board[kingsideRookPosition] != rookChar){
            return false;
        }
        // Check if the squares between the king and kingside rook are empty
        for (int col = getPosition() + 1; col < kingsideRookPosition; ++col) {
            if (board[col] != '.') {
                return false;
            }
        }

        // Check if none of the enemy piece's inVisionPositions attack the squares involved in kingside castling
        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 8; ++col) {
                char piece = board[row * 8 + col];
                if ((!getIsWhite() && isupper(piece)) || (getIsWhite() && islower(piece))) {
                    Piece* enemyPiece = create_piece_from_char(piece, row * 8 + col);
                    set<int> enemyPieceVision = enemyPiece->inVisionPositions(board);

                    // Check if enemyPieceVision includes any of the squares involved in kingside castling
                    if (enemyPieceVision.find(getPosition()) != enemyPieceVision.end() ||
                        enemyPieceVision.find(getPosition() + 1) != enemyPieceVision.end() ||
                        enemyPieceVision.find(getPosition() + 2) != enemyPieceVision.end()) {
                        delete enemyPiece; // Don't forget to delete the created piece object
                        return false;
                    }
                    delete enemyPiece; // Don't forget to delete the created piece object
                }
            }
        }
        
        return true;
    }

    // Check if queenside castling is valid
    bool canCastleQueenside(const string& board) const{
        if (!getQueensideCastlingRights() || isSideInCheck(board, getIsWhite())) {
            return false;
        }
        //Check to make sure the rook is actually there, too
        int queensideRookPosition = getIsWhite() ? 56 : 0;
        char rookChar = getIsWhite() ? 'R' : 'r';
        if(board[queensideRookPosition] != rookChar){
            return false;
        }
        // Check if the squares between the king and queenside rook are empty
        for (int col = getPosition() - 1; col > queensideRookPosition; --col) {
            if (board[col] != '.') {
                return false;
            }
        }

        // Check if none of the enemy piece's inVisionPositions attack the squares involved in queenside castling
        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 8; ++col) {
                char piece = board[row * 8 + col];
                if ((!getIsWhite() && isupper(piece)) || (getIsWhite() && islower(piece))) {
                    Piece* enemyPiece = create_piece_from_char(piece, row * 8 + col);
                    set<int> enemyPieceVision = enemyPiece->inVisionPositions(board);

                    // Check if enemyPieceVision includes any of the squares involved in queenside castling
                    if (enemyPieceVision.find(getPosition()) != enemyPieceVision.end() ||
                        enemyPieceVision.find(getPosition() + 1) != enemyPieceVision.end() ||
                        enemyPieceVision.find(getPosition() + 2) != enemyPieceVision.end()) {
                        delete enemyPiece; // Don't forget to delete the created piece object
                        return false;
                    }
                    delete enemyPiece; // Don't forget to delete the created piece object
                }
            }
        }
        
        return true;
    }

    set<int> validMoves(const string& board) const override{
        set<int> validMoves;
        // Get all the positions the King can "see"
        set<int> visionPositions = this->inVisionPositions(board);

        for (int newPosition : visionPositions) {
            string hypotheticalBoard = simulateMove(board, getPosition(), newPosition);
            if (isSideInCheck(hypotheticalBoard, getIsWhite()))
            {
                // If so, don't add it to the list of valid moves.
                continue;
            }

            char piece = board[newPosition];

            if (piece == '.' || (getIsWhite() && islower(piece)) || (!getIsWhite() && isupper(piece))) {
                // Empty square or enemy piece
                validMoves.insert(newPosition);
            }
        }

        int kingsideRookPosition = getIsWhite() ? 63 : 7;
        int queensideRookPosition = getIsWhite() ? 56 : 0;
        
        // Check for castling kingside
        if (canCastleKingside(board)) {
            validMoves.insert(kingsideRookPosition-1);
        }

        // Check for castling queenside
        if (canCastleQueenside(board)) {
            validMoves.insert(queensideRookPosition+2);
        }
        return validMoves;
    }

private:
    bool hasQueensideCastlingRights = true;
    bool hasKingsideCastlingRights = true;
};


Piece* create_piece_from_char(char piece_char, int position) {
    switch (piece_char) {
        case 'P':
            return new Pawn(true, position);
        case 'p':
            return new Pawn(false, position);
        case 'E':
            {
                Pawn* pawn = new Pawn(true, position);
                pawn->setEnPassantEligibility(true);
                return pawn;
            }
        case 'e':
            {
                Pawn* pawn = new Pawn(false, position);
                pawn->setEnPassantEligibility(true);
                return pawn;
            }
        case 'K':
            {
                King* king = new King(true, position);
                king->setQueensideCastlingRights(false);
                king->setKingsideCastlingRights(false);
                return king;
            }
        case 'k':
            {
                King* king = new King(false, position);
                king->setQueensideCastlingRights(false);
                king->setKingsideCastlingRights(false);
                return king;
            }
        case 'S':
            {
                King* king = new King(true, position);
                king->setQueensideCastlingRights(false);
                king->setKingsideCastlingRights(true);
                return king;
            }
        case 's':
            {
                King* king = new King(false, position);
                king->setQueensideCastlingRights(false);
                king->setKingsideCastlingRights(true);
                return king;
            }
        case 'U':
            {
                King* king = new King(true, position);
                king->setQueensideCastlingRights(true);
                king->setKingsideCastlingRights(false);
                return king;
            }
        case 'u':
            {
                King* king = new King(false, position);
                king->setKingsideCastlingRights(true);
                king->setQueensideCastlingRights(false);
                return king;
            }
        case 'W':
            {
                King* king = new King(true, position);
                king->setQueensideCastlingRights(true);
                king->setKingsideCastlingRights(true);
                return king;
            }
        case 'w':
            {
                King* king = new King(false, position);
                king->setQueensideCastlingRights(true);
                king->setKingsideCastlingRights(true);
                return king;
            }
        case 'R':
            return new Rook(true, position);
        case 'r':
            return new Rook(false, position);
        case 'N':
            return new Knight(true, position);
        case 'n':
            return new Knight(false, position);
        case 'B':
            return new Bishop(true, position);
        case 'b':
            return new Bishop(false, position);
        case 'Q':
            return new Queen(true, position);
        case 'q':
            return new Queen(false, position);
        default:
            return nullptr;  // Return nullptr for empty squares ('.')
    }
}

bool isSideInCheck(const string& board, bool isWhite) {
    // cout << "is side in check called " << endl;
    int kingPosition = findKing(board, isWhite);
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            char pieceChar = board[row * 8 + col];
            if ((isWhite && islower(pieceChar)) || (!isWhite && isupper(pieceChar))) {

                // Create a Piece object for the enemy piece
                Piece* enemyPiece = create_piece_from_char(pieceChar, row * 8 + col);
                // Calculate the vision of the enemy piece
                set<int> enemyPieceVision = enemyPiece->inVisionPositions(board);

                if (enemyPieceVision.find(kingPosition) != enemyPieceVision.end()) {
                    delete enemyPiece; // Clean up the dynamically allocated object
                    return true; // Side is in check
                }

                delete enemyPiece; // Clean up the dynamically allocated object
            }
        }
    }

    return false; // Side is not in check
}

// Function to print vision boards for each piece on the board
void printVisionBoards(const string& board) {
    for (int i = 0; i < 64; ++i) {
        char piece_char = board[i];

        if (piece_char == '.') {
            continue; // Skip empty squares
        }
        // Create a piece object from the character on the board
        Piece* piece = create_piece_from_char(piece_char, i);

        if (piece) {
            cout << "=============================" << endl;
            print_board(board);
            cout << endl << "Vision board for piece at position " << piece->getPosition() << ":\n";
            
            // Calculate and print the vision board
            set<int> visionPositions = piece->inVisionPositions(board);
            for (int row = 0; row < 8; ++row) {
                for (int col = 0; col < 8; ++col) {
                    int position = row * 8 + col;
                    char square = (visionPositions.find(position) != visionPositions.end()) ? 'x' : '.';
                    if(row * 8 + col == i){
                        square = piece_char;
                    }
                    cout << square << ' ';
                }
                cout << endl;
            }

            delete piece; // Clean up the dynamically allocated piece
        }
    }
}

void printValidMovesBoards(const string& board, bool isWhite) {
    for (int i = 0; i < 64; ++i) {
        char pieceChar = board[i];
        if ((isWhite && isupper(pieceChar)) || (!isWhite && islower(pieceChar))) {
            // Create the corresponding piece
            Piece* piece = create_piece_from_char(pieceChar, i);

            // Get valid moves for the piece
            set<int> validMoves = piece->validMoves(board);
            set<int>::iterator itr;
   
            // Displaying set elements
            for (itr = validMoves.begin();
                itr != validMoves.end(); itr++)
            {   
                cout << *itr << " ";
            }
            // // Create a board with X for valid moves
            // string validMovesBoard = board;
            // for (int move : validMoves) {
            //     validMovesBoard[move] = 'X';
            // }

            // // Print the board
            // cout << "Valid moves for piece at position " << i << ":" << endl;
            // for (int row = 0; row < 8; ++row) {
            //     for (int col = 0; col < 8; ++col) {
            //         cout << validMovesBoard[row * 8 + col];
            //     }
            //     cout << endl;
            // }
            // cout << endl;

            // Clean up
            delete piece;
        }
        cout << endl;
    }
}
/* Return a board evaluation (double) from a BoardState struct */
// int evaluations = 0;
double evaluateBoard(const BoardState& boardState) {
    // evaluations++;
    // Call game_over with the BoardState directly
    int game_result = game_over(boardState);

    // Check if the game is over (checkmate or stalemate)
    if (game_result == 1) { // white checkmate
        return 50000;
    }
    else if(game_result == 2){ // draw (stalemate, fifty-move rule, etc.)
        return 0;
    }
    else if (game_result == 3) { // black checkmate
        return -50000;
    } 

    map<char, double> pieceValues = {
        { '.', 0.0 },
        { 'Q', 900 },
        { 'q', -900},
        { 'R', 500 },
        { 'r', -500},
        { 'B', 330 },
        { 'b', -330},
        { 'N', 320 },
        { 'n', -320},
        { 'K', 20000},
        { 'k', -20000},
        { 'P', 100 },
        { 'p', -100},
        { 'E', 100 },
        { 'e', -100},
        { 'S', 20000},
        { 's', -20000},
        { 'W', 20000},
        { 'w', -20000},
        { 'U', 20000},
        { 'u', -20000}
    };

    const double pawnTable[8][8] = {
        {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        {50, 50, 50, 50, 50, 50, 50, 50},
        {10, 10, 20, 30, 30, 20, 10, 10},
        {5, 5, 10, 25, 25, 10, 5, 5},
        {0, 0, 0, 20, 20, 0, 0, 0},
        {5, -5, -10, 0, 0, -10, -5, 5},
        {5, 10, 10,-20,-20, 10, 10, 5},
        {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}
    };

    const double knightTable[8][8] = {
        {-50,-40,-30,-30,-30,-30,-40,-50},
        {-40,-20,  0,  0,  0,  0,-20,-40},
        {-30,  0, 10, 15, 15, 10,  0,-30},
        {-30,  5, 15, 20, 20, 15,  5,-30},
        {-30,  0, 15, 20, 20, 15,  0,-30},
        {-30,  5, 10, 15, 15, 10,  5,-30},
        {-40,-20,  0,  5,  5,  0,-20,-40},
        {-50,-40,-30,-30,-30,-30,-40,-50}
    };

    const double bishopTable[8][8] = {
        {-20,-10,-10,-10,-10,-10,-10,-20},
        {-10,  0,  0,  0,  0,  0,  0,-10},
        {-10,  0,  5, 10, 10,  5,  0,-10},
        {-10,  5,  5, 10, 10,  5,  5,-10},
        {-10,  0, 10, 10, 10, 10,  0,-10},
        {-10, 10, 10, 10, 10, 10, 10,-10},
        {-10,  5,  0,  0,  0,  0,  5,-10},
        {-20,-10,-10,-10,-10,-10,-10,-20}
    };

    const double rookTable[8][8] = {
        { 0,  0,  0,  0,  0,  0,  0,  0},
        { 5, 10, 10, 10, 10, 10, 10,  5},
        {-5,  0,  0,  0,  0,  0,  0, -5},
        {-5,  0,  0,  0,  0,  0,  0, -5},
        {-5,  0,  0,  0,  0,  0,  0, -5},
        {-5,  0,  0,  0,  0,  0,  0, -5},
        {-5,  0,  0,  0,  0,  0,  0, -5},
        { 0,  0,  0,  5,  5,  0,  0,  0},
    };

    const double queenTable[8][8] = {
        {-20,-10,-10, -5, -5,-10,-10,-20},
        {-10,  0,  0,  0,  0,  0,  0,-10},
        {-10,  0,  5,  5,  5,  5,  0,-10},
        { -5,  0,  5,  5,  5,  5,  0, -5},
        {  0,  0,  5,  5,  5,  5,  0, -5},
        {-10,  5,  5,  5,  5,  5,  0,-10},
        {-10,  0,  5,  0,  0,  0,  0,-10},
        {-20,-10,-10, -5, -5,-10,-10,-20}
    };

    const double kingMidGameTable[8][8] = {
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-20,-30,-30,-40,-40,-30,-30,-20},
        {-10,-20,-20,-20,-20,-20,-20,-10},
        { 20, 20,  0,  0,  0,  0, 20, 20},
        { 20, 30, 10,  0,  0, 10, 30, 20}
    };

    const double kingEndGameTable[8][8] = {
        {-50,-40,-30,-20,-20,-30,-40,-50},
        {-30,-20,-10,  0,  0,-10,-20,-30},
        {-30,-10, 20, 30, 30, 20,-10,-30},
        {-30,-10, 30, 40, 40, 30,-10,-30},
        {-30,-10, 30, 40, 40, 30,-10,-30},
        {-30,-10, 20, 30, 30, 20,-10,-30},
        {-30,-30,  0,  0,  0,  0,-30,-30},
        {-50,-30,-30,-30,-30,-30,-30,-50}
    };

    double eval = 0.0;

    for (size_t row = 0; row < 8; row++) {
        for (size_t col = 0; col < 8; col++) {
            //First add up the value of pieces
            char piece = boardState.board[row * 8 + col];
            //Now weight the pieces based on the squares theyre on
            switch(piece){
                case 'P':
                case 'E':
                    eval += pieceValues[piece] + pawnTable[row][col];
                    break;
                case 'p':
                case 'e':
                    eval += pieceValues[piece] - pawnTable[7 - row][col];
                    break;
                case 'B':
                    eval += pieceValues[piece] + bishopTable[row][col];
                    break;
                case 'b':
                    eval += pieceValues[piece] - bishopTable[7 - row][col];
                    break;
                case 'R':
                    eval += pieceValues[piece] + rookTable[row][col];
                    break;
                case 'r':
                    eval += pieceValues[piece] - rookTable[7 - row][col];
                    break;
                case 'N':
                    eval += pieceValues[piece] + knightTable[row][col];
                    break;
                case 'n':
                    eval += pieceValues[piece] - knightTable[7 - row][col];
                    break;
                case 'Q':
                    eval += pieceValues[piece] + queenTable[row][col];
                    break;
                case 'q':
                    eval += pieceValues[piece] - queenTable[7 - row][col];
                    break;
                case 'W':
                case 'S':
                case 'U':
                case 'K':
                    eval += pieceValues[piece] + kingMidGameTable[row][col];
                    break;
                case 'w':
                case 's':
                case 'u':
                case 'k':
                    eval += pieceValues[piece] - kingMidGameTable[7 - row][col];
                    break;
            }
        }
    }
    eval /= 100;
    return eval;
}

int algebraic_to_coordinate(const string& algebraic) {
    if (algebraic.length() == 2 && 'a' <= algebraic[0] && algebraic[0] <= 'h' && '1' <= algebraic[1] && algebraic[1] <= '8') {
        int row = 8 - (algebraic[1] - '0'); // Convert character to integer and make it 0-based
        int column = algebraic[0] - 'a';
        return row * 8 + column;
    } else if (algebraic.length() == 3 && 'a' <= algebraic[0] && algebraic[0] <= 'h' &&
               '1' <= algebraic[1] && algebraic[1] <= '8' &&
               ('q' == algebraic[2] || 'n' == algebraic[2] || 'r' == algebraic[2] || 'b' == algebraic[2])){
        int row = 8 - (algebraic[1] - '0'); // Convert character to integer and make it 0-based
        int column = algebraic[0] - 'a';
        int promotion = 0;
        switch (algebraic[2]) {
            case 'q':
                promotion = 0;
                break;
            case 'n':
                promotion = 1;
                break;
            case 'r':
                promotion = 2;
                break;
            case 'b':
                promotion = 3;
                break;
        }
        if(row == 0){
            int coordinate = 64 + 4*column + promotion;
            return coordinate;
        }
        else if (row == 7){
            int coordinate = 96 + 4*column + promotion;
            return coordinate;
        }
    } else {
        throw invalid_argument("Invalid algebraic notation");
    }
    throw invalid_argument("something bad happened");
    return -1;
}

string coordinate_to_algebraic(int coordinate) {
    if (coordinate >= 0 && coordinate < 64) {
        int row = coordinate / 8;
        int column = coordinate % 8;
        char algebraic[3];
        algebraic[0] = 'a' + column;
        algebraic[1] = '8' - row;
        algebraic[2] = '\0';

        // algebraic[2] = '';
        return string(algebraic);
    } else if (coordinate >= 64 && coordinate < 96) {
        int baseCoordinate = coordinate - 64;
        int column = baseCoordinate / 4;
        char algebraic[4];
        algebraic[0] = 'a' + column;
        algebraic[1] = '1';
        string pieces = "qnrb";
        int piece = baseCoordinate % 4;
        algebraic[2] = pieces.at(piece);
        algebraic[3] = '\0';
        return string(algebraic);
    } else if (coordinate >= 96 && coordinate < 128) {
        int baseCoordinate = coordinate - 96;
        int column = baseCoordinate / 4;
        char algebraic[4];
        algebraic[0] = 'a' + column;
        algebraic[1] = '8';
        string pieces = "qnrb";
        int piece = baseCoordinate % 4;
        algebraic[2] = pieces.at(piece);
        algebraic[3] = '\0';
        return string(algebraic);
    } else {
        throw invalid_argument("Invalid coordinate");
    }
}

pair<int, int> split_algebraic(const string& algebraic) {
    if (algebraic.length() == 4) {
        string firstPart = algebraic.substr(0, 2); // Extract the first two characters
        string secondPart = algebraic.substr(2, 2); // Extract the next two characters
        int firstCoordinate = algebraic_to_coordinate(firstPart);
        int secondCoordinate = algebraic_to_coordinate(secondPart);
        return make_pair(firstCoordinate, secondCoordinate);
    } else if (algebraic.length() == 5) {
        string firstPart = algebraic.substr(0, 2); // Extract the first two characters
        string secondPart = algebraic.substr(2, 3); // Extract the next three characters
        int firstCoordinate = algebraic_to_coordinate(firstPart);
        int secondCoordinate = algebraic_to_coordinate(secondPart);
        return make_pair(firstCoordinate, secondCoordinate);
    } else {
        throw invalid_argument("Invalid algebraic notation");
    }
}

BoardState fen_to_board(const string& pieces, const string& en_passant, char active_color, const string& castling_rights) {
    BoardState boardState;
    boardState.board = "";  // Initialize the board string
    int row = 0, col = 0;

    // Iterate through the FEN string (board part only, before space)
    for (char c : pieces) {
        if (c == '/') {
            row++;
            col = 0;
        } else if (isdigit(c)) {
            // Empty squares: Add the appropriate number of dots ('.')
            int empty_squares = c - '0';
            boardState.board += string(empty_squares, '.');
            col += empty_squares;
        } else {
            // Add piece to the board
            boardState.board += c;
            col++;
        }
    }

    // Modify the kings based on castling rights
    for (char& piece : boardState.board) {
        if (piece == 'K') {
            if (castling_rights.find('K') != string::npos && castling_rights.find('Q') != string::npos) {
                piece = 'W';  // White king has both castling rights
            } else if (castling_rights.find('K') != string::npos) {
                piece = 'S';  // White king has kingside castling only
            } else if (castling_rights.find('Q') != string::npos) {
                piece = 'U';  // White king has queenside castling only
            }
        } else if (piece == 'k') {
            if (castling_rights.find('k') != string::npos && castling_rights.find('q') != string::npos) {
                piece = 'w';  // Black king has both castling rights
            } else if (castling_rights.find('k') != string::npos) {
                piece = 's';  // Black king has kingside castling only
            } else if (castling_rights.find('q') != string::npos) {
                piece = 'u';  // Black king has queenside castling only
            }
        }
    }

    // Handle en passant pawn conversion
    boardState.en_passant = en_passant;  // Store en passant in BoardState
    if (en_passant != "-") {
        // Convert en passant square (like "e3") into row and column indices
        int ep_col = en_passant[0] - 'a';  // 'a' to 'h' -> 0 to 7
        int ep_row = 8 - (en_passant[1] - '0');  // '8' to '1' -> 0 to 7
        
        int pawn_row;
        if (active_color == 'w') {
            // If it's white's turn, en passant target is a black pawn that can be captured
            pawn_row = ep_row + 1;  // Black pawn is above the en passant target
            if (boardState.board[pawn_row * 8 + ep_col] == 'p') {
                boardState.board[pawn_row * 8 + ep_col] = 'e';  // Mark black pawn as en passant eligible
            }
        } else {
            // If it's black's turn, en passant target is a white pawn that can be captured
            pawn_row = ep_row - 1;  // White pawn is below the en passant target
            if (boardState.board[pawn_row * 8 + ep_col] == 'P') {
                boardState.board[pawn_row * 8 + ep_col] = 'E';  // Mark white pawn as en passant eligible
            }
        }
    }

    // Set the active color and castling rights in the BoardState
    boardState.active_color = active_color;
    boardState.castling_rights = castling_rights;
    boardState.halfmove = 0;  // Initialize halfmove (you can adjust this as needed)
    boardState.fullmove = 1;   // Initialize fullmove (you can adjust this as needed)

    return boardState;
}

string board_to_fen(const string& board, char& active_color, int& halfmove_clock, int& fullmove_number) {
    string fen = "";
    int empty_squares = 0;
    string castling_rights = "";
    string en_passant = "-";
    
    // First, we handle the board part
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            char piece = board[row * 8 + col];
            
            // Convert "E"/"e" back to "P"/"p"
            if (piece == 'E') piece = 'P';
            else if (piece == 'e') piece = 'p';
            
            if (piece == '.') {
                ++empty_squares;
            } else {
                if (empty_squares > 0) {
                    fen += to_string(empty_squares);
                    empty_squares = 0;
                }
                fen += piece;
                
                // Check castling rights by the king's letter
                if (piece == 'W') castling_rights += "KQ";
                else if (piece == 'S') castling_rights += "K";
                else if (piece == 'U') castling_rights += "Q";
                else if (piece == 'K') {} // No castling rights, so nothing to add
                else if (piece == 'w') castling_rights += "kq";
                else if (piece == 's') castling_rights += "k";
                else if (piece == 'u') castling_rights += "q";
            }
        }

        if (empty_squares > 0) {
            fen += to_string(empty_squares);
            empty_squares = 0;
        }
        
        if (row < 7) {
            fen += '/';  // Row separator
        }
    }

    // If no castling rights exist, set the string to "-"
    if (castling_rights.empty()) {
        castling_rights = "-";
    }

    // Determine the en passant target square
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            char piece = board[row * 8 + col];
            
            // If we find an en passant pawn ("E" or "e"), calculate the target square
            if (piece == 'E' && row > 0) {
                en_passant = coordinate_to_algebraic((row - 1)*8 + col);  // En passant target for white
            } else if (piece == 'e' && row < 7) {
                en_passant = coordinate_to_algebraic((row + 1)*8 + col);  // En passant target for black
            }
        }
    }

    // Construct the final FEN string
    fen += " ";
    fen += active_color;
    fen += " ";
    fen += castling_rights;
    fen += " ";
    fen += en_passant;
    fen += " ";
    fen += to_string(halfmove_clock);
    fen += " ";
    fen += to_string(fullmove_number);

    return fen;
}

vector<pair<int, int>> legal_moves(const BoardState& boardState) {
    vector<pair<int, int>> allLegalMoves;

    // Loop through the board to find the current player's pieces
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            char piece = boardState.board[row * 8 + col]; // Direct access to board
            char active_color = boardState.active_color;  // Direct access to active color

            // Check if the piece belongs to the active player (upper-case for white, lower-case for black)
            if ((active_color == 'w' && isupper(piece)) || (active_color == 'b' && islower(piece))) {
                Piece* allyPiece = create_piece_from_char(piece, row * 8 + col);
                vector<pair<int, int>> pieceLegalMoves;

                // Get valid moves for this piece
                set<int> destinations = allyPiece->validMoves(boardState.board);
                for (int destination : destinations) {
                    pair<int, int> legalMove;
                    legalMove.first = row * 8 + col; // Start pos
                    legalMove.second = destination;  // End pos
                    pieceLegalMoves.push_back(legalMove);
                }

                // Add the piece's legal moves to the overall list
                allLegalMoves.insert(allLegalMoves.end(), pieceLegalMoves.begin(), pieceLegalMoves.end());
            }
        }
    }

    return allLegalMoves;
}

bool at_least_1_legal_move(const BoardState& boardState) {
    // Loop through the board to find the current player's pieces
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            char piece = boardState.board[row * 8 + col]; // Direct access to board
            char active_color = boardState.active_color;  // Direct access to active color

            // Check if the piece belongs to the active player (upper-case for white, lower-case for black)
            if ((active_color == 'w' && isupper(piece)) || (active_color == 'b' && islower(piece))) {
                Piece* allyPiece = create_piece_from_char(piece, row * 8 + col);

                // Get valid moves for this piece
                set<int> destinations = allyPiece->validMoves(boardState.board);

                // If there is at least one valid move, return true
                if (!destinations.empty()) {
                    return true;
                }
            }
        }
    }

    // If no legal moves were found, return false
    return false;
}

int game_over(const BoardState& boardState) {
    // 1. Check for checkmate or stalemate using the new at_least_1_legal_move function:
    //    If no legal moves are available, and the king is in check -> checkmate
    //    If no legal moves are available, and the king is NOT in check -> stalemate
    if (!at_least_1_legal_move(boardState)) {
        bool isWhite = boardState.active_color == 'w';
        
        if (isSideInCheck(boardState.board, isWhite)) {
            if (boardState.active_color == 'w') {
                // White is in check with no legal moves -> black wins by checkmate.
                return 3; // Black checkmate
            } else {
                // Black is in check with no legal moves -> white wins by checkmate.
                return 1; // White checkmate
            }
        } else {
            return 2; // Stalemate (no legal moves, but no check)
        }
    }

    // 2. Fifty-move rule (check the halfmove clock)
    if (boardState.halfmove >= 100) {
        return 2; // Game over by fifty-move rule
    }

    // 3. (Optional) Implement threefold repetition (would need move history tracking)

    return 0; // The game is not over
}

// double minimax(BoardState& boardState, int depth) {
//     // Base case for recursion
//     if (depth == 0) {
//         double eval = evaluateBoard(boardState);

//         if (eval == -50000) { // black checkmate
//             eval -= depth; // Apply depth-based adjustment
//         } else if (eval == 50000) { // white checkmate
//             eval += depth;
//         }

//         return eval;
//     }

//     // Get legal moves for the active player
//     vector<pair<int, int>> legalMoves = legal_moves(boardState);
//     double best_eval = (boardState.active_color == 'w') ? -2000000000 : 2000000000; // Initialize best_eval based on color

//     for (const auto& move : legalMoves) {
//         string new_board = simulateMove(boardState.board, move.first, move.second);
        
//         // Create a new BoardState for the next recursive call
//         BoardState newBoardState = {
//             new_board,
//             (boardState.active_color == 'w') ? 'b' : 'w', // Switch color
//             boardState.castling_rights,
//             boardState.en_passant,
//             boardState.halfmove,
//             boardState.fullmove
//         };

//         double eval = minimax(newBoardState, depth - 1);

//         // Update the best_eval based on whether it's white's or black's turn
//         if (boardState.active_color == 'w') {
//             best_eval = max(best_eval, eval); // Maximize for white
//         } else {
//             best_eval = min(best_eval, eval); // Minimize for black
//         }
//     }

//     return best_eval;
// }
double minimax(BoardState& boardState, int depth, double alpha, double beta) {
    // Base case for recursion
    if (depth == 0) {
        double eval = evaluateBoard(boardState);

        if (eval == -50000) { // black checkmate
            eval -= depth; // Apply depth-based adjustment
        } else if (eval == 50000) { // white checkmate
            eval += depth;
        }

        return eval;
    }

    // Get legal moves for the active player
    vector<pair<int, int>> legalMoves = legal_moves(boardState);
    double best_eval = (boardState.active_color == 'w') ? -2000000000 : 2000000000; // Initialize best_eval based on color

    for (const auto& move : legalMoves) {
        string new_board = simulateMove(boardState.board, move.first, move.second);
        
        // Create a new BoardState for the next recursive call
        BoardState newBoardState = {
            new_board,
            (boardState.active_color == 'w') ? 'b' : 'w', // Switch color
            boardState.castling_rights,
            boardState.en_passant,
            boardState.halfmove,
            boardState.fullmove
        };

        double eval = minimax(newBoardState, depth - 1, alpha, beta);

        // Update the best_eval based on whether it's white's or black's turn
        if (boardState.active_color == 'w') {
            best_eval = max(best_eval, eval); // Maximize for white
            alpha = max(alpha, best_eval); // Update alpha
        } else {
            best_eval = min(best_eval, eval); // Minimize for black
            beta = min(beta, best_eval); // Update beta
        }

        // Alpha-Beta Pruning
        if (beta <= alpha) {
            break; // Cut off the search
        }
    }

    return best_eval;
}

string choose_best_move(const BoardState& boardState, int depth) {
    // Extract necessary information from the BoardState struct
    const string& board_as_string = boardState.board;
    char active_color = boardState.active_color;
    int halfmove = boardState.halfmove;
    int fullmove = boardState.fullmove;

    vector<pair<int, int>> legalMoves = legal_moves(boardState);  // Use the new legal_moves function
    pair<int, int> best_move;
    double best_eval = (active_color == 'w') ? -INFINITY : INFINITY; // Initialize best_eval based on the active color
    // for (const auto& move : legalMoves) {
    //     cout << coordinate_to_algebraic(move.first) << coordinate_to_algebraic(move.second) << endl;
    // }
    for (const auto& move : legalMoves) {
        string new_board = simulateMove(boardState.board, move.first, move.second);
        
        // Create a new BoardState for the next recursive call
        BoardState newBoardState = {
            new_board,
            (boardState.active_color == 'w') ? 'b' : 'w', // Switch color
            boardState.castling_rights,
            boardState.en_passant,
            boardState.halfmove,
            boardState.fullmove
        };

        // Evaluate the new position using minimax
        double eval = minimax(newBoardState, depth, -2000000000, 2000000000);

        // Update best_move based on the next player's turn (next_color, not active_color)
        if (active_color == 'w') { // Check based on next_color
            if (eval > best_eval) {
                best_eval = eval;
                best_move = move; // Store the current move as the best move
            }
        } else {
            if (eval < best_eval) {
                best_eval = eval;
                best_move = move; // Store the current move as the best move
            }
        }
    }
    // cout << coordinate_to_algebraic(best_move.first) << coordinate_to_algebraic(best_move.second) << endl;
    // Convert best move from coordinates to algebraic notation
    string from = coordinate_to_algebraic(best_move.first);
    string to = coordinate_to_algebraic(best_move.second);
    string algebraic_best = from + to; // Combine to form the algebraic notation of the move

    return algebraic_best; // Return the best move in algebraic format
}

int main(int argc, char* argv[]) {
    // Check if the correct number of arguments is passed (7 arguments: program name + 6 parts of FEN)
    if (argc != 7) {
        cerr << "Error: Please pass the FEN components as separate command-line arguments." << endl;
        cerr << "Usage: ./a.exe <board> <active_color> <castling_rights> <en_passant> <halfmove_clock> <fullmove_number>" << endl;
        return 1;
    }
    // FEN components
    string pieces = argv[1];            // Piece placement
    char active_color = argv[2][0];         // Active color ('b' or 'w')
    string castling_rights = argv[3];  // Castling availability (KQkq or -)
    string en_passant = argv[4];       // En passant target square (e3, - if none)
    int halfmove_clock = stoi(argv[5]);  // Halfmove clock
    int fullmove_number = stoi(argv[6]); // Fullmove number
    BoardState boardState = fen_to_board(pieces, en_passant, active_color, castling_rights);
    // string fen = pieces + ' ' + active_color + ' ' + castling_rights + ' ' + en_passant + ' ' + argv[5] + ' ' + argv[6];
    cout << choose_best_move(boardState, 2);
    // cout << "evaluations: " << test;
    return 0;
}