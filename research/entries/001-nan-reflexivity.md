# Entry 001 — NaN: the value that is not equal to itself, and the `indexOf`/`includes` split

- **Language:** JavaScript (Node)
- **Topic:** NaN reflexivity and the indexOf/includes split
- **Status:** complete
- **Fork:** FORK-001

---

## L1 — Surface (the surprising observable)

Run this:

```js
console.log(NaN === NaN);          // false   ← a thing that is not equal to itself
console.log(NaN !== NaN);          // true

const xs = [NaN];
console.log(xs.indexOf(NaN));      // -1      ← NaN is IN the array, but indexOf can't find it
console.log(xs.includes(NaN));     // true    ← ...yet includes can

console.log(Object.is(NaN, NaN));  // true    ← a third answer, from Object.is
console.log(new Set([NaN, NaN]).size); // 1   ← Set dedupes NaN (treats it as equal)
```

**The naive expectation:** equality is reflexive — `x === x` is always true, and
two array-search methods should agree about whether an element is "in" the array.

**What actually happens:** the language gives three different answers to
"is NaN equal to NaN?" depending on which API you ask. That is the flaw — not
NaN itself.

---

## L2 — Mechanism (the exact reason)

JavaScript has **three distinct equality semantics**, and different standard
library methods silently pick different ones:

| Semantics      | Used by                          | NaN == NaN? | +0 == -0? |
|----------------|----------------------------------|-------------|-----------|
| Strict (`===`) | `===`, `==`, `indexOf`, `lastIndexOf` | **false** | true      |
| SameValue      | `Object.is`                      | **true**    | **false** |
| SameValueZero  | `includes`, `Set`, `Map` keys    | **true**    | true      |

- `===` implements the ECMAScript **Strict Equality Comparison**, which for
  Number values says: *"if x is NaN, return false."* So NaN never equals
  anything, including itself. (ES2024, 7.2.15 / IsStrictlyEqual.)
- `Object.is` implements **SameValue** (ES2015), which makes NaN equal to NaN
  and distinguishes `+0` from `-0`.
- `Array.prototype.includes` (ES2016) implements **SameValueZero** — like
  SameValue but `+0` and `-0` are equal.

Why `indexOf` and `includes` disagree: `indexOf` was specified in ES5 to use
`===`. When `includes` came later, TC39 chose SameValueZero specifically so that
`[NaN].includes(NaN)` would return `true`. They fixed the surprise in the *new*
method but did not (and cannot) change the old one.

**Verdict:** the underlying NaN behavior is principled (it's IEEE 754 — see L4).
The inconsistency *between methods* is the artifact.

---

## L3 — History (how it got this way)

- **1985 — IEEE 754** standardized floating point. It defined NaN as *unordered*:
  every comparison against NaN is false, including `NaN == NaN`. This is
  inherited by almost every language with floats.
- **1995 — JavaScript's first draft** copied IEEE 754 for its numeric equality.
- **ES5 (2009)** specified `indexOf` using `===`, faithfully inheriting the
  IEEE "NaN != NaN" result.
- **ES2015** added `Object.is` (SameValue) because `===`'s treatment of NaN and
  `-0` was considered wrong for specific use cases (e.g. comparing state snapshots).
- **ES2016** added `includes` with SameValueZero, explicitly to avoid the
  `indexOf` NaN footgun. TC39 discussion at the time treats the `indexOf`
  behavior as an accepted historical mistake that cannot be retroactively changed
  because the web depends on it.

**Pattern:** the language accretes a *new* equality semantics every few years,
each chosen for its moment, and the older methods keep the older semantics
forever. Nobody can unify them because changing `indexOf` or `===` breaks the web.

---

## L4 — Principle (the deeper idea)

NaN is floating point's encoding of **"there is no answer."** It is the result
of `0/0`, `Infinity - Infinity`, `Math.sqrt(-1)` — computations that do not
have a value.

The law of identity says a thing equals itself. Does "no answer" equal
"no answer"? IEEE 754 says **no** — and that is the correct call. Two absences
of value are not the *same* absence. `0/0` and `Infinity - Infinity` both produce
NaN, but the "nothing" behind each is a different nothing. Collapsing them to
"equal" would be a false statement, not a true one.

So `NaN !== NaN` is **not** the flaw. It is a principled consequence of treating
"undefined" as a first-class value that refuses to be identified with any other
undefined value. Set theory does the same: two empty sets are equal, but two
"undefined" computations are not sets at all — they're holes in the fabric of
the function.

The genuine flaw is that JavaScript hides this subtlety behind **one symbol,
`===`, that means three different things depending on which method you call**,
and names none of them.

---

## The decisive answer

> **If we change it, does it work anywhere?**

- **Can't change `===` or `indexOf`** — locked in by back-compat. The web runs on
  them. This is the deep lesson: once millions of pages depend on a behavior,
  even a *wrong* one becomes unfixable, and it accretes a parallel "correct"
  version beside it (`includes`, `Object.is`) that lives alongside the wrong one
  forever.
- **What *would* work anywhere (a new language / a clean redesign):** keep
  IEEE-754's `NaN != NaN` as the one true semantics, and expose it consistently —
  one equality operator, plus named predicates (`sameNumber`, `sameValue`) instead
  of three unnamed behaviors. Then `indexOf` and `includes` can never disagree,
  because there is only one semantics for them to share.

**One-liner for the findings log:** *The language doesn't have an equality bug;
it has three equalities and one name for all of them — and the split is permanent
because the web froze the first one in place.*
