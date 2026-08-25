# L2 — Spec: the exact mechanism

## The typeof table (ECMAScript §13.5.3, "typeof" Evaluation)

`typeof` is defined by a **fixed table** (Table 20 in ECMA-262). The algorithm
computes the internal type `Type(val)`, then maps it to a string:

| Type(val)                       | Result of typeof |
|---------------------------------|------------------|
| Undefined                       | "undefined"      |
| **Null**                        | **"object"**     |
| Boolean                         | "boolean"        |
| Number                          | "number"         |
| String                          | "string"         |
| Symbol                          | "symbol"         |
| BigInt                          | "bigint"         |
| Object (ordinary and exotic)    | "object"         |
| Object (callable — a function)  | "function"       |

So the spec **deliberately and explicitly** maps the internal type Null to the
string "object". The spec's own type system knows null is a Null — the sentence
`Type(null) is Null` holds in the spec — but `typeof` reports "object" for it.
The operator and the type system disagree about each other.

## The three-way split, mechanically

- **`typeof`** uses the table above → "object".
- **`instanceof`** (§13.10) runs `HasInstance` against `Object.prototype`. null
  has no prototype chain, so it is not an instance → `false`.
- **`Object.prototype.toString`** (§20.1.3.6) uses a *different* internal table
  of built-in tags, which includes a "Null" row → `"[object Null]"`.

Three different mechanisms, three different partitions of the same value space.

## Why the table says "object": the tagged-pointer accident

The table is not a design choice — it is a fossil of the first implementation.
Netscape's original engine represented every JS value as a machine word whose
**low bits held a type tag**. Object values used tag `0`. The null value was
stored as the literal machine NULL pointer, `0x00000000` — whose tag bits are
also `0`. So reading null's tag produced "object". `typeof` simply read the tag.
(Confirmed by Brendan Eich on the es-discuss list, "typeof null" thread, 2010:
"it was a bug from the first edition.")

## Principled or artifact?

**Artifact.** It is a faithful encoding of an implementation accident into the
language spec. No semantic argument makes a value that throws on property
access, has no prototype, and fails `instanceof Object` an object. The spec's
own Null type proves the language knows better than its own operator.