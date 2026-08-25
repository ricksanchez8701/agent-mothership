# L1 — Recon: `typeof null === "object"` — the locked-in bug

## Smallest runnable repro

```js
typeof null;                              // "object"      ← null is not an object
null instanceof Object;                   // false         ← but instanceof says NO
Object.prototype.toString.call(null);     // "[object Null]"  ← toString knows the truth
```

## Naive expectation

`typeof` is the language's own type operator — its answer to "what kind of thing
is this?" If the language has a type named Null (ECMAScript's internal `Type(val)`
does — see L2), the type operator should report "null". It reports "object".

## What exactly surprised me

1. `typeof null` and `null instanceof Object` **disagree** about whether null is
   an object.
2. `Object.prototype.toString.call(null)` returns `"[object Null]"` — the
   language already HAS the correct answer, in a different API.
3. So three of the language's own type-introspection APIs give **three different
   answers** to "what type is null?" — and the most-used one (`typeof`) is the
   wrong one.
4. The flaw is **unpatchable from user code.** No override, no polyfill, no
   monkeypatch reaches inside `typeof`. The wrong answer is baked into the
   operator itself.
5. Bonus footgun: code that does `if (x && typeof x === "object")` works by
   accident, but code that does `if (typeof x === "object")` as a "maybe has
   properties" guard silently includes null — then crashes on the first
   property access.