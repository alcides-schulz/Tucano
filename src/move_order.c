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
//  Move ordering: history heuristic, killers
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Update history table and killer move list for quiet moves.
//-------------------------------------------------------------------------------------------------
void save_beta_cutoff_data(MOVE_ORDER *move_order, int color, int ply, MOVE best_move, MOVE_LIST *ml, MOVE previous_move)
{
    // Update good cutoff history for best move found
    CUTOFF_HISTORY *slot = &move_order->cutoff_history[color][unpack_piece(best_move)][unpack_to(best_move)];
    if (slot->search_count == UINT16_MAX) {
        slot->search_count >>= 4;
        slot->cutoff_count >>= 4;
    }
    slot->search_count += 1;
    slot->cutoff_count += 1;
    // Update bad cutoff_history for all other quiet moves. Searched but didn't cause a cutoff_history
    MOVE bad_move = prev_move(ml); // discard last move which is the best move
    while ((bad_move = prev_move(ml)) != MOVE_NONE) {
        slot = &move_order->cutoff_history[color][unpack_piece(bad_move)][unpack_to(bad_move)];
        if (slot->search_count == UINT16_MAX) {
            slot->search_count >>= 4;
            slot->cutoff_count >>= 4;
        }
        slot->search_count += 1;
    }
    // update killers
    if (move_order->killers[ply][color][0] != best_move) {
        move_order->killers[ply][color][1] = move_order->killers[ply][color][0];
        move_order->killers[ply][color][0] = best_move;
    }
    // Save counter move data
    int prev_color = flip_color(color);
    int prev_piece = unpack_piece(previous_move);
    int prev_tosq = unpack_to(previous_move);
    if (move_order->counter_move[prev_color][prev_piece][prev_tosq][0] != best_move) {
        move_order->counter_move[prev_color][prev_piece][prev_tosq][1] = move_order->counter_move[prev_color][prev_piece][prev_tosq][0];
        move_order->counter_move[prev_color][prev_piece][prev_tosq][0] = best_move;
    }
}

//-------------------------------------------------------------------------------------------------
//  Indicate if move is in the "killer moves" list.
//-------------------------------------------------------------------------------------------------
int is_killer_move(MOVE_ORDER *move_order, int color, int ply, MOVE move)
{
    return (move == move_order->killers[ply][color][0] || move == move_order->killers[ply][color][1]);
}

//-------------------------------------------------------------------------------------------------
//  Indicate if move is in the "counter move" list.
//-------------------------------------------------------------------------------------------------
int is_counter_move(MOVE_ORDER *move_order, int prev_color, MOVE previous_move, MOVE current_move)
{
    int prev_piece = unpack_piece(previous_move);
    int prev_tosq = unpack_to(previous_move);
    if (move_order->counter_move[prev_color][prev_piece][prev_tosq][0] == current_move) return TRUE;
    if (move_order->counter_move[prev_color][prev_piece][prev_tosq][1] == current_move) return TRUE;
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Pruning margin value based on cutoff percentage
//-------------------------------------------------------------------------------------------------
int get_pruning_margin(MOVE_ORDER *move_order, int color, MOVE move)
{
    CUTOFF_HISTORY *slot = &move_order->cutoff_history[color][unpack_piece(move)][unpack_to(move)];
    if (slot->search_count == 0) {
        return 100; // if move was not searched yet, we assume a margin to avoid an early pruning.
    }
    return slot->cutoff_count * 100 / slot->search_count;
}

//-------------------------------------------------------------------------------------------------
//  History value to be used at move ordering.
//-------------------------------------------------------------------------------------------------
int get_beta_cutoff_percent(MOVE_ORDER *move_order, int color, MOVE move)
{
    CUTOFF_HISTORY *slot = &move_order->cutoff_history[color][unpack_piece(move)][unpack_to(move)];
    if (slot->search_count == 0) {
        return 0;
    }
    return slot->cutoff_count * 100 / slot->search_count;
}

//-------------------------------------------------------------------------------------------------
//  Indicate if move had bad cutoff percentage.
//-------------------------------------------------------------------------------------------------
int get_has_bad_history(MOVE_ORDER *move_order, int color, MOVE move)
{
    CUTOFF_HISTORY *slot = &move_order->cutoff_history[color][unpack_piece(move)][unpack_to(move)];
    if (slot->search_count == 0) {
        return FALSE;
    }
    return slot->cutoff_count * 100 / slot->search_count < 60 ? TRUE : FALSE;
}

// end
