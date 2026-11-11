# Testing and development checks

Run commands from the repository root. The production library needs only C11;
tests add a C++11 compiler and POSIX threads. Python 3.10+ is needed for coverage
and benchmark tooling. No Python packages are required.

## Required checks

```sh
make -j4 check
make sanitize
make thread-sanitize
make coverage
make tooling-test
clang --analyze -Xanalyzer -analyzer-werror -std=c11 -Wall -Wextra esa.c
```

Builds enable `-Wall -Wextra -Wpedantic -Wconversion -Wshadow` and `-Werror`,
plus C strict-prototype checks. Sanitizers use separate build directories so
release and instrumented objects do not mix. When changing compilers or build
flags, choose a fresh directory, for example:

```sh
make BUILD_DIR=build/gcc CC=gcc CXX=g++ check
```

Clang coverage produces `build/coverage/html/index.html`. `tools/coverage.py`
finds LLVM tools directly or through macOS `xcrun`, clears old profiles, runs
the tests, and combines normal and fault-injection objects for engine-only
reporting. It forces an instrumented rebuild so previous compiler flags cannot
silently leave stale objects in the report. Coverage is evidence of exercised
code, rather than proof of correctness. Size-overflow branches that would need
unaddressably large rule strings are retained even when practical tests cannot
execute them.

## Suite design

| Check | Scope |
| --- | --- |
| Golden corpus | 218 lower-case alphabetic cases captured from historical ESA, with three documented corrections |
| Normalization | Case, punctuation, whitespace, all 256 byte values, embedded NULs, exact spans, in-place use, empty and 100,000-byte inputs |
| Rule semantics | Priority vs suffix length, chains, failed continuations, intactness, no-ops, expansion, minimum lengths, no restart/repetition, 4,096-letter suffixes, 1,024-letter appends, numeric limits |
| Errors | NULL/uninitialized arguments, invalid grammar, decimal overflow, capacity overflow, untouched buffers, cleared metrics, legacy failures |
| Metrics | Known application counts, skipped intact rules, exact conservative capacities, zero-rule behavior |
| Differential | 30,000 arbitrary-byte inputs with default rules; 40,000 inputs across 200 generated custom sets, each using exactly the reported capacity |
| Scalability | 10,000 duplicate suffix groups with a bound on priority probes |
| Allocation faults | Every allocation failure site for default/empty initialization and result allocation; no leaks; no allocation on buffer path |
| Concurrency | Four threads share one engine for 40,000 calls |
| C++ | Repeated includes in reverse header order; linkage and ownership |
| Fuzz smoke | 10,000 seeded, mutated, and arbitrary-byte inputs through the fuzz harness |
| Benchmark tooling | Malformed counts and corpora, workload sizes, operation counts, buffer/alloc fingerprints, runner argument validation, and single-sample reports |

The independent interpreter in `tests/reference.c` reparses valid rules and
runs a plain ordered loop. It shares no production parser, matching, or
selection functions. Its fixed field buffers suit generated test rules, not
arbitrary production input. Random tests use fixed seeds for reproducibility.
The golden corpus is committed data; tests never regenerate expected values
from the implementation under test.

`tests/test_oom.c` compiles a private copy of `esa.c` with allocator overrides.
It fails one allocation at a time, tracks live allocations, and verifies cleanup
and statuses. The production header exposes no fault-injection controls.

## Fuzzing

`make check` runs the deterministic smoke driver for the same entry point used
by libFuzzer. With a Clang distribution that includes its libFuzzer runtime:

```sh
make fuzz
# Or select a Clang distribution with the runtime installed:
make FUZZ_CC=/path/to/clang fuzz
# Continue with retained inputs for a longer session:
./build/fuzz_esa -max_total_time=300 -max_len=512 build/fuzz-corpus
```

The harness compares default and generated custom engines with the independent
interpreter, then sends arbitrary rule text through validation and cleanup.
Address and undefined-behavior sanitizers are enabled for libFuzzer. Corpus
mutations and crash artifacts stay untracked; reduce a failure and add a small
regression case before fixing it.

## Local verification

On macOS arm64 with Apple Clang 21, the final suite passed 1,455,409 main-test
checks, all nine allocation failure points, the C++ consumer, and the 10,000
fuzz-smoke inputs. Release, AddressSanitizer/UndefinedBehaviorSanitizer, and
ThreadSanitizer runs passed. Clang static analysis reported no findings.
LLVM reports 97.67% engine line coverage, 100% function coverage, and 94.57%
branch coverage for this configuration.

The default Apple Clang lacks `libclang_rt.fuzzer_osx.a`. A second review found
an installed MacPorts Clang 23.1.4 with the runtime. Running
`make BUILD_DIR=build/review/fuzz FUZZ_CC=clang-mp-23 fuzz` completed 375,775
coverage-guided fuzz executions in 21 seconds without failures. The deterministic
harness also passed under release, address/undefined-behavior, and thread
sanitizer configurations. Five Python tooling tests passed, and the C/C++ suite
also passed with Clang 23. Hosted CI is configured for Linux/GCC/Clang as well;
those remote runs have not been observed as part of this local work.

## Performance checks

Use [the benchmark runner](performance.md) for timing. Unit tests assert
behavior and algorithmic probe bounds rather than noisy time thresholds. CI
runs a small benchmark smoke check and archives JSON and coverage HTML; it
does not impose throughput thresholds on shared runners.
