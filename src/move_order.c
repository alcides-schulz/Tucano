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
//  Move ordering: history heuristic, killers
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Update history table and killer move list for quiet moves.
//-------------------------------------------------------------------------------------------------
void move_order_save(MOVE_ORDER *move_order, int color, int ply, MOVE move, MOVE_LIST *ml)
{
    int     mvpc;
    int     tosq;

    mvpc = unpack_piece(move);
    tosq = unpack_to(move);

    move_order->hist_tot[color][mvpc][tosq] += 1;
    move_order->hist_hit[color][mvpc][tosq] += 1;

    if (move != move_order->killers[ply][color][0]) {
        move_order->killers[ply][color][1] = move_order->killers[ply][color][0];
        move_order->killers[ply][color][0] = move;
    }

    // update bad history for all previous move.
    move = prev_move(ml); // discard last move which is the best move
    while ((move = prev_move(ml)) != MOVE_NONE)
        move_order->hist_tot[color][unpack_piece(move)][unpack_to(move)] += 1;
}

//-------------------------------------------------------------------------------------------------
//  Indicate if move is in the "killer moves" list.
//-------------------------------------------------------------------------------------------------
int is_killer(MOVE_ORDER *move_order, int color, int ply, MOVE move)
{
    return (move == move_order->killers[ply][color][0] || move == move_order->killers[ply][color][1]);
}

//-------------------------------------------------------------------------------------------------
//  History value to be used at move ordering.
//-------------------------------------------------------------------------------------------------
int get_history_value(MOVE_ORDER *move_order, int color, MOVE move)
{
    int mvpc = unpack_piece(move);
    int tosq = unpack_to(move);

    if (move_order->hist_tot[color][mvpc][tosq] == 0)
        return 0;

    return move_order->hist_hit[color][mvpc][tosq] * 100 / move_order->hist_tot[color][mvpc][tosq];
}

//-------------------------------------------------------------------------------------------------
//  Indicate if move had history percentage.
//-------------------------------------------------------------------------------------------------
int has_bad_history(MOVE_ORDER *move_order, int color, MOVE move)
{
    int mvpc = unpack_piece(move);
    int tosq = unpack_to(move);

    if (move_order->hist_tot[color][mvpc][tosq] == 0)
        return FALSE;

    return move_order->hist_hit[color][mvpc][tosq] * 100 / move_order->hist_tot[color][mvpc][tosq] < 60 ? TRUE : FALSE;
}

// end
