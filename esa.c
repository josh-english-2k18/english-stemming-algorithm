/*
 * esa.c
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
 * Written by Joshua S. English.
 */

#include "esa.h"
#include "esa_rules.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Overrides support exhaustive allocation-failure testing without public hooks. */
#ifndef ESA_MALLOC
#define ESA_MALLOC malloc
#endif
#ifndef ESA_CALLOC
#define ESA_CALLOC calloc
#endif
#ifndef ESA_FREE
#define ESA_FREE free
#endif

#define ESA_NONE SIZE_MAX

typedef struct EsaStep {
    const char *suffix;
    const char *append;
    size_t suffix_length;
    size_t append_length;
    size_t remove;
    unsigned char intact;
} EsaStep;

typedef struct EsaGroup {
    size_t first_step;
    size_t step_count;
    size_t terminal_node;
} EsaGroup;

/* Sparse reversed trie: only first steps are indexed. Continuations are local. */
typedef struct EsaNode {
    size_t child;
    size_t sibling;
    size_t first_group;
    size_t group_count;
    unsigned char letter;
} EsaNode;

struct EsaEngine {
    EsaStep *steps;
    EsaGroup *groups;
    EsaNode *nodes;
    size_t *priorities;
    char *strings;
    size_t root_children[26];
    EsaEngineInfo info;
};

static int add_size(size_t a, size_t b, size_t *result)
{
    if (a > SIZE_MAX - b) {
        return 0;
    }
    *result = a + b;
    return 1;
}

static int is_letter(unsigned char c)
{
    return c >= 'a' && c <= 'z';
}

static int is_vowel(char c)
{
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' ||
           c == 'u' || c == 'y';
}

static int letters_only(const char *value, size_t length)
{
    size_t i;
    for (i = 0; i < length; ++i) {
        if (!is_letter((unsigned char)value[i])) {
            return 0;
        }
    }
    return 1;
}

static int field(const char **cursor, const char **value, size_t *length)
{
    const char *end = strchr(*cursor, ',');
    *value = *cursor;
    *length = end != NULL ? (size_t)(end - *cursor) : strlen(*cursor);
    *cursor += *length + (end != NULL ? 1u : 0u);
    return end != NULL;
}

/* Validate once, then run the same parser over owned strings to emit steps. */
static EsaStatus parse_rule(const char *rule, EsaStep *steps,
                            size_t *step_count, size_t *growth,
                            size_t *first_suffix_length)
{
    const char *cursor = rule;
    size_t count = 0;
    int more;

    *growth = 0;
    do {
        EsaStep step;
        const char *value;
        size_t length;
        size_t i;

        if (!field(&cursor, &step.suffix, &step.suffix_length) ||
            step.suffix_length == 0 ||
            !letters_only(step.suffix, step.suffix_length)) {
            return ESA_INVALID_RULE;
        }
        if (count == 0) {
            *first_suffix_length = step.suffix_length;
        }
        if (!field(&cursor, &value, &length) || length != 1 ||
            (value[0] != 'n' && value[0] != 'N' &&
             value[0] != 'y' && value[0] != 'Y')) {
            return ESA_INVALID_RULE;
        }
        step.intact = (unsigned char)(value[0] == 'y' || value[0] == 'Y');
        if (!field(&cursor, &value, &length) || length == 0) {
            return ESA_INVALID_RULE;
        }
        step.remove = 0;
        for (i = 0; i < length; ++i) {
            size_t digit;
            if (value[i] < '0' || value[i] > '9') {
                return ESA_INVALID_RULE;
            }
            digit = (size_t)(value[i] - '0');
            if (step.remove > (SIZE_MAX - digit) / 10u) {
                return ESA_OVERFLOW;
            }
            step.remove = step.remove * 10u + digit;
        }
        more = field(&cursor, &step.append, &step.append_length);
        if (!letters_only(step.append, step.append_length)) {
            return ESA_INVALID_RULE;
        }
        if (step.append_length > step.remove &&
            !add_size(*growth, step.append_length - step.remove, growth)) {
            return ESA_OVERFLOW;
        }
        if (steps != NULL) {
            steps[count] = step;
        }
        if (count == SIZE_MAX) {
            return ESA_OVERFLOW;
        }
        ++count;
    } while (more);
    *step_count = count;
    return ESA_OK;
}

static void free_engine(EsaEngine *engine)
{
    if (engine != NULL) {
        ESA_FREE(engine->strings);
        ESA_FREE(engine->nodes);
        ESA_FREE(engine->priorities);
        ESA_FREE(engine->groups);
        ESA_FREE(engine->steps);
        ESA_FREE(engine);
    }
}

static void init_node(EsaNode *node, unsigned char letter)
{
    node->child = ESA_NONE;
    node->sibling = ESA_NONE;
    node->first_group = 0;
    node->group_count = 0;
    node->letter = letter;
}

static void index_group(EsaEngine *engine, size_t group_index)
{
    EsaGroup *group = &engine->groups[group_index];
    const EsaStep *step = &engine->steps[group->first_step];
    size_t parent = 0;
    size_t i;

    for (i = step->suffix_length; i > 0; --i) {
        unsigned char letter = (unsigned char)step->suffix[i - 1];
        size_t child = engine->nodes[parent].child;
        while (child != ESA_NONE && engine->nodes[child].letter != letter) {
            child = engine->nodes[child].sibling;
        }
        if (child == ESA_NONE) {
            child = engine->info.trie_nodes++;
            init_node(&engine->nodes[child], letter);
            engine->nodes[child].sibling = engine->nodes[parent].child;
            engine->nodes[parent].child = child;
        }
        if (parent == 0) {
            engine->root_children[letter - (unsigned char)'a'] = child;
        }
        parent = child;
    }
    group->terminal_node = parent;
    ++engine->nodes[parent].group_count;
}

EsaStatus esaInitRules(Esa *esa, const char *const *rules, size_t count)
{
    EsaEngine *engine;
    size_t step_count = 0;
    size_t string_bytes = 0;
    size_t node_capacity = 1;
    size_t growth = 0;
    size_t allocated = sizeof(EsaEngine);
    size_t i;
    size_t step_offset = 0;
    size_t string_offset = 0;

    if (esa == NULL) {
        return ESA_INVALID_ARGUMENT;
    }
    memset(esa, 0, sizeof(*esa));
    if (rules == NULL && count != 0) {
        return ESA_INVALID_ARGUMENT;
    }
    if (count > SIZE_MAX / sizeof(EsaGroup)) {
        return ESA_OVERFLOW;
    }
    for (i = 0; i < count; ++i) {
        size_t steps;
        size_t rule_growth;
        size_t first_length;
        size_t bytes;
        EsaStatus status;
        if (rules[i] == NULL) {
            return ESA_INVALID_RULE;
        }
        status = parse_rule(rules[i], NULL, &steps, &rule_growth, &first_length);
        if (status != ESA_OK) {
            return status;
        }
        if (!add_size(strlen(rules[i]), 1, &bytes) ||
            !add_size(string_bytes, bytes, &string_bytes) ||
            !add_size(step_count, steps, &step_count) ||
            !add_size(node_capacity, first_length, &node_capacity) ||
            !add_size(growth, rule_growth, &growth)) {
            return ESA_OVERFLOW;
        }
    }
    if (step_count > SIZE_MAX / sizeof(EsaStep) ||
        node_capacity > SIZE_MAX / sizeof(EsaNode) ||
        !add_size(allocated, step_count * sizeof(EsaStep), &allocated) ||
        !add_size(allocated, count * sizeof(EsaGroup), &allocated) ||
        !add_size(allocated, count * sizeof(size_t), &allocated) ||
        !add_size(allocated, node_capacity * sizeof(EsaNode), &allocated) ||
        !add_size(allocated, string_bytes, &allocated)) {
        return ESA_OVERFLOW;
    }
    engine = ESA_CALLOC(1, sizeof(*engine));
    if (engine == NULL) {
        return ESA_OUT_OF_MEMORY;
    }
    engine->nodes = ESA_MALLOC(node_capacity * sizeof(*engine->nodes));
    if (count != 0) {
        engine->steps = ESA_MALLOC(step_count * sizeof(*engine->steps));
        engine->groups = ESA_MALLOC(count * sizeof(*engine->groups));
        engine->priorities = ESA_MALLOC(count * sizeof(*engine->priorities));
        engine->strings = ESA_MALLOC(string_bytes);
    }
    if (engine->nodes == NULL ||
        (count != 0 && (engine->steps == NULL || engine->groups == NULL ||
                        engine->priorities == NULL || engine->strings == NULL))) {
        free_engine(engine);
        return ESA_OUT_OF_MEMORY;
    }
    engine->info.rule_groups = count;
    engine->info.rule_steps = step_count;
    engine->info.trie_nodes = 1;
    engine->info.max_growth = growth;
    engine->info.allocated_bytes = allocated;
    for (i = 0; i < 26; ++i) {
        engine->root_children[i] = ESA_NONE;
    }
    init_node(&engine->nodes[0], 0);
    for (i = 0; i < count; ++i) {
        size_t steps;
        size_t unused_growth;
        size_t unused_length;
        size_t bytes = strlen(rules[i]) + 1;
        memcpy(engine->strings + string_offset, rules[i], bytes);
        /* Already validated; the owned copy has identical syntax and sizes. */
        (void)parse_rule(engine->strings + string_offset,
                         engine->steps + step_offset, &steps,
                         &unused_growth, &unused_length);
        engine->groups[i].first_step = step_offset;
        engine->groups[i].step_count = steps;
        index_group(engine, i);
        step_offset += steps;
        string_offset += bytes;
    }
    /* Terminal priorities occupy contiguous sorted slices. Binary search keeps
     * repeated/overlapping suffixes from degrading into quadratic list scans. */
    {
        size_t offset = 0;
        for (i = 0; i < engine->info.trie_nodes; ++i) {
            engine->nodes[i].first_group = offset;
            offset += engine->nodes[i].group_count;
            engine->nodes[i].group_count = 0;
        }
        for (i = 0; i < count; ++i) {
            EsaNode *node = &engine->nodes[engine->groups[i].terminal_node];
            engine->priorities[node->first_group + node->group_count++] = i;
        }
    }
    esa->engine = engine;
    return ESA_OK;
}

EsaStatus esaInitDefault(Esa *esa)
{
    return esaInitRules(esa, ESA_RULE_LIST, ESA_DEFAULT_RULE_COUNT);
}

void esaInit(Esa *esa)
{
    (void)esaInitDefault(esa);
}

void esaFree(Esa *esa)
{
    if (esa != NULL) {
        free_engine(esa->engine);
        memset(esa, 0, sizeof(*esa));
    }
}

void esaSetDebugMode(Esa *esa, unsigned char mode)
{
    if (esa != NULL) {
        esa->debug_mode = (unsigned char)(mode != 0);
    }
}

const char *esaStatusString(EsaStatus status)
{
    switch (status) {
    case ESA_OK: return "success";
    case ESA_INVALID_ARGUMENT: return "invalid argument";
    case ESA_INVALID_RULE: return "invalid rule syntax";
    case ESA_OUT_OF_MEMORY: return "out of memory";
    case ESA_BUFFER_TOO_SMALL: return "output buffer too small";
    case ESA_OVERFLOW: return "size overflow";
    case ESA_NOT_INITIALIZED: return "engine not initialized";
    default: return "unknown status";
    }
}

EsaStatus esaGetEngineInfo(const Esa *esa, EsaEngineInfo *info)
{
    if (esa == NULL || info == NULL) {
        return ESA_INVALID_ARGUMENT;
    }
    memset(info, 0, sizeof(*info));
    if (esa->engine == NULL) {
        return ESA_NOT_INITIALIZED;
    }
    *info = esa->engine->info;
    return ESA_OK;
}

EsaStatus esaRequiredCapacity(const Esa *esa, size_t length, size_t *capacity)
{
    size_t result;
    if (esa == NULL || capacity == NULL) {
        return ESA_INVALID_ARGUMENT;
    }
    *capacity = 0;
    if (esa->engine == NULL) {
        return ESA_NOT_INITIALIZED;
    }
    if (!add_size(length, esa->engine->info.max_growth, &result) ||
        !add_size(result, 1, &result)) {
        return ESA_OVERFLOW;
    }
    *capacity = result;
    return ESA_OK;
}

static size_t normalize(const char *token, size_t length, char *output,
                        int *eligible)
{
    size_t i;
    size_t written = 0;
    int vowel = 0;
    int consonant = 0;
    for (i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)token[i];
        if (c >= 'A' && c <= 'Z') {
            c = (unsigned char)(c + ('a' - 'A'));
        }
        if (is_letter(c)) {
            output[written++] = (char)c;
            if (is_vowel((char)c)) {
                vowel = 1;
            } else {
                consonant = 1;
            }
        }
    }
    output[written] = '\0';
    *eligible = written >= ESA_MIN_TOKEN_LENGTH && vowel && consonant;
    return written;
}

static int matches(const char *token, size_t length, const EsaStep *step)
{
    return step->suffix_length <= length &&
           memcmp(token + length - step->suffix_length,
                  step->suffix, step->suffix_length) == 0;
}

/* Select by original priority, never by longest suffix. The cursor advances
 * past each visited group, so new endings cannot restart an earlier rule. */
static size_t next_group(const EsaEngine *engine, const char *token,
                         size_t length, size_t cursor, EsaMetrics *metrics)
{
    size_t parent = 0;
    size_t best = ESA_NONE;
    size_t i;
    for (i = length; i > 0; --i) {
        size_t child;
        if (parent == 0) {
            ++metrics->trie_steps;
            child = engine->root_children[(unsigned char)token[i - 1] -
                                           (unsigned char)'a'];
        } else {
            child = engine->nodes[parent].child;
            while (child != ESA_NONE) {
                ++metrics->trie_steps;
                if (engine->nodes[child].letter == (unsigned char)token[i - 1]) {
                    break;
                }
                child = engine->nodes[child].sibling;
            }
        }
        if (child == ESA_NONE) {
            break;
        }
        parent = child;
        {
            const EsaNode *node = &engine->nodes[parent];
            size_t low = 0;
            size_t high = node->group_count;
            while (low < high) {
                size_t mid = low + (high - low) / 2;
                ++metrics->rule_candidates;
                if (engine->priorities[node->first_group + mid] < cursor) {
                    low = mid + 1;
                } else {
                    high = mid;
                }
            }
            if (low < node->group_count) {
                size_t group = engine->priorities[node->first_group + low];
                if (best == ESA_NONE || group < best) {
                    best = group;
                }
                if (best == cursor) {
                    return best;
                }
            }
        }
    }
    return best;
}

EsaStatus esaStemInto(const Esa *esa, const char *token, size_t length,
                     char *output, size_t capacity, size_t *output_length,
                     EsaMetrics *metrics)
{
    EsaMetrics local = {0};
    EsaStatus status;
    size_t required;
    size_t written;
    size_t cursor = 0;
    int eligible;
    int changed = 0;

    if (output_length != NULL) {
        *output_length = 0;
    }
    if (metrics != NULL) {
        memset(metrics, 0, sizeof(*metrics));
    }
    if (token == NULL || output == NULL) {
        return ESA_INVALID_ARGUMENT;
    }
    status = esaRequiredCapacity(esa, length, &required);
    if (status != ESA_OK) {
        return status;
    }
    if (capacity < required) {
        return ESA_BUFFER_TOO_SMALL;
    }
    written = normalize(token, length, output, &eligible);
    local.input_bytes = length;
    if (eligible) {
        while (cursor < esa->engine->info.rule_groups) {
            size_t index = next_group(esa->engine, output, written,
                                      cursor, &local);
            const EsaGroup *group;
            size_t i;
            if (index == ESA_NONE) {
                break;
            }
            group = &esa->engine->groups[index];
            for (i = 0; i < group->step_count; ++i) {
                const EsaStep *step =
                    &esa->engine->steps[group->first_step + i];
                ++local.rule_steps;
                /* Check subtraction before doing it, including huge removals. */
                if (!matches(output, written, step) ||
                    (step->intact && changed) ||
                    step->remove > written ||
                    written - step->remove < ESA_MIN_STEM_LENGTH) {
                    break;
                }
                written -= step->remove;
                memcpy(output + written, step->append, step->append_length);
                written += step->append_length;
                output[written] = '\0';
                changed = 1; /* A successful no-op also consumes intactness. */
                ++local.rules_applied;
                if (esa->debug_mode) {
                    fprintf(stderr, "[esa] group=%zu step=%zu token=\"%s\"\n",
                            index, i, output);
                }
            }
            cursor = index + 1;
        }
    }
    local.output_bytes = written;
    if (output_length != NULL) {
        *output_length = written;
    }
    if (metrics != NULL) {
        *metrics = local;
    }
    return ESA_OK;
}

EsaStatus esaStemAlloc(const Esa *esa, const char *token, size_t length,
                      char **output, EsaMetrics *metrics)
{
    EsaStatus status;
    size_t capacity;
    char *result;
    if (metrics != NULL) {
        memset(metrics, 0, sizeof(*metrics));
    }
    if (output == NULL) {
        return ESA_INVALID_ARGUMENT;
    }
    *output = NULL;
    if (token == NULL) {
        return ESA_INVALID_ARGUMENT;
    }
    status = esaRequiredCapacity(esa, length, &capacity);
    if (status != ESA_OK) {
        return status;
    }
    result = ESA_MALLOC(capacity);
    if (result == NULL) {
        return ESA_OUT_OF_MEMORY;
    }
    status = esaStemInto(esa, token, length, result, capacity, NULL, metrics);
    if (status != ESA_OK) {
        ESA_FREE(result);
        return status;
    }
    *output = result;
    return ESA_OK;
}

char *esaStemToken(Esa *esa, char *token, int length)
{
    char *output = NULL;
    if (length > 0) {
        (void)esaStemAlloc(esa, token, (size_t)length, &output, NULL);
    }
    return output;
}
