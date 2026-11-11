# Change log

## Unreleased

### Engine and API

- Compile validated rule strings into typed steps and immutable ordered groups.
- Find candidate groups through a reversed suffix trie; binary-search sorted
  priorities for duplicate and overlapping endings.
- Add custom rule-set initialization, explicit status codes, caller-owned
  buffers, size bounds, engine inspection, and per-call execution metrics.
- Preserve the original function entry points and forward-pass chain behavior.

### Correctness

- Repair normalization termination, ASCII byte handling, overlong suffix
  matching, and the missing comma between the `nci` and `izer` rules.
- Check all allocations and size arithmetic; release partial initialization.
- Bound growing transformations and make debug logging safe for byte spans.
- Repair header guards and support repeated inclusion across C/C++ consumers.

### Project tooling

- Add strict Make builds, a static library, and an error-checked example.
- Add golden, independent differential, allocation fault, concurrency,
  scalability, C++ linkage, sanitizer, and fuzz tests.
- Add coverage HTML, reproducible benchmarks with historical comparisons and
  raw JSON samples, and Linux/macOS CI configuration.
- Harden benchmark argument/corpus validation and coverage rebuilds; add tooling
  regression tests and make analyzer findings fail CI.
- Verify exact-capacity randomized outputs, long rules, numeric limits, in-place
  growth, and a local coverage-guided fuzz run with a second Clang toolchain.
- Replace unsupported performance/compatibility claims with measured evidence;
  document the API, rules, architecture, testing, migration, and review.

### Compatibility

- `Esa` layout changed and `EsaRuleList` was removed; recompile consumers and
  replace direct internal-field access with the public API.
- Normalization removes every non-ASCII-letter byte; three lower-case corpus
  stems change after repairing the merged default rules.
- Debug traces use stderr and log successful steps. Invalid inputs no longer
  emit automatic diagnostics. See [migration](docs/migration.md).
