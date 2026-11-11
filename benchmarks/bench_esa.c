/* SPDX-License-Identifier: MIT */
#define _POSIX_C_SOURCE 200809L
#include "esa.h"
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_WORDS 1024u
#define WORD_CAPACITY 2048u

static char words[MAX_WORDS][WORD_CAPACITY];
static size_t lengths[MAX_WORDS];
static size_t word_count;

static void fail(const char *message)
{
    fprintf(stderr, "%s\n", message);
    exit(EXIT_FAILURE);
}

static uint64_t now_ns(void)
{
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0) {
        fail("clock_gettime failed");
    }
    return (uint64_t)value.tv_sec * UINT64_C(1000000000) + (uint64_t)value.tv_nsec;
}

static void load_words(const char *path, const char *workload)
{
    FILE *file = fopen(path, "r");
    char line[512];
    if (file == NULL) {
        fail("cannot read benchmark corpus");
    }
    while (fgets(line, (int)sizeof(line), file) != NULL) {
        char *tab;
        size_t n;
        size_t j;
        if (strchr(line, '\n') == NULL && !feof(file)) {
            fail("corpus row too long");
        }
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        tab = strchr(line, '\t');
        if (tab == NULL || word_count >= MAX_WORDS) {
            fail("invalid or oversized corpus");
        }
        *tab = '\0';
        n = strlen(line);
        if (n == 0 || n + 520 >= WORD_CAPACITY) {
            fail("invalid word length");
        }
        for (j = 0; j < n; ++j) {
            if (line[j] < 'a' || line[j] > 'z') {
                fail("corpus tokens must be lowercase ASCII letters");
            }
        }
        if (strcmp(workload, "mixed") == 0) {
            words[word_count][0] = '(';
            for (j = 0; j < n; ++j) {
                words[word_count][j + 1] = (char)(line[j] - ('a' - 'A'));
            }
            memcpy(words[word_count] + n + 1, ")!42", 5);
        } else if (strcmp(workload, "long") == 0) {
            for (j = 0; j < 512; ++j) {
                words[word_count][j] = "abcdefg"[j % 7u];
            }
            memcpy(words[word_count] + 512, line, n + 1);
        } else if (strcmp(workload, "plain") == 0) {
            memcpy(words[word_count], line, n + 1);
        } else {
            fail("unknown workload");
        }
        lengths[word_count] = strlen(words[word_count]);
        ++word_count;
    }
    if (ferror(file) || fclose(file) != 0 || word_count == 0) {
        fail("failed reading corpus");
    }
}

static char *stem(Esa *esa, size_t i, const char *mode, char *buffer)
{
#ifdef ESA_BENCH_LEGACY
    (void)mode;
    (void)buffer;
    return esaStemToken(esa, words[i], (int)lengths[i]);
#else
    if (strcmp(mode, "buffer") == 0) {
        if (esaStemInto(esa, words[i], lengths[i], buffer, WORD_CAPACITY,
                        NULL, NULL) != ESA_OK) {
            fail("buffer stemming failed");
        }
        return buffer;
    }
    {
        char *output = NULL;
        if (esaStemAlloc(esa, words[i], lengths[i], &output, NULL) != ESA_OK) {
            fail("allocating stemming failed");
        }
        return output;
    }
#endif
}

int main(int argc, char **argv)
{
    const char *mode;
    char *end;
    unsigned long long parsed_rounds;
    size_t rounds;
    size_t round;
    size_t i;
    size_t corpus_bytes = 0;
    uint64_t start;
    uint64_t elapsed;
    uint64_t checksum = 0;
    uint64_t fingerprint = UINT64_C(14695981039346656037);
    char buffer[WORD_CAPACITY];
    Esa esa;
#ifndef ESA_BENCH_LEGACY
    EsaEngineInfo info;
    EsaMetrics totals = {0};
#endif
    if (argc != 5) {
        fail("usage: bench_esa MODE ROUNDS CORPUS WORKLOAD");
    }
    mode = argv[1];
    errno = 0;
    parsed_rounds = strtoull(argv[2], &end, 10);
    if (argv[2][0] < '0' || argv[2][0] > '9' || *end != '\0' ||
        errno == ERANGE || parsed_rounds == 0 || parsed_rounds > 10000000ULL ||
        (strcmp(mode, "alloc") != 0 && strcmp(mode, "buffer") != 0 &&
         strcmp(mode, "init") != 0)) {
        fail("invalid benchmark arguments");
    }
    rounds = (size_t)parsed_rounds;
    load_words(argv[3], argv[4]);
    if (word_count > SIZE_MAX / rounds) {
        fail("operation count overflow");
    }
#ifdef ESA_BENCH_LEGACY
    esaInit(&esa);
#else
    if (esaInitDefault(&esa) != ESA_OK || esaGetEngineInfo(&esa, &info) != ESA_OK) {
        fail("initialization failed");
    }
#endif
    /* Untimed warmup and a complete output fingerprint, independent of rounds. */
    for (i = 0; i < word_count; ++i) {
        char *output = stem(&esa, i, mode, buffer);
        size_t j;
        if (output == NULL) {
            fail("stemming failed");
        }
        for (j = 0; output[j] != '\0'; ++j) {
            fingerprint ^= (unsigned char)output[j];
            fingerprint *= UINT64_C(1099511628211);
        }
        fingerprint ^= 255u;
        fingerprint *= UINT64_C(1099511628211);
        if (output != buffer) {
            free(output);
        }
        corpus_bytes += lengths[i];
#ifndef ESA_BENCH_LEGACY
        {
            EsaMetrics metrics;
            if (esaStemInto(&esa, words[i], lengths[i], buffer, sizeof(buffer),
                            NULL, &metrics) != ESA_OK) {
                fail("metrics collection failed");
            }
            totals.trie_steps += metrics.trie_steps;
            totals.rule_candidates += metrics.rule_candidates;
            totals.rule_steps += metrics.rule_steps;
            totals.rules_applied += metrics.rules_applied;
        }
#endif
    }
    if (strcmp(mode, "init") == 0) {
        esaFree(&esa);
        start = now_ns();
        for (round = 0; round < rounds; ++round) {
#ifdef ESA_BENCH_LEGACY
            esaInit(&esa);
#else
            if (esaInitDefault(&esa) != ESA_OK) {
                fail("initialization failed");
            }
#endif
            esaFree(&esa);
        }
        elapsed = now_ns() - start;
    } else {
        start = now_ns();
        for (round = 0; round < rounds; ++round) {
            for (i = 0; i < word_count; ++i) {
                char *output = stem(&esa, i, mode, buffer);
                if (output == NULL) {
                    fail("stemming failed");
                }
                checksum += strlen(output) + (unsigned char)output[0];
                if (output != buffer) {
                    free(output);
                }
            }
        }
        elapsed = now_ns() - start;
        esaFree(&esa);
    }
    printf("{\"elapsed_ns\":%" PRIu64 ",\"operations\":%zu,"
           "\"corpus_tokens\":%zu,\"corpus_bytes\":%zu,"
           "\"checksum\":%" PRIu64 ",\"fingerprint\":\"%016" PRIx64 "\"",
           elapsed, strcmp(mode, "init") == 0 ? rounds : rounds * word_count,
           word_count, corpus_bytes, checksum, fingerprint);
#ifndef ESA_BENCH_LEGACY
    printf(",\"engine_bytes\":%zu,\"rule_groups\":%zu,\"rule_steps\":%zu,"
           "\"trie_nodes\":%zu,\"max_growth\":%zu,"
           "\"trie_steps_per_corpus\":%zu,\"rule_candidates_per_corpus\":%zu,"
           "\"rule_steps_per_corpus\":%zu,\"rules_applied_per_corpus\":%zu",
           info.allocated_bytes, info.rule_groups, info.rule_steps, info.trie_nodes,
           info.max_growth, totals.trie_steps, totals.rule_candidates,
           totals.rule_steps, totals.rules_applied);
#endif
    puts("}");
    return EXIT_SUCCESS;
}
