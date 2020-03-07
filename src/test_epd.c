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
//  Reads a file with Extended Position Notation (EPD) and find the best move for each position.
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Process the epd file.
//  TODO: review code
//-------------------------------------------------------------------------------------------------
void epd(char *file, SETTINGS *settings)
{
    FILE    *f;
    char    failname[1000];
    FILE    *failed;
    char    line[1000];
    char    fen[1000];
    char    bm[1000];
    int     i, j;
    char    move_desc[100];
    char    move_desc_file[100];

    int     count = 0;
    int     correct = 0;
    double  percent = 0;

    GAME *game = (GAME *)malloc(sizeof(GAME));
    if (game == NULL) {
        fprintf(stderr, "epd.malloc: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        return;
    }

    move_desc[0] = move_desc_file[0] = 0;

    f = fopen(file, "r");
    if (f == NULL) {
        printf("cannot open epd file: '%s'.\n", file);
        return;
    }

    sprintf(failname, "%s.failed", file);
    failed = fopen(failname, "w");

    while (fgets(line, 1000, f) != NULL) {
        i = 0;
        fen[i] = 0;
        while (i < 1000 && line[i] && strncmp(&line[i], " bm ", 4)) {
            fen[i] = line[i];
            i++;
            fen[i] = 0;
        }

        if (strncmp(&line[i], " bm ", 4))
            continue;

        i += 4;
        j = 0;
        bm[j] = 0;
        while (line[i] && line[i] != ';') {
            if (line[i] == ' ' || line[i] == 'x' || line[i] == '+' || line[i] == '#') {
                i++;
                continue;
            }
            bm[j++] = line[i++];
            bm[j] = 0;
        }

        count++;

        new_game(game, fen);

        search_run(game, settings);

        if (!game->search.best_move) {
            move_desc[0] = 0;
        }
        else {
            util_get_move_desc(game->search.best_move, move_desc, 0);
            util_get_move_desc(game->search.best_move, move_desc_file, 1);
        }

        if (strlen(fen) > 30) {
            fen[27] = 0;
            strcat(fen, "...");
        }

        if (strcmp(bm, move_desc) != 0 && strcmp(bm, move_desc_file) == 0)
            strcpy(move_desc, move_desc_file);

        printf("%3d [%s] bm=%-5s found=%-5s ", count, fen, bm, move_desc);
        if (strcmp(bm, move_desc) == 0) {
            correct++;
            printf("  ");
        }
        else  {
            printf("X ");
            fprintf(failed, "%s\n", line);
            fflush(failed);
        }
        percent = (double)correct / (double)count * 100.0;
        printf("%d/%d %4.2f %%\n", correct, count, percent);
    }

    printf("\nNumber of epd tests: %d   found correct: %d   %4.2f %%\n", count, correct, percent);

    fclose(f);
    fclose(failed);
    free(game);
}

//-------------------------------------------------------------------------------------------------
//  Search positions from an epd file and print a report for found scores
//-------------------------------------------------------------------------------------------------
void epd_search(char *file, SETTINGS *settings)
{
    FILE    *f;
    char    line[1000];


    f = fopen(file, "r");
    if (f == NULL) {
        printf("epd_search: cannot open epd file: '%s'.\n", file);
        return;
    }

    GAME *game = (GAME *)malloc(sizeof(GAME));
    if (game == NULL) {
        fprintf(stderr, "epd_search.malloc: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        return;
    }

    int count = 0;
    double wscore = 0;
    double lscore = 0;
    double dscore = 0;

    while (fgets(line, 1000, f) != NULL) {

        char *nl = strchr(line, '\n');
        if (nl != NULL) *nl = '\0';
        nl = strchr(line, '\r');
        if (nl != NULL) *nl = '\0';

        count++;

        new_game(game, line);

        search_run(game, settings);

        int score = game->search.best_score / 2;

        if (score > 10) wscore++;
        if (score < -10) lscore++;
        if (score >= -10 && score <= 10) dscore++;
        
        if (strlen(line) > 100) {
            line[97] = 0;
            strcat(line, "...");
        }

        double wpercent = wscore / (double)count * 100.0;
        double lpercent = lscore / (double)count * 100.0;
        double dpercent = dscore / (double)count * 100.0;

        printf("%d) %s score: %d win: %3.2f loss: %3.2f draw: %3.2f\n", count, line, score, wpercent, lpercent, dpercent);

    }

    fclose(f);
    free(game);
}

//end
