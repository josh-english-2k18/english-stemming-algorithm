/* SPDX-License-Identifier: MIT */
#include "esa.h"
#include "esa_rules.h"
#include "reference.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t checks;
#define CHECK(condition) do { \
    ++checks; \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", \
                __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static uint32_t random_state = UINT32_C(0x6e624eb7);
static uint32_t random_value(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

static void expect(Esa *esa, const char *input, const char *expected)
{
    char *output = NULL;
    EsaMetrics metrics;
    CHECK(esaStemAlloc(esa, input, strlen(input), &output, &metrics) == ESA_OK);
    if (strcmp(output, expected) != 0) {
        fprintf(stderr, "input '%s': expected '%s', got '%s'\n",
                input, expected, output);
        exit(EXIT_FAILURE);
    }
    CHECK(metrics.output_bytes == strlen(expected));
    CHECK(metrics.input_bytes == strlen(input));
    free(output);
}

static void test_golden(Esa *esa)
{
    FILE *file = fopen("tests/corpus.tsv", "r");
    char line[512];
    size_t count = 0;
    CHECK(file != NULL);
    while (fgets(line, (int)sizeof(line), file) != NULL) {
        char *tab;
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        tab = strchr(line, '\t');
        CHECK(tab != NULL);
        *tab++ = '\0';
        tab[strcspn(tab, "\r\n")] = '\0';
        expect(esa, line, tab);
        ++count;
    }
    CHECK(!ferror(file));
    CHECK(fclose(file) == 0);
    CHECK(count >= 218);
}

static void test_normalization(Esa *esa)
{
    char bytes[256];
    char output[1024];
    char exact[] = {'S', 'T', 'E', 'M', 'M', 'I', 'N', 'G'};
    char embedded[] = {'A', '\0', 'B', (char)0xff, 'C'};
    char inplace[128] = "  (Stemming!)  ";
    size_t length;
    size_t i;
    expect(esa, "", "");
    expect(esa, "  (ESA)  ", "esa");
    expect(esa, "2003).", "");
    expect(esa, "\t S.\r\n", "s");
    expect(esa, "A-b_C!", "abc");
    expect(esa, "two words", "twoword");
    expect(esa, "aeiouy", "aeiouy");
    expect(esa, "bcdfgh", "bcdfgh");
    expect(esa, "dogs.", "dogs");
    expect(esa, "running", "run");
    expect(esa, "skies", "ski");
    CHECK(esaStemInto(esa, exact, sizeof(exact), output, sizeof(output),
                      &length, NULL) == ESA_OK);
    CHECK(strcmp(output, "stem") == 0 && length == 4);
    esaSetDebugMode(esa, 1);
    CHECK(esaStemInto(esa, exact, sizeof(exact), output, sizeof(output),
                      NULL, NULL) == ESA_OK);
    esaSetDebugMode(esa, 0);
    CHECK(esaStemInto(esa, embedded, sizeof(embedded), output, sizeof(output),
                      NULL, NULL) == ESA_OK);
    CHECK(strcmp(output, "abc") == 0);
    CHECK(esaStemInto(esa, inplace, strlen(inplace), inplace, sizeof(inplace),
                      NULL, NULL) == ESA_OK);
    CHECK(strcmp(inplace, "stem") == 0);
    for (i = 0; i < sizeof(bytes); ++i) {
        bytes[i] = (char)(unsigned char)i;
    }
    {
        Esa empty;
        CHECK(esaInitRules(&empty, NULL, 0) == ESA_OK);
        CHECK(esaStemInto(&empty, bytes, sizeof(bytes), output, sizeof(output),
                          NULL, NULL) == ESA_OK);
        CHECK(strcmp(output, "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz") == 0);
        esaFree(&empty);
    }
    {
        char *long_input = malloc(100001);
        char *stem = NULL;
        CHECK(long_input != NULL);
        memset(long_input, 'A', 100000);
        long_input[100000] = '\0';
        CHECK(esaStemAlloc(esa, long_input, 100000, &stem, NULL) == ESA_OK);
        CHECK(strlen(stem) == 100000);
        CHECK(stem[0] == 'a' && stem[99999] == 'a');
        free(stem);
        free(long_input);
    }
}

static void custom(const char *const *rules, size_t count,
                   const char *input, const char *expected)
{
    Esa esa;
    CHECK(esaInitRules(&esa, rules, count) == ESA_OK);
    expect(&esa, input, expected);
    esaFree(&esa);
}

static void test_rule_semantics(void)
{
    const char *priority[] = {"s,n,1,x", "es,n,2,y"};
    const char *chain[] = {"s,n,1,,e,n,1,,t,n,1,z"};
    const char *local[] = {"s,n,1,,x,n,1,", "e,n,1,"};
    const char *intact[] = {"s,n,1,", "e,y,1,x", "e,n,1,z"};
    const char *chain_intact[] = {"s,n,1,,e,Y,1,x"};
    const char *no_op[] = {"s,n,0,", "s,y,1,x"};
    const char *no_restart[] = {"x,n,1,z", "s,n,1,x"};
    const char *no_repeat[] = {"s,n,0,s"};
    const char *long_suffix[] = {"zzabcde,n,2,x"};
    const char *guard[] = {"e,n,3,zzzz", "e,n,2,zz"};
    const char *too_many[] = {"e,n,999999,"};
    const char *duplicate[] = {"e,n,0,", "e,n,1,x"};
    const char *growth[] = {"e,n,0,abcdefghij,j,n,0,klmnop", "p,n,0,qrstuv"};
    const char *eligibility[] = {"a,n,0,z", "b,n,0,z", "s,n,1,z"};
    custom(priority, 2, "foxes", "foxex");
    custom(chain, 1, "abctes", "abcz");
    custom(local, 2, "abcdes", "abcd");
    custom(intact, 3, "abcdes", "abcdz");
    custom(chain_intact, 1, "abcdes", "abcde");
    custom(no_op, 2, "abcdes", "abcdes");
    custom(no_restart, 2, "abcdes", "abcdex");
    custom(no_repeat, 1, "abcdes", "abcdess");
    custom(long_suffix, 1, "abcde", "abcde");
    custom(guard, 2, "abcde", "abczz");
    custom(too_many, 1, "abcde", "abcde");
    custom(duplicate, 2, "abcde", "abcdx");
    custom(growth, 2, "abcde", "abcdeabcdefghijklmnopqrstuv");
    custom(eligibility, 3, "aaaaa", "aaaaa");
    custom(eligibility, 3, "bbbbb", "bbbbb");
    custom(eligibility, 3, "dogs", "dogs");
    {
        char owned[] = "s,n,1,x";
        const char *rules[] = {owned};
        Esa esa;
        CHECK(esaInitRules(&esa, rules, 1) == ESA_OK);
        memset(owned, '!', strlen(owned));
        expect(&esa, "abcdes", "abcdex");
        esaFree(&esa);
    }
}

static void test_errors(Esa *esa)
{
    const char *bad[] = {"", "s", "s,n", "s,n,1", ",n,1,", "S,n,1,",
        "s,x,1,", "s,nn,1,", "s,,1,", "s,n,,", "s,n,-1,", "s,n,+1,",
        "s,n, 1,", "s,n,1z,", "s,n,1,X", "s,n,1, ", "s,n,1,,",
        "s,n,1,,a,n,1", "s,n,1,,a,y,1,,", NULL};
    size_t i;
    Esa empty = {0};
    Esa invalid;
    EsaEngineInfo info;
    size_t capacity = 7;
    size_t length = 9;
    char out[32] = "unchanged";
    char *allocated = out;
    EsaMetrics metrics;
    EsaMetrics zero = {0};
    memset(&metrics, 0xff, sizeof(metrics));
    CHECK(esaInitDefault(NULL) == ESA_INVALID_ARGUMENT);
    CHECK(esaInitRules(&invalid, NULL, 1) == ESA_INVALID_ARGUMENT);
    esaFree(&invalid);
    for (i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
        CHECK(esaInitRules(&invalid, &bad[i], 1) == ESA_INVALID_RULE);
        CHECK(invalid.engine == NULL);
        esaFree(&invalid);
    }
    {
        const char *overflow[] = {"e,n,999999999999999999999999999999999999999999999,"};
        CHECK(esaInitRules(&invalid, overflow, 1) == ESA_OVERFLOW);
        esaFree(&invalid);
        CHECK(esaInitRules(&invalid, overflow, SIZE_MAX) == ESA_OVERFLOW);
        esaFree(&invalid);
    }
    CHECK(esaRequiredCapacity(NULL, 0, &capacity) == ESA_INVALID_ARGUMENT);
    CHECK(esaRequiredCapacity(esa, 0, NULL) == ESA_INVALID_ARGUMENT);
    CHECK(esaRequiredCapacity(&empty, 0, &capacity) == ESA_NOT_INITIALIZED);
    CHECK(capacity == 0);
    CHECK(esaRequiredCapacity(esa, SIZE_MAX, &capacity) == ESA_OVERFLOW);
    CHECK(capacity == 0);
    CHECK(esaStemInto(esa, "running", 7, out, 1, &length, &metrics) == ESA_BUFFER_TOO_SMALL);
    CHECK(length == 0 && strcmp(out, "unchanged") == 0);
    CHECK(memcmp(&metrics, &zero, sizeof(metrics)) == 0);
    CHECK(esaStemInto(esa, NULL, 0, out, sizeof(out), NULL, NULL) == ESA_INVALID_ARGUMENT);
    CHECK(esaStemInto(esa, "", 0, NULL, 0, NULL, NULL) == ESA_INVALID_ARGUMENT);
    CHECK(esaStemInto(NULL, "", 0, out, sizeof(out), NULL, NULL) == ESA_INVALID_ARGUMENT);
    CHECK(esaStemInto(&empty, "", 0, out, sizeof(out), NULL, NULL) == ESA_NOT_INITIALIZED);
    CHECK(esaStemInto(esa, out, SIZE_MAX, out, sizeof(out), NULL, NULL) == ESA_OVERFLOW);
    CHECK(esaStemAlloc(esa, NULL, 0, &allocated, NULL) == ESA_INVALID_ARGUMENT);
    CHECK(allocated == NULL);
    CHECK(esaStemAlloc(esa, "", 0, NULL, NULL) == ESA_INVALID_ARGUMENT);
    CHECK(esaStemAlloc(NULL, "", 0, &allocated, NULL) == ESA_INVALID_ARGUMENT);
    CHECK(esaStemAlloc(&empty, "", 0, &allocated, NULL) == ESA_NOT_INITIALIZED);
    CHECK(esaStemAlloc(esa, "", SIZE_MAX, &allocated, NULL) == ESA_OVERFLOW);
    CHECK(esaStemToken(esa, out, 0) == NULL);
    CHECK(esaStemToken(esa, out, -1) == NULL);
    CHECK(esaStemToken(NULL, out, 3) == NULL);
    CHECK(esaGetEngineInfo(NULL, &info) == ESA_INVALID_ARGUMENT);
    CHECK(esaGetEngineInfo(esa, NULL) == ESA_INVALID_ARGUMENT);
    CHECK(esaGetEngineInfo(&empty, &info) == ESA_NOT_INITIALIZED);
    CHECK(esaGetEngineInfo(esa, &info) == ESA_OK);
    CHECK(info.rule_groups == ESA_DEFAULT_RULE_COUNT);
    CHECK(info.rule_steps > info.rule_groups && info.trie_nodes > 1);
    CHECK(info.allocated_bytes > 0);
    for (i = 0; i <= (size_t)ESA_NOT_INITIALIZED; ++i) {
        CHECK(strlen(esaStatusString((EsaStatus)i)) > 0);
    }
    CHECK(strcmp(esaStatusString((EsaStatus)99), "unknown status") == 0);
    esaFree(NULL);
    esaSetDebugMode(NULL, 1);
    esaInit(NULL);
    esaFree(&empty);
    esaFree(&empty);
    {
        Esa legacy;
        char input[] = "Stemming";
        char *stem;
        esaInit(&legacy);
        stem = esaStemToken(&legacy, input, 8);
        CHECK(stem != NULL && strcmp(stem, "stem") == 0);
        free(stem);
        esaFree(&legacy);
        esaFree(&legacy);
    }
}

static void test_capacity_and_metrics(void)
{
    const char *rules[] = {"s,n,1,,e,n,1,x", "x,y,1,y", "x,n,0,abc"};
    Esa esa;
    EsaMetrics metrics;
    EsaEngineInfo info;
    char output[32] = "unchanged";
    size_t capacity;
    size_t length;
    CHECK(esaInitRules(&esa, rules, 3) == ESA_OK);
    CHECK(esaGetEngineInfo(&esa, &info) == ESA_OK);
    CHECK(info.rule_groups == 3 && info.rule_steps == 4 && info.max_growth == 3);
    CHECK(esaRequiredCapacity(&esa, 6, &capacity) == ESA_OK && capacity == 10);
    CHECK(esaStemInto(&esa, "abcdes", 6, output, capacity - 1,
                      &length, &metrics) == ESA_BUFFER_TOO_SMALL);
    CHECK(strcmp(output, "unchanged") == 0 && length == 0);
    CHECK(esaStemInto(&esa, "abcdes", 6, output, capacity,
                      &length, &metrics) == ESA_OK);
    CHECK(strcmp(output, "abcdxabc") == 0 && length == 8);
    CHECK(metrics.input_bytes == 6 && metrics.output_bytes == 8);
    CHECK(metrics.rule_steps == 4 && metrics.rules_applied == 3);
    CHECK(metrics.rule_candidates >= 3 && metrics.trie_steps > 0);
    esaFree(&esa);
    CHECK(esaInitRules(&esa, NULL, 0) == ESA_OK);
    CHECK(esaRequiredCapacity(&esa, 0, &capacity) == ESA_OK && capacity == 1);
    CHECK(esaStemInto(&esa, "", 0, output, capacity, &length, &metrics) == ESA_OK);
    CHECK(output[0] == '\0' && length == 0 && metrics.trie_steps == 0);
    esaFree(&esa);
}

static void test_large_rule_set(void)
{
    const size_t count = 10000;
    const char **rules = malloc(count * sizeof(*rules));
    Esa esa;
    EsaMetrics metrics;
    char output[16];
    size_t i;
    CHECK(rules != NULL);
    for (i = 0; i < count; ++i) {
        rules[i] = "e,n,0,";
    }
    CHECK(esaInitRules(&esa, rules, count) == ESA_OK);
    free(rules);
    CHECK(esaStemInto(&esa, "abcde", 5, output, sizeof(output),
                      NULL, &metrics) == ESA_OK);
    CHECK(strcmp(output, "abcde") == 0);
    CHECK(metrics.rules_applied == count && metrics.rule_steps == count);
    /* At most ceil(log2(10000)) + 1 priority probes per group. */
    CHECK(metrics.rule_candidates <= count * 15);
    esaFree(&esa);
}

static void test_long_rules_and_numeric_limits(void)
{
    const size_t suffix_length = 4096;
    const size_t append_length = 1024;
    char *rule = malloc(suffix_length + 5 + append_length + 1);
    char *input = malloc(suffix_length + 2);
    const char *rules[1];
    Esa esa;
    EsaMetrics metrics;
    size_t capacity;
    size_t written;
    char *output;
    CHECK(rule != NULL && input != NULL);
    memset(rule, 'a', suffix_length);
    memcpy(rule + suffix_length, ",N,1,", 5);
    memset(rule + suffix_length + 5, 'b', append_length);
    rule[suffix_length + 5 + append_length] = '\0';
    input[0] = 'b';
    memset(input + 1, 'a', suffix_length);
    input[suffix_length + 1] = '\0';
    rules[0] = rule;
    CHECK(esaInitRules(&esa, rules, 1) == ESA_OK);
    free(rule);
    CHECK(esaRequiredCapacity(&esa, suffix_length + 1, &capacity) == ESA_OK);
    CHECK(capacity == suffix_length + append_length + 1);
    output = malloc(capacity);
    CHECK(output != NULL);
    CHECK(esaStemInto(&esa, input, suffix_length + 1, output, capacity,
                      &written, &metrics) == ESA_OK);
    CHECK(written == suffix_length + append_length);
    CHECK(memcmp(output, input, suffix_length) == 0);
    CHECK(output[suffix_length] == 'b' && output[written - 1] == 'b');
    CHECK(output[written] == '\0' && metrics.rules_applied == 1);
    free(output);
    free(input);
    esaFree(&esa);
    {
        char maximum[128];
        const char *numeric[] = {maximum, "e,N,0001,x"};
        int n = snprintf(maximum, sizeof(maximum), "e,n,%zu,", SIZE_MAX);
        CHECK(n > 0 && (size_t)n < sizeof(maximum));
        custom(numeric, 2, "abcde", "abcdx");
    }
    {
        const char *growth[] = {"e,n,0,abcdefghijklmn"};
        char inplace[20] = "abcde";
        CHECK(esaInitRules(&esa, growth, 1) == ESA_OK);
        CHECK(esaRequiredCapacity(&esa, 5, &capacity) == ESA_OK && capacity == sizeof(inplace));
        CHECK(esaStemInto(&esa, inplace, 5, inplace, capacity, NULL, NULL) == ESA_OK);
        CHECK(strcmp(inplace, "abcdeabcdefghijklmn") == 0);
        esaFree(&esa);
    }
}

static void compare(Esa *esa, const char *const *rules, size_t count,
                    const char *token, size_t length)
{
    char *actual;
    char expected[4096];
    size_t capacity;
    size_t written;
    EsaMetrics metrics;
    CHECK(esaRequiredCapacity(esa, length, &capacity) == ESA_OK);
    actual = malloc(capacity);
    CHECK(actual != NULL);
    CHECK(reference_stem(rules, count, token, length, expected, sizeof(expected)));
    CHECK(esaStemInto(esa, token, length, actual, capacity,
                      &written, &metrics) == ESA_OK);
    if (strcmp(actual, expected) != 0) {
        fprintf(stderr, "differential mismatch: expected '%s', got '%s'\n", expected, actual);
        exit(EXIT_FAILURE);
    }
    CHECK(written < capacity && written == strlen(actual));
    CHECK(metrics.rules_applied <= metrics.rule_steps);
    {
        size_t j;
        for (j = 0; j < written; ++j) {
            CHECK(actual[j] >= 'a' && actual[j] <= 'z');
        }
    }
    free(actual);
}

static void test_differential(Esa *esa)
{
    size_t i;
    /* Seed each built-in suffix, including conditional continuation suffixes. */
    for (i = 0; i < ESA_DEFAULT_RULE_COUNT; ++i) {
        const char *p = ESA_RULE_LIST[i];
        for (;;) {
            char token[256] = "abacade";
            size_t j = 7;
            size_t fields = 0;
            while (*p != ',') {
                token[j++] = *p++;
            }
            token[j] = '\0';
            compare(esa, ESA_RULE_LIST, ESA_DEFAULT_RULE_COUNT, token, j);
            while (*p != '\0' && fields < 4) {
                if (*p++ == ',') {
                    ++fields;
                }
            }
            if (*p == '\0') {
                break;
            }
        }
    }
    for (i = 0; i < 30000; ++i) {
        char token[256];
        size_t length = random_value() % 100u;
        size_t j;
        for (j = 0; j < length; ++j) {
            token[j] = (char)(unsigned char)random_value();
        }
        compare(esa, ESA_RULE_LIST, ESA_DEFAULT_RULE_COUNT, token, length);
    }
    for (i = 0; i < 200; ++i) {
        char storage[20][512];
        const char *rules[20];
        size_t count = 1u + random_value() % 20u;
        size_t j;
        Esa generated;
        for (j = 0; j < count; ++j) {
            size_t step_count = 1u + random_value() % 4u;
            size_t k;
            size_t offset = 0;
            rules[j] = storage[j];
            for (k = 0; k < step_count; ++k) {
                char suffix[6];
                char append[6];
                size_t n = 1u + random_value() % 5u;
                size_t a = random_value() % 6u;
                size_t t;
                int written;
                for (t = 0; t < n; ++t) {
                    suffix[t] = "abes"[random_value() % 4u];
                }
                suffix[n] = '\0';
                for (t = 0; t < a; ++t) {
                    append[t] = "abes"[random_value() % 4u];
                }
                append[a] = '\0';
                written = snprintf(storage[j] + offset, sizeof(storage[j]) - offset,
                                   "%s%s,%c,%u,%s", k == 0 ? "" : ",",
                                   suffix, (random_value() & 1u) != 0 ? 'Y' : 'n',
                                   random_value() % 12u, append);
                CHECK(written > 0 && (size_t)written < sizeof(storage[j]) - offset);
                offset += (size_t)written;
            }
        }
        CHECK(esaInitRules(&generated, rules, count) == ESA_OK);
        for (j = 0; j < 200; ++j) {
            char token[40];
            size_t length = random_value() % sizeof(token);
            size_t k;
            for (k = 0; k < length; ++k) {
                token[k] = "abes"[random_value() % 4u];
            }
            compare(&generated, rules, count, token, length);
        }
        esaFree(&generated);
    }
}

static void *thread_worker(void *argument)
{
    const Esa *esa = argument;
    size_t i;
    for (i = 0; i < 10000; ++i) {
        char output[64];
        if (esaStemInto(esa, "Stemming", 8, output, sizeof(output),
                        NULL, NULL) != ESA_OK || strcmp(output, "stem") != 0) {
            return (void *)"failed";
        }
    }
    return NULL;
}

static void test_threads(Esa *esa)
{
    pthread_t threads[4];
    size_t i;
    for (i = 0; i < 4; ++i) {
        CHECK(pthread_create(&threads[i], NULL, thread_worker, esa) == 0);
    }
    for (i = 0; i < 4; ++i) {
        void *result;
        CHECK(pthread_join(threads[i], &result) == 0);
        CHECK(result == NULL);
    }
}

int main(void)
{
    Esa esa;
    CHECK(esaInitDefault(&esa) == ESA_OK);
    test_golden(&esa);
    test_normalization(&esa);
    test_rule_semantics();
    test_errors(&esa);
    test_capacity_and_metrics();
    test_large_rule_set();
    test_long_rules_and_numeric_limits();
    test_differential(&esa);
    test_threads(&esa);
    esaFree(&esa);
    printf("PASS: %zu checks; 70,000 randomized differential cases; 40,000 concurrent calls\n", checks);
    return EXIT_SUCCESS;
}
