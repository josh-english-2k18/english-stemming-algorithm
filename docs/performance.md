# Performance report

## Reproduce

```sh
make benchmark
# Include the original implementation from local Git history:
python3 benchmarks/run.py --baseline-ref 8201da3 --output build/comparison.json
# Increase sample size, or choose a compiler:
CC=clang python3 benchmarks/run.py --samples 15 --rounds 10000 --output build/long-run.json
```

The runner uses only Python's standard library and a C compiler. An optional
baseline is extracted from the requested local commit into a temporary directory;
it does not check out or alter the working tree. Compilation and subprocess startup
are outside the C timer. No network access is required.

## Methodology

- Release builds use `-O3 -DNDEBUG`, C11 for the current engine and GNU C11 for
  the historical source. Both use the same compiler and benchmark harness.
- The 218-word regression corpus is equally weighted. `plain` uses its lowercase
  words; `mixed` uppercases and surrounds each with punctuation/digits; `long`
  prepends 512 lowercase letters. These are microbenchmarks, not a distribution
  sampled from production documents.
- Each C process performs an untimed corpus warmup. Each job also has one
  discarded full-process warmup before recorded sampling.
- Recorded jobs run round-robin with a deterministically shuffled order for
  each round, distributing scheduling and thermal drift across configurations.
- Each timed stemming sample makes 5,000 corpus passes: 1,090,000 tokens.
  Seven samples are recorded per job. Initialization uses 2,000 initialize/free
  pairs per sample. `CLOCK_MONOTONIC` measures elapsed wall time.
- `alloc` includes result allocation and free; `buffer` reuses a fixed output
  buffer. Both include normalization, stemming, and a checksum based on output
  length and first byte. A separate complete output fingerprint guards against
  inconsistent results; the runner checks buffer/alloc fingerprints agree.
- JSON contains raw elapsed times, operation counts, fingerprints, median,
  mean, standard deviation, range, sample-mean p95, throughput, input MiB/s,
  compiler/platform metadata, source/corpus hashes, and untimed rule counters.
- The p95 describes whole-sample means. It does **not** measure individual-token
  tail latency. No allocation profiler, CPU cycle counter, or RSS sampler is used.

## Recorded local results

Platform: `macOS-26.6.2-arm64-arm-64bit`; Apple clang version 21.0.0 (clang-2100.3.34.2);
arm64, with no sanitizers. This is a local snapshot, not a performance guarantee.
The committed [raw report](../benchmarks/results.json) includes the exact UTC
recording time, toolchain details, baseline commit, and source hashes.

| Implementation | Workload | API | Median ns/token | Sample-mean p95 ns/token | Million tokens/s | Input MiB/s |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| current | plain | alloc | 41.90 | 43.48 | 23.87 | 167.37 |
| current | plain | buffer | 32.05 | 33.35 | 31.20 | 218.79 |
| current | mixed | alloc | 46.14 | 46.94 | 21.67 | 255.30 |
| current | mixed | buffer | 33.54 | 33.65 | 29.81 | 351.20 |
| current | long | alloc | 436.67 | 446.51 | 2.29 | 1134.25 |
| current | long | buffer | 425.85 | 436.09 | 2.35 | 1163.06 |
| historical | plain | alloc | 163.67 | 167.18 | 6.11 | 42.85 |
| historical | mixed | alloc | 173.36 | 179.51 | 5.77 | 67.96 |
| historical | long | alloc | 638.09 | 715.59 | 1.57 | 776.21 |

On the plain corpus, allocating calls were **3.91×** as fast as historical ESA,
and the buffer API was **5.11×** as fast as historical allocating calls.
API differences are included in that second comparison. Results for other token
sizes and applications can differ; use your own corpus before making capacity
or latency decisions.

Initialization plus destruction cost **3.064 µs** for the compiled engine versus
**1.229 µs** historically. Validation, copying, and index construction make
initialization more expensive. Initialize once and reuse the engine across many
words. The measurements do not support a claim that every operation is faster.

## Engine work and memory

The built-in engine contains 69 groups, 78 steps, and 134 actual trie nodes.
It requests **15,038 bytes** (14.69 KiB) across six owned allocations.
This includes reserved node capacity and strings, excludes allocator bookkeeping,
and is not a measurement of resident memory. The growth bound is five bytes, so
a caller-owned buffer sized for an input of `N` bytes needs `N + 6` bytes.

For one plain-corpus pass, measured outside timed loops:

| Counter | Total | Per token |
| --- | ---: | ---: |
| Trie edge/root probes | 1,819 | 8.34 |
| Terminal priority probes | 421 | 1.93 |
| Steps evaluated | 397 | 1.82 |
| Transformations applied | 272 | 1.25 |

The interpreter previously scanned its raw group list per eligible token. The
new engine skips unmatched suffixes, parses no rules during stemming, and uses
no engine heap allocation on the buffer path. An allocating call makes one
output allocation. These allocation properties are checked by fault injection,
not inferred from timings.

## Interpretation and limits

The historical rules have a missing comma, and historical normalization is
incorrect for punctuation and other nonletters. Three plain-corpus outputs
differ; mixed-workload outputs also differ substantially. Consequently the
benchmarks compare implementations with the documented correctness fixes,
rather than byte-identical algorithms. See [migration](migration.md).

Shared-host activity, CPU scheduling, cache state, thermal limits, allocator
behavior, and compiler choice can change results. Raw ranges and standard
deviations remain in JSON; interleaving reduces drift but does not remove noise.
Instrumentation is disabled for timing. CI runs only a small benchmark smoke
check and imposes no timing threshold. There is no measured comparison against
Porter or Lancaster in this report.
