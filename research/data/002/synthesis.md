# Synthesis — 002 typeof null === 'object' — the locked-in bug

## Contradiction scan (swarm.md rule 2)

- **L2 (spec)** says the typeof table maps Null → "object": **artifact**, not principled.
- **L3 (history)** confirms: 1995 implementation accident (null = NULL pointer,
  object tag = 0), codified in ECMA-262 (1997), fix proposed ~2015 and rejected
  for web compat.
- **L4 (physics)** agrees: no type-theoretic reading makes absence-of-reference an
  object. `instanceof Object` → false; `Object.prototype.toString` → "[object Null]".
- **No contradiction between layers** — every layer independently concludes:
  arbitrary, historical, locked-in.

The genuine finding is the **three-way inconsistency inside the language itself**:
`typeof` / `instanceof` / `Object.prototype.toString` partition the same value
space three different ways, and the most-used operator is the wrong one.

## Decisive statement

> **"If we change `typeof`'s handling of null, it breaks the web — locked in by
> back-compat. Nothing works inside JS. In a new language it works anywhere:
> store null separately from objects (NaN-boxing reserves a payload, not a
> colliding tag), and give the language one canonical type predicate that agrees
> with the spec's own type system."**

## Cross-topic pattern (→ findings/)

Both 001 (NaN equality) and 002 (null typing) share the same disease:
**JS accretes parallel "correct" semantics instead of fixing the frozen wrong
one** (`Object.is`/`includes` vs `===`/`indexOf`; `[object Null]` vs `typeof`).
The web freezes the first behavior in place, forever.

## Fork

Logged **FORK-002** in forks.md: is the flaw the `typeof` output itself (fix the
operator) or the whole three-way type-inconsistency (fix the architecture)?
Chosen side A: the operator's output is the lock-in; the three-way split is the
same disease, but only `typeof` is unfixable.