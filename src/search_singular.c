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
//  Singular move search. Confirm if a move with good score from transposition table is much
//  better than the rest.
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Singular move search
//-------------------------------------------------------------------------------------------------
int search_singular(GAME *game, UINT incheck, int beta, int depth, MOVE exclude_move)
{
    MOVE_LIST   ml;
    int     best_score = -MAX_SCORE;
    int     eval_score = -MAX_SCORE;
    int     move_count = 0;
    int     score = 0;
    UINT    gives_check;
    int     ply = get_ply(&game->board);
    int     turn = side_on_move(&game->board);
    MOVE    move;
    int     alpha;

    assert(incheck == 0 || incheck == 1);
    assert(beta >= -MAX_SCORE && beta <= MAX_SCORE);
    assert(depth <= MAX_DEPTH);

    check_time(game);
    if (game->search.abort) return 0;

    if (depth <= 0) return FALSE; // should not get here.

    game->search.nodes++;
    game->pv_line.pv_size[ply] = ply;

    if (ply > 0 && is_draw(&game->board)) return 0;

    assert(ply >= 0 && ply <= MAX_PLY);
    if (ply >= MAX_PLY) return tnn_eval_incremental(&game->board);

    //  Mate pruning.
    alpha = beta - 1;
    alpha = MAX(-MATE_VALUE + ply, alpha);
    beta = MIN(MATE_VALUE - ply, beta);
    if (alpha >= beta) return alpha;

    select_init(&ml, game, incheck, MOVE_NONE, FALSE);
    while ((move = next_move(&ml)) != MOVE_NONE) {

        assert(is_valid(&game->board, move));

        if (!is_pseudo_legal(&game->board, ml.pins, move)) continue;

        move_count++;

        if (move == exclude_move)  continue;

        int reductions = 0;
        int extensions = 0;

        gives_check = is_check(&game->board, move);

        // extension if move puts opponent in check
        if (gives_check && (depth < 4 || see_move(&game->board, move) >= 0)) {
            extensions = 1;
        }

        // pruning or depth reductions
        if (!extensions && move_count > 1) {

            // Quiet moves pruning/reductions
            if (move_is_quiet(move) && !is_free_pawn(&game->board, turn, move) && !is_killer(&game->move_order, turn, ply, move)) {

                if (!is_counter_move(&game->move_order, flip_color(turn), get_last_move_made(&game->board), move)) {

                    if (eval_score == -MAX_SCORE) eval_score = tnn_eval_incremental(&game->board);

                    // Futility pruning: eval + margin below beta. Uses beta cutoff history.
                    if (!incheck && depth < 10) {
                        int pruning_margin = depth * (50 + get_pruning_margin(&game->move_order, turn, move));
                        if (eval_score + pruning_margin < beta) {
                            continue;
                        }
                    }

                    // Late move reductions: reduce depth for later moves
                    if (move_count > 3 && depth > 2) {
                        reductions = reduction_table[MIN(depth, MAX_DEPTH - 1)][MIN(move_count, MAX_MOVE - 1)];
                        if (reductions > 0 && !get_has_bad_history(&game->move_order, turn, move)) reductions--;
                    }
                }
            }
        }

        // Make move and search new position.
        make_move(&game->board, move);

        assert(valid_is_legal(&game->board, move));

        score = -search_zw(game, gives_check, 1 - beta, depth - 1 + extensions - reductions);
        if (!game->search.abort && score >= beta && reductions > 0) {
            score = -search_zw(game, gives_check, 1 - beta, depth - 1);
        }

        undo_move(&game->board);
        if (game->search.abort) return 0;

        // score verification
        if (score > best_score) {
            if (score >= beta) {
                return score;
            }
            best_score = score;
        }
    }

    //  Draw or checkmate.
    if (best_score == -MAX_SCORE) {
        return beta - 1;
    }

    return best_score;
}

// end
