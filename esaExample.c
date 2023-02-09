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

// preprocessor directives

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esa.h"

// define esa example data

static const char *EXAMPLE_TOKENS[] = {
	"This",
	"is",
	"a",
	"test",
	"of",
	"the",
	"English",
	"Stemming",
	"Algorithm",
	"(ESA)",
	"(English,",
	"Joshua",
	"S.",
	"2003).",
	"Here",
	"is",
	"a",
	"typical",
	"sentence:",
	"the",
	"quick",
	"brown",
	"fox",
	"jumped",
	"over",
	"the",
	"lazy",
	"dogs.",
	"Theses",
	"here",
	"words",
	"type",
	"thingamajiggers",
	"will",
	"hopefully",
	"be",
	"correctly",
	"stemmed",
	"by",
	"this",
	"heary",
	"algorithmic",
	"and",
	"stemmed",
	"out",
	"to",
	"their",
	"proper",
	"selves",
	NULL
};

// main function

int main()
{
	int ii = 0;
	int tokenLength = 0;
	char *token = NULL;
	char *stem = NULL;

	Esa esa;

	printf("Example output from ESA:\n\n");

	esaInit(&esa);
	esaSetDebugMode(&esa, (unsigned char)0);

	for(ii = 0; EXAMPLE_TOKENS[ii] != NULL; ii++) {
		token = (char *)EXAMPLE_TOKENS[ii];
		tokenLength = strlen(token);

		stem = esaStemToken(&esa, token, tokenLength);

		printf("\t ...token '%s' => transform => '%s'\n", token, stem);

		if(stem != NULL) {
			free(stem);
		}
	}

	esaFree(&esa);

	printf("\n\n...Example completed.\n");

	return 0;
}

