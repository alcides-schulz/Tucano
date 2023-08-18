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

GAME nnue_game;

void nnue_test_fen(char *fen)
{
    new_game(&nnue_game, fen);
    printf("nnue_eval: %5d fen: %s\n", nnue_eval_full(&nnue_game), fen);       
}

void nnue_test_fens()
{
    if (!nnue_init("d:/temp/nn.bin", &nnue_param)) {
        return;
    }
    nnue_test_fen("2r3k1/5pp1/1p2p1np/p1q5/P1P4P/1P1Q1NP1/5PK1/R7 w - -");
    nnue_test_fen("2r2k2/r4p2/2b1p1p1/1p1p2Pp/3R1P1P/P1P5/1PB5/2K1R3 w - -");
    nnue_test_fen("2r1r1k1/pbpp1npp/1p1b3q/3P4/4RN1P/1P4P1/PB1Q1PB1/2R3K1 w - -");
    nnue_test_fen("2r1k2r/1p1qbppp/p3pn2/3pBb2/3P4/1QN1P3/PP2BPPP/2R2RK1 b k -");
    nnue_test_fen("2kr2nr/1pp3pp/p1pb4/4p2b/4P1P1/5N1P/PPPN1P2/R1B1R1K1 b - -");
    nnue_test_fen("1r2r1k1/3bnppp/p2q4/2RPp3/4P3/6P1/2Q1NPBP/2R3K1 w - -");
    nnue_test_fen("N1bk4/bp1p4/p1nN1p1p/4p3/1PB1P1pq/2PP4/P4PP1/R2Q1RK1 w -- - 0 19");
    nnue_test_fen("8/2B4k/R6p/1p3pp1/1P6/2P1r1K1/2n5/8 w -- - 0 46");
    nnue_test_fen("2b1k2r/2p2ppp/1qp4n/7B/1p2P3/5Q2/PPPr2PP/R2N1R1K b k -");
    nnue_test_fen("2brr1k1/ppq2ppp/2pb1n2/8/3NP3/2P2P2/P1Q2BPP/1R1R1BK1 w - -");
    nnue_test_fen("1qr3k1/p2nbppp/bp2p3/3p4/3P4/1P2PNP1/P2Q1PBP/1N2R1K1 b - -");
}

void nnue_test(void)
{
    nnue_test_fens();
}

//END
