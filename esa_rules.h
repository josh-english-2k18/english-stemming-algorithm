/*
 * esa_rules.h
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

#if !defined(_ESA_RULES_H)

#define _ESA_H

#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus

// define esa rules constants

const char *ESA_RULE_LIST[] = {
	"s,n,1,,ies,n,2,,si,n,1,s",
	"ed,n,2,,e,n,1,",
	"est,y,2,,gge,n,2,",
	"ing,n,3,",
	"at,n,2,ate",
	"bl,n,2,ble",
	"iz,n,2,ize",
	"y,n,1,i",
	"ional,n,2,,ation,n,5,ate",
	"nci,n,3,nce"
	"izer,n,1,",
	"bli,n,1,e",
	"alli,n,2,",
	"entli,n,2,",
	"eli,n,2,",
	"ousli,n,2,",
	"sion,y,4,",
	"ion,n,3,,t,n,1,,a,n,0,te,izate,n,3,e",
	"ator,n,2,e",
	"alism,n,3,",
	"ness,n,4,",
	"aliti,n,3,",
	"iviti,n,3,e",
	"biliti,n,5,le",
	"logi,n,1,",
	"icate,n,3,",
	"iciti,n,3,",
	"ical,n,2,",
	"ative,n,5,",
	"alize,n,3,",
	"ful,n,3,",
	"al,y,2,",
	"ance,n,4,",
	"ence,n,4,",
	"er,n,2,",
	"ic,y,2,",
	"able,n,4,",
	"ible,n,4,",
	"ant,n,3,",
	"ou,n,2,",
	"ism,n,3,",
	"at,n,2,",
	"idi,n,3,",
	"ize,n,3,",
	"ll,n,2,l",
	"i,n,1,y",
	"e,n,1,,e,n,1,",
	"aa,n,1,",
	"bb,n,1,",
	"cc,n,1,",
	"dd,n,1,",
	"ff,n,1,",
	"gg,n,1,",
	"hh,n,1,",
	"ii,n,1,",
	"jj,n,1,",
	"kk,n,1,",
	"mm,n,1,",
	"nn,n,1,",
	"oo,n,1,",
	"pp,n,1,",
	"qq,n,1,",
	"rr,n,1,",
	"tt,n,1,",
	"uu,n,1,",
	"vv,n,1,",
	"ww,n,1,",
	"xx,n,1,",
	"yy,n,1,",
	NULL
};

#if defined(__cplusplus)
};
#endif // __cplusplus

#endif // _ESA_RULES_H

