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
//  Material evaluation
//-------------------------------------------------------------------------------------------------

#define NO_DRAW_ADJUST 64

#define WQ board->state[WHITE].count[QUEEN]
#define WR board->state[WHITE].count[ROOK]
#define WB board->state[WHITE].count[BISHOP]
#define WN board->state[WHITE].count[KNIGHT]
#define WP board->state[WHITE].count[PAWN]
#define BQ board->state[BLACK].count[QUEEN]
#define BR board->state[BLACK].count[ROOK]
#define BB board->state[BLACK].count[BISHOP]
#define BN board->state[BLACK].count[KNIGHT]
#define BP board->state[BLACK].count[PAWN]

#define WMAJ (WQ + WR)
#define BMAJ (BQ + BR)
#define WMIN (WB + WN)
#define BMIN (BB + BN)

// Material recognizers (white side vs black side)
// Minor pieces and no pawn and no majors
#define MINORSvsMINOR                       (WN == 1 && WB == 1 && BMIN == 1)
#define MINORvsMINORS                       (BN == 1 && BB == 1 && WMIN == 1)
#define BISHOPSvsBISHOP                     (WB == 2 && WN == 0 && BB == 1 && BN == 0)
#define BISHOPvsBISHOPS                     (BB == 2 && BN == 0 && WB == 1 && WN == 0)
#define ROOKvsMINORS                        (WR == 1 && WQ == 0 && BMIN == 2 && BMAJ == 0)
#define MINORSvsROOK                        (BR == 1 && BQ == 0 && WMIN == 2 && WMAJ == 0)
#define ROOK_MINORvsROOK                    (WR == 1 && WQ == 0 && WMIN == 1 && BR == 1 && BQ == 0 && BMIN == 0)
#define ROOKvsROOK_MINOR                    (BR == 1 && BQ == 0 && BMIN == 1 && WR == 1 && WQ == 0 && WMIN == 0)
#define ROOKvsMINOR                         (WR == 1 && WQ == 0 && WMIN == 0 && BMIN == 1 && BMAJ == 0)
#define ROOKvsROOK                          (WR == 1 && WQ == 0 && WMIN == 0 && BR == 1 && BQ == 0 && BMIN == 0)
#define MINORvsROOK                         (BR == 1 && BQ == 0 && BMIN == 0 && WMIN == 1 && WMAJ == 0)

//  END GAMES WITH MAXIMUM OF 2 PAWNS EACH SIDE.
#define KNIGHTS_PAWNvsMINOR_PAWN            (WN == 2 && WB == 0 && WMAJ == 0 && WP == 1 && BMIN == 1 && BMAJ == 0 && BP == 1)
#define MINOR_PAWNvsKNIGHTS_PAWN            (BN == 2 && BB == 0 && BMAJ == 0 && BP == 1 && WMIN == 1 && WMAJ == 0 && WP == 1)
#define ROOKvsMINOR_OR_MINOR_PAWN           (WP == 0 && WMIN == 0 && WR == 1 && WQ == 0 && BMAJ == 0 && BMIN == 1 && BP < 2)
#define MINOR_OR_MINOR_PAWNvsROOK           (BP == 0 && BMIN == 0 && BR == 1 && BQ == 0 && WMAJ == 0 && WMIN == 1 && WP < 2)
#define ROOK_MINORvsROOK_OR_ROOK_PAWN       (WP == 0 && WR == 1 && WQ == 0 && WMIN == 1 && BR == 1 && BQ == 0 && BMIN == 0)
#define ROOK_OR_ROOK_PAWNvsROOK_MINOR       (BP == 0 && BR == 1 && BQ == 0 && BMIN == 1 && WR == 1 && WQ == 0 && WMIN == 0)
#define QUEEN_MINORvsQUEEN_OR_QUEEN_PAWN    (WP == 0 && WR == 0 && WQ == 1 && WMIN == 1 && BR == 0 && BQ == 1 && BMIN == 0)
#define QUEEN_OR_QUEEN_PAWNvsQUEEN_MINOR    (BP == 0 && BR == 0 && BQ == 1 && BMIN == 1 && WR == 0 && WQ == 1 && WMIN == 0)
#define BISHOP_PAWNvsNONE                   (WP == 1 && WB == 1 && WMIN == 1 && WMAJ == 0 && BMIN == 0 && BMAJ == 0 && BP == 0)
#define NONEvsBISHOP_PAWN                   (BP == 1 && BB == 1 && BMIN == 1 && BMAJ == 0 && WMIN == 0 && WMAJ == 0 && WP == 0)
#define MINORvsPAWN                         (WMAJ == 0 && WMIN == 1 && WP == 0 && BMAJ == 0 && BMIN == 0 && BP == 1)
#define PAWNvsMINOR                         (BMAJ == 0 && BMIN == 1 && BP == 0 && WMAJ == 0 && WMIN == 0 && WP == 1)
#define BISHOP_PAWNSvsBISHOP_PAWNS          (WMAJ == 0 && BMAJ == 0 && WN == 0 && BN == 0 && WB != 0 && BB != 0 && WB == BB && ABS(WP - BP) <= 2)

// Material Adjustment
int     eval_kbp_k(BOARD *board, EVALUATION *eval_values);
int     eval_draw_adjust(BOARD *board, EVALUATION *eval_values);


//-------------------------------------------------------------------------------------------------
//  Calculate material score and draw adjustment.
//-------------------------------------------------------------------------------------------------
void eval_material(BOARD *board, EVALUATION *eval_values)
{
    // TODO: review to use material scale instead of a flag
    //  Indicates that king safety should be evaluated:  Opponent has queen and at least one piece.
    if (WQ >= 1 && (WR + WB + WN) >= 1) {
        eval_values->flag_king_safety[BLACK] = TRUE;
    }
    if (BQ >= 1 && (BR + BB + BN) >= 1) {
        eval_values->flag_king_safety[WHITE] = TRUE;
    }

    eval_values->material[WHITE] = WQ * SCORE_QUEEN
                                 + WR * SCORE_ROOK 
                                 + WB * SCORE_BISHOP 
                                 + WN * SCORE_KNIGHT 
                                 + WP * SCORE_PAWN 
                                 + (WB >= 2 ? B_BISHOP_PAIR : 0);
    eval_values->material[BLACK] = BQ * SCORE_QUEEN
                                 + BR * SCORE_ROOK
                                 + BB * SCORE_BISHOP
                                 + BN * SCORE_KNIGHT
                                 + BP * SCORE_PAWN 
                                 + (BB >= 2 ? B_BISHOP_PAIR : 0);

    //  Calculate game phase based on piece values.
    //  Opening = 0, Endgame = 48
    //  Base value is 48: 2Q * 8 + 4R * 4 + 4B * 2 + 4N * 2
    eval_values->phase = 48;
    eval_values->phase -= (WQ + BQ) * 8;
    eval_values->phase -= (WR + BR) * 4;
    eval_values->phase -= (WB + WN + BB + BN) * 2;

    eval_values->draw_adjust = eval_draw_adjust(board, eval_values);

    // Calculate non-mating material flags. Used to adjust score.
    int wm = WN * 2 + WB * 3 + WR * 5 + WQ * 9;
    int bm = BN * 2 + BB * 3 + BR * 5 + BQ * 9;

    if (WP == 0 && (wm <= 4 || (wm <= 8 && wm <= bm + 3))) eval_values->non_mating_material[WHITE] = TRUE;
    if (BP == 0 && (bm <= 4 || (bm <= 8 && bm <= wm + 3))) eval_values->non_mating_material[BLACK] = TRUE;
}

//-------------------------------------------------------------------------------------------------
//  Calculate draw adjustment factor to reduce the material advantage in some cases.
//  Returns between 0 (100% Draw) to 64 (no draw adjustment)
//-------------------------------------------------------------------------------------------------
int eval_draw_adjust(BOARD *board, EVALUATION *eval_values)
{
    // bishop vs pawns, usually draw if there's up 2 pawns difference.
    if (BISHOP_PAWNSvsBISHOP_PAWNS) {
        if (WB == 1 && BB == 1) {
            if (square_color_bb(bb_first_index(bishop_bb(board, WHITE))) != square_color_bb(bb_first_index(bishop_bb(board, BLACK)))) {
                return 8;
            }
            return 16;
        }
        return 32;
    }

    //  Don't calculate draw adjustment when there are many pawns
    if (WP > 2 || BP > 2) {
        return NO_DRAW_ADJUST;
    }

    //  Bare kings draw.
    if (eval_values->material[WHITE] == 0 && eval_values->material[BLACK] == 0) {
        return 0;
    }
    
    //  Draw ajustment for no pawns.
    if (WP == 0 && BP == 0) {
        //  Bishop/knight and no queen/rook.
        if (WMAJ == 0 && BMAJ == 0) {
            if (MINORSvsMINOR || MINORvsMINORS) return 8;
            if (BISHOPSvsBISHOP || BISHOPvsBISHOPS) return 8;
            return NO_DRAW_ADJUST;
        }
        //  Minors vs Majors and no pawns
        if (ROOKvsMINORS || MINORSvsROOK) return 8;
        if (ROOK_MINORvsROOK || ROOKvsROOK_MINOR) return 8;
        if (ROOKvsMINOR || MINORvsROOK) return 8;
        if (ROOKvsROOK) return 0;
        return NO_DRAW_ADJUST;
    }

    //  End games with maximum of 2 pawns 
    if (KNIGHTS_PAWNvsMINOR_PAWN || MINOR_PAWNvsKNIGHTS_PAWN) return 4;
    if (ROOKvsMINOR_OR_MINOR_PAWN || MINOR_OR_MINOR_PAWNvsROOK) return 32;
    if (ROOK_MINORvsROOK_OR_ROOK_PAWN || ROOK_OR_ROOK_PAWNvsROOK_MINOR) return 32;
    if (QUEEN_MINORvsQUEEN_OR_QUEEN_PAWN || QUEEN_OR_QUEEN_PAWNvsQUEEN_MINOR) return 32;
    if (MINORvsPAWN || PAWNvsMINOR) return 0;
    if (BISHOP_PAWNvsNONE || NONEvsBISHOP_PAWN) {
        return eval_kbp_k(board, eval_values);
    }

    return NO_DRAW_ADJUST;
}

//-------------------------------------------------------------------------------------------------
//  King+Bishop+Pawn vs King
//  It is a drawn when pawn is on A or H file, promotion square color is not the same as bishop
//  and king can block pawn.
//-------------------------------------------------------------------------------------------------
int eval_kbp_k(BOARD *board, EVALUATION *eval_values)
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
    kbp_bishop = bb_first_index(bishop_bb(board, kbp_color));
    kbp_pawn = bb_first_index(pawn_bb(board, kbp_color));
    k_king = king_square(board, k_color);

    if (get_file(kbp_pawn) != FILEA && get_file(kbp_pawn) != FILEH) {
        return NO_DRAW_ADJUST;
    }

    if (kbp_color == WHITE)
        prom_square = get_file(kbp_pawn);
    else
        prom_square = 56 + get_file(kbp_pawn);

    prom_distance = square_distance(kbp_pawn, prom_square);
    if (side_on_move(board) == kbp_color) prom_distance--;
    if (square_distance(k_king, prom_square) <= prom_distance) {
        if (square_distance(k_king, prom_square) < square_distance(kbp_king, prom_square)) {
            if (square_color_bb(prom_square) != square_color_bb(kbp_bishop)) {
                eval_values->draw_adjust = 0;
            }
        }
    }

    return NO_DRAW_ADJUST;
}

// END
