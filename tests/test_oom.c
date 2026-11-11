/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t allocation_calls;
static size_t fail_at;
static size_t live_allocations;

static void *checked_malloc(size_t size)
{
    void *result;
    if (++allocation_calls == fail_at) {
        return NULL;
    }
    result = malloc(size);
    if (result != NULL) {
        ++live_allocations;
    }
    return result;
}

static void *checked_calloc(size_t count, size_t size)
{
    void *result;
    if (++allocation_calls == fail_at) {
        return NULL;
    }
    result = calloc(count, size);
    if (result != NULL) {
        ++live_allocations;
    }
    return result;
}

static void checked_free(void *pointer)
{
    if (pointer != NULL) {
        if (live_allocations == 0) {
            abort();
        }
        --live_allocations;
    }
    free(pointer);
}

#define ESA_MALLOC checked_malloc
#define ESA_CALLOC checked_calloc
#define ESA_FREE checked_free
#include "../esa.c"

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
        return EXIT_FAILURE; \
    } \
} while (0)

int main(void)
{
    size_t mode;
    size_t failures = 0;
    for (mode = 0; mode < 2; ++mode) {
        size_t position;
        for (position = 1; ; ++position) {
            Esa esa;
            EsaStatus status;
            allocation_calls = 0;
            fail_at = position;
            status = mode == 0 ? esaInitDefault(&esa) : esaInitRules(&esa, NULL, 0);
            if (status == ESA_OK) {
                esaFree(&esa);
                REQUIRE(live_allocations == 0);
                break;
            }
            REQUIRE(status == ESA_OUT_OF_MEMORY);
            REQUIRE(esa.engine == NULL);
            esaFree(&esa);
            REQUIRE(live_allocations == 0);
            ++failures;
        }
    }
    {
        Esa esa;
        EsaMetrics metrics;
        EsaMetrics zero = {0};
        char *output = (char *)"sentinel";
        char buffer[64];
        fail_at = 0;
        REQUIRE(esaInitDefault(&esa) == ESA_OK);
        allocation_calls = 0;
        fail_at = 1;
        memset(&metrics, 0xff, sizeof(metrics));
        REQUIRE(esaStemAlloc(&esa, "stemming", 8, &output, &metrics) == ESA_OUT_OF_MEMORY);
        REQUIRE(output == NULL && memcmp(&metrics, &zero, sizeof(metrics)) == 0);
        ++failures;
        allocation_calls = 0;
        REQUIRE(esaStemInto(&esa, "stemming", 8, buffer, sizeof(buffer),
                            NULL, NULL) == ESA_OK);
        REQUIRE(allocation_calls == 0 && strcmp(buffer, "stem") == 0);
        fail_at = 0;
        REQUIRE(esaStemAlloc(&esa, "stemming", 8, &output, NULL) == ESA_OK);
        checked_free(output);
        esaFree(&esa);
        REQUIRE(live_allocations == 0);
    }
    printf("PASS: all %zu allocation failure points; zero allocations in esaStemInto\n", failures);
    return EXIT_SUCCESS;
}
