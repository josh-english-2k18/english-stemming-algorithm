/* SPDX-License-Identifier: MIT */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int main(void)
{
    static const char *const seeds[] = {
        "", "Stemming", "s,n,1,,ies,n,2,", "e,n,0,abcdefghijklmn",
        "s,y,99999999999999999999999999999999999,", "abccde", "abcdes"
    };
    uint32_t state = UINT32_C(0xd00dfeed);
    uint8_t data[512] = {0};
    size_t iteration;
    for (iteration = 0; iteration < 10000; ++iteration) {
        size_t length;
        size_t i;
        state = state * UINT32_C(1664525) + UINT32_C(1013904223);
        if (iteration % 2u == 0) {
            const char *seed = seeds[iteration % (sizeof(seeds) / sizeof(seeds[0]))];
            length = strlen(seed);
            memcpy(data, seed, length);
            if (length > 0) {
                data[state % length] ^= (uint8_t)(state >> 24);
            }
        } else {
            length = state % sizeof(data);
            for (i = 0; i < length; ++i) {
                state = state * UINT32_C(1664525) + UINT32_C(1013904223);
                data[i] = (uint8_t)(state >> 24);
            }
        }
        (void)LLVMFuzzerTestOneInput(data, length);
    }
    puts("PASS: 10,000 deterministic fuzz harness inputs");
    return 0;
}
