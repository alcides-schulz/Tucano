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
//  Material evaluation
//-------------------------------------------------------------------------------------------------

// Material recognizers (white side vs black side)
// Minor pieces and no pawn and no majors
#define MINOR_OR_2KNIGHTS_vs_MINORS         ((wmin == 1 || (wn == 2 && wb == 0)) && bmin <= 2)
#define MINORS_vs_MINOR_OR_2KNIGHTS         ((bmin == 1 || (bn == 2 && bb == 0)) && wmin <= 2)
#define MINORSvsMINOR                       (wn == 1 && wb == 1 && bmin == 1)
#define MINORvsMINORS                       (bn == 1 && bb == 1 && wmin == 1)
#define BISHOPSvsBISHOP                     (wb == 2 && wn == 0 && bb == 1 && bn == 0)
#define BISHOPvsBISHOPS                     (bb == 2 && bn == 0 && wb == 1 && wn == 0)
#define ROOKvsMINORS                        (wr == 1 && wq == 0 && bmin == 2 && bmaj == 0)
#define MINORSvsROOK                        (br == 1 && bq == 0 && wmin == 2 && wmaj == 0)
#define ROOK_MINORvsROOK                    (wr == 1 && wq == 0 && wmin == 1 && br == 1 && bq == 0 && bmin == 0)
#define ROOKvsROOK_MINOR                    (br == 1 && bq == 0 && bmin == 1 && wr == 1 && wq == 0 && wmin == 0)
#define ROOKvsMINOR                         (wr == 1 && wq == 0 && wmin == 0 && bmin == 1 && bmaj == 0)
#define ROOKvsROOK                          (wr == 1 && wq == 0 && wmin == 0 && br == 1 && bq == 0 && bmin == 0)
#define MINORvsROOK                         (br == 1 && bq == 0 && bmin == 0 && wmin == 1 && wmaj == 0)

//  End games with maximum of 2 pawns each side.
#define KNIGHTS_PAWNvsMINOR_PAWN            (wn == 2 && wb == 0 && wmaj == 0 && wp == 1 && bmin == 1 && bmaj == 0 && bp == 1)
#define MINOR_PAWNvsKNIGHTS_PAWN            (bn == 2 && bb == 0 && bmaj == 0 && bp == 1 && wmin == 1 && wmaj == 0 && wp == 1)
#define ROOKvsMINOR_OR_MINOR_PAWN           (wp == 0 && wmin == 0 && wr == 1 && wq == 0 && bmaj == 0 && bmin == 1 && bp < 2)
#define MINOR_OR_MINOR_PAWNvsROOK           (bp == 0 && bmin == 0 && br == 1 && bq == 0 && wmaj == 0 && wmin == 1 && wp < 2)
#define ROOK_MINORvsROOK_OR_ROOK_PAWN       (wp == 0 && wr == 1 && wq == 0 && wmin == 1 && br == 1 && bq == 0 && bmin == 0)
#define ROOK_OR_ROOK_PAWNvsROOK_MINOR       (bp == 0 && br == 1 && bq == 0 && bmin == 1 && wr == 1 && wq == 0 && wmin == 0)
#define QUEEN_MINORvsQUEEN_OR_QUEEN_PAWN    (wp == 0 && wr == 0 && wq == 1 && wmin == 1 && br == 0 && bq == 1 && bmin == 0)
#define QUEEN_OR_QUEEN_PAWNvsQUEEN_MINOR    (bp == 0 && br == 0 && bq == 1 && bmin == 1 && wr == 0 && wq == 1 && wmin == 0)
#define BISHOP_PAWNvsNONE                   (wp == 1 && wb == 1 && wmin == 1 && wmaj == 0 && bmin == 0 && bmaj == 0 && bp == 0)
#define NONEvsBISHOP_PAWN                   (bp == 1 && bb == 1 && bmin == 1 && bmaj == 0 && wmin == 0 && wmaj == 0 && wp == 0)
#define MINORvsPAWN                         (wmaj == 0 && wmin == 1 && wp == 0 && bmaj == 0 && bmin == 0 && bp == 1)
#define PAWNvsMINOR                         (bmaj == 0 && bmin == 1 && bp == 0 && wmaj == 0 && wmin == 0 && wp == 1)
#define BISHOP_PAWNSvsBISHOP_PAWNS          (wmaj == 0 && bmaj == 0 && wn == 0 && bn == 0 && wb != 0 && bb != 0 && wb == bb && ABS(wp - bp) <= 2)

#define DRAW_ADJUST(value)                  {eval_values->draw_adjust = (value); return;}

// Material Adjustment
void    eval_kbp_k(BOARD *board, EVALUATION *eval_values);

//-------------------------------------------------------------------------------------------------
//  Calculate material score and draw adjustment.
//-------------------------------------------------------------------------------------------------
void eval_material(BOARD *board, EVALUATION *eval_values)
{
    int wq = queen_count(board, WHITE);
    int wr = rook_count(board, WHITE);
    int wb = bishop_count(board, WHITE);
    int wn = knight_count(board, WHITE);
    int wp = pawn_count(board, WHITE);

    int bq = queen_count(board, BLACK);
    int br = rook_count(board, BLACK);
    int bb = bishop_count(board, BLACK);
    int bn = knight_count(board, BLACK);
    int bp = pawn_count(board, BLACK);

    int wmaj = wq + wr;
    int bmaj = bq + br;
    int wmin = wb + wn;
    int bmin = bb + bn;

    //  Indicates that king safety should be evaluated:  Opponent has queen and at least one piece.
    if (wq >= 1 && (wr + wb + wn) >= 1)
        eval_values->flag_king_safety[BLACK] = TRUE;
    if (bq >= 1 && (br + bb + bn) >= 1)
        eval_values->flag_king_safety[WHITE] = TRUE;

    eval_values->material[WHITE] = wq * SCORE_QUEEN
                                 + wr * SCORE_ROOK
                                 + wb * SCORE_BISHOP
                                 + wn * SCORE_KNIGHT
                                 + wp * SCORE_PAWN
                                 + (wb >= 2 ? B_BISHOP_PAIR : 0);
    eval_values->material[BLACK] = bq * SCORE_QUEEN
                                 + br * SCORE_ROOK
                                 + bb * SCORE_BISHOP
                                 + bn * SCORE_KNIGHT
                                 + bp * SCORE_PAWN
                                 + (bb >= 2 ? B_BISHOP_PAIR : 0);

    //  Calculate the phase of the game based on material.
    //  Opening = 0, Endgame = 24
    //  Base value is 24: 2Q * 4 + 4R * 2 + 4B * 1 + 4N * 1
    eval_values->phase = 24 - ((wq + bq) * 4 + (wr + br) * 2 + (wb + bb + wn + bn) * 1);

    //  Draw adjustment factor to reduce the material advantage in some cases.
    eval_values->draw_adjust = 64;

    // bishop vs pawns, usually draw if there's up 2 pawns difference.
    if (BISHOP_PAWNSvsBISHOP_PAWNS) {
        if (wb == 1 && bb == 1) {
            if (square_color_bb(first_index(bishop_bb(board, WHITE))) != square_color_bb(first_index(bishop_bb(board, BLACK))))
                DRAW_ADJUST(8);
            DRAW_ADJUST(16);
        }
        DRAW_ADJUST(32);
    }

    //  Don't calculate draw adjustment when in middle game or enough pawns
    if (wp > 2 || bp > 2 || OPENING(eval_values->material[WHITE]) > 1400 || OPENING(eval_values->material[BLACK] > 1400))
        return;

    //  Bare kings draw.
    if (eval_values->material[WHITE] == 0 && eval_values->material[BLACK] == 0)
        DRAW_ADJUST(0);
    
    //  Draw ajustment for no pawns.
    if (wp == 0 && bp == 0) {
        //  Bishop/knight and no queen/rook.
        if (wmaj == 0 && bmaj == 0) {
            if (MINOR_OR_2KNIGHTS_vs_MINORS || MINORS_vs_MINOR_OR_2KNIGHTS)
                DRAW_ADJUST(0);
            if (MINORSvsMINOR || MINORvsMINORS)
                DRAW_ADJUST(8);
            if (BISHOPSvsBISHOP || BISHOPvsBISHOPS)
                DRAW_ADJUST(8);
            return;
        }
        //  Minors vs Majors and no pawns
        if (ROOKvsMINORS || MINORSvsROOK)
            DRAW_ADJUST(8);
        if (ROOK_MINORvsROOK || ROOKvsROOK_MINOR)
            DRAW_ADJUST(8);
        if (ROOKvsMINOR || MINORvsROOK)
            DRAW_ADJUST(8);
        if (ROOKvsROOK)
            DRAW_ADJUST(0);
        return;
    }

    //  End games with maximum of 2 pawns 
    if (KNIGHTS_PAWNvsMINOR_PAWN || MINOR_PAWNvsKNIGHTS_PAWN)
        DRAW_ADJUST(4);
    if (ROOKvsMINOR_OR_MINOR_PAWN || MINOR_OR_MINOR_PAWNvsROOK)
        DRAW_ADJUST(32);
    if (ROOK_MINORvsROOK_OR_ROOK_PAWN || ROOK_OR_ROOK_PAWNvsROOK_MINOR)
        DRAW_ADJUST(32);
    if (QUEEN_MINORvsQUEEN_OR_QUEEN_PAWN || QUEEN_OR_QUEEN_PAWNvsQUEEN_MINOR)
        DRAW_ADJUST(32);
    if (MINORvsPAWN || PAWNvsMINOR)
        DRAW_ADJUST(0);
    if (BISHOP_PAWNvsNONE || NONEvsBISHOP_PAWN) {
        eval_kbp_k(board, eval_values);
        return;
    }
}

//-------------------------------------------------------------------------------------------------
//  King+Bishop+Pawn vs King
//  It is a drawn when pawn is on A or H file, promotion square color is not the same as bishop
//  and king can block pawn.
//-------------------------------------------------------------------------------------------------
void eval_kbp_k(BOARD *board, EVALUATION *eval_values)
{
    int     kbp_color;
    int     k_color;
    int     kbp_king;
    int     kbp_bishop;
    int     kbp_pawn;
    int     k_king;
    int     prom_square;
    int     prom_distance;

    if (bishop_bb(board, WHITE))
        kbp_color = WHITE;
    else
        kbp_color = BLACK;
    k_color = flip_color(kbp_color);

    kbp_king = king_square(board, kbp_color);
    kbp_bishop = first_index(bishop_bb(board, kbp_color));
    kbp_pawn = first_index(pawn_bb(board, kbp_color));
    k_king = king_square(board, k_color);

    if (get_file(kbp_pawn) != FILEA && get_file(kbp_pawn) != FILEH)
        return;

    if (kbp_color == WHITE)
        prom_square = get_file(kbp_pawn);
    else
        prom_square = 56 + get_file(kbp_pawn);

    prom_distance = square_distance(kbp_pawn, prom_square);
    if (side_on_move(board) == kbp_color)
        prom_distance--;
    if (square_distance(k_king, prom_square) <= prom_distance)
        if (square_distance(k_king, prom_square) < square_distance(kbp_king, prom_square))
            if (square_color_bb(prom_square) != square_color_bb(kbp_bishop))
                eval_values->draw_adjust = 0;
}

// END
