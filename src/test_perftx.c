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
//    a more detailed perf test using the numbers from chess wiki programming.
//-------------------------------------------------------------------------------------------------

#define PERFX_LABEL "Dep     Nodes Captures   E.p. Castles  Promote   Checks  Mates"

typedef struct
{
    U64     nodes;
    U64     captures;
    U64     en_passants;
    U64     castles;
    U64     promotions;
    U64     checks;
    U64     check_mates;
    int     depth;
}   RESULTS;

typedef struct
{
    char    *fen;
    int     count;
    RESULTS results[10];
}   TESTDATA;

U64     perfx_capture;
U64     perfx_enpassant;
U64     perfx_castle;
U64     perfx_promotions;
U64     perfx_checks;
U64     perfx_mates;

void    perftx_set(RESULTS *results, int depth, U64 nodes, 
                   U64 captures, U64 enpassant, U64 castles, 
                   U64 promotions, U64 checks, U64 mates);
void    perftx_test(TESTDATA *testdata);
U64     perftx_moves(GAME *game, int depth);
int     perftx_is_mated(GAME *game);

void perftx(void)
{
    TESTDATA testdata0;
    TESTDATA testdata1;
    TESTDATA testdata2;
    TESTDATA testdata3;
    
    testdata0.fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    testdata0.count = 6;
    perftx_set(&testdata0.results[0], 1,        20,       0,    0, 0, 0,      0,     0);
    perftx_set(&testdata0.results[1], 2,       400,       0,    0, 0, 0,      0,     0);
    perftx_set(&testdata0.results[2], 3,      8902,      34,    0, 0, 0,     12,     0);
    perftx_set(&testdata0.results[3], 4,    197281,    1576,    0, 0, 0,    469,     8);
    perftx_set(&testdata0.results[4], 5,   4865609,   82719,  258, 0, 0,  27351,   347);
    perftx_set(&testdata0.results[5], 6, 119060324, 2812008, 5248, 0, 0, 809099, 10828);

    testdata1.fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"; //castle
    testdata1.count = 5;
    perftx_set(&testdata1.results[0], 1, 48, 8, 0, 2, 0, 0, 0);
    perftx_set(&testdata1.results[1], 2, 2039, 351, 1, 91, 0, 3, 0);
    perftx_set(&testdata1.results[2], 3, 97862, 17102, 45, 3162, 0, 993, 1);
    perftx_set(&testdata1.results[3], 4, 4085603, 757163, 1929, 128013, 15172, 25523, 43);
    perftx_set(&testdata1.results[4], 5, 193690690, 35043416, 73365, 4993637, 8392, 3309887, 30171);

    testdata2.fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
    testdata2.count = 7;
    perftx_set(&testdata2.results[0], 1, 14, 1, 0, 0, 0, 2, 0);
    perftx_set(&testdata2.results[1], 2, 191, 14, 0, 0, 0, 10, 0);
    perftx_set(&testdata2.results[2], 3, 2812, 209, 2, 0, 0, 267, 0); 
    perftx_set(&testdata2.results[3], 4, 43238, 3348, 123, 0, 0, 1680, 17);
    perftx_set(&testdata2.results[4], 5, 674624, 52051, 1165, 0, 0, 52950, 0);
    perftx_set(&testdata2.results[5], 6, 11030083,  940350,  33325,  0,  7552,  452473,  2733);
    perftx_set(&testdata2.results[6], 7, 178633661, 14519036, 294874, 0, 140024, 12797406, 87);

    testdata3.fen = "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1"; // promotion
    testdata3.count = 6;
    perftx_set(&testdata3.results[0], 1, 24, 11, 0, 0, 12, 3, 0);
    perftx_set(&testdata3.results[1], 2, 496, 203, 0, 0, 252, 59, 0);
    perftx_set(&testdata3.results[2], 3, 9483, 2921, 0, 0, 3224, 992, 0);
    perftx_set(&testdata3.results[3], 4, 182838, 50647, 0, 0, 65620, 20154, 0);
    perftx_set(&testdata3.results[4], 5, 3605103, 754747, 0, 0, 955220, 399962, 134);
    perftx_set(&testdata3.results[5], 6, 71179139, 13902699, 0, 0, 19191520, 8130299, 3308);

    perftx_test(&testdata0);
    perftx_test(&testdata1);
    perftx_test(&testdata2);
    perftx_test(&testdata3);

    printf("\nperftx completed.\n");
}

void perftx_set(RESULTS *results, int depth, U64 nodes, 
                U64 captures, U64 enpassant, U64 castles, 
                U64 promotions, U64 checks, U64 mates)
{
    results->depth = depth;
    results->nodes = nodes;
    results->captures = captures;
    results->en_passants = enpassant;
    results->castles = castles;
    results->promotions = promotions;
    results->checks = checks;
    results->check_mates = mates;
}

void perftx_test(TESTDATA *testdata)
{
    U64     nodes = 0;
    U64     prev_captures = 0;
    U64     prev_enpassant = 0;
    U64     prev_castles = 0;
    U64     prev_promotions = 0;
    U64     prev_checks = 0;
    U64     prev_mates = 0;
    int     d;
    int     diff = 0;
    GAME    *game;

    game = (GAME *)malloc(sizeof(GAME));
    if (game == NULL) {
        fprintf(stderr, "perftx.malloc: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        return;
    }

    new_game(game, testdata->fen);
    printf("FEN: %s\n", testdata->fen);
    board_print(&game->board, NULL);
    
    printf("Expected numbers:\n");
    printf(PERFX_LABEL);
    printf("\n");
    for (d = 1; d <= testdata->count; d++) {
        printf("%3d ", d);
        printf("%9" PRIu64 " ", testdata->results[d-1].nodes);
        printf("%8" PRIu64 " ", testdata->results[d-1].captures);
        printf("%6" PRIu64 " ", testdata->results[d-1].en_passants);
        printf("%7" PRIu64 " ", testdata->results[d-1].castles);
        printf("%8" PRIu64 " ", testdata->results[d-1].promotions);
        printf("%8" PRIu64 " ", testdata->results[d-1].checks);
        printf("%6" PRIu64 " ", testdata->results[d-1].check_mates);
        printf("\n");
    }

    perfx_capture = 0;
    perfx_enpassant = 0;
    perfx_castle = 0;
    perfx_promotions = 0;
    perfx_checks = 0;
    perfx_mates = 0;

    printf("Numbers found:\n");
    printf(PERFX_LABEL);
    printf("\n");

    for (d = 1; d <= testdata->count; d++) {
        prev_captures += perfx_capture;
        prev_enpassant += perfx_enpassant;
        prev_castles += perfx_castle;
        prev_promotions += perfx_promotions;
        prev_checks += perfx_checks;
        prev_mates += perfx_mates;

        perfx_capture = 0;
        perfx_enpassant = 0;
        perfx_castle = 0;
        perfx_promotions = 0;
        perfx_checks = 0;
        perfx_mates = 0;

        nodes = perftx_moves(game, d);

        perfx_capture -= prev_captures;
        perfx_enpassant -= prev_enpassant;
        perfx_castle -= prev_castles;
        perfx_promotions -= prev_promotions;
        perfx_checks -= prev_checks;
        perfx_mates -= prev_mates;

        printf("%3d ", d);
        printf("%9" PRIu64 " ", nodes);
        printf("%8" PRIu64 " ", perfx_capture);
        printf("%6" PRIu64 " ", perfx_enpassant);
        printf("%7" PRIu64 " ", perfx_castle);
        printf("%8" PRIu64 " ", perfx_promotions);
        printf("%8" PRIu64 " ", perfx_checks);
        printf("%6" PRIu64 " ", perfx_mates);

        diff = 0;

        if (testdata->results[d-1].nodes != nodes)
            diff = 1;
        if (testdata->results[d-1].captures != perfx_capture)
            diff = 1;
        if (testdata->results[d-1].en_passants != perfx_enpassant)
            diff = 1;
        if (testdata->results[d-1].castles != perfx_castle)
            diff = 1;
        if (testdata->results[d-1].promotions != perfx_promotions)
            diff = 1;
        if (testdata->results[d-1].checks != perfx_checks)
            diff = 1;
        if (testdata->results[d-1].check_mates != perfx_mates)
            diff = 1;

        if (diff)
            printf("  **** DIFF");

        printf("\n");
    }

    free(game);

    if (diff) {
        printf("Press ENTER to continue...\n");
        while (getchar() != '\n');
    }
}

U64 perftx_moves(GAME *game, int depth) {
    U64         nodes = 0;
    MOVE        move;
    MOVE_LIST   move_list;

    if (depth == 0) return 1;

    select_init(&move_list, game, is_incheck(&game->board, side_on_move(&game->board)), MOVE_NONE, FALSE);
    
    while ((move = next_move(&move_list)) != MOVE_NONE) {

        if (!is_pseudo_legal(&game->board, move_list.pins, move))
            continue;

        make_move(&game->board, move);

        if (is_incheck(&game->board, side_on_move(&game->board))) {
            perfx_checks++;
            if (perftx_is_mated(game)) perfx_mates++;
        }

        if (move_is_capture(move))    perfx_capture++;
        if (move_is_en_passant(move)) perfx_enpassant++;
        if (move_is_castle(move))     perfx_castle++;
        if (move_is_promotion(move))  perfx_promotions++;
        
        nodes += perftx_moves(game, depth - 1);

        undo_move(&game->board);
    }
    
    return nodes;
}


int perftx_is_mated(GAME *game)
{
    MOVE_LIST   move_list;
    MOVE move;

    select_init(&move_list, game, TRUE, MOVE_NONE, FALSE);
    while ((move = next_move(&move_list)) != MOVE_NONE) {
        if (!is_pseudo_legal(&game->board, move_list.pins, move))
            continue;
        return FALSE; // Side on move has at least one legal move
    }
    return TRUE; //no legal moves
}

//END
