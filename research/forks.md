# Forks — decision log

A "fork" is a point where the path genuinely splits and we must choose a branch.
We log it here BEFORE choosing.

## Log

<!-- format:
### FORK-001 (date)
- Topic: ...
- Branches:
  - A: ...
  - B: ...
- Chosen: ...
- Why: ...
-->

### FORK-001 (2026-08-20)
- Topic: NaN equality — is the "fix" (make NaN === NaN) actually correct?
- Branches:
  - A: NaN !== NaN is the principled IEEE-754 behavior; the real bug is that JS
        ships three different equality semantics with no names. Fix = one
        consistent default across the standard library.
  - B: NaN !== NaN itself is the flaw; every equality should treat NaN as equal
        to itself (reflexivity as a law of logic).
- Chosen: A (IEEE-754 is principled; JS's inconsistency is the real flaw).
- Why: Two unknown "no answer" values genuinely aren't the same unknown. IEEE-754
  is right. See entries/001-nan-reflexivity.md L4.

### FORK-002 (2026-08-20)
- Topic: typeof null === 'object' — is the flaw the operator output, or the
        whole three-way type-introspection inconsistency?
- Branches:
  - A: The flaw is `typeof`'s output itself. Fix = make typeof return the spec
        type name ("null"); the three-way split (typeof/instanceof/toString) is
        the same disease, but only typeof is the permanent lock-in.
  - B: The flaw is the architecture — three disagreeing type APIs. Fix = one
        canonical type predicate; typeof's wrong answer is just one symptom and
        could be tolerated if the canonical API existed.
- Chosen: A (typeof's Null→"object" row is the unfixable one; the proposal to
  change it was rejected by TC39 for web compat).
- Why: B is the correct *long-term* design (see philosophy L5), but it doesn't
  exist in JS and never will. The realistic, decisive statement is A: the wrong
  answer sits in the core operator and can't be changed. See entries/002-null-typing.md.

### FORK-003 (2026-08-20)
- Topic: Linux select() reports a closed fd as READY — kernel flaw or user error?
- Branches:
  - A: Kernel flaw. The kernel actively *chooses* "ready" for closed-mid-wait
        fds (documented in select(2) BUGS), cannot represent "invalid" in its
        result bitmap, and even mutates its input timeout. Multi-threaded apps
        cannot always prevent cross-thread fd close. The API's expressive power
        is the cause.
  - B: User error. POSIX declares closed-during-select "undefined"; a correct
        program must never close an fd while select() waits, so the behavior is
        the caller's fault, not the kernel's.
- Chosen: A (kernel flaw).
- Why: "Undefined" is a footnote, not a contract — the kernel's *documented,
  deterministic* choice of "ready" is an active lie, and the DoS shape
  (busy-loop) follows from it. An API that cannot express "this fd is invalid"
  will always ship a lie; B only excuses it. Empirically reproduced on 6.8.
  See entries/101-select-lie.md.
