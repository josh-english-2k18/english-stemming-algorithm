# Contributing

Keep changes focused and include the behavior being changed, its motivation,
and relevant verification in your pull request. The library targets C11 with
no runtime dependencies; public headers also support C++11 consumers.

## Local workflow

1. Read [the API](docs/api.md), [rule semantics](docs/rules.md), and
   [architecture](docs/architecture.md).
2. Add a focused regression case that describes the intended behavior.
3. Change the implementation and update related documentation.
4. Run `make check`, `make sanitize`, `make thread-sanitize` where supported,
   and `make coverage`. Run `make tooling-test` for benchmark/tooling changes.
   Run static analysis for engine changes, with `-Xanalyzer -analyzer-werror`
   so findings fail the check.
5. For execution or allocation changes, run the benchmark with the same
   compiler, hardware, corpus, and options before and after the change.

Use four-space C/Python indentation and tabs in Make recipes. Keep warnings
clean. Avoid compiler extensions, locale-dependent character functions,
unchecked size arithmetic, hidden ownership, and mutable global state.
Document public contracts and non-obvious invariants rather than narrating
individual statements. Preserve the MIT license and author attribution.

## Rules and compatibility

Rule order and chain boundaries are behavior. Cover both a direct match and
interactions with nearby groups. Keep the golden corpus stable unless an
intentional correction is justified; document exact before/after outputs in
the migration guide. Do not describe output as Porter-equivalent without a
separate, reproducible compatibility evaluation.

Initialize fresh `Esa` storage, release every owned result, and free the engine.
Do not add private testing controls to the public API. Allocation-failure tests
use compile-time overrides in their own translation unit.

## Benchmark evidence

Do not run instrumented builds or other heavy jobs during timing. Report raw
samples, sample count, compiler/options, workload, variability, and
initialization costs alongside throughput. The benchmark's p95 is a percentile
of whole-sample mean costs, not single-token tail latency. Note output changes
when comparing different algorithm versions.

See [testing](docs/testing.md) for tool requirements and the deterministic
fallback when libFuzzer is unavailable. Build outputs belong in `build/`;
`make clean` removes that directory.
