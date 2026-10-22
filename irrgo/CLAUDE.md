# Project conventions

This project implements an RYB-color-wheel palette selector for board-style games:
a reusable library that, given partial input, completes a three-color palette
(one board background + two opposing-player piece colors). Full algorithm in
`DESIGN.md` — read it before writing code.

## Build & platform
- C++20, CMake. Must build and run on **both Windows and Debian Linux**.
  Use nothing platform-specific.
- Qt 6 (`Qt6::Widgets`) for the UI only.
- Two CMake targets:
  - `palette_core` — pure algorithms, **no Qt dependency**, unit-tested.
  - `palette_widgets` — thin Qt layer, depends on `palette_core`.
- Build `palette_core` with tests first; verify it; only then build the widget.

## Code style (hard requirements)
- Braces around **every** `if` / `else` body, even single statements.
- **No silent default substitution.** Do not paper over bad input/state with a
  fallback value that hides the bug. Surface it: explicit error/diagnostic or
  throw. (The "least-bad accommodation" results are NOT errors — they are valid
  outputs carrying `Warning` diagnostics; see DESIGN.md.)
- No huge functions or files. Split anything past a few hundred lines.
- Prefer referential transparency / pure functions. Confine side effects to the
  obvious I/O boundary (Qt painting, signals/slots). No programming-by-side-effect
  elsewhere.
- Prefer small immutable value types for colors.

## Workflow
- Generate `palette_core` + tests, run them, then the widget.
- Do not invent the RYB↔sRGB cube corner constants from memory — take them from
  the Gossett–Chen reference / ArtColors (see DESIGN.md "References").
- Open design decisions are marked `TODO(decide)` in DESIGN.md. Do not silently
  pick one; ask if unresolved.
