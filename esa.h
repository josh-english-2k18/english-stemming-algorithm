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
 * Notes:
 *
 * The English Stemming Algorithm (ESA) (see English, Joshua S. 2003)
 * reference implementation for reducing the morphological variation of any
 * given English Lanugage token through ESA-type conflation.
 *
 * Written by Joshua S. English.
 *
 * Boundary Conditions:
 *
 * 		1. input must be greater than 2 characters in length
 * 			a. has at least one vowel
 * 			b. has at least one consonant
 *
 * 		2. output must be:
 * 			a. greater than 2 characters in length
 *
 * Rule Definition:
 *
 * 		1. consists of 4 parts that may be repeated
 * 			a. an ending of 1 or more characters
 * 			b. an intact flag (only modify the token if it has not already
 * 			   been modified)
 * 			c. the number of characters to remove, 0 or more
 * 			d. an append string of 1 or more characters
 *
 * 		2. examples:
 * 			a. 'ies,n,3,y'
 * 			b. 'mu,y,2,'
 * 			c. 'ply,n,0,'
 * 			d. 'soin,n,4,j'
 */

#if !defined(_ESA_H)

#define _ESA_H

#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus

// define esa data types

typedef struct _EsaRuleList {
	int ref;
	int size;
	int *length;
	char **value;
} EsaRuleList;

typedef struct _Esa {
	unsigned char debug_mode;
	int longest_rule_size;
	EsaRuleList rules;
} Esa;


// declare esa public functions

void esaInit(Esa *esa);

void esaFree(Esa *esa);

void esaSetDebugMode(Esa *esa, unsigned char mode);

char *esaStemToken(Esa *esa, char *token, int length);

#if defined(__cplusplus)
};
#endif // __cplusplus

#endif // _ESA_H

