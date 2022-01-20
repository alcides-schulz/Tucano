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

#ifdef TUCANNUE
#include "nnue/nnue.h"
#endif

//-------------------------------------------------------------------------------------------------
//  Main evaluation function
//-------------------------------------------------------------------------------------------------

void    eval_material(BOARD *board, EVALUATION *eval_values);
void    eval_pawns(BOARD *board, PAWN_TABLE *pawn_table, EVALUATION *eval_values);
void    eval_passed(BOARD *board, EVALUATION *eval_values);
void    eval_kings(BOARD *board, EVALUATION *eval_values);
void    eval_pieces(BOARD *board, EVALUATION *eval_values);

#ifdef TUCANNUE
int nnue_squares[64] = {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};
int nnue_pieces[2][6] = {
    {6, 5, 4, 3, 2, 1},
    {12, 11, 10, 9, 8, 7}
};

#endif

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

    int score = 0;

    if (USE_NN_EVAL) {
        // NNUE evaluation
#ifdef TUCANNUE
        /*
        * Piece codes are
        *     wking=1, wqueen=2, wrook=3, wbishop= 4, wknight= 5, wpawn= 6,
        *     bking=7, bqueen=8, brook=9, bbishop=10, bknight=11, bpawn=12,
        * Squares are
        *     A1=0, B1=1 ... H8=63
        * Input format:
        *     piece[0] is white king, square[0] is its location
        *     piece[1] is black king, square[1] is its location
        *     ..
        *     piece[x], square[x] can be in any order
        *     ..
        *     piece[n+1] is set to 0 to represent end of array
        */
        int player = side_on_move(&game->board);
        int pieces[33];
        int squares[33];

        pieces[0] = 1;
        squares[0] = nnue_squares[king_square(&game->board, WHITE)];
        pieces[1] = 7;
        squares[1] = nnue_squares[king_square(&game->board, BLACK)];

        int next_index = 2;
        for (int color = WHITE; color <= BLACK; color++) {
            for (int piece = QUEEN; piece >= PAWN; piece--) {
                U64 pieces_bb = game->board.state[color].piece[piece];
                while (pieces_bb) {
                    int square = bb_first_index(pieces_bb);
                    pieces[next_index] = nnue_pieces[color][piece];
                    squares[next_index] = nnue_squares[square];
                    next_index++;
                    bb_clear_bit(&pieces_bb, square);
                }
            }
        }
        pieces[next_index] = 0;
        squares[next_index] = 0;

        score = (int)nnue_evaluate(player, pieces, squares);
#endif
    }
    else {
       
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
        score = ((opening * (48 - eval_values.phase)) + (endgame * eval_values.phase)) / 48;

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

        if (EVAL_PRINTING) eval_print_values(&eval_values);
    }

    //  Save to eval table and return.
    if (USE_EVAL_TABLE) {
        pet->key = board_key(&game->board);
        pet->score = score;
    }

    return score;
}

// END
