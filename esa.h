/*
 * esa.h
 *
 * Copyright (c) 2023 Josh English
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef ESA_H_INCLUDED
#define ESA_H_INCLUDED

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESA_MIN_TOKEN_LENGTH 5u
#define ESA_MIN_STEM_LENGTH 3u

typedef enum EsaStatus {
    ESA_OK = 0,
    ESA_INVALID_ARGUMENT,
    ESA_INVALID_RULE,
    ESA_OUT_OF_MEMORY,
    ESA_BUFFER_TOO_SMALL,
    ESA_OVERFLOW,
    ESA_NOT_INITIALIZED
} EsaStatus;

typedef struct EsaEngine EsaEngine;

/* Owns a compiled engine. Do not copy or modify its members directly. */
typedef struct Esa {
    unsigned char debug_mode;
    EsaEngine *engine;
} Esa;

/* Per-call counters; no shared mutable state or clock reads. */
typedef struct EsaMetrics {
    size_t input_bytes;
    size_t output_bytes;
    size_t trie_steps;
    size_t rule_candidates;
    size_t rule_steps;
    size_t rules_applied;
} EsaMetrics;

typedef struct EsaEngineInfo {
    size_t rule_groups;
    size_t rule_steps;
    size_t trie_nodes;
    size_t max_growth;
    size_t allocated_bytes;
} EsaEngineInfo;

/* Initialize fresh/uninitialized storage; free before initializing it again.
 * Failure leaves *esa empty and safe to free. Rule strings are copied.
 * NULL rules with count == 0 creates a normalization-only engine. */
EsaStatus esaInitDefault(Esa *esa);
EsaStatus esaInitRules(Esa *esa, const char *const *rules, size_t count);
void esaFree(Esa *esa);

const char *esaStatusString(EsaStatus status);
EsaStatus esaGetEngineInfo(const Esa *esa, EsaEngineInfo *info);

/* Includes the NUL and a conservative bound for all possible rule growth. */
EsaStatus esaRequiredCapacity(const Esa *esa, size_t length, size_t *capacity);

/* Read exactly length bytes; ASCII letters are lowercased, all else removed.
 * Empty inputs are valid. The caller must provide length readable bytes.
 * output may equal token; other overlapping buffers are unsupported.
 * On error, output is unchanged, *output_length is zero, and metrics are zero.
 * output_length and metrics may be NULL. See docs/api.md for ownership. */
EsaStatus esaStemInto(const Esa *esa, const char *token, size_t length,
                     char *output, size_t capacity, size_t *output_length,
                     EsaMetrics *metrics);

/* On success, caller owns *output and must free it. On error, *output is NULL. */
EsaStatus esaStemAlloc(const Esa *esa, const char *token, size_t length,
                      char **output, EsaMetrics *metrics);

/* Legacy entry points retained for existing callers. esaInit discards status;
 * esaStemToken rejects nonpositive lengths and returns NULL on any failure. */
void esaInit(Esa *esa);
void esaSetDebugMode(Esa *esa, unsigned char mode);
char *esaStemToken(Esa *esa, char *token, int length);

#ifdef __cplusplus
}
#endif

#endif /* ESA_H_INCLUDED */
