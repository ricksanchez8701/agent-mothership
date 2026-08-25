# L5 — Philosophy: what this reveals + the honest verdict

## What it reveals about the language

JavaScript was designed in ten days and frozen by its own success. Its type
system is **pragmatic, accretive, and historical**: the shape of the language is
a sediment of what the first engine did, not of what a language should be.
`typeof null` is the purest example — the operator doesn't describe the value, it
describes the *memory layout of a 1995 engine*. Most languages would be ashamed
to admit that; JS just ships it.

A second, deeper habit: **when JS can't change the past, it adds a parallel
correct thing instead of fixing the wrong one.** `includes` was added alongside
`indexOf`; `Object.is` alongside `===`; `Object.prototype.toString` already had
the correct answer for null. The language prefers *accretion* to *correction*.
Philosophically that is honest (the web is the real spec), but it leaves the
type system permanently lopsided: the most-used type check is the wrong one.

## Is the obvious fix philosophically right?

The obvious fix — `typeof null === "null"` — is **philosophically correct**: a
type operator should report the actual type, and the spec's own Null type proves
the language knows the right answer. It is not "a worse problem" in any semantic
sense. The only reason it doesn't ship is **engineering**, not philosophy:
web-compat breakage. That is a legitimate trade — TC39's cardinal rule is "don't
break the web" — and the rejection is the right call *for the ecosystem that
exists*. But it is a choice to preserve a lie, made out of necessity, not because
the lie is good.

## The honest verdict

**This is a genuinely locked-in bug: it can never be fixed in JavaScript.**
Unlike NaN (entry 001) — where the behavior was principled and the flaw was the
inconsistent API surface — here the wrong answer sits in the core operator
itself, and every conceivable fix breaks the web. The bug is permanent.

## "If we change X, it works anywhere"

1. **Inside JS: nothing works.** Changing the typeof Null row breaks web scripts
   that branch on `typeof x === "object"`. Locked in permanently.
2. **A new language / clean redesign — two changes make it work everywhere:**
   - **Encoding:** represent null separately from objects in the value
     representation (e.g. NaN-boxing reserves a canonical NaN payload for null;
     never store null as a tagged pointer whose tag collides with the object
     tag). Then the accident cannot recur.
   - **One predicate:** give the language a single canonical type operator that
     returns the spec type name (`"null"`), and make `instanceof` and the
     toString-tag agree with it by construction. One partition, one answer.

**One-liner:** *`typeof null` is the 1995 memory layout of the first engine,
frozen into the spec and protected by web compat — the language's type operator
lies about its own type system, and it can never be fixed.*