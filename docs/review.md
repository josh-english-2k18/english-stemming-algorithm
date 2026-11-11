# Repository review

This review covers every original source/header, the example, build script,
README, and license. The original working tree was clean. Historical behavior
was captured before replacement, using commit `8201da3` for comparisons.

## Findings and resolutions

| Area | Finding | Resolution |
| --- | --- | --- |
| Normalization | Compaction omitted the final NUL, exposing stale contents | One bounded pass with an explicit terminator |
| Character handling | Impossible range test and signed-byte ambiguity | Deterministic ASCII checks using unsigned bytes |
| Suffix matching | A suffix longer than the token could partially match | Length guard before a bounded comparison |
| Default rules | Adjacent C literals accidentally merged `nci` and `izer` | Restore the missing comma; freeze corrected outputs |
| Rule execution | Repeated parsing and a full rule scan for each word | Compile typed steps and a reversed suffix index |
| Duplicate endings | Repeated priority scans could become quadratic | Sorted terminal slices with binary search |
| Allocation | Unchecked malloc/realloc/strdup results and nonportable strdup | Checked, standard C allocations with complete cleanup |
| Bounds | Fixed output slack could not safely support growing rules | Checked maximum-growth capacity bound |
| Parsing | atoi accepted malformed/overflowing removal counts | Strict grammar and checked decimal conversion |
| Debugging | `%s` could read past a length-delimited input | Log only owned, normalized, terminated output |
| Headers | Broken guard, reserved identifiers, mutable global rule definition | Independent guards, const internal-linkage rules, C++ smoke test |
| API | No status-returning initializer or caller-owned buffer operation | Explicit statuses, custom engines, buffer/alloc APIs, metrics |
| Example | Cast away const, narrowed strlen, printed NULL as a string | Const inputs, size_t lengths, checked errors and ownership |
| Build | Script lacked failure handling and emitted objects into the root | Strict Make build, isolated output, robust executable wrapper |
| Validation | No automated tests, coverage, or reproducible benchmarks | Golden/differential/fault/concurrency/fuzz tests and measurement tooling |
| Documentation | Boundary descriptions disagreed with code; performance claims lacked reproducible evidence | Exact contracts, migration notes, measured historical comparison |

The MIT license and original author attribution are preserved. No existing
user changes were overwritten, and no external services or dependencies were
added to the runtime library.

## Deliberate limits

The review does not establish linguistic optimality, equivalence to another
stemmer, Unicode support, or a stable binary ABI. Public pointers still require
correct C ownership and memory extents. Very large user-defined rule sets can
require correspondingly large memory and execution time. Initialization costs
more than the historical implementation because it validates and indexes the
rules; reuse engines across tokens.

The [testing report](testing.md) records executed checks and local toolchain
limits. CI configuration adds Linux/GCC/Clang coverage, but a local review does
not assert that hosted CI has already run.

## Follow-up audit

A second pass checked the source, ownership and growth invariants, regression
coverage, compiler/tooling behavior, and the raw benchmark calculations. It
found no additional stemming defects. It did identify and resolve these gaps:

- Randomized comparisons now use exactly the capacity returned by the engine,
  allowing AddressSanitizer to check the growth bound at allocation boundaries.
- Added long suffix/append, maximum removal, leading-zero, uppercase-flag, and
  in-place expansion cases; the fixed corpus cannot silently shrink below 218.
- Benchmark rounds now reject partial numbers, signs, whitespace, and overflow;
  corpora reject invalid tokens and oversized rows. Operation-count arithmetic
  is checked before timing. Five Python tests cover those contracts and reports.
- Coverage forces a fresh instrumented build, and CI treats analyzer findings
  as errors. The metrics header now promises no clock reads instead of making
  an ambiguous overhead claim.
- An available second Clang toolchain enabled a successful local libFuzzer run,
  resolving the limitation reported after the initial implementation.

The benchmark report and testing documentation were refreshed after these
changes. The [testing report](testing.md) records the executed checks.
