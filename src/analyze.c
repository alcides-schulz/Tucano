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
//  Analyze feature
//-------------------------------------------------------------------------------------------------

static GAME        analysis_game;
static THREAD_ID   analysis_thread;
static char        analysis_command[MAX_READ];

static void analysis_start(GAME *game);

void analyze_mode(GAME *game)
{
    memcpy(&analysis_game, game, sizeof(GAME));

    THREAD_CREATE(analysis_thread, analysis_start, &analysis_game);

    while (TRUE) {
        if (fgets(analysis_command, MAX_READ, stdin) == NULL) if (feof(stdin)) break;

        char *p = strchr(analysis_command, '\n');
        if (p != NULL) *p = '\0';

        if (!strcmp(analysis_command, ".")) {
            double etime = (double)(util_get_time() - analysis_game.search.start_time) / 10.0;
            printf("stat01: %0.f %llu %d %d %d\n",
                etime,
                analysis_game.search.nodes,
                analysis_game.search.cur_depth,
                (analysis_game.search.root_move_count - analysis_game.search.root_move_search),
                analysis_game.search.root_move_count);
            fflush(stdout);
            continue;
        }

        if (!strcmp(analysis_command, "bk")) {
            // not supported
            continue;
        }

        if (!strcmp(analysis_command, "hint")) {
            if (analysis_game.pv_line.pv_line[0][0] != MOVE_NONE) {
                char move_string[100];
                util_get_move_string(analysis_game.pv_line.pv_line[0][0], move_string);
                printf("Hint: %s\n", move_string);
                fflush(stdout);
            }
            continue;
        }

        // Stop analysis for the commands below

        analysis_game.search.abort = TRUE;
        THREAD_WAIT(analysis_thread);

        if (!strcmp(analysis_command, "exit")) {
            break;
        }

        if (!strcmp(analysis_command, "undo")) {
            undo_move(&analysis_game.board);
            THREAD_CREATE(analysis_thread, analysis_start, &analysis_game);
            continue;
        }

        if (!strcmp(analysis_command, "new")) {
            set_fen(&analysis_game.board, FEN_NEW_GAME) ;
            THREAD_CREATE(analysis_thread, analysis_start, &analysis_game);
            continue;
        }
        
        if (!strncmp(analysis_command, "setboard", 8)) {
            if (strlen(analysis_command) > 9)
                set_fen(&analysis_game.board, &analysis_command[9]);
            THREAD_CREATE(analysis_thread, analysis_start, &analysis_game);
            continue;
        }

        MOVE input_move = util_parse_move(&analysis_game, analysis_command);
        if (input_move != MOVE_NONE && is_valid(&analysis_game.board, input_move)) {
            make_move(&analysis_game.board, input_move);
            if (is_illegal(&analysis_game.board, input_move))
                undo_move(&analysis_game.board);
        }
        THREAD_CREATE(analysis_thread, analysis_start, &analysis_game);
    }

}

//-------------------------------------------------------------------------------------------------
//  Start analysis search thread
//-------------------------------------------------------------------------------------------------
void analysis_start(GAME *game)
{
    SETTINGS    settings;

    settings.max_depth = MAX_DEPTH;
    settings.moves_per_level = 0;
    settings.post_flag = POST_XBOARD;
    settings.single_move_time = MAX_TIME;
    settings.total_move_time = MAX_TIME;
    settings.use_book = FALSE;

    search_run(game, &settings);
}

// EOF//
