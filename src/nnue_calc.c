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

#if defined(USE_SSE2)
#undef USE_MMX
#endif

#define VECTOR

#ifdef USE_AVX2
#define SIMD_WIDTH 256
typedef __m256i vec16_t;
typedef __m256i vec8_t;
typedef uint32_t mask_t;
#define vec_add_16(a,b) _mm256_add_epi16(a,b)
#define vec_sub_16(a,b) _mm256_sub_epi16(a,b)
#define vec_packs(a,b) _mm256_packs_epi16(a,b)
#define vec_mask_pos(a) _mm256_movemask_epi8(_mm256_cmpgt_epi8(a,_mm256_setzero_si256()))
#define NUM_REGS 16
#elif USE_SSE2
#define SIMD_WIDTH 128
typedef __m128i vec16_t;
typedef __m128i vec8_t;
typedef uint16_t mask_t;
#define vec_add_16(a,b) _mm_add_epi16(a,b)
#define vec_sub_16(a,b) _mm_sub_epi16(a,b)
#define vec_packs(a,b) _mm_packs_epi16(a,b)
#define vec_mask_pos(a) _mm_movemask_epi8(_mm_cmpgt_epi8(a,_mm_setzero_si128()))
#define NUM_REGS 16
#elif USE_MMX
#define SIMD_WIDTH 64
typedef __m64 vec16_t;
typedef __m64 vec8_t;
typedef uint8_t mask_t;
#define vec_add_16(a,b) _mm_add_pi16(a,b)
#define vec_sub_16(a,b) _mm_sub_pi16(a,b)
#define vec_packs(a,b) _mm_packs_pi16(a,b)
#define vec_mask_pos(a) _mm_movemask_pi8(_mm_cmpgt_pi8(a,_mm_setzero_si64()))
#define NUM_REGS 8
#elif USE_NEON
#define SIMD_WIDTH 128
typedef int16x8_t vec16_t;
typedef int8x16_t vec8_t;
typedef uint16_t mask_t;
#define vec_add_16(a,b) vaddq_s16(a,b)
#define vec_sub_16(a,b) vsubq_s16(a,b)
#define vec_packs(a,b) vcombine_s8(vqmovn_s16(a),vqmovn_s16(b))
#define vec_mask_pos(a) neon_movemask(vcgtq_s8(a,vdupq_n_u8(0)))
#ifdef IS_64BIT
#define NUM_REGS 16
#else
#define NUM_REGS 8
#endif
#else
#undef VECTOR
#define SIMD_WIDTH 16 // dummy
typedef uint8_t mask_t; // dummy
#endif

#ifdef VECTOR
#define TILE_HEIGHT (NUM_REGS * SIMD_WIDTH / 16)
#endif

uint32_t PIECE_TO_INDEX[2][14] = {
  { 0, 0, PS_W_QUEEN, PS_W_ROOK, PS_W_BISHOP, PS_W_KNIGHT, PS_W_PAWN,
       0, PS_B_QUEEN, PS_B_ROOK, PS_B_BISHOP, PS_B_KNIGHT, PS_B_PAWN, 0},
  { 0, 0, PS_B_QUEEN, PS_B_ROOK, PS_B_BISHOP, PS_B_KNIGHT, PS_B_PAWN,
       0, PS_W_QUEEN, PS_W_ROOK, PS_W_BISHOP, PS_W_KNIGHT, PS_W_PAWN, 0}
};

//-------------------------------------------------------------------------------------------------
//  Translate square position regarding color
//-------------------------------------------------------------------------------------------------
int nnue_orient(int c, int s)
{
    return s ^ (c == NNUE_WHITE ? 0x00 : 0x3f);
}

//-------------------------------------------------------------------------------------------------
//  Index for the piece in relation to color/king.
//-------------------------------------------------------------------------------------------------
unsigned nnue_make_index(int c, int s, int pc, int ksq)
{
    return nnue_orient(c, s) + PIECE_TO_INDEX[c][pc] + PS_END * ksq;
}

//-------------------------------------------------------------------------------------------------
//  Calculate and list indexes for all pieces in the position
//-------------------------------------------------------------------------------------------------
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

//-------------------------------------------------------------------------------------------------
//  Calculate indexes for pieces that have changed in the position
//-------------------------------------------------------------------------------------------------
void nnue_half_kp_append_changed_indices(const NNUE_POSITION *pos, const int c,
    const NNUE_CHANGE *changes,
    NNUE_INDEXES *removed, NNUE_INDEXES *added)
{
    int ksq = pos->squares[c];
    ksq = nnue_orient(c, ksq);
    for (int i = 0; i < changes->count; i++) {
        int pc = changes->piece[i];
        if (NNUE_IS_KING(pc)) {
            continue;
        }
        if (changes->from[i] != 64) {
            removed->values[removed->size++] = nnue_make_index(c, changes->from[i], pc, ksq);
        }
        if (changes->to[i] != 64) {
            added->values[added->size++] = nnue_make_index(c, changes->to[i], pc, ksq);
        }
    }
}

//-------------------------------------------------------------------------------------------------
//  Used to refresh accumulators
//-------------------------------------------------------------------------------------------------
void nnue_append_active_indices(const NNUE_POSITION *pos, NNUE_INDEXES active[2])
{
    for (unsigned c = 0; c < 2; c++) {
        nnue_half_kp_append_active_indices(pos, c, &active[c]);
    }
}

//-------------------------------------------------------------------------------------------------
//  Indicate a king move in the changed pieces, in this case will reset accumulators
//-------------------------------------------------------------------------------------------------
int nnue_has_king_move(const NNUE_CHANGE *changes)
{
    if (NNUE_IS_KING(changes->piece[0])) {
        return TRUE;
    }
    if (changes->count > 0 && NNUE_IS_KING(changes->piece[1])) {
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Use list of changed pieces (aka dirty pieces) to generate list of accumulators indexes
//-------------------------------------------------------------------------------------------------
void nnue_append_changed_indices(const NNUE_POSITION *pos, NNUE_INDEXES removed[2], NNUE_INDEXES added[2], int reset[2])
{
    NNUE_CHANGE *changes = &pos->current_nnue_data->changes;
    for (unsigned c = 0; c < 2; c++) {
        reset[c] = nnue_has_king_move(changes);
        if (reset[c]) {
            nnue_half_kp_append_active_indices(pos, c, &added[c]);
        }
        else {
            nnue_half_kp_append_changed_indices(pos, c, changes, &removed[c], &added[c]);
        }
    }
}

//-------------------------------------------------------------------------------------------------
//  Calculate cumulative value without using difference calculation
//-------------------------------------------------------------------------------------------------
void nnue_refresh_accumulator(NNUE_POSITION *pos)
{
    NNUE_ACCUM *accumulator = &(pos->current_nnue_data->accumulator);
    NNUE_INDEXES activeIndices[2];
    activeIndices[0].size = activeIndices[1].size = 0;
    nnue_append_active_indices(pos, activeIndices);
    for (unsigned c = 0; c < 2; c++) {
#ifdef VECTOR
        for (unsigned i = 0; i < KHALF_DIMENSIONS / TILE_HEIGHT; i++) {
            vec16_t *ft_biases_tile = (vec16_t *)&nnue_param.ft_biases[i * TILE_HEIGHT];
            vec16_t *accTile = (vec16_t *)&accumulator->accumulation[c][i * TILE_HEIGHT];
            vec16_t acc[NUM_REGS];
            for (unsigned j = 0; j < NUM_REGS; j++) {
                acc[j] = ft_biases_tile[j];
            }
            for (size_t k = 0; k < activeIndices[c].size; k++) {
                unsigned index = activeIndices[c].values[k];
                unsigned offset = KHALF_DIMENSIONS * index + i * TILE_HEIGHT;
                vec16_t *column = (vec16_t *)&nnue_param.ft_weights[offset];
                for (unsigned j = 0; j < NUM_REGS; j++) {
                    acc[j] = vec_add_16(acc[j], column[j]);
                }
            }
            for (unsigned j = 0; j < NUM_REGS; j++) {
                accTile[j] = acc[j];
            }
        }
#else
        memcpy(accumulator->accumulation[c], nnue_param.ft_biases, KHALF_DIMENSIONS * sizeof(int16_t));
        for (size_t k = 0; k < activeIndices[c].size; k++) {
            unsigned index = activeIndices[c].values[k];
            unsigned offset = KHALF_DIMENSIONS * index;
            for (unsigned j = 0; j < KHALF_DIMENSIONS; j++) {
                accumulator->accumulation[c][j] += nnue_param.ft_weights[offset + j];
            }
        }
#endif
    }
    accumulator->computed = TRUE;
}

//-------------------------------------------------------------------------------------------------
//  Update current position accumulator using previous position info
//-------------------------------------------------------------------------------------------------
void nnue_update_accumulator(NNUE_POSITION *pos)
{
    NNUE_ACCUM *accumulator = &(pos->current_nnue_data->accumulator);
    NNUE_ACCUM *prevAcc = &(pos->previous_nnue_data->accumulator);
    NNUE_INDEXES removed_indices[2], added_indices[2];
    removed_indices[0].size = removed_indices[1].size = 0;
    added_indices[0].size = added_indices[1].size = 0;
    int reset[2];
    nnue_append_changed_indices(pos, removed_indices, added_indices, reset);
#ifdef VECTOR
    for (unsigned i = 0; i < KHALF_DIMENSIONS / TILE_HEIGHT; i++) {
        for (unsigned c = 0; c < 2; c++) {
            vec16_t *accTile = (vec16_t *)&accumulator->accumulation[c][i * TILE_HEIGHT];
            vec16_t acc[NUM_REGS];
            if (reset[c]) {
                vec16_t *ft_b_tile = (vec16_t *)&nnue_param.ft_biases[i * TILE_HEIGHT];
                for (unsigned j = 0; j < NUM_REGS; j++) {
                    acc[j] = ft_b_tile[j];
                }
            }
            else {
                vec16_t *prevAccTile = (vec16_t *)&prevAcc->accumulation[c][i * TILE_HEIGHT];
                for (unsigned j = 0; j < NUM_REGS; j++) {
                    acc[j] = prevAccTile[j];
                }
                // Difference calculation for the deactivated features
                for (unsigned k = 0; k < removed_indices[c].size; k++) {
                    unsigned index = removed_indices[c].values[k];
                    const unsigned offset = KHALF_DIMENSIONS * index + i * TILE_HEIGHT;
                    vec16_t *column = (vec16_t *)&nnue_param.ft_weights[offset];
                    for (unsigned j = 0; j < NUM_REGS; j++) {
                        acc[j] = vec_sub_16(acc[j], column[j]);
                    }
                }
            }
            // Difference calculation for the activated features
            for (unsigned k = 0; k < added_indices[c].size; k++) {
                unsigned index = added_indices[c].values[k];
                const unsigned offset = KHALF_DIMENSIONS * index + i * TILE_HEIGHT;
                vec16_t *column = (vec16_t *)&nnue_param.ft_weights[offset];
                for (unsigned j = 0; j < NUM_REGS; j++) {
                    acc[j] = vec_add_16(acc[j], column[j]);
                }
            }
            for (unsigned j = 0; j < NUM_REGS; j++) {
                accTile[j] = acc[j];
            }
        }
    }
#else
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
#endif
    accumulator->computed = TRUE;
}

//-------------------------------------------------------------------------------------------------
//  Calculate output layer
//-------------------------------------------------------------------------------------------------
int32_t nnue_affine_propagate(int8_t *input, int32_t *biases, weight_t *weights)
{
#if defined(USE_AVX2)
    __m256i *iv = (__m256i *)input;
    __m256i *row = (__m256i *)weights;
#if defined(USE_VNNI)
    __m256i prod = _mm256_dpbusd_epi32(_mm256_setzero_si256(), iv[0], row[0]);
#else
    __m256i prod = _mm256_maddubs_epi16(iv[0], row[0]);
    prod = _mm256_madd_epi16(prod, _mm256_set1_epi16(1));
#endif
    __m128i sum = _mm_add_epi32(_mm256_castsi256_si128(prod), _mm256_extracti128_si256(prod, 1));
    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0x1b));
    return _mm_cvtsi128_si32(sum) + _mm_extract_epi32(sum, 1) + biases[0];
#elif defined(USE_SSE2)
    __m128i *iv = (__m128i *)input;
    __m128i *row = (__m128i *)weights;
#if defined(AVOID_USE_SSSE3)
    const __m128i kOnes = _mm_set1_epi16(1);
    __m128i p0 = _mm_madd_epi16(_mm_maddubs_epi16(iv[0], row[0]), kOnes);
    __m128i p1 = _mm_madd_epi16(_mm_maddubs_epi16(iv[1], row[1]), kOnes);
    __m128i sum = _mm_add_epi32(p0, p1);
#else
    __m128i p0 = _mm_madd_epi16(iv[0], row[0]);
    __m128i p1 = _mm_madd_epi16(iv[1], row[1]);
    __m128i p2 = _mm_madd_epi16(iv[2], row[2]);
    __m128i p3 = _mm_madd_epi16(iv[3], row[3]);
    __m128i sum = _mm_add_epi32(_mm_add_epi32(p0, p1), _mm_add_epi32(p2, p3));
#endif
    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0xb));
#if defined(USE_SSE41)
    return _mm_cvtsi128_si32(sum) + _mm_extract_epi32(sum, 1) + biases[0];
#else
    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0x1));
    return _mm_cvtsi128_si32(sum) + biases[0];
#endif
#elif defined(USE_MMX)
    __m64 *iv = (__m64 *)input;
    __m64 s0 = _mm_setzero_si64(), s1 = s0;
    __m64 *row = (__m64 *)weights;
    for (unsigned j = 0; j < 4; j++) {
        s0 = _mm_add_pi32(s0, _mm_madd_pi16(row[2 * j], iv[2 * j]));
        s1 = _mm_add_pi32(s1, _mm_madd_pi16(row[2 * j + 1], iv[2 * j + 1]));
    }
    __m64 sum = _mm_add_pi32(s0, s1);
    sum = _mm_add_pi32(sum, _mm_unpackhi_pi32(sum, sum));
    return _mm_cvtsi64_si32(sum) + biases[0];
#elif defined(USE_NEON)
    int8x8_t *iv = (int8x8_t *)input;
    int32x4_t sum = { biases[0] };
    int8x8_t *row = (int8x8_t *)weights;
    int16x8_t p0 = vmull_s8(iv[0], row[0]);
    int16x8_t p1 = vmull_s8(iv[1], row[1]);
    p0 = vmlal_s8(p0, iv[2], row[2]);
    sum = vpadalq_s16(sum, p0);
    p1 = vmlal_s8(p1, iv[3], row[3]);
    sum = vpadalq_s16(sum, p1);
    return sum[0] + sum[1] + sum[2] + sum[3];
#else
    int32_t sum = biases[0];
    for (unsigned j = 0; j < 32; j++)
        sum += weights[j] * input[j];
    return sum;
#endif
}

#if defined(__GNUC__)
#define bsf(b) __builtin_ctzll(b)
#define bsr(b) (63 - __builtin_clzll(b))
#elif defined(_WIN32)
#include <intrin.h>
int bsf(uint64_t b) {
    unsigned long x;
    _BitScanForward64(&x, b);
    return (int)x;
}
int bsr(uint64_t b) {
    unsigned long x;
    _BitScanReverse64(&x, b);
    return (int)x;
}
#endif

#ifdef VECTOR
int nnue_next_index(unsigned *idx, unsigned *offset, mask2_t *v, mask_t *mask, unsigned inDims)
{
    while (*v == 0) {
        *offset += 8 * sizeof(mask2_t);
        if (*offset >= inDims) return FALSE;
        memcpy(v, (char *)mask + (*offset / 8), sizeof(mask2_t));
    }
    *idx = *offset + bsf(*v);
    *v &= *v - 1;
    return TRUE;
}
#if defined(USE_MMX) && !defined(USE_SSE)
int _mm_movemask_pi8(__m64 v)
{
    const __m64 powers = _mm_set_pi8(-128, 64, 32, 16, 8, 4, 2, 1);
    __m64 m = _mm_and_si64(v, powers);
    m = _mm_or_si64(m, _mm_srli_si64(m, 32));
    m = _mm_or_si64(m, _mm_srli_pi32(m, 16));
    m = _mm_or_si64(m, _mm_srli_pi16(m, 8));
    return _mm_cvtsi64_si32(m) & 0xff;
}
#elif defined(USE_NEON)
int neon_movemask(uint8x16_t v)
{
    const uint8_t __attribute__((aligned(16))) powers[16] =
    { 1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128 };
    const uint8x16_t kPowers = vld1q_u8(powers);

    uint64x2_t mask = vpaddlq_u32(vpaddlq_u16(vpaddlq_u8(vandq_u8(v, kPowers))));
    return   vgetq_lane_u8((uint8x16_t)mask, 0)
        | (vgetq_lane_u8((uint8x16_t)mask, 8) << 8);
}
#endif
#endif

#if defined(USE_AVX2)
void nnue_affine_txfm(int8_t *input, void *output, unsigned inDims,
    unsigned outDims, const int32_t *biases, const weight_t *weights,
    mask_t *inMask, mask_t *outMask, const int pack8_and_calc_mask)
{
    assert(outDims == 32);

    (void)outDims;
    const __m256i kZero = _mm256_setzero_si256();
    __m256i out_0 = ((__m256i *)biases)[0];
    __m256i out_1 = ((__m256i *)biases)[1];
    __m256i out_2 = ((__m256i *)biases)[2];
    __m256i out_3 = ((__m256i *)biases)[3];
    __m256i first, second;
    mask2_t v;
    unsigned idx;

    memcpy(&v, inMask, sizeof(mask2_t));
    for (unsigned offset = 0; offset < inDims;) {
        if (!nnue_next_index(&idx, &offset, &v, inMask, inDims))
            break;
        first = ((__m256i *)weights)[idx];
        uint16_t factor = input[idx];
        if (nnue_next_index(&idx, &offset, &v, inMask, inDims)) {
            second = ((__m256i *)weights)[idx];
            factor |= input[idx] << 8;
        }
        else {
            second = kZero;
        }
        __m256i mul = _mm256_set1_epi16(factor), prod, signs;
        prod = _mm256_maddubs_epi16(mul, _mm256_unpacklo_epi8(first, second));
        signs = _mm256_cmpgt_epi16(kZero, prod);
        out_0 = _mm256_add_epi32(out_0, _mm256_unpacklo_epi16(prod, signs));
        out_1 = _mm256_add_epi32(out_1, _mm256_unpackhi_epi16(prod, signs));
        prod = _mm256_maddubs_epi16(mul, _mm256_unpackhi_epi8(first, second));
        signs = _mm256_cmpgt_epi16(kZero, prod);
        out_2 = _mm256_add_epi32(out_2, _mm256_unpacklo_epi16(prod, signs));
        out_3 = _mm256_add_epi32(out_3, _mm256_unpackhi_epi16(prod, signs));
    }

    __m256i out16_0 = _mm256_srai_epi16(_mm256_packs_epi32(out_0, out_1), SHIFT);
    __m256i out16_1 = _mm256_srai_epi16(_mm256_packs_epi32(out_2, out_3), SHIFT);

    __m256i *outVec = (__m256i *)output;
    outVec[0] = _mm256_packs_epi16(out16_0, out16_1);
    if (pack8_and_calc_mask)
        outMask[0] = _mm256_movemask_epi8(_mm256_cmpgt_epi8(outVec[0], kZero));
    else
        outVec[0] = _mm256_max_epi8(outVec[0], kZero);
}
#elif AVOID_USE_SSSE3
void nnue_affine_txfm(int8_t *input, void *output, unsigned inDims,
    unsigned outDims, const int32_t *biases, const weight_t *weights,
    mask_t *inMask, mask_t *outMask, const bool pack8_and_calc_mask)
{
    assert(outDims == 32);

    const __m128i kZeros[2] = { 0 };
    __m128i out_0 = ((__m128i *)biases)[0];
    __m128i out_1 = ((__m128i *)biases)[1];
    __m128i out_2 = ((__m128i *)biases)[2];
    __m128i out_3 = ((__m128i *)biases)[3];
    __m128i out_4 = ((__m128i *)biases)[4];
    __m128i out_5 = ((__m128i *)biases)[5];
    __m128i out_6 = ((__m128i *)biases)[6];
    __m128i out_7 = ((__m128i *)biases)[7];
    const __m128i *first, *second;
    mask2_t v;
    unsigned idx;

    memcpy(&v, inMask, sizeof(mask2_t));
    for (unsigned offset = 0; offset < inDims;) {
        if (!nnue_next_index(&idx, &offset, &v, inMask, inDims))
            break;
        first = (__m128i *)&weights[outDims * idx];
        uint16_t factor = input[idx];
        if (nnue_next_index(&idx, &offset, &v, inMask, inDims)) {
            second = (__m128i *)&weights[outDims * idx];
            factor |= input[idx] << 8;
        }
        else {
            second = kZeros;
        }
        __m128i mul = _mm_set1_epi16(factor), prod, signs;
        prod = _mm_maddubs_epi16(mul, _mm_unpacklo_epi8(first[0], second[0]));
        signs = _mm_cmpgt_epi16(kZeros[0], prod);
        out_0 = _mm_add_epi32(out_0, _mm_unpacklo_epi16(prod, signs));
        out_1 = _mm_add_epi32(out_1, _mm_unpackhi_epi16(prod, signs));
        prod = _mm_maddubs_epi16(mul, _mm_unpackhi_epi8(first[0], second[0]));
        signs = _mm_cmpgt_epi16(kZeros[0], prod);
        out_2 = _mm_add_epi32(out_2, _mm_unpacklo_epi16(prod, signs));
        out_3 = _mm_add_epi32(out_3, _mm_unpackhi_epi16(prod, signs));
        prod = _mm_maddubs_epi16(mul, _mm_unpacklo_epi8(first[1], second[1]));
        signs = _mm_cmpgt_epi16(kZeros[0], prod);
        out_4 = _mm_add_epi32(out_4, _mm_unpacklo_epi16(prod, signs));
        out_5 = _mm_add_epi32(out_5, _mm_unpackhi_epi16(prod, signs));
        prod = _mm_maddubs_epi16(mul, _mm_unpackhi_epi8(first[1], second[1]));
        signs = _mm_cmpgt_epi16(kZeros[0], prod);
        out_6 = _mm_add_epi32(out_6, _mm_unpacklo_epi16(prod, signs));
        out_7 = _mm_add_epi32(out_7, _mm_unpackhi_epi16(prod, signs));
    }

    __m128i out16_0 = _mm_srai_epi16(_mm_packs_epi32(out_0, out_1), SHIFT);
    __m128i out16_1 = _mm_srai_epi16(_mm_packs_epi32(out_2, out_3), SHIFT);
    __m128i out16_2 = _mm_srai_epi16(_mm_packs_epi32(out_4, out_5), SHIFT);
    __m128i out16_3 = _mm_srai_epi16(_mm_packs_epi32(out_6, out_7), SHIFT);

    __m128i *outVec = (__m128i *)output;
    if (pack8_and_calc_mask) {
        outVec[0] = _mm_packs_epi16(out16_0, out16_1);
        outMask[0] = _mm_movemask_epi8(_mm_cmpgt_epi8(outVec[0], kZeros[0]));
        outVec[1] = _mm_packs_epi16(out16_2, out16_3);
        outMask[1] = _mm_movemask_epi8(_mm_cmpgt_epi8(outVec[1], kZeros[0]));
    }
    else {
#if defined(USE_SSE41)
        outVec[0] = _mm_max_epi8(_mm_packs_epi16(out16_0, out16_1), kZeros[0]);
        outVec[1] = _mm_max_epi8(_mm_packs_epi16(out16_2, out16_3), kZeros[0]);
#else
        outVec[0] = _mm_packs_epi16(
            _mm_max_epi16(out16_0, kZeros[0]), _mm_max_epi16(out16_1, kZeros[0]));
        outVec[1] = _mm_packs_epi16(
            _mm_max_epi16(out16_2, kZeros[0]), _mm_max_epi16(out16_3, kZeros[0]));
#endif
    }
}
#elif defined(USE_SSE2)
void nnue_affine_txfm(clipped_t *input, void *output, unsigned inDims,
    unsigned outDims, const int32_t *biases, const weight_t *weights,
    mask_t *inMask, mask_t *outMask, const int pack8_and_calc_mask)
{
    assert(outDims == 32);

    const __m128i kZeros[4] = { 0 };
    __m128i out_0 = ((__m128i *)biases)[0];
    __m128i out_1 = ((__m128i *)biases)[1];
    __m128i out_2 = ((__m128i *)biases)[2];
    __m128i out_3 = ((__m128i *)biases)[3];
    __m128i out_4 = ((__m128i *)biases)[4];
    __m128i out_5 = ((__m128i *)biases)[5];
    __m128i out_6 = ((__m128i *)biases)[6];
    __m128i out_7 = ((__m128i *)biases)[7];
    const __m128i *first, *second;
    mask2_t v;
    unsigned idx;

    memcpy(&v, inMask, sizeof(mask2_t));
    for (unsigned offset = 0; offset < inDims;) {
        if (!nnue_next_index(&idx, &offset, &v, inMask, inDims))
            break;
        first = (__m128i *)&weights[outDims * idx];
        uint32_t factor = input[idx];
        if (nnue_next_index(&idx, &offset, &v, inMask, inDims)) {
            second = (__m128i *)&weights[outDims * idx];
            factor |= input[idx] << 16;
        }
        else {
            second = kZeros;
        }
        __m128i mul = _mm_set1_epi32(factor);
        out_0 = _mm_add_epi32(out_0, _mm_madd_epi16(mul, _mm_unpacklo_epi16(first[0], second[0])));
        out_1 = _mm_add_epi32(out_1, _mm_madd_epi16(mul, _mm_unpackhi_epi16(first[0], second[0])));
        out_2 = _mm_add_epi32(out_2, _mm_madd_epi16(mul, _mm_unpacklo_epi16(first[1], second[1])));
        out_3 = _mm_add_epi32(out_3, _mm_madd_epi16(mul, _mm_unpackhi_epi16(first[1], second[1])));
        out_4 = _mm_add_epi32(out_4, _mm_madd_epi16(mul, _mm_unpacklo_epi16(first[2], second[2])));
        out_5 = _mm_add_epi32(out_5, _mm_madd_epi16(mul, _mm_unpackhi_epi16(first[2], second[2])));
        out_6 = _mm_add_epi32(out_6, _mm_madd_epi16(mul, _mm_unpacklo_epi16(first[3], second[3])));
        out_7 = _mm_add_epi32(out_7, _mm_madd_epi16(mul, _mm_unpackhi_epi16(first[3], second[3])));
    }

    __m128i out16_0 = _mm_srai_epi16(_mm_packs_epi32(out_0, out_1), SHIFT);
    __m128i out16_1 = _mm_srai_epi16(_mm_packs_epi32(out_2, out_3), SHIFT);
    __m128i out16_2 = _mm_srai_epi16(_mm_packs_epi32(out_4, out_5), SHIFT);
    __m128i out16_3 = _mm_srai_epi16(_mm_packs_epi32(out_6, out_7), SHIFT);

    __m128i *outVec = (__m128i *)output;
    if (pack8_and_calc_mask) {
        outVec[0] = _mm_packs_epi16(out16_0, out16_1);
        outMask[0] = (mask_t)_mm_movemask_epi8(_mm_cmpgt_epi8(outVec[0], kZeros[0]));
        outVec[1] = _mm_packs_epi16(out16_2, out16_3);
        outMask[1] = (mask_t)_mm_movemask_epi8(_mm_cmpgt_epi8(outVec[1], kZeros[0]));
    }
    else {
        const __m128i kx07f = _mm_set1_epi16(127);
        outVec[0] = _mm_min_epi16(_mm_max_epi16(out16_0, kZeros[0]), kx07f);
        outVec[1] = _mm_min_epi16(_mm_max_epi16(out16_1, kZeros[0]), kx07f);
        outVec[2] = _mm_min_epi16(_mm_max_epi16(out16_2, kZeros[0]), kx07f);
        outVec[3] = _mm_min_epi16(_mm_max_epi16(out16_3, kZeros[0]), kx07f);
    }
}
#elif defined(USE_MMX)
void nnue_affine_txfm(clipped_t *input, void *output, unsigned inDims,
    unsigned outDims, const int32_t *biases, const weight_t *weights,
    mask_t *inMask, mask_t *outMask, const bool pack8_and_calc_mask)
{
    assert(outDims == 32);

#if 0
    const __m64 kZeros[2] = { 0 };
    for (unsigned t = 0; t < 4; t++) {
        __m64 out_0 = ((__m64 *)biases)[4 * t + 0];
        __m64 out_1 = ((__m64 *)biases)[4 * t + 1];
        __m64 out_2 = ((__m64 *)biases)[4 * t + 2];
        __m64 out_3 = ((__m64 *)biases)[4 * t + 3];
        const __m64 *first, *second;
        mask2_t v;
        unsigned idx;

        memcpy(&v, inMask, sizeof(mask2_t));
        for (unsigned offset = 0; offset < inDims;) {
            if (!nnue_next_index(&idx, &offset, &v, inMask, inDims))
                break;
            first = &((__m64 *)&weights[outDims * idx])[2 * t];
            uint32_t factor = input[idx];
            if (nnue_next_index(&idx, &offset, &v, inMask, inDims)) {
                second = &((__m64 *)&weights[outDims * idx])[2 * t];
                factor |= input[idx] << 16;
            }
            else {
                second = kZeros;
            }
            __m64 mul = _mm_set1_pi32(factor);
            out_0 = _mm_add_pi32(out_0, _mm_madd_pi16(mul, _mm_unpacklo_pi16(first[0], second[0])));
            out_1 = _mm_add_pi32(out_1, _mm_madd_pi16(mul, _mm_unpackhi_pi16(first[0], second[0])));
            out_2 = _mm_add_pi32(out_2, _mm_madd_pi16(mul, _mm_unpacklo_pi16(first[1], second[1])));
            out_3 = _mm_add_pi32(out_3, _mm_madd_pi16(mul, _mm_unpackhi_pi16(first[1], second[1])));
        }

        __m64 out16_0 = _mm_srai_pi16(_mm_packs_pi32(out_0, out_1), SHIFT);
        __m64 out16_1 = _mm_srai_pi16(_mm_packs_pi32(out_2, out_3), SHIFT);

        __m64 *outVec = (__m64 *)output;
        if (pack8_and_calc_mask) {
            outVec[t] = _mm_packs_pi16(out16_0, out16_1);
            outMask[t] = _mm_movemask_pi8(_mm_cmpgt_pi8(outVec[t], kZeros[0]));
        }
        else {
#ifdef USE_SSE
            const __m64 kx07f = _mm_set1_pi16(127);
            outVec[2 * t] = _mm_min_pi16(_mm_max_pi16(out16_0, kZeros[0]), kx07f);
            outVec[2 * t + 1] = _mm_min_pi16(_mm_max_pi16(out16_1, kZeros[0]), kx07f);
#else
            const __m64 k0x7f80 = _mm_set1_pi16(0x7f80);
            const __m64 k0x0080 = _mm_set1_pi16(0x0080);
            const __m64 k0x8000 = _mm_set1_pi16(-0x8000);
            outVec[2 * t] = _mm_subs_pu16(_mm_add_pi16(_mm_adds_pi16(out16_0, k0x7f80), k0x0080), k0x8000);
            outVec[2 * t + 1] = _mm_subs_pu16(_mm_add_pi16(_mm_adds_pi16(out16_1, k0x7f80), k0x0080), k0x8000);
#endif
        }
    }
#else
    const __m64 kZeros[8] = { 0 };
    __m64 out_0 = ((__m64 *)biases)[0];
    __m64 out_1 = ((__m64 *)biases)[1];
    __m64 out_2 = ((__m64 *)biases)[2];
    __m64 out_3 = ((__m64 *)biases)[3];
    __m64 out_4 = ((__m64 *)biases)[4];
    __m64 out_5 = ((__m64 *)biases)[5];
    __m64 out_6 = ((__m64 *)biases)[6];
    __m64 out_7 = ((__m64 *)biases)[7];
    __m64 out_8 = ((__m64 *)biases)[8];
    __m64 out_9 = ((__m64 *)biases)[9];
    __m64 out_10 = ((__m64 *)biases)[10];
    __m64 out_11 = ((__m64 *)biases)[11];
    __m64 out_12 = ((__m64 *)biases)[12];
    __m64 out_13 = ((__m64 *)biases)[13];
    __m64 out_14 = ((__m64 *)biases)[14];
    __m64 out_15 = ((__m64 *)biases)[15];
    const __m64 *first, *second;
    mask2_t v;
    unsigned idx;

    memcpy(&v, inMask, sizeof(mask2_t));
    for (unsigned offset = 0; offset < inDims;) {
        if (!nnue_next_index(&idx, &offset, &v, inMask, inDims))
            break;
        first = (__m64 *)&weights[outDims * idx];
        uint32_t factor = input[idx];
        if (nnue_next_index(&idx, &offset, &v, inMask, inDims)) {
            second = (__m64 *)&weights[outDims * idx];
            factor |= input[idx] << 16;
        }
        else {
            second = kZeros;
        }
        __m64 mul = _mm_set1_pi32(factor);
        out_0 = _mm_add_pi32(out_0, _mm_madd_pi16(mul, _mm_unpacklo_pi16(first[0], second[0])));
        out_1 = _mm_add_pi32(out_1, _mm_madd_pi16(mul, _mm_unpackhi_pi16(first[0], second[0])));
        out_2 = _mm_add_pi32(out_2, _mm_madd_pi16(mul, _mm_unpacklo_pi16(first[1], second[1])));
        out_3 = _mm_add_pi32(out_3, _mm_madd_pi16(mul, _mm_unpackhi_pi16(first[1], second[1])));
        out_4 = _mm_add_pi32(out_4, _mm_madd_pi16(mul, _mm_unpacklo_pi16(first[2], second[2])));
        out_5 = _mm_add_pi32(out_5, _mm_madd_pi16(mul, _mm_unpackhi_pi16(first[2], second[2])));
        out_6 = _mm_add_pi32(out_6, _mm_madd_pi16(mul, _mm_unpacklo_pi16(first[3], second[3])));
        out_7 = _mm_add_pi32(out_7, _mm_madd_pi16(mul, _mm_unpackhi_pi16(first[3], second[3])));
        out_8 = _mm_add_pi32(out_8, _mm_madd_pi16(mul, _mm_unpacklo_pi16(first[4], second[4])));
        out_9 = _mm_add_pi32(out_9, _mm_madd_pi16(mul, _mm_unpackhi_pi16(first[4], second[4])));
        out_10 = _mm_add_pi32(out_10, _mm_madd_pi16(mul, _mm_unpacklo_pi16(first[5], second[5])));
        out_11 = _mm_add_pi32(out_11, _mm_madd_pi16(mul, _mm_unpackhi_pi16(first[5], second[5])));
        out_12 = _mm_add_pi32(out_12, _mm_madd_pi16(mul, _mm_unpacklo_pi16(first[6], second[6])));
        out_13 = _mm_add_pi32(out_13, _mm_madd_pi16(mul, _mm_unpackhi_pi16(first[6], second[6])));
        out_14 = _mm_add_pi32(out_14, _mm_madd_pi16(mul, _mm_unpacklo_pi16(first[7], second[7])));
        out_15 = _mm_add_pi32(out_15, _mm_madd_pi16(mul, _mm_unpackhi_pi16(first[7], second[7])));
    }

    __m64 out16_0 = _mm_srai_pi16(_mm_packs_pi32(out_0, out_1), SHIFT);
    __m64 out16_1 = _mm_srai_pi16(_mm_packs_pi32(out_2, out_3), SHIFT);
    __m64 out16_2 = _mm_srai_pi16(_mm_packs_pi32(out_4, out_5), SHIFT);
    __m64 out16_3 = _mm_srai_pi16(_mm_packs_pi32(out_6, out_7), SHIFT);
    __m64 out16_4 = _mm_srai_pi16(_mm_packs_pi32(out_8, out_9), SHIFT);
    __m64 out16_5 = _mm_srai_pi16(_mm_packs_pi32(out_10, out_11), SHIFT);
    __m64 out16_6 = _mm_srai_pi16(_mm_packs_pi32(out_12, out_13), SHIFT);
    __m64 out16_7 = _mm_srai_pi16(_mm_packs_pi32(out_14, out_15), SHIFT);

    __m64 *outVec = (__m64 *)output;
    if (pack8_and_calc_mask) {
        outVec[0] = _mm_packs_pi16(out16_0, out16_1);
        outMask[0] = _mm_movemask_pi8(_mm_cmpgt_pi8(outVec[0], kZeros[0]));
        outVec[1] = _mm_packs_pi16(out16_2, out16_3);
        outMask[1] = _mm_movemask_pi8(_mm_cmpgt_pi8(outVec[1], kZeros[0]));
        outVec[2] = _mm_packs_pi16(out16_4, out16_5);
        outMask[2] = _mm_movemask_pi8(_mm_cmpgt_pi8(outVec[2], kZeros[0]));
        outVec[3] = _mm_packs_pi16(out16_6, out16_7);
        outMask[3] = _mm_movemask_pi8(_mm_cmpgt_pi8(outVec[3], kZeros[0]));
    }
    else {
#ifdef USE_SSE
        const __m64 kx07f = _mm_set1_pi16(127);
        outVec[0] = _mm_min_pi16(_mm_max_pi16(out16_0, kZeros[0]), kx07f);
        outVec[1] = _mm_min_pi16(_mm_max_pi16(out16_1, kZeros[0]), kx07f);
        outVec[2] = _mm_min_pi16(_mm_max_pi16(out16_2, kZeros[0]), kx07f);
        outVec[3] = _mm_min_pi16(_mm_max_pi16(out16_3, kZeros[0]), kx07f);
        outVec[4] = _mm_min_pi16(_mm_max_pi16(out16_4, kZeros[0]), kx07f);
        outVec[5] = _mm_min_pi16(_mm_max_pi16(out16_5, kZeros[0]), kx07f);
        outVec[6] = _mm_min_pi16(_mm_max_pi16(out16_6, kZeros[0]), kx07f);
        outVec[7] = _mm_min_pi16(_mm_max_pi16(out16_7, kZeros[0]), kx07f);
#else
        const __m64 k0x7f80 = _mm_set1_pi16(0x7f80);
        const __m64 k0x0080 = _mm_set1_pi16(0x0080);
        const __m64 k0x8000 = _mm_set1_pi16(-0x8000);
        outVec[0] = _mm_subs_pu16(_mm_add_pi16(_mm_adds_pi16(out16_0, k0x7f80), k0x0080), k0x8000);
        outVec[1] = _mm_subs_pu16(_mm_add_pi16(_mm_adds_pi16(out16_1, k0x7f80), k0x0080), k0x8000);
        outVec[2] = _mm_subs_pu16(_mm_add_pi16(_mm_adds_pi16(out16_2, k0x7f80), k0x0080), k0x8000);
        outVec[3] = _mm_subs_pu16(_mm_add_pi16(_mm_adds_pi16(out16_3, k0x7f80), k0x0080), k0x8000);
        outVec[4] = _mm_subs_pu16(_mm_add_pi16(_mm_adds_pi16(out16_4, k0x7f80), k0x0080), k0x8000);
        outVec[5] = _mm_subs_pu16(_mm_add_pi16(_mm_adds_pi16(out16_5, k0x7f80), k0x0080), k0x8000);
        outVec[6] = _mm_subs_pu16(_mm_add_pi16(_mm_adds_pi16(out16_6, k0x7f80), k0x0080), k0x8000);
        outVec[7] = _mm_subs_pu16(_mm_add_pi16(_mm_adds_pi16(out16_7, k0x7f80), k0x0080), k0x8000);
#endif
    }
#endif
}
#elif defined(USE_NEON)
void nnue_affine_txfm(clipped_t *input, void *output, unsigned inDims,
    unsigned outDims, const int32_t *biases, const weight_t *weights,
    mask_t *inMask, mask_t *outMask, const bool pack8_and_calc_mask)
{
    assert(outDims == 32);

    int32x4_t out_0 = ((int32x4_t *)biases)[0];
    int32x4_t out_1 = ((int32x4_t *)biases)[1];
    int32x4_t out_2 = ((int32x4_t *)biases)[2];
    int32x4_t out_3 = ((int32x4_t *)biases)[3];
    int32x4_t out_4 = ((int32x4_t *)biases)[4];
    int32x4_t out_5 = ((int32x4_t *)biases)[5];
    int32x4_t out_6 = ((int32x4_t *)biases)[6];
    int32x4_t out_7 = ((int32x4_t *)biases)[7];
    const int8x8_t *first;
    mask2_t v;
    unsigned idx;

    memcpy(&v, inMask, sizeof(mask2_t));
    for (unsigned offset = 0; offset < inDims;) {
        if (!nnue_next_index(&idx, &offset, &v, inMask, inDims))
            break;
        first = (int8x8_t *)&weights[outDims * idx];
        int16_t factor = input[idx];

        int16x8_t prod;
        prod = vmulq_n_s16(vmovl_s8(first[0]), factor);
        out_0 = vaddq_s32(out_0, vmovl_s16(vget_low_s16(prod)));
        out_1 = vaddq_s32(out_1, vmovl_high_s16(prod));
        prod = vmulq_n_s16(vmovl_s8(first[1]), factor);
        out_2 = vaddq_s32(out_2, vmovl_s16(vget_low_s16(prod)));
        out_3 = vaddq_s32(out_3, vmovl_high_s16(prod));
        prod = vmulq_n_s16(vmovl_s8(first[2]), factor);
        out_4 = vaddq_s32(out_4, vmovl_s16(vget_low_s16(prod)));
        out_5 = vaddq_s32(out_5, vmovl_high_s16(prod));
        prod = vmulq_n_s16(vmovl_s8(first[3]), factor);
        out_6 = vaddq_s32(out_6, vmovl_s16(vget_low_s16(prod)));
        out_7 = vaddq_s32(out_7, vmovl_high_s16(prod));
    }

    int16x8_t out16_0 = vcombine_s16(vqshrn_n_s32(out_0, SHIFT), vqshrn_n_s32(out_1, SHIFT));
    int16x8_t out16_1 = vcombine_s16(vqshrn_n_s32(out_2, SHIFT), vqshrn_n_s32(out_3, SHIFT));
    int16x8_t out16_2 = vcombine_s16(vqshrn_n_s32(out_4, SHIFT), vqshrn_n_s32(out_5, SHIFT));
    int16x8_t out16_3 = vcombine_s16(vqshrn_n_s32(out_6, SHIFT), vqshrn_n_s32(out_7, SHIFT));

    if (pack8_and_calc_mask) {
        const int8x16_t kZero = { 0 };
        int8x16_t *outVec = (int8x16_t *)output;
        outVec[0] = vcombine_s8(vqmovn_s16(out16_0), vqmovn_s16(out16_1));
        outMask[0] = neon_movemask(vcgtq_s8(outVec[0], kZero));
        outVec[1] = vcombine_s8(vqmovn_s16(out16_2), vqmovn_s16(out16_3));
        outMask[1] = neon_movemask(vcgtq_s8(outVec[1], kZero));
    }
    else {
        // The next step takes int8x8_t as input, so store as int8x8_t
        const int8x8_t kZero = { 0 };
        int8x8_t *outVec = (int8x8_t *)output;
        outVec[0] = vmax_s8(vqmovn_s16(out16_0), kZero);
        outVec[1] = vmax_s8(vqmovn_s16(out16_1), kZero);
        outVec[2] = vmax_s8(vqmovn_s16(out16_2), kZero);
        outVec[3] = vmax_s8(vqmovn_s16(out16_3), kZero);
    }
}
#else /* generic fallback */
void nnue_affine_txfm(clipped_t *input, void *output, unsigned inDims,
    unsigned outDims, int32_t *biases, const weight_t *weights,
    mask_t *inMask, mask_t *outMask, const int pack8_and_calc_mask)
{
    (void)inMask; (void)outMask; (void)pack8_and_calc_mask;

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
#endif


//-------------------------------------------------------------------------------------------------
//  Convert input features using updated accumulators
//-------------------------------------------------------------------------------------------------
// Convert input features
void nnue_transform(NNUE_POSITION *pos, clipped_t *output, mask_t *outMask)
{
    int16_t(*accumulation)[2][256] = &pos->current_nnue_data->accumulator.accumulation;
    (void)outMask; // avoid compiler warning

    int perspectives[2];
    perspectives[0] = pos->player;
    perspectives[1] = !pos->player;
    for (unsigned p = 0; p < 2; p++) {
        const unsigned offset = KHALF_DIMENSIONS * p;
#ifdef VECTOR
        const unsigned numChunks = (16 * KHALF_DIMENSIONS) / SIMD_WIDTH;
        vec8_t *out = (vec8_t *)&output[offset];
        for (unsigned i = 0; i < numChunks / 2; i++) {
            vec16_t s0 = ((vec16_t *)(*accumulation)[perspectives[p]])[i * 2];
            vec16_t s1 = ((vec16_t *)(*accumulation)[perspectives[p]])[i * 2 + 1];
            out[i] = vec_packs(s0, s1);
            *outMask++ = (mask_t)vec_mask_pos(out[i]);
        }
#else
        for (unsigned i = 0; i < KHALF_DIMENSIONS; i++) {
            int16_t sum = (*accumulation)[perspectives[p]][i];
            output[offset + i] = (clipped_t)clamp(sum, 0, 127);
        }
#endif
    }
}

//-------------------------------------------------------------------------------------------------
//  Network calculation, assume position is already updated, see calling method nnue_evaluate.
//-------------------------------------------------------------------------------------------------
int nnue_calculate(NNUE_POSITION *pos)
{
    NNUE_CALC_DATA ncd;
#ifdef _MSC_VER
    AL08 mask_t input_mask[FT_OUT_DIMS / (8 * sizeof(mask_t))];
    AL08 mask_t hidden1_mask[8 / sizeof(mask_t)] = { 0 };
#else
    mask_t input_mask[FT_OUT_DIMS / (8 * sizeof(mask_t))] AL08;
    mask_t hidden1_mask[8 / sizeof(mask_t)] AL08 = { 0 };
#endif
    nnue_transform(pos, ncd.input, input_mask);
    nnue_affine_txfm(ncd.input, ncd.hidden1_out, FT_OUT_DIMS, 32, nnue_param.hidden1_biases, nnue_param.hidden1_weights, input_mask, hidden1_mask, TRUE);
    nnue_affine_txfm(ncd.hidden1_out, ncd.hidden2_out, 32, 32, nnue_param.hidden2_biases, nnue_param.hidden2_weights, hidden1_mask, NULL, FALSE);
    int32_t out_value = nnue_affine_propagate((int8_t *)ncd.hidden2_out, nnue_param.output_biases, nnue_param.output_weights);
    return out_value / FV_SCALE;
}

// END