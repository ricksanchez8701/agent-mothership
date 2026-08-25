# Entry 002 — `typeof null === 'object'`: the locked-in bug

- **Language:** JavaScript (Node)
- **Topic:** `typeof null === 'object'`
- **Status:** complete
- **Fork:** FORK-002

---

## L1 — Surface (the surprising observable)

```js
typeof null;                              // "object"      ← null is not an object
null instanceof Object;                   // false         ← but instanceof says NO
Object.prototype.toString.call(null);     // "[object Null]"  ← toString knows the truth
```

**The naive expectation:** `typeof` is the language's type operator. The spec has
a type called Null, so the type operator should report "null".

**What actually happens:** three of the language's own type-introspection APIs
give three different answers to "what type is null?" — and the most-used one
(`typeof`) is the wrong one. The wrong answer is baked into the operator and
cannot be fixed from user code (no polyfill can reach inside `typeof`).

---

## L2 — Mechanism (the exact reason)

`typeof` is defined by a **fixed table** (ECMA-262 §13.5.3, Table 20). The
algorithm computes the internal type `Type(val)` and maps it to a string — and
one row of that table is **Null → "object"**. The spec's own type system knows
null is Null; the operator reports "object" for it anyway. The operator and the
type system disagree about each other.

The three APIs use three different mechanisms:
- `typeof` — the table above → "object".
- `instanceof` — `HasInstance` against `Object.prototype`; null has no prototype
  chain → `false`.
- `Object.prototype.toString` — a *different* internal built-in-tag table that
  includes a "Null" row → `"[object Null]"`.

**Why the table says "object": the tagged-pointer accident.** Netscape's original
engine stored every value as a machine word whose low bits held a type tag;
objects used tag `0`. Null was stored as the literal machine NULL pointer
`0x00000000` — whose tag bits are also `0`. So `typeof` read null's tag and got
"object". (Confirmed by Brendan Eich, es-discuss "typeof null" thread, 2010.)

**Verdict: artifact.** A faithful encoding of an implementation accident into the
spec. No semantic argument makes a value that throws on property access, has no
prototype, and fails `instanceof Object` an object.

---

## L3 — History (how it got this way)

- **May 1995** — Brendan Eich writes JavaScript at Netscape in ~10 days. 32-bit
  tagged representation: object tag `0`, null stored as the NULL pointer. So
  `typeof null === "object"` from day one.
- **1997** — ECMA-262 codifies it: the typeof table maps Null → "object",
  reproducing what Netscape shipped instead of fixing it.
- **1998–2009** — Every edition keeps the table; the behavior is observable
  across the whole web. Scripts use `typeof x === "object"` as a "might be a
  container" guard, and millions rely on null hitting that branch.
- **2010** — Eich confirms on es-discuss it was a bug from the first edition.
- **~2014–2015** — A proposal to make `typeof null` return `"null"` circulates
  in TC39 and is **rejected**: changing the answer would silently change behavior
  in shipped scripts. Real breakage for a cosmetic cleanup. The bug is kept.

**Pattern:** a 1995 implementation accident → enshrined in the 1997 spec →
protected by web compatibility → formally proposed to fix → rejected. The bug is
now a feature, permanently. Same rejection pattern as `document.all` and the
ES2016 `indexOf`/`includes` split (entry 001).

---

## L4 — Principle (the deeper idea)

`typeof` is the language's **type-discerning operator** — its answer to "which
category is this value in?" The flaw is a failure of that operator to identify
its own category.

**Verdict: arbitrary.** Null-as-object is not forced by logic, math, or type
theory — the opposite is true:
- **Type theory:** null inhabits the singleton type `Null`. It is the bottom —
  absence of a reference. It is precisely *not* an object (a thing with fields
  and methods). Calling it one is a category error.
- **Set theory:** null-as-absence is the empty set; but the JS object type is not
  "the universe of sets", it's a class with a prototype. An empty set is not an
  instance of `Object` — and JS itself agrees: `null instanceof Object` is `false`.

**A sound type operator partitions the value space into disjoint classes.** JS's
`typeof` breaks the partition: it has an "object" class and dumps null — which is
not in that class — into it anyway. And the language is **self-inconsistent**
about it: three introspection APIs partition the same value space three different
ways. This is the same disease as entry 001 (NaN): multiple semantics, one name,
silently disagreeing.

**The one correct concept:** if types are disjoint unions, then `typeof null`
should be the string `"null"`. The bug is that the union is wrong — null is a
member of the wrong set.

---

## L5 — Verdict (what this reveals + the decisive answer)

**What it reveals:** JS was designed in ten days and frozen by its own success.
Its type system is a sediment of what the first engine did, not of what a
language should be — `typeof null` describes the *memory layout of a 1995
engine*, not the value. And when JS can't change the past it **accretes a
parallel correct thing** instead of fixing the wrong one (`Object.is` alongside
`===`; `[object Null]` alongside `typeof`). The most-used type check is the wrong
one, permanently.

**Is the obvious fix philosophically right?** Yes — `typeof null === "null"` is
correct: a type operator should report the actual type, and the spec's own Null
type proves the language knows the answer. The only reason it doesn't ship is
engineering (web-compat), not philosophy. TC39's rejection is the right call for
the ecosystem that exists — but it is a choice to preserve a lie, made out of
necessity, not because the lie is good.

> **If we change X, it works anywhere:**
> 1. **Inside JS: nothing works.** Changing the typeof Null row breaks web
>    scripts that branch on `typeof x === "object"`. Locked in permanently.
> 2. **A new language / clean redesign:** represent null separately from objects
>    in the value encoding (e.g. NaN-boxing reserves a canonical NaN payload for
>    null — never a tagged pointer whose tag collides with the object tag), and
>    give the language one canonical type predicate that returns the spec type
>    name (`"null"`) and is consistent with `instanceof` and the toString-tag by
>    construction. One partition, one answer.

**One-liner for the findings log:** *`typeof null` is the 1995 memory layout of
the first engine, frozen into the spec and protected by web compat — the
language's type operator lies about its own type system, and it can never be
fixed.*