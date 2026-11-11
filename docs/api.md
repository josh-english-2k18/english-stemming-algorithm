# API reference

Include `esa.h`; link `esa.c` or `build/libesa.a`. The implementation requires
C11. The header also supports C++11 and later with C linkage. All lengths and
counts in the new API are `size_t`.

## Lifecycle and ownership

`Esa` owns a compiled, immutable rule set. Initialize fresh storage with either:

```c
EsaStatus esaInitDefault(Esa *esa);
EsaStatus esaInitRules(Esa *esa, const char *const *rules, size_t count);
```

`esaInitDefault` loads the built-in 69 groups and 78 steps. `esaInitRules` copies
exactly `count` NUL-terminated rule strings; the array needs no sentinel. You
may release or modify the source strings after the call. `NULL, 0` creates a
normalization-only engine. Malformed rules fail the entire initialization.

An initialization failure leaves the supplied `Esa` empty and safe to destroy.
`esaFree(&esa)` releases all engine allocations and clears the object; repeated
calls are safe. `esaFree(NULL)` is a no-op. A zero-initialized `Esa` is also safe
to destroy. Never destroy storage that has neither been zeroed nor initialized.

Free an existing engine **before** initializing that object again. Do not copy
an owning `Esa` by assignment, edit its members, or free `engine` yourself.
Copying the struct would duplicate ownership of the same allocation.

## Stemming

```c
EsaStatus esaStemAlloc(const Esa *esa, const char *token, size_t length,
                      char **output, EsaMetrics *metrics);

EsaStatus esaRequiredCapacity(const Esa *esa, size_t length, size_t *capacity);

EsaStatus esaStemInto(const Esa *esa, const char *token, size_t length,
                     char *output, size_t capacity, size_t *output_length,
                     EsaMetrics *metrics);
```

The caller must provide `length` readable input bytes. Embedded NULs are removed
like other nonletters; they do not end the span. A non-NULL empty string with
length zero is valid. No function infers the readable extent of your memory.

`esaStemAlloc` allocates one NUL-terminated result. On success, release it using
`free`. On failure, `*output` is NULL. An empty result is still an allocated
string that must be freed.

`esaRequiredCapacity` returns `length + maximum_growth + 1`, with checked size
arithmetic. The bound includes a NUL and the sum of every step's positive net
growth, including mutually exclusive steps. It is deliberately conservative.
The built-in rules have a maximum growth bound of five bytes. Buffer sizes
smaller than this bound are rejected even when a particular word would fit.

`esaStemInto` performs no heap allocations in the engine. Size a reusable buffer
for the largest input in a batch:

```c
size_t capacity;
EsaStatus status = esaRequiredCapacity(&esa, largest_token_length, &capacity);
if (status == ESA_OK) {
    char *buffer = malloc(capacity);
    if (buffer != NULL) {
        size_t written;
        status = esaStemInto(&esa, "Stemming", 8, buffer, capacity, &written, NULL);
        /* On success: buffer == "stem", written == 4. */
        free(buffer);
    }
}
```

This fragment assumes `largest_token_length >= 8`; handle allocation failure
in your application. Normalization can run in place when `output == token` and
the writable buffer has the required capacity. Other input/output overlap is
unsupported. Metadata outputs must not alias input, output, or engine storage.

On a reported error, `esaStemInto` leaves the output buffer unchanged, sets
`*output_length` to zero when supplied, and clears the metrics when supplied.
On success, `output_length` excludes the NUL. Both metadata pointers are optional.
All validation happens before normalization writes the output.

## Status codes

`esaStatusString(status)` provides a static human-readable description. It does
not allocate, and its returned string must not be freed.

| Code | Meaning |
| --- | --- |
| `ESA_OK` | Operation completed |
| `ESA_INVALID_ARGUMENT` | A required pointer is NULL |
| `ESA_INVALID_RULE` | A rule violates the grammar |
| `ESA_OUT_OF_MEMORY` | An allocation failed; partial state was released |
| `ESA_BUFFER_TOO_SMALL` | Capacity is below the conservative bound |
| `ESA_OVERFLOW` | A size calculation or rule removal number exceeds `SIZE_MAX` |
| `ESA_NOT_INITIALIZED` | The object has no compiled engine |

`esaRequiredCapacity` clears its capacity output on engine/overflow errors.
`esaGetEngineInfo` clears its info output for an uninitialized engine. If a
required output pointer itself is NULL, the operation returns invalid argument.
As with any C API, dangling pointers and incorrect readable/writable lengths
are caller errors.

## Inspection and diagnostics

`esaGetEngineInfo(esa, &info)` returns group count, step count, actual trie node
count, the growth bound, and requested engine allocation bytes. The byte total
includes reserved array capacity and owned strings, but excludes allocator
metadata, stack memory, and output buffers.

Optional per-call `EsaMetrics` fields are:

| Field | Count |
| --- | --- |
| `input_bytes` | Supplied span length |
| `output_bytes` | Result length, excluding NUL |
| `trie_steps` | Root lookups and trie edge comparisons, including sibling scans |
| `rule_candidates` | Priority entries examined by terminal binary searches |
| `rule_steps` | Steps evaluated, including blocked or failed continuations |
| `rules_applied` | Successful transformations, including successful no-ops |

Counters are local to the call. They accumulate across all selected groups,
with no shared state. `esaSetDebugMode(esa, nonzero)` logs successful steps to
stderr, with zero-based group and step indices and the current token. Debug
output has I/O overhead and may allocate inside the C runtime; disable it for
performance measurements.

## Threading and legacy calls

Share an initialized engine across readers while giving each call independent
output and metadata storage. Keep the source rule strings stable during
initialization. Do not initialize, free, or change debug mode while other
threads use the object. The library has no global mutable state.

`esaInit(Esa *)` calls `esaInitDefault` and discards its status. Prefer the
status-returning initializer for new code. `esaStemToken(Esa *, char *, int)`
returns an owned result or NULL, leaves the input untouched, and rejects zero
or negative lengths. The new APIs accept const inputs and empty spans. None of
the error paths prints diagnostics automatically.
