# L4 — Physics: the deeper principle

## What it touches

`typeof` is the language's **type-discerning operator** — its answer to "which
category is this value in?" The flaw is a failure of that operator to identify
its own category: the spec has a Null type, but `typeof` reports null as Object.

## Principled or arbitrary?

**Arbitrary.** Null-as-object is not forced by logic, math, or type theory.
The opposite is true:

- **Type theory.** The value `null` inhabits the singleton type `Null`. It is
  the bottom — absence of a reference. It is precisely *not* an object (a thing
  with fields and methods). Calling it one is a category error.
- **Set theory.** null-as-absence is the empty set, and the empty set is
  technically a set — but the JS object type is not "the universe of sets"; it
  is a class with a prototype. An empty *set* is not an *instance* of `Object`,
  and JS itself agrees: `null instanceof Object` is `false`.
- There is no reading under which "absence of reference" is a member of
  "references with a prototype". The "object" label is a lie told by the
  representation, not by the semantics.

## The real physics: a type system is a partition

A sound type operator partitions the value space into disjoint classes, and
every value lands in exactly one. JS's `typeof` breaks the partition: it has an
"object" class and dumps null — which is not in that class — into it anyway.

The interesting part is that the language is **self-inconsistent** about it:
three introspection APIs partition the same value space into **three different
partitions**:

- `typeof null` → "object" (null in the object class)
- `null instanceof Object` → false (null in the non-object class)
- `Object.prototype.toString.call(null)` → "[object Null]" (null in its own class)

This is the same disease as entry 001 (NaN): multiple semantics, one name,
silently disagreeing.

## The one correct concept

If types are disjoint unions, then `typeof null` should be the string `"null"`,
full stop. The bug is that the union is wrong: null is a member of the wrong set.