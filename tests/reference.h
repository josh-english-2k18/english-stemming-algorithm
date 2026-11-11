#ifndef ESA_TEST_REFERENCE_H_INCLUDED
#define ESA_TEST_REFERENCE_H_INCLUDED
#include <stddef.h>
/* Independent, deliberately simple interpreter for valid test rule sets. */
int reference_stem(const char *const *rules, size_t count,
                   const char *input, size_t length,
                   char *output, size_t capacity);
#endif
