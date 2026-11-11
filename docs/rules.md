# Rule sets

A rule set is an ordered array of rule-group strings. Compile it once with
`esaInitRules`; keep the engine for repeated stemming. Built-in rules live in
`esa_rules.h` and include a NULL sentinel for historical convenience.

## Grammar

```text
group  = step ("," step)*
step   = suffix "," intact "," remove "," append
suffix = one or more lowercase ASCII letters
intact = "y" | "Y" | "n" | "N"
remove = one or more decimal digits
append = zero or more lowercase ASCII letters
```

There is no escaping or whitespace syntax. Empty suffixes, signed numbers,
uppercase suffixes/appends, partial steps, and trailing extra delimiters are
invalid. Leading zeroes in the removal count are accepted. A numeric value
larger than `SIZE_MAX` produces `ESA_OVERFLOW`. A representable removal count
larger than the token is valid syntax; that step simply cannot apply.

Examples:

| Rule group | Operation |
| --- | --- |
| `s,n,1,` | Match `s`, remove one character, append nothing |
| `ies,n,2,` | Match `ies`, remove two characters |
| `y,n,1,i` | Replace final `y` with `i` |
| `at,n,2,ate` | Replace final `at` with `ate` |
| `est,y,2,` | Remove two characters only while the token is intact |
| `s,n,1,,e,n,1,` | Remove `s`, then remove `e` if the new ending matches |

Empty append fields explain the double comma between some chained steps. A
final comma can be the delimiter before an empty append field: `s,n,1,` is
valid, whereas `s,n,1,,` starts an incomplete additional step and is invalid.

## Execution contract

1. Normalize the byte span to lowercase ASCII letters. The token must initially
   have at least five letters, at least one vowel, and at least one consonant.
   Vowels are `a e i o u y`. Otherwise return the normalized token immediately.
2. Find the earliest remaining group whose first suffix matches the current
   token. Declaration order takes priority over suffix length.
3. Evaluate its first step. The intact flag must permit it, the removal must
   fit, and removal must leave at least three letters before appending.
4. On success, remove, append, mark the token changed, and attempt the next step
   in the same group against the new token. A successful no-op marks it changed.
5. On failure, stop that chain. Keep any earlier successful changes in it.
6. Continue with later groups, using the updated suffix. Never revisit an
   earlier group, retry a step, or execute a group until a fixed point.

The three-letter intermediate constraint is intentional: an append cannot
rescue a removal that would leave only two letters. Initial vowel/consonant
eligibility is checked once; it is not reconsidered after each transformation.
An intact step requires that **no** earlier step in this call has succeeded,
including earlier steps of the same chain.

## A custom chain

```c
const char *const rules[] = {
    "s,n,1,,e,n,1,", /* abcdes -> abcde -> abcd */
    "d,y,1,x",      /* blocked, because the token has changed */
    "d,n,1,z"       /* abcd -> abcz */
};
Esa esa;
EsaStatus status = esaInitRules(&esa, rules, sizeof(rules) / sizeof(rules[0]));
/* Check status, stem tokens, then call esaFree(&esa). */
```

Use small, focused examples to test a new rule. Check earlier and later
interactions, suffix collisions, intactness, minimum lengths, and intended
conflation pairs. The engine validates syntax and memory bounds; it cannot
judge whether a rule improves linguistic quality. The default rules preserve
ESA's historical behavior with the documented bug fixes, rather than claiming
linguistic equivalence with other stemming algorithms.
