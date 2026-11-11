/* SPDX-License-Identifier: MIT */
#include "esa.h"
#include "esa_rules.h"
#include "reference.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    Esa esa;
    char output[4096];
    char expected[4096];
    char malformed[513];
    char storage[16][64];
    const char *rules[16];
    size_t i;
    size_t count = size < 16 ? size : 16;
    if (size > 512) {
        return 0;
    }
    if (esaInitDefault(&esa) != ESA_OK) {
        abort();
    }
    if (esaStemInto(&esa, (const char *)data, size, output, sizeof(output),
                    NULL, NULL) != ESA_OK ||
        !reference_stem(ESA_RULE_LIST, ESA_DEFAULT_RULE_COUNT,
                        (const char *)data, size, expected, sizeof(expected)) ||
        strcmp(output, expected) != 0) {
        abort();
    }
    esaFree(&esa);
    for (i = 0; i < count; ++i) {
        unsigned int byte = data[i];
        (void)snprintf(storage[i], sizeof(storage[i]), "%c,%c,%u,%c,%c,n,%u,",
                       'a' + (int)(byte % 26u), (byte & 1u) != 0 ? 'y' : 'n',
                       byte % 8u, 'a' + (int)((byte / 2u) % 26u),
                       'a' + (int)((byte / 3u) % 26u), byte % 5u);
        rules[i] = storage[i];
    }
    if (esaInitRules(&esa, rules, count) != ESA_OK ||
        esaStemInto(&esa, (const char *)data, size, output, sizeof(output),
                    NULL, NULL) != ESA_OK ||
        !reference_stem(rules, count, (const char *)data, size,
                        expected, sizeof(expected)) || strcmp(output, expected) != 0) {
        abort();
    }
    esaFree(&esa);
    memcpy(malformed, data, size);
    malformed[size] = '\0';
    rules[0] = malformed;
    (void)esaInitRules(&esa, rules, 1);
    esaFree(&esa);
    return 0;
}
