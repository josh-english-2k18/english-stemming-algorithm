/* SPDX-License-Identifier: MIT */
#include "reference.h"
#include <stdlib.h>
#include <string.h>

int reference_stem(const char *const *rules, size_t count,
                   const char *input, size_t length,
                   char *output, size_t capacity)
{
    size_t n = 0;
    size_t i;
    int vowel = 0;
    int consonant = 0;
    int changed = 0;
    for (i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)input[i];
        if (c >= 65 && c <= 90) {
            c = (unsigned char)(c + 32);
        }
        if (c >= 97 && c <= 122) {
            if (n + 1 >= capacity) {
                return 0;
            }
            output[n++] = (char)c;
            if (strchr("aeiouy", (int)c) != NULL) {
                vowel = 1;
            } else {
                consonant = 1;
            }
        }
    }
    if (n >= capacity) {
        return 0;
    }
    output[n] = '\0';
    if (n < 5 || !vowel || !consonant) {
        return 1;
    }
    for (i = 0; i < count; ++i) {
        const char *p = rules[i];
        for (;;) {
            char parts[4][128] = {{0}};
            size_t part;
            size_t suffix_length;
            size_t remove;
            size_t append_length;
            int more = 0;
            for (part = 0; part < 4; ++part) {
                size_t j = 0;
                while (*p != '\0' && *p != ',') {
                    if (j + 1 >= sizeof(parts[part])) {
                        return 0;
                    }
                    parts[part][j++] = *p++;
                }
                more = *p == ',';
                if (more) {
                    ++p;
                }
            }
            suffix_length = strlen(parts[0]);
            remove = (size_t)strtoull(parts[2], NULL, 10);
            append_length = strlen(parts[3]);
            if (suffix_length > n ||
                strcmp(output + n - suffix_length, parts[0]) != 0 ||
                ((parts[1][0] == 'y' || parts[1][0] == 'Y') && changed) ||
                remove > n || n - remove < 3) {
                break;
            }
            n -= remove;
            if (append_length >= capacity - n) {
                return 0;
            }
            memcpy(output + n, parts[3], append_length + 1);
            n += append_length;
            changed = 1;
            if (!more) {
                break;
            }
        }
    }
    return 1;
}
