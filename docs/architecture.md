# Engine architecture

The former engine stored rule strings, scanned all of them for each token, and
reparsed every candidate and continuation. The replacement compiles a rule set
into owned strings, typed steps, ordered groups, a reversed suffix trie, and
sorted terminal priority slices.

## Compilation

Initialization has two parsing passes. The first validates every rule and
computes step count, string bytes, a trie capacity bound, and maximum possible
growth. Checked arithmetic prevents wraparound before any allocation. The
second copies strings and emits steps that point into those immutable copies.
No pointer into the caller's original rule storage survives initialization.

Only each group's first suffix enters the trie. Traversal starts at a token's
last character, so groups sharing an ending share a trie path. A direct
26-entry root table handles the busiest level; sparse child and sibling indices
avoid that table size at every deeper node. Node storage reserves at most
one root plus the sum of all first-suffix lengths; the engine reports both
actual nodes and total allocated bytes.

Each terminal owns a contiguous, sorted slice of group priorities. Building
these slices takes a counting pass and a fill pass over groups in their
original order. A binary search finds the first priority at or after the
current cursor. This handles duplicate suffixes without repeatedly walking all
already-consumed groups. A 10,000-group repeated-suffix regression checks that
priority probes stay below 15 per group.

A nonempty compiled engine owns six allocations: the engine itself, nodes,
steps, groups, priority entries, and string storage. An empty engine needs two.
Failure releases every successful allocation and publishes no partial engine.

## Applying rules

Each call normalizes into its result buffer, checks initial eligibility, then
walks suffix paths to choose the smallest matching group priority at or after
the cursor. This is ordered selection: a longer matching suffix has no special
priority. Groups whose suffixes cannot match are skipped entirely.

The selected group's typed steps run in order. Each checks the suffix,
intactness, and removal bounds before writing. Failed continuations retain
previous changes but stop the local chain. The cursor advances past the
selected group regardless of success; the trie is queried against the new
ending for the next selection. Successful no-ops still advance the cursor.
These properties make execution finite, even with expansion and cyclic-looking
custom transformations.

Let `G` be groups, `S` steps, and `B` rule-string bytes. Compilation uses
`O(B + S + G)` work up to the fixed 26-letter child lookup factor and reserves
`O(B + S + G)` space. Normalization is linear in input length. A candidate
selection traverses no farther than the current token length or longest
indexed suffix, performs at most 26 edge comparisons per level, and binary
searches priority slices on that path. At most `G` selections and `S` step
evaluations occur. Matching and copying a step also cost time proportional to
its suffix and append lengths. Rules that grow tokens therefore affect work
as well as capacity; there is no claim of constant-time stemming.

## Capacity and concurrency

Every step runs at most once. Summing the positive value of
`append_length - remove` over all steps bounds total expansion. Adding this to
the input byte length and one NUL bounds every intermediate buffer size,
because normalization never expands the input. All three sums are checked for
overflow. The runtime removal guard prevents underflow independently of the
capacity proof.

The engine is read-only during stemming. Cursor, changed flag, buffer, and
metrics belong to the call. Reader threads can share an engine without locks;
ownership changes and debug-mode writes require external synchronization.
Library code allocates nothing on the buffer path. The allocating path adds a
single output allocation, and the legacy function delegates to it.

## Boundaries

This refactor changes rule execution and safety. It retains the ordered ESA
model and its short-token/vowel rules. It does not add a general pattern
language, Unicode normalization, recursive rewriting, or a linguistic model.
Binary compatibility with the old public struct layout is not retained; see
the [migration guide](migration.md).
