/*
 * esaExample.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const EXAMPLE_TOKENS[] = {
    "This", "is", "a", "test", "of", "the", "English", "Stemming", "Algorithm",
    "(ESA)", "(English,", "Joshua", "S.", "2003).", "Here", "is", "a", "typical",
    "sentence:", "the", "quick", "brown", "fox", "jumped", "over", "the", "lazy",
    "dogs.", "Theses", "here", "words", "type", "thingamajiggers", "will",
    "hopefully", "be", "correctly", "stemmed", "by", "this", "heary",
    "algorithmic", "and", "stemmed", "out", "to", "their", "proper", "selves"
};

int main(int argc, char **argv)
{
    Esa esa;
    EsaStatus status = esaInitDefault(&esa);
    size_t i;
    size_t count = argc > 1 ? (size_t)(argc - 1) :
                   sizeof(EXAMPLE_TOKENS) / sizeof(EXAMPLE_TOKENS[0]);
    if (status != ESA_OK) {
        fprintf(stderr, "ESA initialization failed: %s\n", esaStatusString(status));
        return EXIT_FAILURE;
    }
    puts("Example output from ESA:\n");
    for (i = 0; i < count; ++i) {
        const char *token = argc > 1 ? argv[i + 1] : EXAMPLE_TOKENS[i];
        char *stem = NULL;
        status = esaStemAlloc(&esa, token, strlen(token), &stem, NULL);
        if (status != ESA_OK) {
            fprintf(stderr, "Stemming failed: %s\n", esaStatusString(status));
            esaFree(&esa);
            return EXIT_FAILURE;
        }
        printf("  '%s' => '%s'\n", token, stem);
        free(stem);
    }
    esaFree(&esa);
    puts("\nExample completed.");
    return EXIT_SUCCESS;
}
