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
//  Main evaluation function
//-------------------------------------------------------------------------------------------------

void    eval_material(BOARD *board, EVALUATION *eval_values);
void    eval_pawns(BOARD *board, PAWN_TABLE *pawn_table, EVALUATION *eval_values);
void    eval_passed(BOARD *board, EVALUATION *eval_values);
void    eval_kings(BOARD *board, EVALUATION *eval_values);
void    eval_pieces(BOARD *board, EVALUATION *eval_values);

//-------------------------------------------------------------------------------------------------
//  Calculate the score for current position.
//-------------------------------------------------------------------------------------------------
int evaluate(GAME *game, int alpha, int beta)
{
    //  Return score from eval table if available.
    EVAL_TABLE *pet = game->eval_table + (board_key(&game->board) % EVAL_TABLE_SIZE);
    if (USE_EVAL_TABLE && pet->key == board_key(&game->board) && !EVAL_PRINTING) {
        return pet->score;
    }

    //  Prepare eval_values.
    EVALUATION eval_values;
    eval_values.phase = 0;
    eval_values.material[WHITE] = eval_values.material[BLACK] = 0;
    eval_values.pawn[WHITE] = eval_values.pawn[BLACK] = 0;
    eval_values.king[WHITE] = eval_values.king[BLACK] = 0;
    eval_values.passed[WHITE] = eval_values.passed[BLACK] = 0;
    eval_values.pieces[WHITE] = eval_values.pieces[BLACK] = 0;
    eval_values.mobility[WHITE] = eval_values.mobility[BLACK] = 0;
    eval_values.draw_adjust = 0;
    eval_values.flag_king_safety[WHITE] = eval_values.flag_king_safety[BLACK] = 0;
    eval_values.bb_passers[WHITE] = eval_values.bb_passers[BLACK] = 0;
    eval_values.non_mating_material[WHITE] = eval_values.non_mating_material[BLACK] = FALSE;

    //  Material.
    eval_material(&game->board, &eval_values);

    //  Position is draw.
    if (USE_EVAL_TABLE && eval_values.draw_adjust == 0) {
        pet->key = board_key(&game->board);
        pet->score = 0;
        return 0;
    }

    // Lazy evaluation. If there's a big material difference return early.
    int opening = OPENING(eval_values.material[WHITE]) - OPENING(eval_values.material[BLACK]);
    int endgame = ENDGAME(eval_values.material[WHITE]) - ENDGAME(eval_values.material[BLACK]);
    
    int lazy_eval = ((opening * (48 - eval_values.phase)) + (endgame * eval_values.phase)) / 48;

    if (side_on_move(&game->board) == BLACK) lazy_eval = -lazy_eval;

    if (lazy_eval > MAX_EVAL) lazy_eval = MAX_EVAL;
    if (lazy_eval < -MAX_EVAL) lazy_eval = -MAX_EVAL;

    assert(lazy_eval >= -MAX_EVAL && lazy_eval <= MAX_EVAL);

    if (lazy_eval + 800 < alpha || lazy_eval - 800 > beta) return lazy_eval;

    //  Evaluation.
    eval_pawns(&game->board, game->pawn_table, &eval_values);
    eval_passed(&game->board, &eval_values);
    eval_kings(&game->board, &eval_values);
    eval_pieces(&game->board, &eval_values);

    //  Score calculation
    opening = endgame = 0;

    opening += OPENING(eval_values.material[WHITE]) - OPENING(eval_values.material[BLACK]);
    endgame += ENDGAME(eval_values.material[WHITE]) - ENDGAME(eval_values.material[BLACK]);

    opening += OPENING(eval_values.king[WHITE]) - OPENING(eval_values.king[BLACK]);
    endgame += ENDGAME(eval_values.king[WHITE]) - ENDGAME(eval_values.king[BLACK]);

    opening += OPENING(eval_values.pawn[WHITE]) - OPENING(eval_values.pawn[BLACK]);
    endgame += ENDGAME(eval_values.pawn[WHITE]) - ENDGAME(eval_values.pawn[BLACK]);

    opening += OPENING(eval_values.passed[WHITE]) - OPENING(eval_values.passed[BLACK]);
    endgame += ENDGAME(eval_values.passed[WHITE]) - ENDGAME(eval_values.passed[BLACK]);

    opening += OPENING(eval_values.pieces[WHITE]) - OPENING(eval_values.pieces[BLACK]);
    endgame += ENDGAME(eval_values.pieces[WHITE]) - ENDGAME(eval_values.pieces[BLACK]);

    opening += OPENING(eval_values.mobility[WHITE]) - OPENING(eval_values.mobility[BLACK]);
    endgame += ENDGAME(eval_values.mobility[WHITE]) - ENDGAME(eval_values.mobility[BLACK]);

    //  Adjust score according material.
    int score = ((opening * (48 - eval_values.phase)) + (endgame * eval_values.phase)) / 48;

    if (side_on_move(&game->board) == WHITE)
        score += B_TEMPO;
    else
        score -= B_TEMPO;

    // Score reduction in non-mating material positions.
    if (score > 0 && eval_values.non_mating_material[WHITE]) score /= 10;
    if (score < 0 && eval_values.non_mating_material[BLACK]) score /= 10;

    // Draw adjustment from material on the board.
    score = score * eval_values.draw_adjust / 64;

    //  Adjustment for side on move.
    if (side_on_move(&game->board) == BLACK) score = -score;

    if (score > MAX_EVAL) score = MAX_EVAL;
    if (score < -MAX_EVAL) score = -MAX_EVAL;

    assert(score >= -MAX_EVAL && score <= MAX_EVAL);

    //  Save to eval table and return.
    if (USE_EVAL_TABLE) {
        pet->key = board_key(&game->board);
        pet->score = score;
    }

    if (EVAL_PRINTING) eval_print_values(&eval_values);

    return score;
}

// END
