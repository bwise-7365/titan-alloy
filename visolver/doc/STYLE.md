<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
# visolver C++ style guide

The personal C++ style that all of `include/`, `lib/`, and `src/` follow (applied in
the 2026-07-03 restyle). It was reverse-engineered from Ben's `abzar` library; this is
the visolver-specific, authoritative version. `CLAUDE.md` carries a condensed summary;
this file is the detailed reference. The deterministic slice (indent, braces, return-
type break, namespace indent, `else`/`catch` on their own line) is captured by the
repo-root `.clang-format` — run CLion's bundled formatter; the rest is applied by hand.

**Correctness overrides surface style in any conflict** — see "Deliberately not
adopted" at the end.

---

## 1. File framing
- `.cpp` / `.hpp` / `.h` open and close with a **ruler-banner copyright block**. The
  top banner frames the copyright, then a one-line file-purpose description, then a
  closing ruler; the file ends with the bare banner (no description):

      // ----------------------------------------------
      // Copyright Ben Paul Wise. All Rights Reserved.
      // ----------------------------------------------
      // <one-line description of what this file is>
      // ----------------------------------------------
      ...
      // ----------------------------------------------
      // Copyright Ben Paul Wise. All Rights Reserved.
      // ----------------------------------------------

  The ruler is dashes. `.md` uses `<!-- Copyright ... -->` as the first and last line;
  `.txt` uses the bare line first and last. `CLAUDE.md` is exempt.

## 2. Header guards & includes
- `#ifndef VIMCP_<FILE>_HPP` / `#define` / `#endif // VIMCP_<FILE>_HPP` — include
  guards, **never `#pragma once`**.
- Modern C++ standard headers (`<cstdio>`, `<cmath>`, …), not the C names.
- Grouped includes: the module's own / project headers first, then `<Eigen/Dense>`,
  then the standard library. Inline "why" comments where a header is non-obvious
  (`#include <memory>  // unique_ptr`).

## 3. `using` and type aliases
- Pull the **common std types** into scope at file top so they read bare —
  `using std::string;`, `using std::vector;`, `using std::function;`,
  `using std::chrono::system_clock;`. Keep `std::` on calls / utilities / exceptions
  (`std::sqrt`, `std::to_string`, `std::invalid_argument`, …).
- **`Eigen::` is named ONLY in `vimcp.hpp`**, which pulls the Eigen types into
  `namespace VIMCP` (so everything else writes `VectorXd`, `MatrixXd`, `Index`).

## 4. Naming
- **Types / classes / enums:** PascalCase (`VIResult`, `DHan06Params`, `InnerMethod`).
- **Functions / methods:** camelCase (`dHan06`, `smoothingContinuationSolve`,
  `makeMixedProjector`).
- **Variables / members:** camelCase, **no `m_` prefix and no trailing underscore**.
  Short locals are idiomatic (`nd`, `bk`, `lam`).
- **Predicate booleans take a trailing `P`** (`doneP`).

## 5. Braces & function-definition layout  *(most distinctive signature)*
- Control flow and `class` / `struct` / `namespace`: **K&R, brace on the same line**
  (`if (...) {`, `for (...) {`, `struct X {`).
- **Out-of-line function DEFINITIONS: the return type on its own line and the opening
  brace on its own line (Allman):**

      VIResult
      dHan06(const VectorXd& x0,
             const MatrixXd& M,
             ...)
      {
        ...
      }

  Continuation arguments align under the first argument. Declarations keep the return
  type inline. Constructors/destructors follow the same Allman brace.
- `else` (and `catch`) on their own line after the closing brace.

## 6. Indentation & spacing
- **2 spaces, no tabs; namespace contents are indented** one level (nested/anonymous
  namespaces indent again). Captured by `.clang-format`.

## 7. Comments
- Frequent, terse comments; trailing comments documenting fields
  (`double currTime = 0.0;  // ...`). `// TODO:` markers kept in place. Sources are
  kept clean — no commented-out/dead code left behind.

## 8. Class layout
- **All three access labels present and in order** — `public:`, `protected:`,
  `private:` — even when a section is empty.
- `explicit` on single-argument constructors.
- Copy operations explicitly deleted, each with a `// no copy` comment.
- Inline one-line getters in the header; members carry inline default initializers.

## 9. Conditionals — Yoda ordering
- When one operand is a **numeric literal (or `nullptr`)**, put it on the LEFT:
  `0.0 < x`, `0 == n % 2`, `nullptr == p`. This catches `=`-for-`==` typos on near-
  obsolete toolchains that do not warn. Two-sided ranges read ascending
  (`0.0 < x && x < 1.0`); variable-vs-variable comparisons stay natural.

## 10. Control flow
- **Explicit `return;` ends every `void` function.**

---

## Deliberately NOT adopted from abzar (correctness over surface style)
The following abzar habits are **not** used in visolver; they conflict with its
correctness invariants or Eigen/value-semantic design, and correctness wins:
- **`throw`, never `assert`** for preconditions/invariants (surfaced even in release;
  tests depend on it).
- **Value semantics + Eigen + RAII** — no raw `new`/`delete`, no owning raw pointers.
- **`static_cast` and modern `<c…>` headers** — never C-style casts (a silent-
  reinterpretation hazard in Eigen code) or `<stdio.h>`-style headers.
- **Clean sources** — no retained dead / commented-out code.
<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
