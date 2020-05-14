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
// validations when running in ASSERT mode (see NDEBUG in globals.h)
//-------------------------------------------------------------------------------------------------

#ifndef NDEBUG

int is_move_in_list(GAME *game, MOVE test_move)
{
    MOVE_LIST   ml;
    MOVE		move;

    select_init(&ml, game, is_incheck(&game->board, side_on_move(&game->board)), MOVE_NONE, FALSE);
    while ((move = next_move(&ml)) != MOVE_NONE)  {
        if (!is_pseudo_legal(&game->board, ml.pins, move))
            continue;
        if (move == test_move)
            return TRUE;
    }
    return FALSE;
}

int valid_is_legal(BOARD *board, MOVE move)
{
    if (!is_illegal(board, move))
        return TRUE;
    util_print_move(move, TRUE);
    board_print(board, "move not legal");
    return FALSE;
}

int valid_piece(BOARD *board, int pcsq, int color, int type)
{
    if (piece_on_square(board, color, pcsq) != type)
        return FALSE;
    if (!(piece_bb(board, color, type) & square_bb(pcsq)))
        return FALSE;
    return TRUE;
}

int pawn_is_candidate(BOARD *board, int pcsq, int color)
{
    int     sq;
    int     own = 0;
    int     enemy = 0;

    if (pawn_is_passed(board, pcsq, color)) return FALSE;
    if (pawn_is_isolated(board, pcsq, color)) return FALSE;
    if (get_relative_rank(color, get_rank(pcsq)) < 2 || get_relative_rank(color, get_rank(pcsq)) > 5) return FALSE;
    if (pawn_is_weak(board, pcsq, color)) return FALSE;

    // count supporting pawns on the side or behind.
    sq = pcsq;
    while (get_rank(sq) != RANK1 && get_rank(sq) != RANK8) {
        if (get_file(sq) > 0 && piece_on_square(board, color, sq - 1) == PAWN)
            own++;
        if (get_file(sq) < 7 && piece_on_square(board, color, sq + 1) == PAWN)
            own++;
        sq = (color == WHITE ? SQ_S(sq) : SQ_N(sq));
    }
    // count enemy pawns in front.
    sq = (color == WHITE ? SQ_N(pcsq) : SQ_S(pcsq));
    while (get_rank(sq) != RANK1 && get_rank(sq) != RANK8) {
        if (piece_on_square(board, flip_color(color), sq) == PAWN)
            return FALSE;
        if (get_file(sq) > 0 && piece_on_square(board, flip_color(color), sq - 1) == PAWN)
            enemy++;
        if (get_file(sq) < 7 && piece_on_square(board, flip_color(color), sq + 1) == PAWN)
            enemy++;
        sq = (color == WHITE ? SQ_N(sq) : SQ_S(sq));
    }
    if (own >= enemy)
        return TRUE;
    return FALSE;
}

int pawn_is_isolated(BOARD *board, int pcsq, int color)
{
    int     sq = get_file(pcsq) + 8;

    while (get_rank(sq) < 7) {
        if (get_file(sq) > 0 && piece_on_square(board, color, sq - 1) == PAWN)
            return FALSE;
        if (get_file(sq) < 7 && piece_on_square(board, color, sq + 1) == PAWN)
            return FALSE;
        sq += 8;
    }
    return TRUE;
}

int pawn_is_weak(BOARD *board, int pcsq, int color)
{
    int     sq = pcsq;

    while (get_rank(sq) != RANK1 && get_rank(sq) != RANK8) {
        if (get_file(sq) > 0) {
            if (piece_on_square(board, color, sq - 1) == PAWN)
                return FALSE;
        }
        if (get_file(sq) < 7) {
            if (piece_on_square(board, color, sq + 1) == PAWN)
                return FALSE;
        }
        sq = (color == WHITE ? SQ_S(sq) : SQ_N(sq));
    }
    return TRUE;
}

int pawn_is_passed(BOARD *board, int pcsq, int color)
{
    int     sq = (color == WHITE ? SQ_N(pcsq) : SQ_S(pcsq));

    while (get_rank(sq) != RANK1 && get_rank(sq) != RANK8) {
        if (get_file(sq) > 0) {
            if (piece_on_square(board, flip_color(color), sq - 1) == PAWN)
                return FALSE;
        }
        if (get_file(sq) < 7) {
            if (piece_on_square(board, flip_color(color), sq + 1) == PAWN)
                return FALSE;
        }
        if (piece_on_square(board, flip_color(color), sq) == PAWN)
            return FALSE;
        if (piece_on_square(board, color, sq) == PAWN)
            return FALSE;
        sq = (color == WHITE ? SQ_N(sq) : SQ_S(sq));
    }
    return TRUE;
}

int pawn_is_doubled(BOARD *board, int pcsq, int color)
{
    if (get_rank(pcsq) == RANK1 || get_rank(pcsq) == RANK8)
        return FALSE;
    if (color == WHITE) {
        pcsq = SQ_N(pcsq);
        while (valid_square(pcsq)) {
            if (piece_on_square(board, color, pcsq) == PAWN)
                return TRUE;
            pcsq = SQ_N(pcsq);
        }
    }
    else {
        pcsq = SQ_S(pcsq);
        while (valid_square(pcsq)) {
            if (piece_on_square(board, color, pcsq) == PAWN)
                return TRUE;
            pcsq = SQ_S(pcsq);
        }
    }
    return FALSE;
}

int pawn_is_connected(BOARD *board, int pcsq, int color)
{
    if (get_rank(pcsq) == RANK1 || get_rank(pcsq) == RANK8)
        return FALSE;
    if (get_file(pcsq) > 0) {
        if (piece_on_square(board, color, SQ_NW(pcsq)) == PAWN) {
            assert(valid_piece(board, SQ_NW(pcsq), color, PAWN));
            return TRUE;
        }
        if (piece_on_square(board, color, SQ_W(pcsq)) == PAWN) {
            assert(valid_piece(board, SQ_W(pcsq), color, PAWN));
            return TRUE;
        }
        if (piece_on_square(board, color, SQ_SW(pcsq)) == PAWN) {
            assert(valid_piece(board, SQ_SW(pcsq), color, PAWN));
            return TRUE;
        }
    }
    if (get_file(pcsq) < 7) {
        if (piece_on_square(board, color, SQ_NE(pcsq)) == PAWN) {
            assert(valid_piece(board, SQ_NE(pcsq), color, PAWN));
            return TRUE;
        }
        if (piece_on_square(board, color, SQ_E(pcsq)) == PAWN) {
            assert(valid_piece(board, SQ_E(pcsq), color, PAWN));
            return TRUE;
        }
        if (piece_on_square(board, color, SQ_SE(pcsq)) == PAWN) {
            assert(valid_piece(board, SQ_SE(pcsq), color, PAWN));
            return TRUE;
        }
    }
    return FALSE;
}

int valid_rank_file(void)
{
    int r,f;

    // square<->rank/file translation
    for (r = 0; r < 8; r++) {
        for (f = 0; f < 8; f++) {
            if (get_rank(r * 8 + f) != r) {
                printf("assert rank(%d) %d != %d \n", r * 8 + f, r, get_rank(r * 8 + f));
                return FALSE;
            }
            if (get_file(r * 8 + f) != f) {
                printf("assert file(%d) %d != %d \n", r * 8 + f, r, get_rank(r * 8 + f));
                return FALSE;
            }
        }
    }

    return TRUE;
}

//  Return piece material value
int pieces_value(BOARD *board, int color)
{
    return (queen_count(board, color)  * VALUE_QUEEN) +
           (rook_count(board, color)   * VALUE_ROOK) +
           (knight_count(board, color) * VALUE_KNIGHT) +
           (bishop_count(board, color) * VALUE_BISHOP);
}

int pawns_value(BOARD *board, int color)
{
    return (pawn_count(board, color) * VALUE_PAWN);
}

int valid_material(BOARD *board)
{
    int s, wm, bm;

    for (s = wm = bm = 0; s < 64; s++) {
        if (piece_on_square(board, WHITE, s))
            wm += piece_value(piece_on_square(board, WHITE, s));
        if (piece_on_square(board, BLACK, s))
            bm += piece_value(piece_on_square(board, BLACK, s));
    }

    if (wm != (pieces_value(board, WHITE) + pawns_value(board, WHITE))) {
        printf("white material error\n");
        return FALSE;
    }
    if (bm != (pieces_value(board, BLACK) + pawns_value(board, BLACK))) {
        printf("black material error\n");
        return FALSE;
    }
    if (wm != material_value(board, WHITE)) {
        printf("white material error (wm != material(WHITE))\n");
        return FALSE;
    }
    if (bm != material_value(board, BLACK)) {
        printf("black material error (bm != material(BLACK))\n");
        return FALSE;
    }
    return TRUE;
}

//  Assert valid square
int valid_square(int square)
{
    return square >= 0 && square < 64 ? TRUE : FALSE;
}

//  Assert valid color
int valid_color(int color)
{
    return color == WHITE || color == BLACK ? TRUE : FALSE;
}

int board_state_is_ok(BOARD *board)
{
    //    int         can_castle_ks;        // king side
    //    int         can_castle_qs;        // queen side
    //    U64            piece[7];            // bb for each piece type
    //    U64            all_pieces;            // bb for all pieces combined
    //    int         square[64];            // piece location on the board
    //    int            material;            // piece material value 
    //    int            pawn_mat;            // pawn material value
    //}                player_state[2];    // 0-white, 1-black

    int     i;
    U64     temp;
    int     c;
    int     t;
    int     m, p;

    for (c = 0; c < 2; c++) {
        temp = m = p = 0;
        for (i = PAWN; i <= KING; i++) {
            temp |= piece_bb(board, c, i);
        }
        if (temp != all_pieces_bb(board, c)) {
            bb_print("temp.u64", temp);
            bb_print("all_pieces_bb", all_pieces_bb(board, c));
            board_print(board, NULL);
            printf("piece_bb != all_pieces for player: %d\n", c);
            return FALSE;
        }

        // king
        if (king_count(board, c) != 1) {
            board_print(board, NULL);
            printf("king(%d) count is not 1\n", c);
            return FALSE;
        }
        if (king_bb(board, c) != square_bb(king_square(board, c))) {
            board_print(board, NULL);
            printf("king(%d) square does not match\n", c);
            return FALSE;
        }

        // pieces
        for (t = PAWN; t < NUM_PIECES; t++) {
            temp = piece_bb(board, c, t);
            while (temp) {
                if (piece_on_square(board, c, bb_first_index(temp)) != t) {
                    board_print(board, NULL);
                    printf("player %c piece %d not on square: %d\n", c, t, bb_first_index(temp));
                    return FALSE;
                }
                bb_clear_bit(&temp, bb_first_index(temp));
                if (t == PAWN)
                    p += piece_value(t);
                else
                    m += piece_value(t);
            }
        }
        if (p != pawns_value(board, c)) {
            board_print(board, NULL);
            printf("diff pawn mat: %d calc: %d  board: %d\n", c, p, pawns_value(board, c));
            return FALSE;
        }
        if (m != pieces_value(board, c)) {
            board_print(board, NULL);
            printf("diff mat: color: %d calc: %d board: %d\n", c, m, pieces_value(board, c));
            return FALSE;
        }

        for (i = 0; i < 64; i++) {
            if (piece_on_square(board, c, i) != NO_PIECE) {
                if (piece_on_square(board, c, i) < PAWN || piece_on_square(board, c, i) > KING) {
                    board_print(board, NULL);
                    printf("player %c invalid piece: type %d square: %d\n", c, piece_on_square(board, c, i), i);
                    return FALSE;
                }
                if (!(square_bb(i) & piece_bb(board, c, piece_on_square(board, c, i)))) {
                    board_print(board, NULL);
                    printf("player %c invalid square: type %d square %d\n", c, piece_on_square(board, c, i), i);
                    return FALSE;
                }
            }
        }
    }

    return TRUE;

}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard with all attacks from both sides to a square.
//-------------------------------------------------------------------------------------------------
U64 get_attackers(BOARD *board, int to, U64 occup)
{
    U64     attacks = 0;

    attacks |= pawn_attack_bb(WHITE, to) & occup & pawn_bb(board, WHITE);
    attacks |= pawn_attack_bb(BLACK, to) & occup & pawn_bb(board, BLACK);
    attacks |= knight_moves_bb(to) & occup & (knight_bb(board, WHITE) | knight_bb(board, BLACK));
    attacks |= bb_bishop_attacks(to, occup) & /*diagonal_rays[to] &*/ (queen_bishop_bb(board, WHITE) | queen_bishop_bb(board, BLACK));
    attacks |= bb_rook_attacks(to, occup) & /*rankfile_rays[to] & */(queen_rook_bb(board, WHITE) | queen_rook_bb(board, BLACK));
    attacks |= king_moves_bb(to) & occup & (king_bb(board, WHITE) | king_bb(board, BLACK));

    return attacks;
}

//int new_see_value[NUM_PIECES] = {0, 100, 300, 300, 500, 900, 10000};
//
//int test_see(MOVE move)
//{
//    int     type = unpack_type(move);
//    int     frsq = unpack_from(move);
//    int     tosq = unpack_to(move);
//    int     som = side_on_move(board);
//    U64     attackers;
//    U64     som_attackers;
//    U64     occup = occupied_bb(board);
//    int     piece = unpack_piece(move);
//    int     capture;
//    int     value[32];
//    int     index = 1;
//    BBIX    temp;
//
//    assert(is_valid(move));
//
//    //util_print_move(move, 1);
//    //board_print("move");
//
//    //    // Determine captured piece
//    //    if (m.flag() == EN_PASSANT) {
//    //        clear_bit(&occ, pawn_push(opp_color(stm), tsq));
//    //        capture = PAWN;
//    //    } else
//    //        capture = B.get_piece_on(tsq);
//    //    assert(capture != KING);
//    //
//    //    swap_list[0] = see_val[capture];
//    //    clear_bit(&occ, fsq);
//    if (type == MT_EPCAP) {
//        capture = PAWN;
//        bb_clear_bit(&occup, unpack_ep_pawn_square(move));
//    }
//    else
//        capture = piece_on_square(flip_color(som), tosq);
//    value[0] = new_see_value[capture];
//    bb_clear_bit(&occup, frsq);
//    //
//    //    // Handle promotion
//    //    if (m.flag() == PROMOTION) {
//    //        swap_list[0] += see_val[m.prom()] - see_val[PAWN];
//    //        capture = QUEEN;
//    //    } else
//    //        capture = B.get_piece_on(fsq);
//
//    if (type == MT_PROMO || type == MT_CPPRM) {
//        value[0] += new_see_value[unpack_prom_piece(move)] - new_see_value[PAWN];
//        capture = unpack_prom_piece(move);
//    }
//    else
//        capture = piece_on_square(som, frsq);
//
//    assert(capture >= EMPTY && capture <= KING);
//    //
//    //    // If the opponent has no attackers we are finished
//    //    attackers = test_bit(B.st().attacked, tsq) ? calc_attackers(B, tsq, occ) : 0;
//    //    stm = opp_color(stm);
//    //    stm_attackers = attackers & B.get_pieces(stm);
//    //    if (!stm_attackers)
//    //        return swap_list[0];
//    attackers = get_attackers(tosq, occup);
//    som = flip_color(som);
//    som_attackers = attackers & all_pieces_bb(som);
//    if (!som_attackers)
//        return value[0];
//    //
//    //    /* The destination square is defended, which makes things more complicated. We proceed by
//    //     * building a "swap list" containing the material gain or loss at each stop in a sequence of
//    //     * captures to the destination square, where the sides alternately capture, and always capture
//    //     * with the least valuable piece. After each capture, we look for new X-ray attacks from behind
//    //     * the capturing piece. */
//    //    do {
//    //        /* Locate the least valuable attacker for the side to move. The loop below looks like it is
//    //         * potentially infinite, but it isn't. We know that the side to move still has at least one
//    //         * attacker left. */
//    //        for (piece = PAWN; !(stm_attackers & B.get_pieces(stm, piece)); ++piece)
//    //            assert(piece < KING);
//
//    do {
//
//        //bb_print("attackers", attackers);
//        //bb_print("som_attackers", som_attackers);
//        //printf("som: %d index: %d\n", som, index);
//        //board_print("som_at");
//
//        for (piece = PAWN; !(som_attackers & piece_bb(som, piece)); piece++)
//            ;
//
//        assert(piece >= PAWN && piece <= KING);
//        //
//        //        // remove the piece (from wherever it came)
//        //        clear_bit(&occ, lsb(stm_attackers & B.get_pieces(stm, piece)));
//        //        // scan for new X-ray attacks through the vacated square
//        //        attackers |= (B.get_RQ() & RPseudoAttacks[tsq] & rook_attack(tsq, occ))
//        //                     | (B.get_BQ() & BPseudoAttacks[tsq] & bishop_attack(tsq, occ));
//        //        // cut out pieces we've already done
//        //        attackers &= occ;
//        temp.u64 = som_attackers & piece_bb(som, piece);
//        assert(temp.u64);
//        bb_clear_bit(&occup, bb_first(temp));
//        assert(valid_square(tosq));
//
//        attackers |= bb_bishop_attacks(tosq, occup) & /*diagonal_rays[tosq] & */(queen_bishop_bb(WHITE) | queen_bishop_bb(BLACK));
//        attackers |= bb_rook_attacks(tosq, occup) & /*rankfile_rays[tosq] &*/ (queen_rook_bb(WHITE) | queen_rook_bb(BLACK));
//        attackers &= occup;
//
//        //
//        //        // add the new entry to the swap list (beware of promoting pawn captures)
//        //        assert(sl_idx < 32);
//        //        swap_list[sl_idx] = -swap_list[sl_idx - 1] + see_val[capture];
//        //        if (piece == PAWN && test_bit(PPromotionRank[stm], tsq)) {
//        //            swap_list[sl_idx] += see_val[QUEEN] - see_val[PAWN];
//        //            capture = QUEEN;
//        //        } else
//        //            capture = piece;
//        //        sl_idx++;
//        value[index] = -value[index - 1] + new_see_value[capture];
//        if (piece == PAWN && get_rank(tosq) == (som == WHITE ? RANK8 : RANK1)) {
//            value[index] += new_see_value[QUEEN] - new_see_value[PAWN];
//            capture = QUEEN;
//        }
//        else
//            capture = piece;
//        index++;
//        assert(index < 32);
//        //
//        //        stm = opp_color(stm);
//        //        stm_attackers = attackers & B.get_pieces(stm);
//
//        som = flip_color(som);
//        som_attackers = attackers & all_pieces_bb(som);
//        //
//        //        // Stop after a king capture
//        //        if (piece == KING && stm_attackers) {
//        //            assert(sl_idx < 32);
//        //            swap_list[sl_idx++] = see_val[KING];
//        //            break;
//        //        }
//        if (piece == KING && som_attackers) {
//            value[index++] = new_see_value[KING];
//            break;
//        }
//        //    } while (stm_attackers);
//        //
//    } while(som_attackers);
//
//    //    /* Having built the swap list, we negamax through it to find the best achievable score from the
//    //     * point of view of the side to move */
//    //    while (--sl_idx)
//    //        swap_list[sl_idx-1] = std::min(-swap_list[sl_idx], swap_list[sl_idx-1]);
//    //
//    //    return swap_list[0];
//    //}
//
//    while (--index)
//        value[index - 1] = MIN(-value[index], value[index - 1]);
//
//    return value[0];
//}


#endif
