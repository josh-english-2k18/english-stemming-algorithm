# Migrating from the original C reference

The four original function names and signatures remain: `esaInit`, `esaFree`,
`esaSetDebugMode`, and `esaStemToken`. Code that uses those functions, keeps an
`Esa` on the stack, and frees returned strings can continue after recompilation.
Use the new status-returning API for reliable error handling and const inputs.

## Source and binary changes

The old public `EsaRuleList` type and mutable rule-list fields are removed.
`Esa` now owns a pointer to a compiled engine. Direct field access and copying
an owning instance are unsupported. Recompile all consumers: the struct size
and layout have changed, so old binaries are incompatible.

Replace edits to `esa.rules` with an explicit array passed to `esaInitRules`.
Replace reads of internal rule counts or allocation estimates with
`esaGetEngineInfo`. Rule strings are copied at initialization, so changing a
source array afterward does not alter the engine. Free and initialize a new
engine to change its rules.

The built-in `ESA_RULE_LIST` is now `static const` data with a working header
guard and an `ESA_DEFAULT_RULE_COUNT` macro. It can safely appear in multiple
translation units, but is not an exported mutable global symbol.

## Corrected outputs

The old normalizer compacted characters without writing a new terminator. It
could leave punctuation, digits, and stale letters in the result. It also had
an impossible byte-range check, treated signed bytes inconsistently, and
handled whitespace unreliably. The new policy is explicit: keep ASCII letters,
lowercase them, and discard everything else throughout the supplied byte span.

| Input | Original output | Corrected output |
| --- | --- | --- |
| `(ESA)` | `esa))` | `esa` |
| `S.` | `s.` | `s` |
| `2003).` | `.003).` | empty string |
| `dogs.` | `dogs.` | `dogs` |

Whitespace inside a span is also removed. Split documents into tokens before
calling ESA. UTF-8 is not decoded; for example, a non-ASCII letter's bytes are
removed rather than transliterated. Embedded NUL no longer truncates a
length-delimited span.

A missing comma between the C literals `"nci,n,3,nce"` and `"izer,n,1,"`
previously merged two intended groups. Splitting them repairs three expected
outputs in the 218-word regression corpus:

| Input | Original output | Corrected output |
| --- | --- | --- |
| `valenci` | `valenceiz` | `val` |
| `hesitanci` | `hesitanceiz` | `hesit` |
| `digitizer` | `digitiz` | `digit` |

Suffix matching now rejects a suffix longer than the token instead of allowing
partial matches. Custom malformed rules are rejected at initialization rather
than partially interpreted. Appends are bounded, allocation failures are
reported, and debug logging does not read beyond non-NUL-terminated spans.

## Preserved behavior

The historical implementation actually skipped rules for words shorter than
five letters, despite its old header describing a three-letter input minimum.
This refactor preserves the implemented five-letter threshold. Removals must
still leave at least three letters before appending. `y` is always a vowel.

Group order, conditional continuations, successful no-op intactness, and the
single forward pass remain unchanged. ESA does not guarantee idempotence for
arbitrary custom rules; applying the engine again is a separate pass.

## Errors and diagnostics

`esaInit` still has no return value, but a failed initialization leaves an
empty, safely destructible object. Prefer `esaInitDefault` to see the failure.
The legacy stemmer still rejects nonpositive lengths and returns NULL on
failure. New APIs accept empty spans. NULL arguments no longer print errors
automatically; report returned statuses in your application.

Debug mode now logs successful transformations to stderr instead of printing
every attempted raw-string match to stdout. Do not depend on debug text as a
stable machine-readable format; use per-call metrics instead.
