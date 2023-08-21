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

typedef struct s_net_data {
#ifdef _MSC_VER
    AL64 clipped_t  input[FT_OUT_DIMS];
#else
    clipped_t       input[FT_OUT_DIMS] AL64;
#endif
    clipped_t       hidden1_out[32];
    int8_t          hidden2_out[32];
}   NNUE_CALC_DATA;
typedef uint8_t mask_t;

uint32_t PIECE_TO_INDEX[2][14] = {
  { 0, 0, PS_W_QUEEN, PS_W_ROOK, PS_W_BISHOP, PS_W_KNIGHT, PS_W_PAWN,
       0, PS_B_QUEEN, PS_B_ROOK, PS_B_BISHOP, PS_B_KNIGHT, PS_B_PAWN, 0},
  { 0, 0, PS_B_QUEEN, PS_B_ROOK, PS_B_BISHOP, PS_B_KNIGHT, PS_B_PAWN,
       0, PS_W_QUEEN, PS_W_ROOK, PS_W_BISHOP, PS_W_KNIGHT, PS_W_PAWN, 0}
};

int nnue_orient(int c, int s)
{
    return s ^ (c == NNUE_WHITE ? 0x00 : 0x3f);
}

unsigned nnue_make_index(int c, int s, int pc, int ksq)
{
    return nnue_orient(c, s) + PIECE_TO_INDEX[c][pc] + PS_END * ksq;
}

void nnue_half_kp_append_active_indices(const NNUE_POSITION *pos, const int c, NNUE_INDEXES *active)
{
    int ksq = pos->squares[c];
    ksq = nnue_orient(c, ksq);
    for (int i = 2; pos->pieces[i]; i++) {
        int sq = pos->squares[i];
        int pc = pos->pieces[i];
        active->values[active->size++] = nnue_make_index(c, sq, pc, ksq);
    }
}

void nnue_half_kp_append_changed_indices(const NNUE_POSITION *pos, const int c, const NNUE_DIRTY *dp, NNUE_INDEXES *removed, NNUE_INDEXES *added)
{
    int ksq = pos->squares[c];
    ksq = nnue_orient(c, ksq);
    for (int i = 0; i < dp->count; i++) {
        int pc = dp->piece[i];
        if (NNUE_IS_KING(pc)) {
            continue;
        }
        if (dp->from[i] != 64) {
            removed->values[removed->size++] = nnue_make_index(c, dp->from[i], pc, ksq);
        }
        if (dp->to[i] != 64) {
            added->values[added->size++] = nnue_make_index(c, dp->to[i], pc, ksq);
        }
    }
}

void nnue_append_active_indices(const NNUE_POSITION *pos, NNUE_INDEXES active[2])
{
    for (unsigned c = 0; c < 2; c++) {
        nnue_half_kp_append_active_indices(pos, c, &active[c]);
    }
}

int nnue_king_move(const NNUE_DIRTY *dp)
{
    if (NNUE_IS_KING(dp->piece[0])) {
        return TRUE;
    }
    if (dp->count > 1 && NNUE_IS_KING(dp->piece[1])) {
        return TRUE;
    }
    return FALSE;
}

void nnue_append_changed_indices(const NNUE_POSITION *pos, NNUE_INDEXES removed[2], NNUE_INDEXES added[2], int reset[2])
{
    const NNUE_DIRTY *dp = &(pos->nnue_data[0]->dirty_piece);
    if (pos->nnue_data[1]->accumulator.computed) {
        for (unsigned c = 0; c < 2; c++) {
            //reset[c] = dp->piece[0] == (int)NNUE_KING(c);
            reset[c] = nnue_king_move(dp);
            if (reset[c]) {
                nnue_half_kp_append_active_indices(pos, c, &added[c]);
            }
            else {
                nnue_half_kp_append_changed_indices(pos, c, dp, &removed[c], &added[c]);
            }
        }
    }
    else {
        const NNUE_DIRTY *dp2 = &(pos->nnue_data[1]->dirty_piece);
        for (unsigned c = 0; c < 2; c++) {
            //reset[c] = dp->piece[0] == (int)NNUE_KING(c) || dp2->piece[0] == (int)NNUE_KING(c);
            reset[c] = nnue_king_move(dp) || nnue_king_move(dp2);
            if (reset[c]) {
                nnue_half_kp_append_active_indices(pos, c, &added[c]);
            }
            else {
                nnue_half_kp_append_changed_indices(pos, c, dp, &removed[c], &added[c]);
                nnue_half_kp_append_changed_indices(pos, c, dp2, &removed[c], &added[c]);
            }
        }
    }
}

// Calculate cumulative value without using difference calculation
void nnue_refresh_accumulator(NNUE_POSITION *pos)
{
    NNUE_ACCUM *accumulator = &(pos->nnue_data[0]->accumulator);
    NNUE_INDEXES activeIndices[2];
    activeIndices[0].size = activeIndices[1].size = 0;
    nnue_append_active_indices(pos, activeIndices);
    for (unsigned c = 0; c < 2; c++) {
        memcpy(accumulator->accumulation[c], nnue_param.ft_biases, KHALF_DIMENSIONS * sizeof(int16_t));
        for (size_t k = 0; k < activeIndices[c].size; k++) {
            unsigned index = activeIndices[c].values[k];
            unsigned offset = KHALF_DIMENSIONS * index;
            for (unsigned j = 0; j < KHALF_DIMENSIONS; j++) {
                accumulator->accumulation[c][j] += nnue_param.ft_weights[offset + j];
            }
        }
    }
    accumulator->computed = TRUE;
}

int nnue_update_accumulator(NNUE_POSITION *pos)
{
    NNUE_ACCUM *accumulator = &(pos->nnue_data[0]->accumulator);
    if (accumulator->computed) {
        return TRUE;
    }
    NNUE_ACCUM *prevAcc;
    if ((!pos->nnue_data[1] || !(prevAcc = &pos->nnue_data[1]->accumulator)->computed) 
        && (!pos->nnue_data[2] || !(prevAcc = &pos->nnue_data[2]->accumulator)->computed)) {
        return FALSE;
    }
    NNUE_INDEXES removed_indices[2], added_indices[2];
    removed_indices[0].size = removed_indices[1].size = 0;
    added_indices[0].size = added_indices[1].size = 0;
    int reset[2];
    nnue_append_changed_indices(pos, removed_indices, added_indices, reset);
    for (unsigned c = 0; c < 2; c++) {
        if (reset[c]) {
            memcpy(accumulator->accumulation[c], nnue_param.ft_biases, KHALF_DIMENSIONS * sizeof(int16_t));
        }
        else {
            memcpy(accumulator->accumulation[c], prevAcc->accumulation[c], KHALF_DIMENSIONS * sizeof(int16_t));
            // Difference calculation for the deactivated features
            for (unsigned k = 0; k < removed_indices[c].size; k++) {
                unsigned index = removed_indices[c].values[k];
                const unsigned offset = KHALF_DIMENSIONS * index;
                for (unsigned j = 0; j < KHALF_DIMENSIONS; j++) {
                    accumulator->accumulation[c][j] -= nnue_param.ft_weights[offset + j];
                }
            }
        }
        // Difference calculation for the activated features
        for (unsigned k = 0; k < added_indices[c].size; k++) {
            unsigned index = added_indices[c].values[k];
            const unsigned offset = KHALF_DIMENSIONS * index;
            for (unsigned j = 0; j < KHALF_DIMENSIONS; j++) {
                accumulator->accumulation[c][j] += nnue_param.ft_weights[offset + j];
            }
        }
    }
    accumulator->computed = TRUE;
    return TRUE;
}

int32_t nnue_affine_propagate(int8_t *input, int32_t *biases, weight_t *weights)
{
    int32_t sum = biases[0];
    for (unsigned j = 0; j < 32; j++) {
        sum += weights[j] * input[j];
    }
    return sum;
}

void nnue_affine_txfm(clipped_t *input, void *output, unsigned inDims, unsigned outDims, int32_t *biases, const weight_t *weights)
{
    int32_t tmp[32];
    for (unsigned i = 0; i < outDims; i++) {
        tmp[i] = biases[i];
    }
    for (unsigned idx = 0; idx < inDims; idx++) {
        if ((int8_t)input[idx]) {
            for (unsigned i = 0; i < outDims; i++) {
                tmp[i] += (int8_t)input[idx] * weights[outDims * idx + i];
            }
        }
    }
    clipped_t *outVec = (clipped_t *)output;
    for (unsigned i = 0; i < outDims; i++) {
        outVec[i] = (clipped_t)clamp(tmp[i] >> SHIFT, 0, 127);
    }
}

// Convert input features
void nnue_transform(NNUE_POSITION *pos, clipped_t *output)
{
    if (!nnue_update_accumulator(pos)) {
        nnue_refresh_accumulator(pos);
    }
    int16_t(*accumulation)[2][256] = &pos->nnue_data[0]->accumulator.accumulation;
    int perspectives[2];
    perspectives[0] = pos->player;
    perspectives[1] = !pos->player;
    for (unsigned p = 0; p < 2; p++) {
        const unsigned offset = KHALF_DIMENSIONS * p;
        for (unsigned i = 0; i < KHALF_DIMENSIONS; i++) {
            int16_t sum = (*accumulation)[perspectives[p]][i];
            output[offset + i] = (clipped_t)clamp(sum, 0, 127);
        }
    }
}

int nnue_calculate(NNUE_POSITION *pos)
{
    NNUE_CALC_DATA ncd;

    //transform(pos, B(input), input_mask);
    nnue_transform(pos, ncd.input);

    //affine_txfm(B(input), B(hidden1_out), FtOutDims, 32, hidden1_biases, hidden1_weights, input_mask, hidden1_mask, true);
    nnue_affine_txfm(ncd.input, ncd.hidden1_out, FT_OUT_DIMS, 32, nnue_param.hidden1_biases, nnue_param.hidden1_weights);

    //affine_txfm(B(hidden1_out), B(hidden2_out), 32, 32, hidden2_biases, hidden2_weights, hidden1_mask, NULL, false);
    nnue_affine_txfm(ncd.hidden1_out, ncd.hidden2_out, 32, 32, nnue_param.hidden2_biases, nnue_param.hidden2_weights);

    //out_value = affine_propagate((int8_t *)B(hidden2_out), output_biases, output_weights);
    int32_t out_value = nnue_affine_propagate(ncd.hidden2_out, nnue_param.output_biases, nnue_param.output_weights);

    return out_value / FV_SCALE;
}

int nnue_evaluate(int player, int* pieces, int* squares, NNUE_DATA* nnue[3])
{
    NNUE_POSITION pos;
    pos.nnue_data[0] = nnue[0];
    pos.nnue_data[1] = nnue[1];
    pos.nnue_data[2] = nnue[2];
    pos.player = player;
    pos.pieces = pieces;
    pos.squares = squares;
    return nnue_calculate(&pos);
}

// END