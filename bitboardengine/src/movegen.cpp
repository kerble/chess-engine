#include "movegen.hpp"

std::array<uint64_t, 64> king_threats_table;
std::array<uint64_t, 64> knight_threats_table;
std::array<uint64_t, 64> wpawn_threats_table;
std::array<uint64_t, 64> bpawn_threats_table;

// Helper function to pop the least significant bit and return its index
inline int popLSB(uint64_t& bitboard) {
    int index = __builtin_ctzll(bitboard);  // Count trailing zeros
    bitboard &= bitboard - 1;               // Clear the least significant bit
    return index;
}

/*
Generate a bitboard with 1s on every square between a king and a piece actively
checking it, including the attacking piece, excluding the king. Pass allOccupancy
in the event of blocking/capturing and pass only enemy occupancy in the event
of pin detection/enforcement.
*/
static uint64_t generateRayMask(int kingSquare, int attackerSquare, uint64_t occupancy) {
    // Create a bitboard where only the attacking piece is "occupied"
    uint64_t attackerOnlyBoard = 1ULL << attackerSquare;

    // Determine if the attack is along a diagonal or a straight line
    bool isDiagonalAttack = (Bmagic(kingSquare, 0) & attackerOnlyBoard) != 0;

    // Generate the ray from the king to the attacker based on the type of attack
    uint64_t kingRayMask;
    if (isDiagonalAttack) {
        kingRayMask = Bmagic(kingSquare, attackerOnlyBoard);  // Diagonal ray
    } else {
        kingRayMask = Rmagic(kingSquare, attackerOnlyBoard);  // Rank/file ray
    }

    // Generate the attack mask of the attacker based on the passed-in occupancy
    // We need to make sure that if this occupancy is only the enemy occupancy we
    // include the king's square to prevent a bug in positions like this:
    // https://i.imgur.com/25slm58.png
    // If we pass the total occupancy this will do nothing.
    uint64_t kingOnlyBoard = (1ULL << kingSquare);
    occupancy |= kingOnlyBoard;
    uint64_t attackerAttackMask =
        isDiagonalAttack ? Bmagic(attackerSquare, occupancy) : Rmagic(attackerSquare, occupancy);

    // Restrict the ray to the squares between the king and the attacker (including the attacker)
    uint64_t rayMask = kingRayMask & attackerAttackMask;
    // std::cout << "debug gen ray mask" << std::endl;
    // std::cout << "Attacker only board: " << bitboardToBinaryString(attackerOnlyBoard) <<
    // std::endl; std::cout << "isDiagonalAttack " << isDiagonalAttack << std::endl; std::cout <<
    // "king ray mask" << bitboardToBinaryString(kingRayMask) << std::endl; std::cout << "attacker
    // attack mask" << bitboardToBinaryString(attackerAttackMask) << std::endl; std::cout <<
    // "rayMask = kingRayMask & attackerAttackMask" << bitboardToBinaryString(rayMask) << std::endl;
    rayMask |= (1ULL << attackerSquare);  // Include the attacker's square for capturing
    // std::cout << "rayMask = kingRayMask & attackerAttackMask" << bitboardToBinaryString(rayMask)
    // << std::endl;
    return rayMask;
}

// Return the position of the piece attacking the king. Assumed to be only one piece
static int findAttackerSquare(const BoardState& board) {
    bool isWhite = board.getTurn();
    uint64_t kingBB = board.getBitboard(isWhite ? WHITE_KINGS : BLACK_KINGS);

    // Extract the king's position
    int kingSquare = __builtin_ctzll(kingBB);

    uint64_t allOccupancy = board.getAllOccupancy();

    // Check for knight attacks
    uint64_t knightAttacks = knight_threats_table[kingSquare];
    uint64_t enemyKnights = board.getBitboard(isWhite ? BLACK_KNIGHTS : WHITE_KNIGHTS);
    if (knightAttacks & enemyKnights) {
        return __builtin_ctzll(knightAttacks & enemyKnights);
    }

    // Check for pawn attacks
    uint64_t pawnAttacks =
        isWhite ? wpawn_threats_table[kingSquare] : bpawn_threats_table[kingSquare];
    uint64_t enemyPawns = board.getBitboard(isWhite ? BLACK_PAWNS : WHITE_PAWNS);
    if (pawnAttacks & enemyPawns) {
        return __builtin_ctzll(pawnAttacks & enemyPawns);
    }

    // Check for bishop/queen attacks
    uint64_t bishopAttacks = Bmagic(kingSquare, allOccupancy);
    uint64_t enemyBishopsQueens = board.getBitboard(isWhite ? BLACK_BISHOPS : WHITE_BISHOPS) |
                                  board.getBitboard(isWhite ? BLACK_QUEENS : WHITE_QUEENS);
    if (bishopAttacks & enemyBishopsQueens) {
        return __builtin_ctzll(bishopAttacks & enemyBishopsQueens);
    }
    // Check for rook/queen attacks
    uint64_t rookAttacks = Rmagic(kingSquare, allOccupancy);
    uint64_t enemyRooksQueens = board.getBitboard(isWhite ? BLACK_ROOKS : WHITE_ROOKS) |
                                board.getBitboard(isWhite ? BLACK_QUEENS : WHITE_QUEENS);
    if (rookAttacks & enemyRooksQueens) {
        return __builtin_ctzll(rookAttacks & enemyRooksQueens);
    }

    // If no attackers are found, throw an error
// std::cout << board << std::endl;
    throw std::logic_error(
        "No attacker found for the king. This function is only called when there is an attacker. "
        "Something is wrong");
}

// Returns the bitboard including only the checking knight or pawn.
// If no pawns nor knights are checking the king, return 0 (false).
static uint64_t isImmediateCheckByKnightOrPawn(const BoardState& board) {
    // Determine if it's White's turn
    bool isWhite = board.getTurn();

    // Get the king's bitboard and extract its square
    uint64_t kingBB = board.getBitboard(isWhite ? WHITE_KINGS : BLACK_KINGS);
    int kingSquare = __builtin_ctzll(kingBB);

    // Check knight attacks
    uint64_t knightAttacks = knight_threats_table[kingSquare];  // Threats from all potential
                                                                // knights to the king's square
    uint64_t enemyKnights = board.getBitboard(isWhite ? BLACK_KNIGHTS : WHITE_KNIGHTS);
    uint64_t attacker = knightAttacks & enemyKnights;
    if (attacker) return attacker;

    // Check pawn attacks
    uint64_t pawnAttacks = isWhite ? wpawn_threats_table[kingSquare]
                                   : bpawn_threats_table[kingSquare];  // Potential pawn threats
    uint64_t enemyPawns = board.getBitboard(isWhite ? BLACK_PAWNS : WHITE_PAWNS);
    attacker = pawnAttacks & enemyPawns;
    if (attacker) return attacker;

    // No check by knight or pawn
    return 0;
}

// Initialize king threat masks
void initKingThreatMasks() {
    for (int square = 0; square < 64; ++square) {
        uint64_t threat_mask = 0;

        int rank = square / 8;  // Row number (0-7)
        int file = square % 8;  // Column number (0-7)

        if (rank > 0 && file > 0) threat_mask |= (1ULL << square - 9);  // Up-left
        if (rank > 0)             threat_mask |= (1ULL << square - 8);  // Up
        if (rank > 0 && file < 7) threat_mask |= (1ULL << square - 7);  // Up-right
        if (file < 7)             threat_mask |= (1ULL << square + 1);  // Right
        if (rank < 7 && file < 7) threat_mask |= (1ULL << square + 9);  // Down-right
        if (rank < 7)             threat_mask |= (1ULL << square + 8);  // Down
        if (rank < 7 && file > 0) threat_mask |= (1ULL << square + 7);  // Down-left
        if (file > 0)             threat_mask |= (1ULL << square - 1);  // Left

        // Store the computed threat mask
        king_threats_table[square] = threat_mask;
    }
}

// Initialize knight threat masks
void initKnightThreatMasks() {
    for (int square = 0; square < 64; ++square) {
        uint64_t threat_mask = 0;

        int rank = square / 8;  // Row number (0-7)
        int file = square % 8;  // Column number (0-7)

        // All potential knight moves
        if (rank > 1 && file > 0) threat_mask |= (1ULL << square - 17);  // Up 2, Left 1
        if (rank > 1 && file < 7) threat_mask |= (1ULL << square - 15);  // Up 2, Right 1
        if (rank > 0 && file > 1) threat_mask |= (1ULL << square - 10);  // Up 1, Left 2
        if (rank > 0 && file < 6) threat_mask |= (1ULL << square - 6);   // Up 1, Right 2
        if (rank < 7 && file > 1) threat_mask |= (1ULL << square + 6);   // Down 1, Left 2
        if (rank < 7 && file < 6) threat_mask |= (1ULL << square + 10);  // Down 1, Right 2
        if (rank < 6 && file > 0) threat_mask |= (1ULL << square + 15);  // Down 2, Left 1
        if (rank < 6 && file < 7) threat_mask |= (1ULL << square + 17);  // Down 2, Right 1

        // Store the computed threat mask
        knight_threats_table[square] = threat_mask;
    }
}

// Initialize pawn threat masks for white and black
void initPawnThreatMasks() {
    for (int square = 0; square < 64; ++square) {
        uint64_t white_threats = 0;
        uint64_t black_threats = 0;

        int rank = square / 8;  // Row number (0-7)
        int file = square % 8;  // Column number (0-7)

        // White pawn threats (upward diagonals)
        if (file > 0) white_threats |= (1ULL << square + 7);  // Up-left
        if (file < 7) white_threats |= (1ULL << square + 9);  // Up-right

        // Black pawn threats (downward diagonals)
        if (file > 0) black_threats |= (1ULL << square - 9);  // Down-left
        if (file < 7) black_threats |= (1ULL << square - 7);  // Down-right

        // Store in the respective tables
        wpawn_threats_table[square] = white_threats;
        bpawn_threats_table[square] = black_threats;
    }
}

uint64_t generateThreatMask(int pieceType, int attackerSquare, uint64_t allOccupancy) {
    switch (pieceType) {
        case WHITE_KINGS:
        case BLACK_KINGS:
            return king_threats_table[attackerSquare];  // Precomputed king attacks

        case WHITE_KNIGHTS:
        case BLACK_KNIGHTS:
            return knight_threats_table[attackerSquare];  // Precomputed knight attacks

        case WHITE_PAWNS:
            return wpawn_threats_table[attackerSquare];  // Precomputed white pawn attacks

        case BLACK_PAWNS:
            return bpawn_threats_table[attackerSquare];  // Precomputed black pawn attacks

        case WHITE_ROOKS:
        case BLACK_ROOKS:
            return Rmagic(attackerSquare, allOccupancy);  // Rook attacks depend on occupancy

        case WHITE_BISHOPS:
        case BLACK_BISHOPS:
            return Bmagic(attackerSquare, allOccupancy);  // Bishop attacks depend on occupancy

        case WHITE_QUEENS:
        case BLACK_QUEENS:
            return Qmagic(attackerSquare, allOccupancy);  // Queen attacks depend on occupancy

        default:
            return 0ULL;  // Invalid piece type
    }
}

// Returns the number of pieces checking the king
static int numCheckers(const BoardState& board) {
    bool isWhite = board.getTurn();
    // Get the king's bitboard and occupancy for the side to check
    uint64_t kingBB = board.getBitboard(isWhite ? WHITE_KINGS : BLACK_KINGS);

    // Get the opponent's bitboards
    uint64_t opponentPawns = board.getBitboard(isWhite ? BLACK_PAWNS : WHITE_PAWNS);
    uint64_t opponentKnights = board.getBitboard(isWhite ? BLACK_KNIGHTS : WHITE_KNIGHTS);
    uint64_t opponentBishops = board.getBitboard(isWhite ? BLACK_BISHOPS : WHITE_BISHOPS);
    uint64_t opponentRooks = board.getBitboard(isWhite ? BLACK_ROOKS : WHITE_ROOKS);
    uint64_t opponentQueens = board.getBitboard(isWhite ? BLACK_QUEENS : WHITE_QUEENS);
    uint64_t allOccupancy = board.getAllOccupancy();

    // Select the appropriate threat table for pawns
    const std::array<uint64_t, 64>& pawnThreatsTable =
        isWhite ? bpawn_threats_table : wpawn_threats_table;
    int checkers = 0;

    // Check threats from opponent pawns
    uint64_t remainingPawns = opponentPawns;
    while (remainingPawns) {
        int pawnSquare = popLSB(remainingPawns);
        if (pawnThreatsTable[pawnSquare] & kingBB) {
            checkers += 1;
        }
    }
    // Check threats from opponent bishops and queens (diagonal attacks)
    uint64_t bishopsAndQueens = opponentBishops | opponentQueens;
    while (bishopsAndQueens) {
        int square = popLSB(bishopsAndQueens);
        if (Bmagic(square, allOccupancy) & kingBB) {
            checkers += 1;
        }
    }

    // Check threats from opponent rooks and queens (straight-line attacks)
    uint64_t rooksAndQueens = opponentRooks | opponentQueens;
    while (rooksAndQueens) {
        int square = popLSB(rooksAndQueens);
        if (Rmagic(square, allOccupancy) & kingBB) {
            checkers += 1;
        }
    }

    // Check threats from opponent knights
    uint64_t knights = opponentKnights;
    while (knights) {
        int knightSquare = popLSB(knights);
        if (knight_threats_table[knightSquare] & kingBB) {
            checkers += 1;
        }
    }


    // If no threats are found, the king is not in check
    return checkers;
}

// Exact same as num checkers except it returns early.
bool is_in_check(const BoardState& board) {
    bool isWhite = board.getTurn();
    // Get the king's bitboard and occupancy for the side to check
    uint64_t kingBB = board.getBitboard(isWhite ? WHITE_KINGS : BLACK_KINGS);

    // Get the opponent's bitboards
    uint64_t opponentPawns = board.getBitboard(isWhite ? BLACK_PAWNS : WHITE_PAWNS);
    uint64_t opponentKnights = board.getBitboard(isWhite ? BLACK_KNIGHTS : WHITE_KNIGHTS);
    uint64_t opponentBishops = board.getBitboard(isWhite ? BLACK_BISHOPS : WHITE_BISHOPS);
    uint64_t opponentRooks = board.getBitboard(isWhite ? BLACK_ROOKS : WHITE_ROOKS);
    uint64_t opponentQueens = board.getBitboard(isWhite ? BLACK_QUEENS : WHITE_QUEENS);
    uint64_t allOccupancy = board.getAllOccupancy();

    // Select the appropriate threat table for pawns
    const std::array<uint64_t, 64>& pawnThreatsTable =
        isWhite ? bpawn_threats_table : wpawn_threats_table;

    // Check threats from opponent bishops and queens (diagonal attacks)
    uint64_t bishopsAndQueens = opponentBishops | opponentQueens;
    while (bishopsAndQueens) {
        int square = popLSB(bishopsAndQueens);
        if (Bmagic(square, allOccupancy) & kingBB) {
            return true;
        }
    }

    // Check threats from opponent rooks and queens (straight-line attacks)
    uint64_t rooksAndQueens = opponentRooks | opponentQueens;
    while (rooksAndQueens) {
        int square = popLSB(rooksAndQueens);
        if (Rmagic(square, allOccupancy) & kingBB) {
            return true;
        }
    }

    // Check threats from opponent knights
    uint64_t knights = opponentKnights;
    while (knights) {
        int knightSquare = popLSB(knights);
        if (knight_threats_table[knightSquare] & kingBB) {
            return true;
        }
    }

    // Check threats from opponent pawns
    uint64_t remainingPawns = opponentPawns;
    while (remainingPawns) {
        int pawnSquare = popLSB(remainingPawns);
        if (pawnThreatsTable[pawnSquare] & kingBB) {
            return true;
        }
    }

    // If no threats are found, the king is not in check
    return false;
}

/*
Returns a pin mask for every single pinned piece, if there are any.
*/
static std::vector<uint64_t> detectPinnedPieces(int kingSquare, uint64_t enemyOccupancy,
                                                uint64_t enemyQueens, uint64_t enemyBishops,
                                                uint64_t enemyRooks, uint64_t alliedOccupancy) {
    std::vector<uint64_t> pinMasks;  // To store pin masks for pinned pieces

    // Diagonal (bishop and queen) attacks
    // King's diagonal attacks
    uint64_t bishopAttackMask = Bmagic(kingSquare, enemyOccupancy);
    // Only bishops/queens
    uint64_t diagonalAttackers = bishopAttackMask & (enemyBishops | enemyQueens);
    while (diagonalAttackers) {
        int attackerSquare = popLSB(diagonalAttackers);

        // Generate ray between the king and attacker
        uint64_t rayMask = generateRayMask(kingSquare, attackerSquare, enemyOccupancy);
        rayMask |= (1ULL << attackerSquare);  // Include attacker square

        // Find allied pieces on this ray
        uint64_t pinnedPieces = rayMask & alliedOccupancy;

        if (__builtin_popcountll(pinnedPieces) == 1) {
            pinMasks.push_back(rayMask);  // Add pin mask for a single pinned piece
        }
    }

    // Straight-line (rook and queen) attacks
    uint64_t rookAttackMask = Rmagic(kingSquare, enemyOccupancy);  // King's straight-line attacks
    uint64_t straightAttackers = rookAttackMask & (enemyRooks | enemyQueens);  // Only rooks/queens

    // std::cout << "straight attackers:\n" << bitboardToBinaryString(straightAttackers) << std::endl;

    while (straightAttackers) {
        int attackerSquare = popLSB(straightAttackers);

        // Generate ray between the king and attacker
        uint64_t rayMask = generateRayMask(kingSquare, attackerSquare, enemyOccupancy);
        // std::cout << "raymask:\n" << bitboardToBinaryString(rayMask) << std::endl;
        rayMask |= (1ULL << attackerSquare);  // Include attacker square

        // Find allied pieces on this ray
        uint64_t pinnedPieces = rayMask & alliedOccupancy;

        if (__builtin_popcountll(pinnedPieces) == 1) {
            pinMasks.push_back(rayMask);  // Add pin mask for a single pinned piece
        }
    }
    return pinMasks;  // Return all pin masks
}

/*
Generates a vector of encoded king moves, minus castling.
Can be used in either check or not check.
*/
std::vector<uint16_t> generateKingMoves(const BoardState& board) {
    std::vector<uint16_t> legalKingMoves;

    // Determine the side to move
    bool isWhite = board.getTurn();

    // Locate the king's position
    uint64_t kingBB = board.getBitboard(isWhite ? WHITE_KINGS : BLACK_KINGS);
    int kingSquare = __builtin_ctzll(kingBB);

    // Allied and enemy occupancies
    uint64_t alliedOccupancy = board.getOccupancy(isWhite);
    uint64_t allOccupancy = board.getAllOccupancy();

    // We remove the king from the board to calculate threats properly
    uint64_t occupancyMinusKing = allOccupancy & ~(1ULL << kingSquare);

    // Collect all enemy attack masks
    uint64_t enemyAttackMask = 0;
    for (int pieceType = (isWhite ? BLACK_PAWNS : WHITE_PAWNS);
         pieceType <= (isWhite ? BLACK_KINGS : WHITE_KINGS); ++pieceType) {
        uint64_t pieceBB = board.getBitboard(pieceType);
        while (pieceBB) {
            int square = popLSB(pieceBB);
            enemyAttackMask |= generateThreatMask(pieceType, square, occupancyMinusKing);
        }
    }

    // Generate potential king moves
    uint64_t kingMoves = king_threats_table[kingSquare];

    // Filter king moves: Remove allied pieces and enemy-attacked squares
    kingMoves &= ~alliedOccupancy;  // Exclude allied pieces
    kingMoves &= ~enemyAttackMask;  // Exclude enemy threats

    // Convert bitboard of king moves to encoded uint16_t moves
    while (kingMoves) {
        int toSquare = popLSB(kingMoves);
        uint16_t move = encodeMove(kingSquare, toSquare);
        legalKingMoves.push_back(move);
    }

    return legalKingMoves;
}

static std::vector<uint16_t> generateCastlingMoves(const BoardState& board) {
    std::vector<uint16_t> castlingMoves;

    // Determine the side to move
    bool isWhite = board.getTurn();

    // Castling rights
    bool canCastleKingside = board.canCastleKingside(isWhite);
    bool canCastleQueenside = board.canCastleQueenside(isWhite);

    // Early return if no castling rights
    if (!canCastleKingside && !canCastleQueenside) {
        return castlingMoves;
    }

    // King and rook positions
    int kingSquare = isWhite ? 4 : 60;  // e1 (White) or e8 (Black)

    // Masks for empty squares between king and rooks
    uint64_t kingsideMask =
        isWhite ? (1ULL << 5) | (1ULL << 6) : (1ULL << 61) | (1ULL << 62);  // f1, g1 or f8, g8
    uint64_t queensideMask =
        isWhite ? (1ULL << 1) | (1ULL << 2) | (1ULL << 3)
                : (1ULL << 57) | (1ULL << 58) | (1ULL << 59);  // b1, c1, d1 or b8, c8, d8

    // Allied and enemy occupancies
    uint64_t allOccupancy = board.getAllOccupancy();

    // Collect all enemy attack masks
    uint64_t enemyAttackMask = 0;
    for (int pieceType = (isWhite ? BLACK_PAWNS : WHITE_PAWNS);
         pieceType <= (isWhite ? BLACK_KINGS : WHITE_KINGS); ++pieceType) {
        uint64_t pieceBB = board.getBitboard(pieceType);
        while (pieceBB) {
            int square = __builtin_ctzll(pieceBB);
            enemyAttackMask |= generateThreatMask(pieceType, square, allOccupancy);
            pieceBB &= pieceBB - 1;  // Clear the current bit
        }
    }

    // Check kingside castling
    if (canCastleKingside) {
        // Ensure squares between king and rook are empty
        if (!(kingsideMask & allOccupancy)) {
            // Ensure those squares are not under attack
            if (!(kingsideMask & enemyAttackMask)) {
                uint16_t kingsideCastleMove =
                    encodeMove(kingSquare, isWhite ? 6 : 62, CASTLING_KINGSIDE);  // e1g1 or e8g8
                castlingMoves.push_back(kingsideCastleMove);
            }
        }
    }

    // Check queenside castling
    if (canCastleQueenside) {
        // Ensure squares between king and rook are empty
        if (!(queensideMask & allOccupancy)) {
            // Ensure those squares are not under attack
            if (!(queensideMask & enemyAttackMask)) {
                uint16_t queensideCastleMove =
                    encodeMove(kingSquare, isWhite ? 2 : 58, CASTLING_QUEENSIDE);  // e1c1 or e8c8
                castlingMoves.push_back(queensideCastleMove);
            }
        }
    }

    return castlingMoves;
}

/*
This function generates a bitboard with all the potential moves
for a single pawn on pawnSquare. It does not detect for check,
pins, or the legality of these potential moves. It does check
for en passant. It does not check for queening. For these
reasons, it does not generate a vector of encoded moves.
The output of this function is intended to have bitwise
operations done to it.
*/
static uint64_t generatePawnBitboard(
    int pawnSquare,           // The square of the pawn
    uint64_t enemyOccupancy,  // Bitboard of enemy pieces
    uint64_t allOccupancy,    // Bitboard of all occupied squares
    int enPassantSquare,      // The en passant target square (-1 if none)
    bool isWhite              // Whether the pawn is white
) {
    uint64_t moves = 0;
    uint64_t singlePawn = 1ULL << pawnSquare;

    // Forward moves
    if (isWhite) {
        // Single push
        uint64_t singlePush = singlePawn << 8;
        if (!(singlePush & allOccupancy)) {
            moves |= singlePush;

            // Double push
            if (pawnSquare >= 8 && pawnSquare < 16) {  // Starting rank
                uint64_t doublePush = singlePawn << 16;
                if (!(doublePush & allOccupancy)) {
                    moves |= doublePush;
                }
            }
        }
    } else {
        // Single push
        uint64_t singlePush = singlePawn >> 8;
        if (!(singlePush & allOccupancy)) {
            moves |= singlePush;

            // Double push
            if (pawnSquare >= 48 && pawnSquare < 56) {  // Starting rank
                uint64_t doublePush = singlePawn >> 16;
                if (!(doublePush & allOccupancy)) {
                    moves |= doublePush;
                }
            }
        }
    }

    // Capture moves
    std::array<uint64_t, 64> pawnThreatsTable = isWhite ? wpawn_threats_table : bpawn_threats_table;
    // uint64_t enPassantMask = 0;
    // if(enPassantSquare != NO_EN_PASSANT){
    //     enPassantMask = (1ULL << enPassantSquare);
    // }
    uint64_t enPassantMask = enPassantSquare != NO_EN_PASSANT ? (1ULL << enPassantSquare) : 0;
    // std::cout << "en passant square: " << enPassantSquare << "\n";
    // std::cout << "epm\n" << bitboardToBinaryString(enPassantMask) << "\n";
    uint64_t captures = pawnThreatsTable[pawnSquare] & (enemyOccupancy | enPassantMask);
    // std::cout << "eo or ep:\n" << bitboardToBinaryString((enemyOccupancy | enPassantMask)) << "\n";
    // std::cout << "captures:\n" << bitboardToBinaryString(captures) << "\n";
    moves |= captures;
    return moves;
}

// Function to convert potential pawn moves from a bitboard to uint16_t encoded moves
static std::vector<uint16_t> pawnBitboardToMoves(int pawn_square, uint64_t move_bitboard, uint8_t epsquare) {
    std::vector<uint16_t> encoded_moves;

    while (move_bitboard) {
        int dest_square = popLSB(move_bitboard);

        // Check for promotion
        if (dest_square >= 56 || dest_square <= 7) {  // 8th or 1st rank
            // Add four promotion moves: Queen, Rook, Bishop, Knight
            encoded_moves.push_back(encodeMove(pawn_square, dest_square, PROMOTION_QUEEN));
            encoded_moves.push_back(encodeMove(pawn_square, dest_square, PROMOTION_KNIGHT));
            encoded_moves.push_back(encodeMove(pawn_square, dest_square, PROMOTION_ROOK));
            encoded_moves.push_back(encodeMove(pawn_square, dest_square, PROMOTION_BISHOP));
        } else if (dest_square == epsquare) {  // En passant move
            encoded_moves.push_back(encodeMove(pawn_square, dest_square, EN_PASSANT));
        } else if (dest_square == pawn_square + 16 || dest_square == pawn_square - 16) {
            encoded_moves.push_back(encodeMove(pawn_square, dest_square, DOUBLE_PAWN_PUSH));
        } else {
            // Normal move
            encoded_moves.push_back(
                encodeMove(pawn_square, dest_square, SPECIAL_NONE));  // No promotion or special
        }
    }

    return encoded_moves;
}

// Function to generate all potential moves without checks or pins
static std::vector<uint16_t> generateMovesNoCheckNoPins(const BoardState& board) {
    std::vector<uint16_t> legalMoves;
    bool isWhite = board.getTurn();
    int knights = isWhite ? WHITE_KNIGHTS : BLACK_KNIGHTS;
    int queens = isWhite ? WHITE_QUEENS : BLACK_QUEENS;

    // Generate moves for all pieces from knights to queens (excludes pawns and kings)
    for (int pieceType = knights; pieceType <= queens; ++pieceType) {
        uint64_t pieceBB = board.getBitboard(pieceType);
        while (pieceBB) {
            int fromSquare = popLSB(pieceBB);
            uint64_t alliedOccupancy = board.getOccupancy(isWhite);
            uint64_t legalDestinations =
                generateThreatMask(pieceType, fromSquare, board.getAllOccupancy()) &
                ~alliedOccupancy;
            while (legalDestinations) {
                int toSquare = popLSB(legalDestinations);
                legalMoves.push_back(encodeMove(fromSquare, toSquare));
            }
        }
    }

    // Generate pawn moves
    uint64_t pawnsBB = board.getBitboard(isWhite ? WHITE_PAWNS : BLACK_PAWNS);
    while (pawnsBB) {
        int pawnSquare = popLSB(pawnsBB);
        uint64_t enemyOccupancy = board.getOccupancy(!isWhite);

        // std::cout << "enemyocc:\n" << bitboardToBinaryString(enemyOccupancy);
        uint64_t pawnMoves = generatePawnBitboard(
            pawnSquare, enemyOccupancy, board.getAllOccupancy(), board.getEnPassant(), isWhite);

        // Use the helper function to convert the bitboard to encoded moves
        std::vector<uint16_t> pawnMoveList = pawnBitboardToMoves(pawnSquare, pawnMoves, board.getEnPassant());
        legalMoves.insert(legalMoves.end(), pawnMoveList.begin(), pawnMoveList.end());
    }

    // Add king moves
    std::vector<uint16_t> kingMoves = generateKingMoves(board);
    legalMoves.insert(legalMoves.end(), kingMoves.begin(), kingMoves.end());

    // Add castling moves
    std::vector<uint16_t> castlingMoves = generateCastlingMoves(board);
    legalMoves.insert(legalMoves.end(), castlingMoves.begin(), castlingMoves.end());

    return legalMoves;
}

static std::vector<uint16_t> generateMovesNoCheckWithPins(const BoardState& board,
                                                          std::vector<uint64_t>& pinMasks) {
    std::vector<uint16_t> legalMoves;

    // Determine whose turn it is
    bool isWhite = board.getTurn();
    int knights = isWhite ? WHITE_KNIGHTS : BLACK_KNIGHTS;
    int queens = isWhite ? WHITE_QUEENS : BLACK_QUEENS;
    uint64_t alliedOccupancy = board.getOccupancy(isWhite);

    // Generate a bitboard of all pinned pieces
    uint64_t pinnedPieces = 0;
    for (uint64_t pinMask : pinMasks) {
        pinnedPieces |= (pinMask & alliedOccupancy);
    }

    // Generate moves for all pieces except pawns and king
    for (int pieceType = knights; pieceType <= queens; ++pieceType) {
        uint64_t pieceBB = board.getBitboard(pieceType);
        while (pieceBB) {
            int fromSquare = popLSB(pieceBB);
            uint64_t legalDestinations =
                generateThreatMask(pieceType, fromSquare, board.getAllOccupancy()) &
                ~alliedOccupancy;

            // Apply pin restrictions if the piece is pinned
            if (pinnedPieces & (1ULL << fromSquare)) {
                // Find and remove the specific pin mask for this piece
                for (auto it = pinMasks.begin(); it != pinMasks.end(); ++it) {
                    if (*it & (1ULL << fromSquare)) {
                        legalDestinations &= *it;
                        pinMasks.erase(it);  // Remove the pin mask after using it
                        break;
                    }
                }
            }

            // Add legal moves
            while (legalDestinations) {
                int toSquare = popLSB(legalDestinations);
                legalMoves.push_back(encodeMove(fromSquare, toSquare));
            }
        }
    }

    // Generate moves for pawns
    uint64_t pawnsBB = board.getBitboard(isWhite ? WHITE_PAWNS : BLACK_PAWNS);
    while (pawnsBB) {
        int pawnSquare = popLSB(pawnsBB);
        uint64_t enemyOccupancy = board.getOccupancy(!isWhite);

        uint64_t pawnMoves = generatePawnBitboard(
            pawnSquare, enemyOccupancy, board.getAllOccupancy(), board.getEnPassant(), isWhite);

        // Apply pin restrictions if the pawn is pinned
        if (pinnedPieces & (1ULL << pawnSquare)) {
            // Find and remove the specific pin mask for this piece
            for (auto it = pinMasks.begin(); it != pinMasks.end(); ++it) {
                if (*it & (1ULL << pawnSquare)) {
                    pawnMoves &= *it;
                    pinMasks.erase(it);  // Remove the pin mask after using it
                    break;
                }
            }
        }
        // Use helper function to handle pawn moves
        std::vector<uint16_t> pawnMoveList = pawnBitboardToMoves(pawnSquare, pawnMoves, board.getEnPassant());
        legalMoves.insert(legalMoves.end(), pawnMoveList.begin(), pawnMoveList.end());
    }

    // Add king moves
    std::vector<uint16_t> kingMoves = generateKingMoves(board);
    legalMoves.insert(legalMoves.end(), kingMoves.begin(), kingMoves.end());

    // Add castling moves
    std::vector<uint16_t> castlingMoves = generateCastlingMoves(board);
    legalMoves.insert(legalMoves.end(), castlingMoves.begin(), castlingMoves.end());

    return legalMoves;
}

static std::vector<uint16_t> generateMovesSingleCheckNoPins(const BoardState& board) {
    std::vector<uint16_t> legalMoves;

    bool isWhite = board.getTurn();
    uint64_t kingBB = board.getBitboard(isWhite ? WHITE_KINGS : BLACK_KINGS);

    int kingSquare = __builtin_ctzll(kingBB);

    // Grab the bitboard including only the checking knight/pawn.
    uint64_t attackerBitBoard = isImmediateCheckByKnightOrPawn(board);

    // Grab the int for the square number this piece is on.
    int attackerSquare = findAttackerSquare(board);

    // If the
    uint64_t blockOrCaptureMask =
        attackerBitBoard ? attackerBitBoard  // Mask for knight or pawn check
                         : generateRayMask(kingSquare, attackerSquare, board.getAllOccupancy());

    int knights = isWhite ? WHITE_KNIGHTS : BLACK_KNIGHTS;
    int queens = isWhite ? WHITE_QUEENS : BLACK_QUEENS;
    uint64_t enemyOccupancy = board.getOccupancy(!isWhite);

    // Generate moves for all pieces except pawns and king
    for (int pieceType = knights; pieceType <= queens; ++pieceType) {
        uint64_t pieceBB = board.getBitboard(pieceType);
        while (pieceBB) {
            int fromSquare = popLSB(pieceBB);
            uint64_t legalDestinations =
                generateThreatMask(pieceType, fromSquare, board.getAllOccupancy()) &
                blockOrCaptureMask;
            while (legalDestinations) {
                int toSquare = popLSB(legalDestinations);
                legalMoves.push_back(encodeMove(fromSquare, toSquare));
            }
        }
    }

    // Generate moves for pawns
    uint64_t pawnsBB = board.getBitboard(isWhite ? WHITE_PAWNS : BLACK_PAWNS);
    while (pawnsBB) {
        int pawnSquare = popLSB(pawnsBB);
        uint64_t pawnMoves =
            generatePawnBitboard(pawnSquare, enemyOccupancy, board.getAllOccupancy(),
                                 board.getEnPassant(), isWhite) &
            blockOrCaptureMask;

        // Use helper function to convert to encoded moves
        std::vector<uint16_t> pawnMoveList = pawnBitboardToMoves(pawnSquare, pawnMoves, board.getEnPassant());
        legalMoves.insert(legalMoves.end(), pawnMoveList.begin(), pawnMoveList.end());
    }

    // Add king moves
    std::vector<uint16_t> kingMoves = generateKingMoves(board);
    legalMoves.insert(legalMoves.end(), kingMoves.begin(), kingMoves.end());

    return legalMoves;
}

static std::vector<uint16_t> generateMovesSingleCheckWithPins(const BoardState& board,
                                                              std::vector<uint64_t>& pinMasks) {
    std::vector<uint16_t> legalMoves;
        // Use getTurn to determine whose turn it is
    bool isWhite = board.getTurn();

    uint64_t kingBB = board.getBitboard(isWhite ? WHITE_KINGS : BLACK_KINGS);
    // Extract the king's position
    int kingSquare = __builtin_ctzll(kingBB);

    // Grab the bitboard including only the checking knight/pawn.
    uint64_t attackerBitBoard = isImmediateCheckByKnightOrPawn(board);

    // Grab the int for the square number this piece is on.
    int attackerSquare = findAttackerSquare(board);

    // If the
    uint64_t blockOrCaptureMask =
        attackerBitBoard ? attackerBitBoard  // Mask for knight or pawn check
                         : generateRayMask(kingSquare, attackerSquare, board.getAllOccupancy());

    uint64_t enemyOccupancy = board.getOccupancy(!isWhite);

    // Generate moves for all pieces except the king and pawns (from knights to queens)
    for (int pieceType = (isWhite ? WHITE_KNIGHTS : BLACK_KNIGHTS);
         pieceType <= (isWhite ? WHITE_QUEENS : BLACK_QUEENS); ++pieceType) {
        uint64_t pieceBB = board.getBitboard(pieceType);
        while (pieceBB) {
            int fromSquare = popLSB(pieceBB);
            uint64_t legalDestinations =
                generateThreatMask(pieceType, fromSquare, board.getAllOccupancy()) &
                blockOrCaptureMask;

            // Check if the piece is pinned and restrict its legal moves accordingly
            for (size_t i = 0; i < pinMasks.size(); ++i) {
                uint64_t pinMask = pinMasks[i];
                if (pinMask & (1ULL << fromSquare)) {
                    // Restrict the legal destinations based on the pin mask
                    legalDestinations &= pinMask;
                    // Remove the pin mask from the vector, no need to check it again
                    pinMasks.erase(pinMasks.begin() + i);
                    break;  // No need to check other pin masks after using this one
                }
            }

            // Add valid moves for the piece
            while (legalDestinations) {
                int toSquare = popLSB(legalDestinations);
                legalMoves.push_back(encodeMove(fromSquare, toSquare));
            }
        }
    }

    // Generate moves for pawns
    uint64_t pawnsBB = board.getBitboard(isWhite ? WHITE_PAWNS : BLACK_PAWNS);
    while (pawnsBB) {
        int pawnSquare = popLSB(pawnsBB);
        uint64_t pawnMoves =
            generatePawnBitboard(pawnSquare, enemyOccupancy, board.getAllOccupancy(),
                                 board.getEnPassant(), isWhite) &
            blockOrCaptureMask;

        // Check if the pawn is pinned
        for (size_t i = 0; i < pinMasks.size(); ++i) {
            uint64_t pinMask = pinMasks[i];
            if (pinMask & (1ULL << pawnSquare)) {
                // Restrict the pawn's legal moves based on the pin mask
                pawnMoves &= pinMask;
                // Remove the pin mask from the vector, no need to check it again
                pinMasks.erase(pinMasks.begin() + i);
                break;  // No need to check other pin masks after using this one
            }
        }

        // Convert pawn moves from bitboard to encoded moves
        std::vector<uint16_t> pawnMoveList = pawnBitboardToMoves(pawnSquare, pawnMoves, board.getEnPassant());
        legalMoves.insert(legalMoves.end(), pawnMoveList.begin(), pawnMoveList.end());
    }

    // Add king moves
    std::vector<uint16_t> kingMoves = generateKingMoves(board);
    legalMoves.insert(legalMoves.end(), kingMoves.begin(), kingMoves.end());

    return legalMoves;
}

std::vector<uint16_t> allLegalMoves(const BoardState& board) {
    // Determine the number of attackers
    int attacking_pieces = numCheckers(board);  // Returns 0, 1, or 2
    std::vector<uint16_t> legalMoves;
    bool isWhite = board.getTurn();
    uint64_t kingBB = board.getBitboard(isWhite ? WHITE_KINGS : BLACK_KINGS);

    // Extract the king's position
    int kingSquare = __builtin_ctzll(kingBB);
    uint64_t enemyOccupancy = board.getOccupancy(!isWhite);
    uint64_t alliedOccupancy = board.getOccupancy(isWhite);

    uint64_t enemyQueens =
        isWhite ? board.getBitboard(BLACK_QUEENS) : board.getBitboard(WHITE_QUEENS);
    uint64_t enemyRooks = isWhite ? board.getBitboard(BLACK_ROOKS) : board.getBitboard(WHITE_ROOKS);
    uint64_t enemyBishops =
        isWhite ? board.getBitboard(BLACK_BISHOPS) : board.getBitboard(WHITE_BISHOPS);

    // Case 1: No checks (attacking_pieces == 0)
    if (attacking_pieces == 0) {
        // Detect pinned pieces
        std::vector<uint64_t> pinMasks = detectPinnedPieces(
            kingSquare, enemyOccupancy, enemyQueens, enemyBishops, enemyRooks, alliedOccupancy);

        // If there are no pinned pieces, generate moves without checks or pins
        if (pinMasks.empty()) {
            legalMoves = generateMovesNoCheckNoPins(board);
        } else {
            // Otherwise, generate moves considering pins
            legalMoves = generateMovesNoCheckWithPins(board, pinMasks);
        }

    }
    // Case 2: Single check (attacking_pieces == 1)
    else if (attacking_pieces == 1) {
        // Detect pinned pieces
        std::vector<uint64_t> pinMasks = detectPinnedPieces(
            kingSquare, enemyOccupancy, enemyQueens, enemyBishops, enemyRooks, alliedOccupancy);

        // If there are no pinned pieces, generate moves considering 1 check and no pins
        if (pinMasks.empty()) {
            legalMoves = generateMovesSingleCheckNoPins(board);
        } else {
            // Otherwise, generate moves considering 1 check with pins
            legalMoves = generateMovesSingleCheckWithPins(board, pinMasks);
        }

    }
    // Case 3: Double check (attacking_pieces == 2)
    else {
        // In a double check, only the king can move
        legalMoves = generateKingMoves(board);
    }

    return legalMoves;
}
