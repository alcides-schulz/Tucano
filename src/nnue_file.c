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

FD nnue_open_file(const char *name)
{
#ifndef _WIN32
    return open(name, O_RDONLY);
#else
    return CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
#endif
}

void nnue_close_file(FD fd)
{
#ifndef _WIN32
    close(fd);
#else
    CloseHandle(fd);
#endif
}

size_t nnue_file_size(FD fd)
{
#ifndef _WIN32
    struct stat statbuf;
    fstat(fd, &statbuf);
    return statbuf.st_size;
#else
    DWORD size_low, size_high;
    size_low = GetFileSize(fd, &size_high);
    return ((uint64_t)size_high << 32) | size_low;
#endif
}

const void *nnue_map_file(FD fd, map_t *map)
{
#ifndef _WIN32
    *map = nnue_file_size(fd);
    void *data = mmap(NULL, *map, PROT_READ, MAP_SHARED, fd, 0);
#ifdef MADV_RANDOM
    madvise(data, *map, MADV_RANDOM);
#endif
    return data == MAP_FAILED ? NULL : data;
#else
    DWORD size_low, size_high;
    size_low = GetFileSize(fd, &size_high);
    *map = CreateFileMapping(fd, NULL, PAGE_READONLY, size_high, size_low, NULL);
    if (*map == NULL) {
        return NULL;
    }
    return MapViewOfFile(*map, FILE_MAP_READ, 0, 0, 0);
#endif
}

void nnue_unmap_file(const void *data, map_t map)
{
    if (!data) return;
#ifndef _WIN32
    munmap((void *)data, map);
#else
    UnmapViewOfFile(data);
    CloseHandle(map);
#endif
}

uint32_t nnue_read_u32(const void *p)
{
    const uint8_t *q = (const uint8_t*)p;
    return q[0] | (q[1] << 8) | (q[2] << 16) | (q[3] << 24);
}

uint16_t nnue_read_u16(const void *p)
{
    const uint8_t *q = (const uint8_t*)p;
    return q[0] | (q[1] << 8);
}

int nnue_verify_net(const void *file_data, size_t size)
{
    if (size != 21022697) return FALSE;
    const char *d = (const char*)file_data;
    if (nnue_read_u32(d) != NNUE_VERSION) return FALSE;
    if (nnue_read_u32(d + 4) != 0x3e5aa6eeU) return FALSE;
    if (nnue_read_u32(d + 8) != 177) return FALSE;
    if (nnue_read_u32(d + TRANSFORMER_START) != 0x5d69d7b8) return FALSE;
    if (nnue_read_u32(d + NETWORK_START) != 0x63337156) return FALSE;
    return TRUE;
}

#ifdef USE_AVX2
void nnue_permute_biases(int32_t *biases)
{
    __m128i *b = (__m128i *)biases;
    __m128i tmp[8];
#ifdef USE_AVX512
    tmp[0] = b[0];
    tmp[1] = b[2];
    tmp[2] = b[4];
    tmp[3] = b[6];
    tmp[4] = b[1];
    tmp[5] = b[3];
    tmp[6] = b[5];
    tmp[7] = b[7];
#elif USE_AVX2
    tmp[0] = b[0];
    tmp[1] = b[4];
    tmp[2] = b[1];
    tmp[3] = b[5];
    tmp[4] = b[2];
    tmp[5] = b[6];
    tmp[6] = b[3];
    tmp[7] = b[7];
#else
#error
#endif
    memcpy(b, tmp, 8 * sizeof(__m128i));
}
#endif

unsigned nnue_weight_index(unsigned r, unsigned c, unsigned dims)
{
    (void)dims;
#if defined(USE_AVX512)
    if (dims > 32) {
        unsigned b = c & 0x38;
        b = (b << 1) | (b >> 2);
        c = (c & ~0x38) | (b & 0x38);
    }
    else if (dims == 32) {
        unsigned b = c & 0x18;
        b = (b << 1) | (b >> 1);
        c = (c & ~0x18) | (b & 0x18);
    }
#elif defined(USE_AVX2)
    if (dims > 32) {
        unsigned b = c & 0x18;
        b = (b << 1) | (b >> 1);
        c = (c & ~0x18) | (b & 0x18);
    }
#endif
#if defined(USE_AVX512)
    return c * 64 + r + (r & ~7);
#else
    return c * 32 + r;
#endif
}

const char *nnue_read_hidden_weights(weight_t *w, unsigned dims, const char *d)
{
    for (unsigned r = 0; r < 32; r++) {
        for (unsigned c = 0; c < dims; c++) {
            w[nnue_weight_index(r, c, dims)] = *d++;
        }
    }
    return d;
}

void nnue_read_output_weights(weight_t *w, const char *d)
{
    for (unsigned i = 0; i < 32; i++) {
        unsigned c = i;
#if defined(USE_AVX512)
        unsigned b = c & 0x18;
        b = (b << 1) | (b >> 1);
        c = (c & ~0x18) | (b & 0x18);
#endif
        w[c] = *d++;
    }
}

void nnue_init_weights(const void *file_data, NNUE_PARAM *p_nnue_param)
{
    const char *d = (const char *)file_data + TRANSFORMER_START + 4;
    // Read transformer
    for (unsigned i = 0; i < KHALF_DIMENSIONS; i++, d += 2) {
        p_nnue_param->ft_biases[i] = nnue_read_u16(d);
    }
    for (unsigned i = 0; i < KHALF_DIMENSIONS * FT_IN_DIMS; i++, d += 2) {
        p_nnue_param->ft_weights[i] = nnue_read_u16(d);
    }
    // Read network
    d += 4;
    for (unsigned i = 0; i < 32; i++, d += 4) {
        p_nnue_param->hidden1_biases[i] = nnue_read_u32(d);
    }
    d = nnue_read_hidden_weights(p_nnue_param->hidden1_weights, 512, d);
    for (unsigned i = 0; i < 32; i++, d += 4) {
        p_nnue_param->hidden2_biases[i] = nnue_read_u32(d);
    }
    d = nnue_read_hidden_weights(p_nnue_param->hidden2_weights, 32, d);
    for (unsigned i = 0; i < 1; i++, d += 4) {
        p_nnue_param->output_biases[i] = nnue_read_u32(d);
    }
    nnue_read_output_weights(p_nnue_param->output_weights, d);
#ifdef USE_AVX2
    nnue_permute_biases(p_nnue_param->hidden1_biases);
    nnue_permute_biases(p_nnue_param->hidden2_biases);
#endif
}

int nnue_load_eval_file(const char *eval_file, NNUE_PARAM *p_nnue_param)
{
    FD fd = nnue_open_file(eval_file);
    if (fd == FD_ERR) {
        return FALSE;
    }
    map_t mapping;
    const void *file_data = nnue_map_file(fd, &mapping);
    size_t size = nnue_file_size(fd);
    nnue_close_file(fd);
    int success = nnue_verify_net(file_data, size);
    if (success) {
        nnue_init_weights(file_data, p_nnue_param);
    }
    if (mapping) {
        nnue_unmap_file(file_data, mapping);
    }
    return success;
}

int nnue_init(const char* eval_file_name, NNUE_PARAM *p_nnue_param)
{
    if (nnue_load_eval_file(eval_file_name, p_nnue_param)) {
        printf("\nEval file '%s' loaded !\n", eval_file_name);
        fflush(stdout);
        return TRUE;
    }
    printf("\nEval file '%s' not found!\n", eval_file_name);
    fflush(stdout);
    return FALSE;
}

//EOF