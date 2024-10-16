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

#define STAT_NULL_DEPTH 4
static int STAT_NULL_MARGIN[STAT_NULL_DEPTH] = { 0, 80, 160, 320 };

#define RAZOR_DEPTH 6
static int RAZOR_MARGIN[RAZOR_DEPTH] = { 0, 250, 500, 750, 1000, 1250 };

//-------------------------------------------------------------------------------------------------
//  Principal variation search.
//-------------------------------------------------------------------------------------------------
int search(GAME *game, UINT incheck, int alpha, int beta, int depth, MOVE exclude_move)
{
    assert(incheck == TRUE || incheck == FALSE);
    assert(alpha >= -MAX_SCORE && alpha <= MAX_SCORE);
    assert(beta >= -MAX_SCORE && beta <= MAX_SCORE);
    assert(beta > alpha);
    assert(depth <= MAX_DEPTH);

    if (depth <= 0) return quiesce(game, incheck, alpha, beta, 0);

    int ply = get_ply(&game->board);
    int turn = side_on_move(&game->board);
    int pv_node = alpha != beta - 1 ? TRUE : FALSE;
    int root_node = ply == 0 ? TRUE : FALSE;
    int singular_move_search = exclude_move != MOVE_NONE ? TRUE : FALSE;

    if (ply >= MAX_PLY) return evaluate(game);

    game->pv_line.size[ply] = ply;
    game->search.nodes++;

    if (!root_node) {
        // drawn position
        if (is_draw(&game->board)) {
            return 0;
        }
        //  Mate pruning.
        alpha = MAX(-MATE_VALUE + ply, alpha);
        beta = MIN(MATE_VALUE - ply, beta);
        if (alpha >= beta) return alpha;
    }

    check_time(game);
    if (game->search.abort) return 0;

    // transposition table score or move hint
    TT_RECORD tt_record;
    tt_read(game->board.key, &tt_record);
    if (!pv_node && tt_record.data && tt_record.info.depth >= depth && exclude_move == MOVE_NONE) {
        int score = score_from_tt(tt_record.info.score, ply);
        if (tt_record.info.flag == TT_EXACT) return score;
        if (score >= beta && tt_record.info.flag == TT_LOWER) return score;
        if (score <= alpha && tt_record.info.flag == TT_UPPER) return score;
    }
    MOVE trans_move = tt_record.info.move;

#ifdef EGTB_SYZYGY
    if (!pv_node && !root_node && !singular_move_search) {
        // endgame tablebase probe
        U32 tbresult = egtb_probe_wdl(&game->board, depth, ply);
        if (tbresult != TB_RESULT_FAILED) {
            game->search.tbhits++;
            S8 tt_flag;
            int score;
            switch (tbresult) {
            case TB_WIN:
                score = EGTB_WIN - ply;
                tt_flag = TT_LOWER;
                break;
            case TB_LOSS:
                score = -EGTB_WIN + ply;
                tt_flag = TT_UPPER;
                break;
            default:
                score = 0;
                tt_flag = TT_EXACT;
                break;
            }
            if (tt_flag == TT_EXACT || (tt_flag == TT_LOWER && score >= beta) || (tt_flag == TT_UPPER && score < alpha)) {
                tt_record.info.move = MOVE_NONE;
                tt_record.info.depth = (S8)depth;
                tt_record.info.flag = tt_flag;
                tt_record.info.score = score_to_tt(score, ply);
                tt_save(game->board.key, &tt_record);
                return score;
            }
        }
    }
#endif 

    // Capture current eval and verify if this line is improving the score.
    int eval_score = evaluate(game);
    game->eval_hist[ply] = eval_score;
    int improving = ply > 1 && game->eval_hist[ply] > game->eval_hist[ply - 2];

    if (!pv_node && !incheck && !singular_move_search) {
        
        // Razoring: eval score + margin is lower than alpha, so just performs quiesce search and avoid regular search
        if (depth < RAZOR_DEPTH && eval_score + RAZOR_MARGIN[depth] < alpha && !is_mate_score(alpha) && trans_move == MOVE_NONE) {
            int razor_margin = alpha - RAZOR_MARGIN[depth];
            int score = quiesce(game, FALSE, razor_margin, razor_margin + 1, 0);
            if (game->search.abort) return 0;
            if (score < razor_margin) return score;
        }

        // Static null move: eval score + margin is higher that current beta, so it can skip the search.
        if (depth < STAT_NULL_DEPTH && eval_score - STAT_NULL_MARGIN[depth] >= beta && !is_mate_score(beta)) {
            return eval_score - STAT_NULL_MARGIN[depth];
        }

        // Null move heuristic: side to move has advantage that even allowing an extra move to opponent, still keeps advantage.
        if (has_pieces(&game->board, turn) && !has_recent_null_move(&game->board)) {
            // null move search
            if (depth >= 2 && eval_score >= beta) {
                MOVE last_move = get_last_move_made(&game->board);
                int null_depth = depth - 5 - ((depth - 4) / 4) - MIN(3, (eval_score - beta) / 200) - (move_is_quiet(last_move) ? 0 : 1);
                make_move(&game->board, NULL_MOVE);
                int null_score = -search(game, FALSE, -beta, -beta + 1, null_depth, MOVE_NONE);
                undo_move(&game->board);
                if (game->search.abort) return 0;
                if (null_score >= beta) {
                    if (is_mate_score(null_score)) {
                        null_score = beta; // don't accept mate score from null move search
                    }
                    return null_score;
                }
            }
        }

        //  Prob-Cut: after a capture a low depth with reduced beta indicates it is safe to ignore this node
        int pc_beta = beta + 100;
        if (depth >= 5 && !is_mate_score(beta)) {
            if (!tt_record.data || tt_record.info.score >= pc_beta || tt_record.info.depth < depth - 3) {
                MOVE pc_move;
                MOVE_LIST pc_move_list;
                select_init(&pc_move_list, game, incheck, trans_move, TRUE);
                while ((pc_move = next_move(&pc_move_list)) != MOVE_NONE) {
                    if (move_is_quiet(pc_move) || eval_score + see_move(&game->board, pc_move) < pc_beta) continue;
                    if (!is_pseudo_legal(&game->board, pc_move_list.pins, pc_move)) continue;
                    make_move(&game->board, pc_move);
                    int pc_incheck = is_incheck(&game->board, side_on_move(&game->board));
                    int pc_score = -search(game, pc_incheck, -pc_beta, -pc_beta + 1, depth - 4, MOVE_NONE);
                    undo_move(&game->board);
                    if (game->search.abort) return 0;
                    if (pc_score >= pc_beta) {
                        return pc_score;
                    }
                }
            }
        }

    }

    // Reduction when position is not on transposition table. Idea from Prodeo chess engine (from Ed Schroder).
    if (depth > 3 && trans_move == MOVE_NONE && !incheck) {
        depth--;
    }

    //TODO test best_score = eval_score
    //  Loop through move list
    MOVE_LIST   ml;
    MOVE        best_move = MOVE_NONE;
    int         best_score = -MAX_SCORE;
    int         score = 0;
    int         move_count = 0;
    MOVE        move;

    select_init(&ml, game, incheck, trans_move, FALSE);
    while ((move = next_move(&ml)) != MOVE_NONE) {

        assert(is_valid(&game->board, move));
        
        if (move == exclude_move) continue;

        if (!is_pseudo_legal(&game->board, ml.pins, move)) continue;

        move_count++;

        int reductions = 0;
        int extensions = 0;
        int gives_check = is_check(&game->board, move);

        //  Extension if move puts opponent in check
        if (gives_check && (depth < 4 || see_move(&game->board, move) >= 0)) {
            extensions = 1;
        }

        //  Singular move extension/cutoff/reduction
        if (!root_node && depth >= 8 && !extensions && !singular_move_search) {
            if (tt_record.data && move == trans_move && tt_record.info.flag != TT_UPPER) {
                int trans_score = score_from_tt(tt_record.info.score, game->board.ply);
                if (tt_record.info.depth >= depth - 3 && !is_mate_score(trans_score)) {
                    int reduced_beta = trans_score - 4 * depth;
                    int singular_score = search(game, incheck, reduced_beta - 1, reduced_beta, depth / 2, move);
                    if (game->search.abort) return 0;
                    if (singular_score + 50 < reduced_beta) {
                        extensions = 2;
                    }
                    else {
                        if (singular_score < reduced_beta) {
                            extensions = 1;
                        }
                        else {
                            if (reduced_beta >= beta) {
                                return reduced_beta;
                            }
                            else {
                                if (trans_score <= alpha || trans_score >= beta) {
                                    reductions = 1;
                                }
                            }
                        }
                    }
                }
            }
        }

        //  Pruning or depth reductions
        if (!extensions && move_count > 1 && move_is_quiet(move)) {
            if (!is_killer_move(&game->move_order, turn, ply, move)) {
                if (!is_counter_move(&game->move_order, flip_color(turn), get_last_move_made(&game->board), move)) {
                    int move_has_bad_history = get_has_bad_history(&game->move_order, turn, move);
                    // Move count pruning: prune moves based on move count.
                    if (!pv_node && move_has_bad_history && depth < 8 && !incheck) {
                        int pruning_threshold = 4 + depth * 2;
                        if (!improving) pruning_threshold = pruning_threshold - 3;
                        if (move_count > pruning_threshold) continue;
                    }
                    // Futility pruning: eval + margin below beta. Uses beta cutoff history.
                    if (depth < 8 && (!pv_node || !incheck) && !is_mate_score(alpha)) {
                        int pruning_margin = depth * (50 + get_pruning_margin(&game->move_order, turn, move));
                        if (eval_score + pruning_margin < alpha) {
                            continue;
                        }
                    }
                    // Late move reductions: reduce depth for later moves
                    if (move_count > 3 && depth > 2) {
                        reductions += reduction_table[MIN(depth, MAX_DEPTH - 1)][MIN(move_count, MAX_MOVE - 1)];
                        if (!pv_node) {
                            if (move_has_bad_history || !improving || (incheck && unpack_piece(move) == KING)) reductions++;
                            if (trans_move != MOVE_NONE && !move_is_quiet(trans_move)) reductions++;
                        }
                        else {
                            if (reductions > 0 && !move_has_bad_history) reductions--;
                            if (reductions > 0 && incheck) reductions--;
                        }
                    }
                }
            }
        }

        if (move_count > 5 && !extensions && !incheck && depth < 5 && !move_is_quiet(move)) {
            if (best_score + 100 * depth + see_move(&game->board, move) <= alpha) {
                continue;
            }
        }

        //  Make move and search new position.
        make_move(&game->board, move);

        assert(valid_is_legal(&game->board, move));

        if (move_count == 1) {
            score = -search(game, gives_check, -beta, -alpha, depth - 1 + extensions, MOVE_NONE);
        }
        else {
            score = -search(game, gives_check, -alpha - 1, -alpha, depth - 1 + extensions - reductions, MOVE_NONE);
            if (!game->search.abort && score > alpha && reductions) {
                score = -search(game, gives_check, -alpha - 1, -alpha, depth - 1 + extensions, MOVE_NONE);
            }
            if (!game->search.abort && score > alpha && score < beta) {
                score = -search(game, gives_check, -beta, -alpha, depth - 1 + extensions, MOVE_NONE);
            }
        }

        undo_move(&game->board);
        if (game->search.abort) return 0;

        //  Score verification and updates
        if (score > best_score) {
            if (score > alpha) {
                update_pv(&game->pv_line, ply, move);
                if (root_node) {
                    post_info(game, score, depth);
                }
                alpha = score;
                best_move = move;
                if (score >= beta) {
                    if (!singular_move_search) {
                        if (move_is_quiet(move)) {
                            save_beta_cutoff_data(&game->move_order, turn, ply, move, &ml, get_last_move_made(&game->board));
                        }
                        tt_record.info.move = move;
                        tt_record.info.depth = (S8)depth;
                        tt_record.info.flag = TT_LOWER;
                        tt_record.info.score = score_to_tt(score, ply);
                        tt_save(game->board.key, &tt_record);
                    }
                    return score;
                }
            }
            best_score = score;
        }
    }

    if (singular_move_search) {
        //  Special case for singular move search
        if (best_score == -MAX_SCORE) {
            return beta - 1;
        }
    }
    else {
        //  Regular search score verification
        if (best_score == -MAX_SCORE) {
            return (incheck ? -MATE_VALUE + ply : 0);
        }
        //  Record transposition table information
        if (best_move != MOVE_NONE) {
            tt_record.info.move = best_move;
            tt_record.info.depth = (S8)depth;
            tt_record.info.flag = TT_EXACT;
            tt_record.info.score = score_to_tt(best_score, ply);
        }
        else {
            tt_record.info.move = MOVE_NONE;
            tt_record.info.depth = (S8)depth;
            tt_record.info.flag = TT_UPPER;
            tt_record.info.score = score_to_tt(best_score, ply);
        }
        tt_save(game->board.key, &tt_record);
    }

    return best_score;
}

// end
