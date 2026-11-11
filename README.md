# English Stemming Algorithm (ESA)

A small C11 library for reducing English word variants to common stems. ESA
applies ordered suffix rules, including conditional chains of transformations.
The original algorithm is by Joshua S. English (2003); this repository contains
its C reference implementation and a compiled rules engine.

```text
Stemming  → stem
jumped    → jump
proper    → prop
(ESA)     → esa
```

Stems are retrieval keys, so they need not be dictionary words. ESA is a distinct
algorithm; its default rules do not promise identical output to Porter or
Lancaster stemming.

## Build and run

A C11 compiler and Make are sufficient for the library and example. The library
has no third-party dependencies.

```sh
make
./build/esaExample Stemming jumped proper '(ESA)'
# Or compile and run the supplied example:
./buildExample.sh
```

Build outputs stay in `build/`. `make` produces `libesa.a` and the example.
You can also compile `esa.c` directly into your application:

```sh
cc -std=c11 -O3 -I/path/to/esa application.c /path/to/esa/esa.c -o application
```

## Use the library

```c
#include "esa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    Esa esa;
    char *stem = NULL;
    const char *word = "Stemming";
    EsaStatus status = esaInitDefault(&esa);
    if (status != ESA_OK) {
        fprintf(stderr, "%s\n", esaStatusString(status));
        return EXIT_FAILURE;
    }
    status = esaStemAlloc(&esa, word, strlen(word), &stem, NULL);
    if (status == ESA_OK) {
        puts(stem);
        free(stem);
    }
    esaFree(&esa);
    return status == ESA_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
```

For batch processing, `esaStemInto` writes to a caller-owned buffer without
allocating. `esaRequiredCapacity` computes a safe capacity, and optional
`EsaMetrics` counters explain rule execution. Custom rule sets use
`esaInitRules`. The original `esaInit`, `esaSetDebugMode`, `esaStemToken`, and
`esaFree` entry points remain available.

## Behavior and compatibility

- Inputs are byte spans, with no required NUL terminator. ASCII letters are
  lowercased; all other bytes are removed. Tokenize text before stemming.
- Rules run only if the normalized token has at least five letters, including
  a vowel and a consonant. `y` always counts as a vowel.
- Every removal must leave at least three letters before appending anything.
- Rule groups run in declaration order. Chains are conditional, and earlier
  groups are never restarted.
- An initialized engine can serve concurrent readers with separate output
  buffers. Initialization, destruction, and debug-mode changes require
  exclusive access.

Normalization and three corpus outputs have intentionally changed to fix bugs.
The `Esa` struct layout also changed: **recompile consumers**, and stop accessing
its internals. Read the [migration guide](docs/migration.md) before upgrading.
This library handles ASCII English tokens; it does not perform Unicode case
folding, tokenization, lemmatization, or language detection.

## Verify and measure

```sh
make check             # Strict C/C++ builds, unit/differential/concurrency tests
make sanitize          # AddressSanitizer + UndefinedBehaviorSanitizer
make thread-sanitize   # ThreadSanitizer, where supported
make coverage          # Clang/LLVM coverage + HTML report; needs Python 3.10+
make tooling-test      # Benchmark argument, workload, and report regression tests
make benchmark         # Raw timing samples + statistics in build/benchmark.json
make fuzz              # Optional libFuzzer run; needs its Clang runtime
# Select another compiler if the default Clang lacks that runtime:
make FUZZ_CC=/path/to/clang fuzz
```

The suite includes 218 fixed regression cases, 70,000 generated differential
cases, 40,000 concurrent calls, allocation fault injection, a 10,000-rule
scalability check, and 10,000 deterministic fuzz inputs. The
[performance report](docs/performance.md) records measured results and tradeoffs;
benchmarks compare against historical ESA, with no claim about other stemmers.

## Documentation

- [API and ownership](docs/api.md)
- [Rule grammar and examples](docs/rules.md)
- [Engine architecture](docs/architecture.md)
- [Testing and tooling](docs/testing.md)
- [Performance methodology and results](docs/performance.md)
- [Compatibility and migration](docs/migration.md)
- [Repository review and resolved defects](docs/review.md)
- [Contributing](CONTRIBUTING.md) and [change log](CHANGELOG.md)

Licensed under the [MIT License](LICENSE).
