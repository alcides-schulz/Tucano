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

//------------------------------------------------------------------------------------
//    Pawn structure evaluation.
//------------------------------------------------------------------------------------

int is_candidate(BOARD *board, int myc, int pcsq);
int space_bonus(BOARD *board, int myc);

int USE_PAWN_TABLE = TRUE;

//------------------------------------------------------------------------------------
//    Evaluate pawns and indicate what pawns are passed to be evaluated later.
//------------------------------------------------------------------------------------
void eval_pawns(BOARD *board, PAWN_TABLE *pawn_table, EVALUATION *eval_values)
{
    // Probe pawn evaluation table.
    PAWN_TABLE *ppt = pawn_table + (board_pawn_key(board) % PAWN_TABLE_SIZE);
    if (USE_PAWN_TABLE && board_pawn_key(board) != 0 && ppt->key == board_pawn_key(board) && !EVAL_PRINTING) {
        eval_values->pawn[WHITE] = ppt->pawn_eval[WHITE];
        eval_values->pawn[BLACK] = ppt->pawn_eval[BLACK];
        eval_values->bb_passers[WHITE] = ppt->bb_passers[WHITE];
        eval_values->bb_passers[BLACK] = ppt->bb_passers[BLACK];
        return;
    }

    // Evaluate each pawn.
    for (int myc = WHITE; myc <= BLACK; myc++) {
        int opp = flip_color(myc);
        U64 pawns = pawn_bb(board, myc);
        while (pawns) {
            int pcsq = bb_first_index(pawns);
            int rank = get_rank(pcsq);
            int relative_rank = get_relative_rank(myc, rank);

            assert(piece_on_square(board, myc, pcsq) == PAWN);

            eval_values->pawn[myc] += eval_pst_pawn(myc, pcsq);

            int doubled = (forward_path_bb(myc, pcsq) & pawn_bb(board, myc)) ? TRUE : FALSE;
            int connected = (connected_mask_bb(pcsq) & pawn_bb(board, myc)) ? TRUE : FALSE;
            int passed = (!doubled && !(passed_mask_bb(myc, pcsq) & pawn_bb(board, opp))) ? TRUE : FALSE;
            int weak = (!(weak_mask_bb(myc, pcsq) & pawn_bb(board, myc))) ? TRUE : FALSE;
            int isolated = (weak && !(isolated_mask_bb(pcsq) & pawn_bb(board, myc))) ? TRUE : FALSE;
            int opposing = (forward_path_bb(myc, pcsq) & pawn_bb(board, opp)) ? TRUE : FALSE;
            int candidate = (!passed && !isolated && !weak && !opposing && is_candidate(board, myc, pcsq)) ? TRUE : FALSE;

            eval_values->pawn[myc] -= doubled ? P_DOUBLED : 0;
            eval_values->pawn[myc] += connected ? B_CONNECTED : 0;
            eval_values->pawn[myc] -= isolated ? (opposing ? P_ISOLATED : P_ISOLATED_OPEN) : 0;
            eval_values->pawn[myc] += candidate ? B_CANDIDATE * relative_rank : 0;
            eval_values->pawn[myc] += space_bonus(board, myc);
            eval_values->pawn[myc] -= weak ? P_WEAK : 0;

            if (passed) {
                bb_set_bit(&eval_values->bb_passers[myc], pcsq);
                eval_values->pawn[myc] += connected ? B_CONNECTED : 0;
            }
            
            assert(pawn_is_doubled(board, pcsq, myc) == doubled);
            assert(pawn_is_connected(board, pcsq, myc) == connected);
            assert(pawn_is_passed(board, pcsq, myc) == passed);
            assert(pawn_is_weak(board, pcsq, myc) == weak);
            assert(pawn_is_isolated(board, pcsq, myc) == isolated);
            assert(pawn_is_candidate(board, pcsq, myc) == candidate);
            
            bb_clear_bit(&pawns, pcsq);
        }
    }

    // Save information to pawn table.
    if (USE_PAWN_TABLE) {
        ppt->key = board_pawn_key(board);
        ppt->pawn_eval[WHITE] = eval_values->pawn[WHITE];
        ppt->pawn_eval[BLACK] = eval_values->pawn[BLACK];
        ppt->bb_passers[WHITE] = eval_values->bb_passers[WHITE];
        ppt->bb_passers[BLACK] = eval_values->bb_passers[BLACK];
    }
}

//------------------------------------------------------------------------------------
//    Controled squares that are behind pawn chain.
//------------------------------------------------------------------------------------
int space_bonus(BOARD *board, int myc) 
{
    U64 space = pawn_bb(board, myc);

    if (myc == WHITE) {
        space |= space >> 8;
        space |= space >> 8;
        space |= space >> 8;
        space &= (U64)0x000000FFFFFF0000; // ranks 3, 4, 5
    }
    else {
        space |= space << 8;
        space |= space << 8;
        space |= space << 8;
        space &= (U64)0x0000FFFFFF000000; // ranks 6, 5, 4
    }

    space &= (all_pieces_bb(board, myc) | empty_bb(board)) & ~pawn_bb(board, myc);

    return bb_count_u64(space) * B_PAWN_SPACE;
}

//------------------------------------------------------------------------------------
// Candidate passer pawn: has more own pawns behind than opp pawns in files forward
//------------------------------------------------------------------------------------
int is_candidate(BOARD *board, int myc, int pcsq)
{
    int relative_rank = get_relative_rank(myc, get_rank(pcsq));

    if (relative_rank < 2 || relative_rank > 5) return FALSE;

    U64 bb_my_pawn = 0;
    U64 bb_op_pawn = 0;
    int opp = flip_color(myc);

    if (get_file(pcsq) > 0) {
        bb_my_pawn |= (square_bb(pcsq - 1) & pawn_bb(board, myc));
        bb_my_pawn |= (backward_path_bb(myc, pcsq - 1) & pawn_bb(board, myc));
        bb_op_pawn |= (forward_path_bb(myc, pcsq - 1) & pawn_bb(board, opp));
    }
    if (get_file(pcsq) < 7) {
        bb_my_pawn |= (square_bb(pcsq + 1) & pawn_bb(board, myc));
        bb_my_pawn |= (backward_path_bb(myc, pcsq + 1) & pawn_bb(board, myc));
        bb_op_pawn |= (forward_path_bb(myc, pcsq + 1) & pawn_bb(board, opp));
    }

    return (bb_count_u64(bb_op_pawn) <= bb_count_u64(bb_my_pawn)) ? TRUE : FALSE;
}

// END
