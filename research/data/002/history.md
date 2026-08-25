# L3 — History: why it got this way

- **May 1995** — Brendan Eich writes Mocha/LiveScript (later JavaScript) for
  Netscape in about 10 days. The engine uses a 32-bit tagged representation: the
  object tag is `0`, and null is stored as the NULL pointer `0x00000000`. The tag
  bits read as the object tag, so `typeof null === "object"` from day one.
- **1997** — ECMA-262, the first ECMAScript standard, codifies the behavior
  instead of fixing it: the typeof table maps the internal Null type to the
  string "object", reproducing what Netscape shipped.
- **1998–2009** — Every edition keeps the table. The behavior is now observable
  across the entire web; scripts use `typeof x === "object"` as a "might be a
  container" guard, and millions of them rely on null hitting that branch.
- **2010** — Brendan Eich, on the es-discuss mailing list ("typeof null"
  thread), confirms it was a bug from the first edition. A fix is floated for
  the next spec.
- **~2014–2015** — A concrete proposal to make `typeof null` return `"null"`
  circulates in TC39. It is **rejected**: web-compat analysis shows that
  changing the answer would silently change behavior in shipped scripts that
  branch on `typeof x === "object"` to decide "has properties / is a container".
  The cost is real breakage for a cosmetic cleanup. TC39 keeps the bug.
- **Same rejection pattern** as `document.all` (falsy-but-typed-object) and the
  ES2016 `includes` vs `indexOf` split (entry 001): the web freezes the first
  behavior in place, forever.

**Timeline:** 1995 implementation accident → enshrined in the 1997 spec →
protected by web compatibility → formally proposed to fix → rejected. The bug is
now a feature, permanently.