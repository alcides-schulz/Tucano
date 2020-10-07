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
//  Generate, sort and select the next move.
//-------------------------------------------------------------------------------------------------

enum    {TRANS, 
         GEN_CAP, 
         NEXT_CAP, 
         GEN_QUIET, 
         NEXT_QUIET, 
         NEXT_LATE_MOVE, 
         GEN_EVASION, 
         NEXT_EVASION};

#define    SORT_CAPTURE   100000000
#define    SORT_KILLER     10000000
#define    SORT_COUNTER     1000000

void    select_next(MOVE_LIST *ml);
int     is_badcap(BOARD *board, MOVE move);
void    assign_tactical_score(MOVE_LIST *ml);
void    assign_quiet_score(MOVE_LIST *ml);

static const int PIECE_VALUE[NUM_PIECES] = {100, 300, 300, 500, 900, 10000};
static const int VICTIM_VALUE[NUM_PIECES]   = {6, 12, 13, 18, 24, 0};
static const int ATTACKER_VALUE[NUM_PIECES] = {4,  3,  3,  2,  1, 9};

//-------------------------------------------------------------------------------------------------
//  Preparation for move generation and selection.
//-------------------------------------------------------------------------------------------------
void select_init(MOVE_LIST *ml, GAME *game, int incheck, MOVE ttm, int caps)
{
    ml->next = 0;
    ml->count = 0;
    ml->incheck = incheck;
    ml->caps = caps;
    ml->late_moves_count = 0;
    ml->late_moves_next = 0;
    ml->ttm = ttm;
    ml->phase = TRANS;
    ml->sort = TRUE;
    ml->board = &game->board;
    ml->move_order = &game->move_order;
}

//-------------------------------------------------------------------------------------------------
//  Add a move to the list
//-------------------------------------------------------------------------------------------------
void add_move(MOVE_LIST *ml, MOVE move)
{
    assert(ml != NULL);
    assert(ml->count < MAX_MOVE);

    ml->moves[ml->count++] = move;
}

//-------------------------------------------------------------------------------------------------
//  Add all promotion moves.
//-------------------------------------------------------------------------------------------------
void add_all_promotions(MOVE_LIST *ml, int from_square, int to_square)
{
    add_move(ml, pack_promotion(from_square, to_square, QUEEN));
    add_move(ml, pack_promotion(from_square, to_square, ROOK));
    add_move(ml, pack_promotion(from_square, to_square, BISHOP));
    add_move(ml, pack_promotion(from_square, to_square, KNIGHT));
}

//-------------------------------------------------------------------------------------------------
//  Add all capture/promotion moves.
//-------------------------------------------------------------------------------------------------
void add_all_capture_promotions(MOVE_LIST *ml, int from_square, int to_square, int captured_piece)
{
    add_move(ml, pack_capture_promotion(captured_piece, from_square, to_square, QUEEN));
    add_move(ml, pack_capture_promotion(captured_piece, from_square, to_square, ROOK));
    add_move(ml, pack_capture_promotion(captured_piece, from_square, to_square, BISHOP));
    add_move(ml, pack_capture_promotion(captured_piece, from_square, to_square, KNIGHT));
}

int is_late_moves(MOVE_LIST *ml)
{
    if (ml->phase >= GEN_QUIET)
        return TRUE;
    else
        return FALSE;
}

int is_bad_capture(MOVE_LIST *ml)
{
    if (ml->phase >= NEXT_LATE_MOVE)
        return TRUE;
    else
        return FALSE;
}

int skip_trans_move(MOVE_LIST *ml)
{
    if (ml->moves[ml->next] == ml->ttm) {
        ml->next++;
        return TRUE;
    }
    return FALSE;
}

int skip_bad_capture(MOVE_LIST *ml)
{
    if (!ml->incheck && !ml->caps && is_badcap(ml->board, ml->moves[ml->next])) {
        ml->late_moves[ml->late_moves_count++] = ml->moves[ml->next++];
        return TRUE;
    }
    return FALSE;
}

int skip_under_promotion(MOVE_LIST *ml) 
{
    if (!ml->incheck && !ml->caps && unpack_type(ml->moves[ml->next]) == MT_PROMO && unpack_prom_piece(ml->moves[ml->next]) != QUEEN) {
        ml->late_moves[ml->late_moves_count++] = ml->moves[ml->next++];
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Return next move.
//-------------------------------------------------------------------------------------------------
MOVE next_move(MOVE_LIST *ml)
{
    switch (ml->phase) {
    case TRANS:
        ml->pins = find_pins(ml->board);
        ml->phase = GEN_CAP;
        if (ml->ttm != MOVE_NONE)  {
            if (is_valid(ml->board, ml->ttm)) {
                // Have to make the move in order to test for check legality.
                // Some moves coming from tt are valid for the position but leave the king in check.
                make_move(ml->board, ml->ttm);
                if (is_illegal(ml->board, ml->ttm)) ml->ttm = MOVE_NONE;
                undo_move(ml->board);
                if (ml->ttm != MOVE_NONE) return ml->ttm;
            }
            ml->ttm = MOVE_NONE;
        }
        /* FALLTHROUGH */
    case GEN_CAP:
        if (ml->incheck) {
            ml->phase = GEN_EVASION;
            return next_move(ml);
        }
        ml->next = 0;
        ml->count = 0;
        gen_caps(ml->board, ml);
        assign_tactical_score(ml);
        ml->phase = NEXT_CAP;
        /* FALLTHROUGH */
    case NEXT_CAP:
        while (ml->next < ml->count) {
            select_next(ml);
            if (skip_trans_move(ml)) continue;
            if (skip_bad_capture(ml)) continue;
            if (skip_under_promotion(ml)) continue;
            return ml->moves[ml->next++];
        }
        ml->phase = GEN_QUIET;
        /* FALLTHROUGH */
    case GEN_QUIET:
        if (ml->caps) {
            ml->phase = NEXT_LATE_MOVE;
            return next_move(ml);
        }
        ml->next = 0;
        ml->count = 0;
        gen_moves(ml->board, ml);
        assign_quiet_score(ml);
        ml->phase = NEXT_QUIET;
    case NEXT_QUIET:
        while (ml->next < ml->count) {
            if (ml->sort) {
                select_next(ml);
                ml->sort = ml->score[ml->next];
            }
            if (skip_trans_move(ml)) continue;
            return ml->moves[ml->next++];
        }
        ml->phase = NEXT_LATE_MOVE;
        /* FALLTHROUGH */
    case NEXT_LATE_MOVE:
        if (ml->late_moves_next < ml->late_moves_count) {
            return ml->late_moves[ml->late_moves_next++];
        }
        return MOVE_NONE;
    case GEN_EVASION:
        ml->count = 0;
        ml->next = 0;
        gen_check_evasions(ml->board, ml);
        assign_tactical_score(ml);
        ml->phase = NEXT_EVASION;
    case NEXT_EVASION:
        while (ml->next < ml->count) {
            select_next(ml);
            if (skip_trans_move(ml)) continue;
            return ml->moves[ml->next++];
        }
        return MOVE_NONE;
    }

    // should not get here...
    assert(0);
    return MOVE_NONE;
}

//-------------------------------------------------------------------------------------------------
//  Return previous move, used by history update.
//-------------------------------------------------------------------------------------------------
MOVE prev_move(MOVE_LIST *ml)
{
    if (--ml->next >= 0)
        return ml->moves[ml->next];
    else
        return MOVE_NONE;
}

//-------------------------------------------------------------------------------------------------
//  Put the move with highest score in front of the list to be picked next.
//-------------------------------------------------------------------------------------------------
void select_next(MOVE_LIST *ml) 
{
    int     i;
    int     best_index = ml->next;
    MOVE    temp_move;
    int     temp_score;

    for (i = ml->next + 1; i < ml->count; i++)  {
        if (ml->score[i] > ml->score[best_index]) 
            best_index = i;
    }
    if (best_index != ml->next) {
        temp_move = ml->moves[ml->next];
        ml->moves[ml->next] = ml->moves[best_index];
        ml->moves[best_index] = temp_move;

        temp_score = ml->score[ml->next];
        ml->score[ml->next] = ml->score[best_index];
        ml->score[best_index] = temp_score;
    }
}

//-------------------------------------------------------------------------------------------------
//  Assign a score value to each tatical move to be used for sorting.
//-------------------------------------------------------------------------------------------------
void assign_tactical_score(MOVE_LIST *ml)
{
    for (int i = 0; i < ml->count; i++) {
        switch (unpack_type(ml->moves[i])) {
        case MT_CAPPC:
            ml->score[i] = SORT_CAPTURE + VICTIM_VALUE[unpack_capture(ml->moves[i])] + ATTACKER_VALUE[unpack_piece(ml->moves[i])];
            break;
        case MT_EPCAP:
            ml->score[i] = SORT_CAPTURE + VICTIM_VALUE[PAWN] + ATTACKER_VALUE[PAWN];
            break;
        case MT_PROMO:
            ml->score[i] = SORT_CAPTURE + VICTIM_VALUE[unpack_prom_piece(ml->moves[i])];
            break;
        case MT_CPPRM:
            ml->score[i] = SORT_CAPTURE + VICTIM_VALUE[unpack_prom_piece(ml->moves[i])] + VICTIM_VALUE[unpack_capture(ml->moves[i])];
            break;
        default:
            ml->score[i] = get_beta_cutoff_percent(ml->move_order, side_on_move(ml->board), ml->moves[i]);
            if (is_killer(ml->move_order, side_on_move(ml->board), get_ply(ml->board), ml->moves[i])) {
                ml->score[i] += SORT_KILLER;
            }
            if (is_counter_move(ml->move_order, flip_color(side_on_move(ml->board)), get_last_move_made(ml->board), ml->moves[i])) {
                ml->score[i] += SORT_COUNTER;
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------
//  Assign a score value to each move to be used for sorting.
//-------------------------------------------------------------------------------------------------
void assign_quiet_score(MOVE_LIST *ml)
{
    int     i;

    for (i = 0; i < ml->count; i++) {
        ml->score[i] = get_beta_cutoff_percent(ml->move_order, side_on_move(ml->board), ml->moves[i]);
        if (is_killer(ml->move_order, side_on_move(ml->board), get_ply(ml->board), ml->moves[i])) {
            ml->score[i] += SORT_CAPTURE;
        }
        if (is_counter_move(ml->move_order, flip_color(side_on_move(ml->board)), get_last_move_made(ml->board), ml->moves[i])) {
            ml->score[i] += SORT_KILLER;
        }
    }
}

//-------------------------------------------------------------------------------------------------
//  Bad capture moves according SEE score.
//-------------------------------------------------------------------------------------------------
int is_badcap(BOARD *board, MOVE move)
{
    if (unpack_type(move) != MT_CAPPC)
        return FALSE;
    if (PIECE_VALUE[unpack_capture(move)] >= PIECE_VALUE[unpack_piece(move)])
        return FALSE;
    if (see_move(board, move) >= 0)
        return FALSE;
    else
        return TRUE;
}

//-------------------------------------------------------------------------------------------------
//  Verify if move still valid for current position. Usually moves from transposition table.
//-------------------------------------------------------------------------------------------------
int is_valid(BOARD *board, MOVE move)
{
    static int promo_rank_to[2] = {RANK8, RANK1};
    static int is_slider[NUM_PIECES] = {0, 0, 1, 1, 1, 0};

    int moving_piece = unpack_piece(move);
    int from_square = unpack_from(move);
    int to_square = unpack_to(move);
    int turn = side_on_move(board);

    assert(from_square != to_square);

    //  moving piece is not on square
    if (moving_piece != piece_on_square(board, turn, from_square)) return FALSE;

    //  pawn should be moving forward
    if (moving_piece == PAWN) {
        if (turn == WHITE) {
            if (get_rank(from_square) < get_rank(to_square)) return FALSE;
        }
        else {
            if (get_rank(from_square) > get_rank(to_square)) return FALSE;
        }
    }

    //  Specific type validation
    switch (unpack_type(move))  {
    case MT_QUIET:
        if (bb_is_one(occupied_bb(board), to_square)) return FALSE;
        if (is_slider[piece_on_square(board, turn, from_square)]) {
            if (from_to_path_bb(from_square, to_square) & occupied_bb(board)) return FALSE;
        }
        break;
    case MT_CAPPC:
        if (piece_on_square(board, flip_color(turn), to_square) != unpack_capture(move)) {
            return FALSE;
        }
        if (is_slider[piece_on_square(board, turn, from_square)]) {
            if (from_to_path_bb(from_square, to_square) & occupied_bb(board)) return FALSE;
        }
        break;
    case MT_EPCAP:
        if (to_square != ep_square(board)) return FALSE;
        if (piece_on_square(board, flip_color(turn), unpack_ep_pawn_square(move)) != PAWN) return FALSE;
        break;
    case MT_PAWN2:
        if (bb_is_one(occupied_bb(board), to_square)) return FALSE;
        if (from_to_path_bb(from_square, to_square) & occupied_bb(board)) return FALSE;
        break;
    case MT_PROMO:
        if (bb_is_one(occupied_bb(board), to_square)) return FALSE;
        if (get_rank(to_square) != promo_rank_to[turn]) return FALSE;
        return TRUE;
    case MT_CPPRM:
        if (piece_on_square(board, flip_color(turn), to_square) != unpack_capture(move)) return FALSE;
        if (get_rank(to_square) != promo_rank_to[turn]) return FALSE;
        break;
    case MT_CSWKS:
        if (!can_generate_castle_ks(board, turn)) return FALSE;
        break;
    case MT_CSWQS:
        if (!can_generate_castle_qs(board, turn)) return FALSE;
        break;
    case MT_CSBKS:
        if (!can_generate_castle_ks(board, turn)) return FALSE;
        break;
    case MT_CSBQS:
        if (!can_generate_castle_qs(board, turn)) return FALSE;
        break;
    }

    return TRUE;
}

//END
