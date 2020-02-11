/*-------------------------------------------------------------------------------
  tucano is a chess playing engine developed by Alcides Schulz.
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
//  Functions to detect attacks on square, king or castle.
//-------------------------------------------------------------------------------------------------

//    Masks for attacks of pieces(knight, king and pawn) on white king side castle 
#define BB_WKSN     ((U64)0x00000000001F3B00)
#define BB_WKSK     ((U64)0x0000000000001F1F)
#define BB_WKSP     ((U64)0x0000000000001F00)

//    Masks for attacks of pieces(knight, king and pawn) on white queen side castle
#define BB_WQSN     ((U64)0x00000000007CEE00)
#define BB_WQSK     ((U64)0x0000000000007C7C)
#define BB_WQSP     ((U64)0x0000000000007C00)

//    Masks for attacks of pieces(knight, king and pawn) on black king side castle
#define BB_BKSN     ((U64)0x003B1F0000000000)
#define BB_BKSK     ((U64)0x1F1F000000000000)
#define BB_BKSP     ((U64)0x001F000000000000)

//    Masks for attacks of pieces(knight, king and pawn) on black queen side castle
#define BB_BQSN     ((U64)0x00EE7C0000000000)
#define BB_BQSK     ((U64)0x7C7C000000000000)
#define BB_BQSP     ((U64)0x007C000000000000)

//    Castle functions.
int     is_white_kingside_attacked(BOARD *board);
int     is_white_queenside_attacked(BOARD *board);
int     is_black_kingside_attacked(BOARD *board);
int     is_black_queenside_attacked(BOARD *board);

//  Utility functions.
int     is_square_attacked(BOARD *board, int square, int by_color, U64 occup);
int     is_aligned(int s1, int s2, int s3);

//-------------------------------------------------------------------------------------------------
//    Verify if move made leaves king in check, or in case of castle move, the castle is under attack
//-------------------------------------------------------------------------------------------------
int is_illegal(BOARD *board, MOVE move)
{
    assert(move);

    switch (unpack_type(move)) {
    case MT_CSBQS: return is_black_queenside_attacked(board);
    case MT_CSBKS: return is_black_kingside_attacked(board);
    case MT_CSWQS: return is_white_queenside_attacked(board);
    case MT_CSWKS: return is_white_kingside_attacked(board);
    }
    if (is_incheck(board, flip_color(side_on_move(board))))
        return TRUE;
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Verify if pseudo move leaves king in check, or in case of castle move, the castle is under 
//  attack. Test is made before making the move.
//-------------------------------------------------------------------------------------------------
int is_pseudo_legal(BOARD *board, U64 pins, MOVE move)
{
    U64     temp;

    switch (unpack_type(move)) {
    case MT_CSBQS: return is_black_queenside_attacked(board) ? FALSE : TRUE;
    case MT_CSBKS: return is_black_kingside_attacked(board) ? FALSE : TRUE;
    case MT_CSWQS: return is_white_queenside_attacked(board) ? FALSE : TRUE;
    case MT_CSWKS: return is_white_kingside_attacked(board) ? FALSE : TRUE;
    case MT_EPCAP:
        make_move(board, move);
        if (is_illegal(board, move)) {
            undo_move(board);
            return FALSE;
        }
        undo_move(board);
        return TRUE;
    }

    if (unpack_piece(move) == KING) {
        temp = occupied_bb(board) ^ square_bb(unpack_from(move));
        return is_square_attacked(board, unpack_to(move), flip_color(side_on_move(board)), temp) ? FALSE : TRUE;
    }

    if (bb_is_one(pins, unpack_from(move)))
        if (!is_aligned(unpack_from(move), unpack_to(move), king_square(board, side_on_move(board))))
            return FALSE;

    return TRUE;
}

//-------------------------------------------------------------------------------------------------
//    Locate pinned pieces to king. Used by is_pseudo_legal in order to avoid leaving king in check.
//-------------------------------------------------------------------------------------------------
U64 find_pins(BOARD *board)
{
    U64 pins = 0;
    int opp = flip_color(side_on_move(board));

    U64 attacks = 0;
    attacks |= rankfile_moves_bb(king_square(board, side_on_move(board))) & (queen_rook_bb(board, opp));
    attacks |= diagonal_moves_bb(king_square(board, side_on_move(board))) & (queen_bishop_bb(board, opp));
    
    while (attacks) {
        int from = bb_first_index(attacks);
        U64 pinned = from_to_path_bb(king_square(board, side_on_move(board)), from) & occupied_bb(board);
        if (bb_bit_count(pinned) == 1 && (pinned & all_pieces_bb(board, side_on_move(board))))
            pins |= pinned;
        bb_clear_bit(&attacks, from);
    }

    return pins;
}

//-------------------------------------------------------------------------------------------------
//  Test if squares are aligned. (Copied from stockfish)
//-------------------------------------------------------------------------------------------------
int is_aligned(int s1, int s2, int s3)
{
    return (from_to_path_bb(s1, s2) | from_to_path_bb(s1, s3) | from_to_path_bb(s2, s3))
         & (square_bb(s1) | square_bb(s2) | square_bb(s3)) ? TRUE : FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Verify if move made leaves king in check.
//-------------------------------------------------------------------------------------------------
int is_incheck(BOARD *board, int color)
{
    int        square = king_square(board, color);
    int        opp = flip_color(color);

    assert(valid_color(color));
    assert(valid_square(king_square(board, color)));
    
    // Attacked by knight
    if (knight_moves_bb(square) & knight_bb(board, opp))
        return TRUE;
    // Attacked by sliding piece
    if (bb_rook_attacks(square, occupied_bb(board)) & (queen_rook_bb(board, opp)))
        return TRUE;
    if (bb_bishop_attacks(square, occupied_bb(board)) & (queen_bishop_bb(board, opp)))
        return TRUE;
    // Attacked by pawn
    if (pawn_attack_bb(opp, square) & pawn_bb(board, opp))
        return TRUE;
    // Attacked by king
    if (king_moves_bb(square) & king_bb(board, opp))
        return TRUE;
    // No attacks
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Verify if square is attacked by color
//-------------------------------------------------------------------------------------------------
int is_square_attacked(BOARD *board, int square, int by_color, U64 occup)
{
    assert(valid_color(by_color));
    
    // Attacked by knight
    if (knight_moves_bb(square) & knight_bb(board, by_color))
        return TRUE;
    // Attacked by sliding piece
    if (bb_rook_attacks(square, occup) & (queen_rook_bb(board, by_color)))
        return TRUE;
    if (bb_bishop_attacks(square, occup) & (queen_bishop_bb(board, by_color)))
        return TRUE;
    // Attacked by pawn
    if (pawn_attack_bb(by_color, square) & pawn_bb(board, by_color))
        return TRUE;
    // Attacked by king
    if (king_moves_bb(square) & king_bb(board, by_color))
        return TRUE;
    // No attacks
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Verify if squares E1,F1,G1 are attacked by black player
//-------------------------------------------------------------------------------------------------
int is_white_kingside_attacked(BOARD *board)
{
    int     square;
    U64     pc_qr;
    U64     pc_qb;

    // Attacked by knight
    if (knight_bb(board, BLACK) & BB_WKSN)
        return TRUE;
    // Attacked by rook, bishop or queen
    pc_qr = (queen_rook_bb(board, BLACK));
    pc_qb = (queen_bishop_bb(board, BLACK));
    for (square = E1; square <= G1; square++) {
        if (bb_rook_attacks(square, occupied_bb(board)) & pc_qr)
            return TRUE;
        if (bb_bishop_attacks(square, occupied_bb(board)) & pc_qb)
            return TRUE;
    }
    // Attacked by pawn
    if (pawn_bb(board, BLACK) & BB_WKSP)
        return TRUE;
    // Attacked by king
    if (king_bb(board, BLACK) & BB_WKSK)
        return TRUE;
    // No attacks
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Verify if squares C1,D1,E1 are attacked by black player
//-------------------------------------------------------------------------------------------------
int is_white_queenside_attacked(BOARD *board)
{
    int     square;
    U64     pc_qr;
    U64     pc_qb;

    // Attacked by knight
    if (knight_bb(board, BLACK) & BB_WQSN)
        return TRUE;
    // Attacked by rook, bishop or queen
    pc_qr = (queen_rook_bb(board, BLACK));
    pc_qb = (queen_bishop_bb(board, BLACK));
    for (square = C1; square <= E1; square++) {
        if (bb_rook_attacks(square, occupied_bb(board)) & pc_qr)
            return TRUE;
        if (bb_bishop_attacks(square, occupied_bb(board)) & pc_qb)
            return TRUE;
    }
    // Attacked by pawn
    if (pawn_bb(board, BLACK) & BB_WQSP)
        return TRUE;
    // Attacked by king
    if (king_bb(board, BLACK) & BB_WQSK)
        return TRUE;
    // No attacks
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Verify if squares E8,F8,G8 are attacked by white player
//-------------------------------------------------------------------------------------------------
int is_black_kingside_attacked(BOARD *board)
{
    int     square;
    U64     pc_qr;
    U64     pc_qb;

    // Attacked by knight
    if (knight_bb(board, WHITE) & BB_BKSN)
        return TRUE;
    // Attacked by rook, bishop or queen
    pc_qr = (queen_rook_bb(board, WHITE));
    pc_qb = (queen_bishop_bb(board, WHITE));
    for (square = E8; square <= G8; square++) {
        if (bb_rook_attacks(square, occupied_bb(board)) & pc_qr)
            return TRUE;
        if (bb_bishop_attacks(square, occupied_bb(board)) & pc_qb)
            return TRUE;
    }
    // Attacked by pawn
    if (pawn_bb(board, WHITE) & BB_BKSP)
        return TRUE;
    // Attacked by king
    if (king_bb(board, WHITE) & BB_BKSK)
        return TRUE;
    // No attacks
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Verify if squares C8,D8,E8 are attacked by white player
//-------------------------------------------------------------------------------------------------
int is_black_queenside_attacked(BOARD *board)
{
    int     square;
    U64     pc_qr;
    U64     pc_qb;

    // Attacked by knight
    if (knight_bb(board, WHITE) & BB_BQSN)
        return TRUE;
    // Attacked by rook, bishop or queen
    pc_qr = (queen_rook_bb(board, WHITE));
    pc_qb = (queen_bishop_bb(board, WHITE));
    for (square = C8; square <= E8; square++) {
        if (bb_rook_attacks(square, occupied_bb(board)) & pc_qr)
            return TRUE;
        if (bb_bishop_attacks(square, occupied_bb(board)) & pc_qb)
            return TRUE;
    }
    // Attacked by pawn
    if (pawn_bb(board, WHITE) & BB_BQSP)
        return TRUE;
    // Attacked by king
    if (king_bb(board, WHITE) & BB_BQSK)
        return TRUE;
    // No attacks
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Test if is a checking move (before making the move).
//-------------------------------------------------------------------------------------------------
int is_check(BOARD *board, MOVE move)
{
    int     mvpc = unpack_piece(move);
    int     tosq = unpack_to(move);
    int     frsq = unpack_from(move);
    U64     piece;
    int     my_color = side_on_move(board);
    int     op_color = flip_color(my_color);
    U64     opp_king = king_bb(board, op_color);
    int     oksq = king_square(board, op_color);
    U64     occup = occupied_bb(board);
    int     pcsq;
    int     epsq;

    bb_clear_bit(&occup, frsq);
    bb_set_bit(&occup, tosq);

    // direct attack by pieces to the king
    switch (mvpc) {
    case QUEEN:
        if ((diagonal_moves_bb(tosq) | rankfile_moves_bb(tosq)) & opp_king)
            if (!(from_to_path_bb(tosq, oksq) & occup))
                return TRUE;
        break;
    case ROOK:
        if (rankfile_moves_bb(tosq) & opp_king)
            if (!(from_to_path_bb(tosq, oksq) & occup))
                return TRUE;        
        break;
    case BISHOP:
        if (diagonal_moves_bb(tosq) & opp_king)
            if (!(from_to_path_bb(tosq, oksq) & occup))
                return TRUE;   
        break;
    case KNIGHT:
        if (knight_moves_bb(tosq) & opp_king)
            return TRUE;
        break;
    case PAWN:
        if (pawn_attack_bb(my_color, oksq) & square_bb(tosq))
            return TRUE;
        break;
    }
    // castle, promotions
    switch (unpack_type(move)) {
    case MT_CSBQS: 
        if ((rankfile_moves_bb(D8) & opp_king) && !(from_to_path_bb(D8, oksq) & occup))
            return TRUE;
        else
            return FALSE;
    case MT_CSBKS: 
        if ((rankfile_moves_bb(F8) & opp_king) && !(from_to_path_bb(F8, oksq) & occup))
            return TRUE;
        else
            return FALSE;
    case MT_CSWQS: 
        if ((rankfile_moves_bb(D1) & opp_king) && !(from_to_path_bb(D1, oksq) & occup))
            return TRUE;
        else
            return FALSE;
    case MT_CSWKS:
        if ((rankfile_moves_bb(F1) & opp_king) && !(from_to_path_bb(F1, oksq) & occup))
            return TRUE;
        else
            return FALSE;
    case MT_PROMO:
    case MT_CPPRM:
        switch (unpack_prom_piece(move)) {
        case QUEEN:
            if ((diagonal_moves_bb(tosq) | rankfile_moves_bb(tosq)) & opp_king)
                if (!(from_to_path_bb(tosq, oksq) & occup))
                    return TRUE;
            break;
        case ROOK:
            if (rankfile_moves_bb(tosq) & opp_king)
                if (!(from_to_path_bb(tosq, oksq) & occup))
                    return TRUE;        
            break;
        case BISHOP:
            if (diagonal_moves_bb(tosq) & opp_king)
                if (!(from_to_path_bb(tosq, oksq) & occup))
                    return TRUE;    
            break;
        case KNIGHT:
            if (knight_moves_bb(tosq) & opp_king)
                return TRUE;
            break;
        }
        break;
    case MT_EPCAP:
        epsq = unpack_ep_pawn_square(move);
        bb_clear_bit(&occup, epsq);
        piece = rankfile_moves_bb(epsq) & queen_rook_bb(board, my_color);
        while (piece) {
            pcsq = bb_first_index(piece);
            if ((rankfile_moves_bb(pcsq) & opp_king) && !(from_to_path_bb(pcsq, oksq) & occup))
                return TRUE;
            bb_clear_bit(&piece, pcsq);
        }
        piece = diagonal_moves_bb(epsq) & queen_bishop_bb(board, my_color);
        while (piece) {
            pcsq = bb_first_index(piece);
            if ((diagonal_moves_bb(pcsq) & opp_king) && !(from_to_path_bb(pcsq, oksq) & occup))
                return TRUE;
            bb_clear_bit(&piece, pcsq);
        }
        break;
    }

    // discovered checks after moving piece
    if (rankfile_moves_bb(frsq) & opp_king) {
        piece = rankfile_moves_bb(frsq) & queen_rook_bb(board, my_color);
        while (piece) {
            pcsq = bb_first_index(piece);
            if ((rankfile_moves_bb(pcsq) & opp_king) && !(from_to_path_bb(pcsq, oksq) & occup))
                return TRUE;
            bb_clear_bit(&piece, pcsq);
        }
    }
    if (diagonal_moves_bb(frsq) & opp_king) {
        piece = diagonal_moves_bb(frsq) & queen_bishop_bb(board, my_color);
        while (piece) {
            pcsq = bb_first_index(piece);
            if ((diagonal_moves_bb(pcsq) & opp_king) && !(from_to_path_bb(pcsq, oksq) & occup))
                return TRUE;
            bb_clear_bit(&piece, pcsq);
        }
    }
    return FALSE;
}

//END
