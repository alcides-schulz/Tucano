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
//  Principal variation search.
//-------------------------------------------------------------------------------------------------
int search_pv(GAME *game, UINT incheck, int alpha, int beta, int depth)
{
    MOVE_LIST   ml;
    MOVE    best_move  = MOVE_NONE;
    MOVE    trans_move  = MOVE_NONE;
    int     best_score = -MAX_SCORE;
    int     score = 0;
    UINT    gives_check;
    int     move_count = 0;
    int     extensions;
    int     ply = get_ply(&game->board);
    int     turn = side_on_move(&game->board);
    MOVE    move;
    int     reductions;
    int     trans_score;
    int     reduced_beta;

    assert(incheck == 0 || incheck == 1);
    assert(alpha >= -MAX_SCORE && alpha <= MAX_SCORE);
    assert(beta >= -MAX_SCORE && beta <= MAX_SCORE);
    assert(beta > alpha);
    assert(depth <= MAX_DEPTH);

	if (depth <= 0) return quiesce(game, incheck, alpha, beta, 0);

	game->pv_line.pv_size[ply] = ply;
	game->search.nodes++;

    if (ply > 0 && is_draw(&game->board)) return 0;

    check_time(game);
        
    if (game->search.abort) return 0;

    assert(ply >= 0 && ply <= MAX_PLY);
    if (ply >= MAX_PLY) return evaluate(game, alpha, beta);

    //  Mate pruning.
    alpha = MAX(-MATE_VALUE + ply, alpha);
    beta = MIN(MATE_VALUE - ply, beta);
    if (alpha >= beta) return alpha;

    //  Get move hint from transposition table
    trans_move = tt_move(&game->board);

    // Internal Iterative Deepening.
    if (depth > 3 && trans_move == MOVE_NONE) {
        score = search_pv(game, incheck, alpha, beta, depth - 3);
        if (score <= alpha) score = search_pv(game, incheck, -MAX_SCORE, beta, depth - 3);
        if (game->search.abort) return 0;
        trans_move = tt_move(&game->board);
    }

    //  Loop through move list
    select_init(&ml, game, incheck, trans_move, FALSE);
    while ((move = next_move(&ml)) != MOVE_NONE) {

        assert(is_valid(&game->board, move));

        if (!is_pseudo_legal(&game->board, ml.pins, move)) continue;
        
        move_count++;

        reductions = 0;
        extensions = 0;

        gives_check = is_check(&game->board, move);
        
        // extension if move puts opponent in check
        if (gives_check && (depth < 4 || see_move(&game->board, move) >= 0)) {
            extensions = 1;
        }

        // singular move extension
        if (move == trans_move && depth >= 8 && !extensions) {
            if (tt_score(&game->board, depth - 3, &trans_score)) {
                if (!is_mate_score(trans_score)) {
                    reduced_beta = trans_score - 4 * depth;
                    score = search_singular(game, incheck, reduced_beta, depth / 2, move);
                    if (score < reduced_beta) {
                        extensions = 1;
                    }
                }
            }
        }

        // Pruning or depth reductions
        if (!incheck && !extensions && move_count > 1) {

            assert(move != trans_move);

            // Quiet moves pruning/reductions
            if (move_is_quiet(move) && !is_killer(&game->move_order, turn, ply, move))  {

                if (!is_counter_move(&game->move_order, flip_color(turn), get_last_move_made(&game->board), move)) {

                    // Futility pruning: eval + margin below beta.
                    if (depth < 10) {
                        int pruning_margin = depth * (50 + get_pruning_margin(&game->move_order, turn, move));
                        if (evaluate(game, alpha, beta) + pruning_margin < alpha) {
                            continue;
                        }
                    }

                    // Late move reductions: reduce depth for later moves
                    if (move_count > 3 && depth > 2) {
                        reductions = 1;
                        if (depth > 5 && get_has_bad_history(&game->move_order, turn, move)) {
                            reductions += depth / 6;
                        }
                        reductions = MIN(reductions, 5);
                    }
                }
            }
        }

        // Make move and search new position.
        make_move(&game->board, move);

        assert(valid_is_legal(&game->board, move));

        if (best_score == -MAX_SCORE) {
            score = -search_pv(game, gives_check, -beta, -alpha, depth - 1 + extensions - reductions);
        }
        else  {
            score = -search_zw(game, gives_check, -alpha, depth - 1 + extensions - reductions);
            if (!game->search.abort && score > alpha && reductions) {
                score = -search_zw(game, gives_check, -alpha, depth - 1 + extensions);
            }
            if (!game->search.abort && score > alpha) {
                score = -search_pv(game, gives_check, -beta, -alpha, depth - 1 + extensions);
            }
        }

        undo_move(&game->board);
        if (game->search.abort) return 0;

        //  Score verification.
        if (score > best_score) {
            if (score > alpha) {
                update_pv(&game->pv_line, ply, move);
                if (ply == 0) post_info(game, score, depth);
                alpha = score;
                best_move = move;
                if (score >= beta) {
                    if (move_is_quiet(move)) {
                        save_beta_cutoff_data(&game->move_order, turn, ply, move, &ml, get_last_move_made(&game->board));
                    }
                    tt_save(&game->board, depth, score, TT_LOWER, move);
                    return score;
                }
            }
            best_score = score;
        }
    }

    //  Draw or checkmate.
    if (best_score == -MAX_SCORE) {
        return (incheck ? -MATE_VALUE + ply : 0);
    }

    if (best_move != MOVE_NONE) 
        tt_save(&game->board, depth, best_score, TT_EXACT, best_move);
    else
        tt_save(&game->board, depth, best_score, TT_UPPER, MOVE_NONE);

    return best_score;
}

// end
