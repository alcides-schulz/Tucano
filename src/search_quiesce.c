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
//  Quiescence search
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Search nodes until a quiet position is found.
//-------------------------------------------------------------------------------------------------
int quiesce(GAME *game, UINT incheck, int alpha, int beta, int depth)
{
    MOVE_LIST   ml;
    int         score;
    MOVE        move = MOVE_NONE;
    int         best_score = -MAX_SCORE;
    int         ply = get_ply(&game->board);
    MOVE        best_move = MOVE_NONE;
    
    assert(alpha <= beta);

    check_time(game);
    if (game->search.abort) return 0;

    game->pv_line.size[ply] = ply;
    game->search.nodes++;

    if (ply > 0 && is_draw(&game->board)) return 0;

    assert(ply >= 0 && ply <= MAX_PLY);
    if (ply >= MAX_PLY) return evaluate(game);

    //  Mate pruning.
    alpha = MAX(-MATE_SCORE + ply, alpha);
    beta = MIN(MATE_SCORE - ply, beta);
    if (alpha >= beta) return alpha;

    S8 quiesce_depth = incheck || depth >= 0 ? 0 : -1;

    // transposition table score or move hint
    TT_RECORD tt_record;
    tt_read(game->board.key, &tt_record);
    if (tt_record.data && tt_record.info.depth >= quiesce_depth) {
        score = score_from_tt(tt_record.info.score, game->board.ply);
        if (tt_record.info.flag == TT_EXACT) {
            return score;
        }
        if (score >= beta && tt_record.info.flag == TT_LOWER) {
            return score;
        }
        if (score <= alpha && tt_record.info.flag == TT_UPPER) {
            return score;
        }
    }
    MOVE trans_move = tt_record.info.move;

    if (!incheck) {
        best_score = evaluate(game);
        if (best_score >= beta) {
            return best_score;
        }
        if (best_score > alpha) {
            alpha = best_score;
        }
    }

    select_init(&ml, game, incheck, trans_move, TRUE);

    while ((move = next_move(&ml)) != MOVE_NONE) {

        assert(is_valid(&game->board, move));

        //  Skip moves that are not going to improve the position.
        if (!incheck && unpack_type(move) == MT_CAPPC) {

            // Skip captures that will not improve alpha (delta pruning). Note: depth is negative here.
            if (best_score + MAX(100, 500 + depth * 10) + piece_value(unpack_capture(move)) <= alpha) {
                continue;
            }

            // Skip losing captures based on Static Exchange Evaluation (SEE).
            if (piece_value_see(unpack_capture(move)) < piece_value_see(unpack_piece(move))) {
                if (see_move(&game->board, move) < 0) {
                    continue;
                }
            }
        }

        if (!is_pseudo_legal(&game->board, ml.pins, move)) continue;

        int gives_check = is_check(&game->board, move);

        make_move(&game->board, move);

        assert(gives_check == is_incheck(&game->board, side_on_move(&game->board)));
        assert(valid_is_legal(&game->board, move));

        //  Search new position.
        score = -quiesce(game, gives_check, -beta, -alpha, depth - 1);

        undo_move(&game->board);
        if (game->search.abort) return 0;

        //  Score verification
        if (score > best_score) {
            if (score > alpha) {
                update_pv(&game->pv_line, ply, move);
                if (score >= beta) {
                    tt_record.info.move = move;
                    tt_record.info.depth = quiesce_depth;
                    tt_record.info.flag = TT_LOWER;
                    tt_record.info.score = score_to_tt(score, ply);
                    tt_save(game->board.key, &tt_record);
                    return score;
                }
                alpha = score;
                best_move = move;
            }
            best_score = score;
        }
    }

    //  Return only checkmate scores. We don't look at all moves unless in check.
    if (best_score == -MAX_SCORE && incheck) {
        return -MATE_SCORE + ply;
    }

    if (best_move != MOVE_NONE) {
        tt_record.info.move = best_move;
        tt_record.info.depth = quiesce_depth;
        tt_record.info.flag = TT_EXACT;
        tt_record.info.score = score_to_tt(best_score, ply);
    }
    else {
        tt_record.info.move = MOVE_NONE;
        tt_record.info.depth = quiesce_depth;
        tt_record.info.flag = TT_UPPER;
        tt_record.info.score = score_to_tt(best_score, ply);
    }
    tt_save(game->board.key, &tt_record);

    return best_score;
}

// END
