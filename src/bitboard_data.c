/*-------------------------------------------------------------------------------
  tucano is a XBoard chess playing engine developed by Alcides Schulz.
  Copyright (C) 2011-present - Alcides Schulz

  tucano is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  tucano is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You can find the GNU General Public License at http://www.gnu.org/licenses/
-------------------------------------------------------------------------------*/

#include "globals.h"

//-------------------------------------------------------------------------------------------------
//  Initialization of several bitboards used in the program.
//-------------------------------------------------------------------------------------------------

void    init_king_moves(void);
void    init_knight_moves(void);
void    init_rankfile_moves(void);
void    init_diagonal_moves(void);
void    init_from_to_path(void);
void    init_pawn_attack(void);
void    init_rays(void);
void    init_passed_pawn(void);
void    init_weak_pawn(void);
void    init_connected_pawn(void);
void    init_isolated_pawn(void);

void    gen_move_bit(U64 *bb, int rank, int file);
U64     gen_path_bb(int rank_from, int file_from, int rank_direction, int file_direction);

U64     king_moves[64];         // possible king moves from each square
U64     knight_moves[64];       // possible knight moves from each square
U64     west_moves[64];         // possible moves to west from each square
U64     east_moves[64];         // possible moves to east from each square
U64     north_moves[64];        // possible moves to north from each square
U64     south_moves[64];        // possible moves to south from each square
U64     se_moves[64];           // possible moves to southeast from each square 
U64     sw_moves[64];           // possible moves to southwest from each square
U64     ne_moves[64];           // possible moves to northeast from each square
U64     nw_moves[64];           // possible moves to northwest from each square
U64     rankfile_moves[64];     // all rank and file moves from square
U64     diagonal_moves[64];     // all diagonal moves from square
U64     from_to_path[64][64];   // path from square to square: eg. from A1 to A8 = A2-A7
U64     pawn_attack[2][64];     // position for pawns to attack a square
U64     pawn_pass_mask[2][64];  // mask to test passed pawns
U64     pawn_weak_mask[2][64];  // mask to test weak pawns
U64     pawn_conn_mask[64];     // mask to test connected pawns
U64     pawn_isol_mask[64];     // mask to test isolated pawns

//-------------------------------------------------------------------------------------------------
//  Initialize data.
//-------------------------------------------------------------------------------------------------
void bb_data_init(void) 
{
    assert(valid_rank_file());
    init_king_moves();
    init_knight_moves();
    init_rankfile_moves();
    init_diagonal_moves();
    init_from_to_path();
    init_pawn_attack();
    init_rays();
    init_passed_pawn();
    init_weak_pawn();
    init_connected_pawn();
    init_isolated_pawn();
}

//-------------------------------------------------------------------------------------------------
//  Create bitboards to represent kings moves from a square
//  Example from E3
//  00000000
//  00000000
//  00000000
//  00000000
//  00011100
//  00010100
//  00011100
//  00000000
//-------------------------------------------------------------------------------------------------
void init_king_moves(void)
{
    int        from_square;
    int        rank;
    int        file;

    for (from_square = 0; from_square < 64; from_square++) {
        king_moves[from_square] = 0;
        
        rank = get_rank(from_square);
        file = get_file(from_square);

        gen_move_bit(&king_moves[from_square], rank - 1, file);
        gen_move_bit(&king_moves[from_square], rank + 1, file);
        gen_move_bit(&king_moves[from_square], rank, file - 1);
        gen_move_bit(&king_moves[from_square], rank, file + 1);
        gen_move_bit(&king_moves[from_square], rank - 1, file + 1);
        gen_move_bit(&king_moves[from_square], rank - 1, file - 1);
        gen_move_bit(&king_moves[from_square], rank + 1, file + 1);
        gen_move_bit(&king_moves[from_square], rank + 1, file - 1);
    }
}

//-------------------------------------------------------------------------------------------------
//  Create bitboards to represent knight moves from a square
//  Example from E5
//  00000000
//  00010100
//  00100010
//  00000000
//  00100010
//  00010100
//  00000000
//  00000000
//-------------------------------------------------------------------------------------------------
void init_knight_moves(void)
{
    int        from_square;
    int        rank;
    int        file;

    for (from_square = 0; from_square < 64; from_square++) {
        knight_moves[from_square] = 0;

        rank = get_rank(from_square);
        file = get_file(from_square);

        gen_move_bit(&knight_moves[from_square], rank + 2, file + 1);
        gen_move_bit(&knight_moves[from_square], rank + 2, file - 1);
        gen_move_bit(&knight_moves[from_square], rank - 2, file + 1);
        gen_move_bit(&knight_moves[from_square], rank - 2, file - 1);
        gen_move_bit(&knight_moves[from_square], rank + 1, file + 2);
        gen_move_bit(&knight_moves[from_square], rank - 1, file + 2);
        gen_move_bit(&knight_moves[from_square], rank + 1, file - 2);
        gen_move_bit(&knight_moves[from_square], rank - 1, file - 2);
    }
}

//-------------------------------------------------------------------------------------------------
//  Create bitboards to represent rankfile (rook) moves from a square
//  Example: east moves from B2
//  00000000
//  00000000
//  00000000
//  00000000
//  00000000
//  00000000
//  00111111
//  00000000
//-------------------------------------------------------------------------------------------------
void init_rankfile_moves(void)
{
    int        from_square;
    int        from_rank;
    int        from_file;

    for (from_square = 0; from_square < 64; from_square++) {
        from_rank = get_rank(from_square);
        from_file = get_file(from_square);

        east_moves[from_square]  = gen_path_bb(from_rank, from_file,  0,  1);
        west_moves[from_square]  = gen_path_bb(from_rank, from_file,  0, -1);
        south_moves[from_square] = gen_path_bb(from_rank, from_file,  1,  0);
        north_moves[from_square] = gen_path_bb(from_rank, from_file, -1,  0);
    }
}

//-------------------------------------------------------------------------------------------------
//  Create bitboards to represent diagonal (bishop) moves from a square.
//  Example: southeast moves from B8
//  00000000
//  00100000
//  00010000
//  00001000
//  00000100
//  00000010
//  00000001
//  00000000
//-------------------------------------------------------------------------------------------------
void init_diagonal_moves(void)
{
    int        from_square;
    int        from_rank;
    int        from_file;

    for (from_square = 0; from_square < 64; from_square++) {
        from_rank = get_rank(from_square);
        from_file = get_file(from_square);

        se_moves[from_square] = gen_path_bb(from_rank, from_file,  1,  1);
        ne_moves[from_square] = gen_path_bb(from_rank, from_file, -1,  1);
        sw_moves[from_square] = gen_path_bb(from_rank, from_file,  1, -1);
        nw_moves[from_square] = gen_path_bb(from_rank, from_file, -1, -1);
    }
}

//-------------------------------------------------------------------------------------------------
//  Create bitboards to represent a path between 2 squares. 
//  Example: path A1-H8
//  00000000
//  00000010
//  00000100
//  00001000
//  00010000
//  00100000
//  01000000
//  00000000
//  NOTE: use aleady generated rankfile and diagonal bitboards
//-------------------------------------------------------------------------------------------------
void init_from_to_path(void)
{
    int        from_square;
    int        to_square;

    for (from_square = 0; from_square < 64; from_square++) {
        for (to_square = 0; to_square < 64; to_square++) {
            from_to_path[from_square][to_square] = 0;
            from_to_path[from_square][to_square] |= east_moves_bb(from_square) & west_moves_bb(to_square);
            from_to_path[from_square][to_square] |= west_moves_bb(from_square) & east_moves_bb(to_square);
            from_to_path[from_square][to_square] |= south_moves_bb(from_square) & north_moves_bb(to_square);
            from_to_path[from_square][to_square] |= north_moves_bb(from_square) & south_moves_bb(to_square);
            from_to_path[from_square][to_square] |= se_moves_bb(from_square) & nw_moves_bb(to_square);
            from_to_path[from_square][to_square] |= nw_moves_bb(from_square) & se_moves_bb(to_square);
            from_to_path[from_square][to_square] |= sw_moves_bb(from_square) & ne_moves_bb(to_square);
            from_to_path[from_square][to_square] |= ne_moves_bb(from_square) & sw_moves_bb(to_square);
        }
    }
}

//-------------------------------------------------------------------------------------------------
//  Create bitboards to represent a where a pawn can attack a square. 
//  Example: positions where a white pawn can attack E4:
//  00000000
//  00000000
//  00000000
//  00000000
//  00000000
//  00010100
//  00000000
//  00000000
//  NOTE: use aleady generated rankfile and diagonal bitboards
//-------------------------------------------------------------------------------------------------
void init_pawn_attack(void)
{
    int        rank;
    int        file;
    int        square;

    for (rank = 0; rank < 6; rank++) {
        for (file = 0; file < 8; file++) {
            square = get_square(rank, file);
            pawn_attack[WHITE][square] = 0;
            gen_move_bit(&pawn_attack[WHITE][square], rank + 1, file + 1);
            gen_move_bit(&pawn_attack[WHITE][square], rank + 1, file - 1);
        }
    }

    for (rank = 1; rank < 8; rank++) {
        for (file = 0; file < 8; file++) {
            square = get_square(rank, file);
            pawn_attack[BLACK][square] = 0;
            gen_move_bit(&pawn_attack[BLACK][square], rank - 1, file + 1);
            gen_move_bit(&pawn_attack[BLACK][square], rank - 1, file - 1);
        }
    }
}

//-------------------------------------------------------------------------------------------------
//  Create bitboards to represent a rank/file and diagonal rays from square. 
//  Example: rankfile rays from E4:
//  00001000
//  00001000
//  00001000
//  00001000
//  11110111
//  00001000
//  00001000
//  00001000
//  NOTE: use aleady generated rankfile and diagonal bitboards
//-------------------------------------------------------------------------------------------------
void init_rays(void)
{
    int        square;

    for (square = 0; square < 64; square++) {
        rankfile_moves[square] = 0;
        rankfile_moves[square] |= north_moves_bb(square);
        rankfile_moves[square] |= south_moves_bb(square);
        rankfile_moves[square] |= east_moves_bb(square);
        rankfile_moves[square] |= west_moves_bb(square);
        diagonal_moves[square] = 0;
        diagonal_moves[square] |= ne_moves_bb(square);
        diagonal_moves[square] |= nw_moves_bb(square);
        diagonal_moves[square] |= se_moves_bb(square);
        diagonal_moves[square] |= sw_moves_bb(square);
    }
}

//-------------------------------------------------------------------------------------------------
//  Create bitboards to represent a mask for passed pawn evaluation
//  Example: mask for white pawn on E4
//  00000000
//  00011100
//  00011100
//  00011100
//  00000000
//  00000000
//  00000000
//  00000000
//-------------------------------------------------------------------------------------------------
void init_passed_pawn(void)
{
    int        rank;
    int        file;
    int        temp;
    int        square;
    
    for (rank = 0; rank < 8; rank++)  {
        for (file = 0; file < 8; file++) {
            square = get_square(rank, file);
            pawn_pass_mask[WHITE][square] = 0;
            pawn_pass_mask[BLACK][square] = 0;
            for (temp = rank - 1; temp >= 1; temp--) {
                bb_set_bit_rf(&pawn_pass_mask[WHITE][square], temp, file);
                if (file > 0) bb_set_bit_rf(&pawn_pass_mask[WHITE][square], temp, file - 1);
                if (file < 7) bb_set_bit_rf(&pawn_pass_mask[WHITE][square], temp, file + 1);
            }
            for (temp = rank + 1; temp <= 6; temp++) {
                bb_set_bit_rf(&pawn_pass_mask[BLACK][square], temp, file);
                if (file > 0) bb_set_bit_rf(&pawn_pass_mask[BLACK][square], temp, file - 1);
                if (file < 7) bb_set_bit_rf(&pawn_pass_mask[BLACK][square], temp, file + 1);
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------
//  Create bitboards to represent a mask for weak pawn evaluation
//  Example: mask for white pawn on E4
//  00000000
//  00000000
//  00000000
//  00000000
//  00010100
//  00010100
//  00010100
//  00010100
//-------------------------------------------------------------------------------------------------
void init_weak_pawn(void)
{
    int     rank;
    int        file;
    int     square;

    for (square = 0; square < 64; square++) {
        pawn_weak_mask[WHITE][square] = 0;
        pawn_weak_mask[BLACK][square] = 0;
        file = get_file(square);
        rank = get_rank(square);
        if (rank > 0 && rank < 7) {
            if (file > 0) pawn_weak_mask[WHITE][square] |= gen_path_bb(rank - 1, file - 1, +1, 0);
            if (file < 7) pawn_weak_mask[WHITE][square] |= gen_path_bb(rank - 1, file + 1, +1, 0);
            if (file > 0) pawn_weak_mask[BLACK][square] |= gen_path_bb(rank + 1, file - 1, -1, 0);
            if (file < 7) pawn_weak_mask[BLACK][square] |= gen_path_bb(rank + 1, file + 1, -1, 0);
        }
    }
}

//-------------------------------------------------------------------------------------------------
//  Create bitboards to represent a mask for connected pawns evaluation
//  Example: mask for pawn on E4 (black or white)
//  00000000
//  00000000
//  00000000
//  00010100
//  00010100
//  00010100
//  00000000
//  00000000
//-------------------------------------------------------------------------------------------------
void init_connected_pawn(void)
{
    int     rank;
    int        file;
    int     square;

    for (square = 0; square < 64; square++) {
        pawn_conn_mask[square] = 0;
        pawn_conn_mask[square] = 0;
        file = get_file(square);
        rank = get_rank(square);
        if (rank > 0 && rank < 7) {
            if (file > 0) {
                bb_set_bit(&pawn_conn_mask[square], square - 1);
                bb_set_bit(&pawn_conn_mask[square], square - 9);
                bb_set_bit(&pawn_conn_mask[square], square + 7);
            }
            if (file < 7) {
                bb_set_bit(&pawn_conn_mask[square], square + 1);
                bb_set_bit(&pawn_conn_mask[square], square - 7);
                bb_set_bit(&pawn_conn_mask[square], square + 9);
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------
//  Create bitboards to represent a mask for isolated pawns evaluation
//  Example: mask for pawn on E4 (black or white)
//  00000000
//  00010100
//  00010100
//  00010100
//  00010100
//  00010100
//  00010100
//  00000000
//-------------------------------------------------------------------------------------------------
void init_isolated_pawn(void)
{
    int     rank;
    int        file;
    int     square;

    for (square = 0; square < 64; square++) {
        pawn_isol_mask[square] = 0;
        file = get_file(square);
        if (file > 0) {
            for (rank = 1 ; rank < 7; rank++)
                bb_set_bit_rf(&pawn_isol_mask[square], rank, file - 1);
        }
        if (file < 7) {
            for (rank = 1 ; rank < 7; rank++)
                bb_set_bit_rf(&pawn_isol_mask[square], rank, file + 1);
        }
    }
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard for king moves
//-------------------------------------------------------------------------------------------------
U64 king_moves_bb(int from_square)
{
    assert(from_square >= 0 && from_square < 64);
    return king_moves[from_square];
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard for knight moves
//-------------------------------------------------------------------------------------------------
U64 knight_moves_bb(int from_square)
{
    assert(from_square >= 0 && from_square < 64);
    return knight_moves[from_square];
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard with moves to west
//-------------------------------------------------------------------------------------------------
U64 west_moves_bb(int from_square)
{
    assert(from_square >= 0 && from_square < 64);
    return west_moves[from_square];
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard with moves to east
//-------------------------------------------------------------------------------------------------
U64 east_moves_bb(int from_square)
{
    assert(from_square >= 0 && from_square < 64);
    return east_moves[from_square];
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard with moves to north
//-------------------------------------------------------------------------------------------------
U64 north_moves_bb(int from_square)
{
    assert(from_square >= 0 && from_square < 64);
    return north_moves[from_square];
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard with moves to south
//-------------------------------------------------------------------------------------------------
U64 south_moves_bb(int from_square)
{
    assert(from_square >= 0 && from_square < 64);
    return south_moves[from_square];
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard with moves to southeast
//-------------------------------------------------------------------------------------------------
U64 se_moves_bb(int from_square)
{
    assert(from_square >= 0 && from_square < 64);
    return se_moves[from_square];
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard with moves to southwest
//-------------------------------------------------------------------------------------------------
U64 sw_moves_bb(int from_square)
{
    assert(from_square >= 0 && from_square < 64);
    return sw_moves[from_square];
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard with moves to northeast
//-------------------------------------------------------------------------------------------------
U64 ne_moves_bb(int from_square)
{
    assert(from_square >= 0 && from_square < 64);
    return ne_moves[from_square];
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard with moves to northwest
//-------------------------------------------------------------------------------------------------
U64 nw_moves_bb(int from_square)
{
    assert(from_square >= 0 && from_square < 64);
    return nw_moves[from_square];
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard for all rankfile (rook) moves
//-------------------------------------------------------------------------------------------------
U64 rankfile_moves_bb(int from_square)
{
    assert(from_square >= 0 && from_square < 64);
    return rankfile_moves[from_square];
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard for all diagonal (bishop) moves
//-------------------------------------------------------------------------------------------------
U64 diagonal_moves_bb(int from_square)
{
    assert(from_square >= 0 && from_square < 64);
    return diagonal_moves[from_square];
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard for moves between 2 square
//-------------------------------------------------------------------------------------------------
U64 from_to_path_bb(int from_square, int to_square)
{
    assert(from_square >= 0 && from_square < 64);
    assert(to_square >= 0 && to_square < 64);
    return from_to_path[from_square][to_square];
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard from where pawns can attack a square
//-------------------------------------------------------------------------------------------------
U64 pawn_attack_bb(int color, int square)
{
    assert(color == WHITE || color == BLACK);
    assert(square >= 0 && square < 64);
    return pawn_attack[color][square];
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard with forward path depending on color: white=north, black=south
//-------------------------------------------------------------------------------------------------
U64 forward_path_bb(int color, int from_square)
{
    assert(color == WHITE || color == BLACK);
    assert(from_square >= 0 && from_square < 64);
    return color == WHITE ? north_moves[from_square] : south_moves[from_square];
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard with backward path depending on color: white=south, black=north
//-------------------------------------------------------------------------------------------------
U64 backward_path_bb(int color, int from_square)
{
    assert(color == WHITE || color == BLACK);
    assert(from_square >= 0 && from_square < 64);
    return color == WHITE ? south_moves[from_square] : north_moves[from_square];
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard to represent a mask used for passed pawns evaluation.
//-------------------------------------------------------------------------------------------------
U64 passed_mask_bb(int color, int from_square)
{
    assert(color == WHITE || color == BLACK);
    assert(from_square >= 0 && from_square < 64);
    return pawn_pass_mask[color][from_square];
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard to represent a mask used for weak pawns evaluation.
//-------------------------------------------------------------------------------------------------
U64 weak_mask_bb(int color, int from_square)
{
    assert(color == WHITE || color == BLACK);
    assert(from_square >= 0 && from_square < 64);
    return pawn_weak_mask[color][from_square];
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard to represent a mask used for connected pawns evaluation.
//-------------------------------------------------------------------------------------------------
U64 connected_mask_bb(int from_square)
{
    assert(from_square >= 0 && from_square < 64);
    return pawn_conn_mask[from_square];
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard to represent a mask used for isolated pawns evaluation.
//-------------------------------------------------------------------------------------------------
U64 isolated_mask_bb(int from_square)
{
    assert(from_square >= 0 && from_square < 64);
    return pawn_isol_mask[from_square];
}

//-------------------------------------------------------------------------------------------------
//  Utility function to set bit if rank and file are inside board.
//-------------------------------------------------------------------------------------------------
void gen_move_bit(U64 *bb, int rank, int file)
{
    assert(bb != NULL);

    if (is_rank_valid(rank) && is_file_valid(file)) {
        bb_set_bit_rf(bb, rank, file);
    }
}

//-------------------------------------------------------------------------------------------------
//  Utility function to generate a path in the board from a starting location to board limits.
//-------------------------------------------------------------------------------------------------
U64 gen_path_bb(int rank_from, int file_from, int rank_direction, int file_direction)
{
    int     new_rank;
    int        new_file;
    U64        bb = 0;

    assert(is_rank_valid(rank_from));
    assert(is_file_valid(file_from));
    assert(rank_direction >= -1 && rank_direction <= 1);
    assert(file_direction >= -1 && file_direction <= 1);

    new_rank = rank_from + rank_direction;
    new_file = file_from + file_direction;

    while (is_rank_valid(new_rank) && is_file_valid(new_file)) {
        bb_set_bit_rf(&bb, new_rank, new_file);
        new_rank += rank_direction;
        new_file += file_direction;
    }

    return bb;
}

//END
