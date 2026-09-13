# hexgames — project conventions

Hex-and-counter wargame engine (C++20, headless), Qt6 GUI, and four XML languages. Read `PLAN.md`
first: its RESUME HERE block is the state of the work; its decision log is binding.

## The languages (all validated by `tools/validate-xml.py`, ctest label `xsd`)
- `game_rules/xml/hexrules.xsd` — rules: typed tables plus prose `<rule @id>` elements.
- `map_graphics/xml/hexsheet.xsd` — map sheets: `grid` (orientation, offset, id-format), terrain,
  edges, links, regions, panels. Hexes are addressed by PRINTED ID everywhere.
- `unit_graphics/xml/hexcounters.xsd` — counters: styles, two faces, sheets.
- `game_records/xml/hexsave.xsd` — scenarios, saves, scripts, goldens (one schema, `@kind`).
- `packages/xml/hexpackage.xsd` — a game package: paths + id bindings across the languages.
Reference renderers: `map_graphics/xml/hexsheet2svg.py`, `unit_graphics/xml/counters2svg.py`. Their
conventions are the spec the C++ renderers reproduce.

## Build and test
- C++20, CMake ≥ 3.20, presets in `CMakePresets.json`: `win-msvc-debug`, `win-msvc-release`,
  `headless` (no Qt at all), `linux-debug`, `linux-release`. Must build on Windows 11 (Ninja + MSVC) and
  Debian. Qt 6.8.3 (`C:/Qt/6.8.3/msvc2022_64`) for the GUI only.
- `cmake --preset win-msvc-debug && cmake --build --preset win-msvc-debug && ctest --preset win-msvc-debug`
- ctest labels: `xsd hygiene core package records golden long gui trc pgg ds ddat`. `ctest -LE long` is
  the fast set.
- Only `hexqt` and the `*_gui` executables may link Qt. Everything else must build under the `headless`
  preset.
- Google Test via FetchContent; one `<thing>_test.cpp` per suite; `hexgames_add_gtest(...)`; literal
  seeds; `gtest_discover_tests`.

## Hard style rules (from the request; enforced where a script can)
- Copyright banner at the START and END of every `.h/.cpp`:
  `// ----------------------------------------------` / `// Copyright Ben Paul Wise. All Rights Reserved.` /
  `// ----------------------------------------------`. `.md/.txt`: `Copyright Ben Paul Wise. All Rights
  Reserved.` as the first and last line. CMake/Python/PowerShell: `# Copyright Ben Paul Wise. All Rights
  Reserved.` first and last line. `tools/banner-check.py` runs as ctest `hygiene_banner_check`.
- Clean domain types over defensive checks: encode invariants in strong types and concepts; no scattered
  null checks or boundary branching. Validation happens once, at the boundary (XML load, user input),
  and throws `std::invalid_argument` with `file:line` or the offending id.
- No silent default substitution. Surface bad state.
- Concision and clarity over efficiency. Small functions and files.
- `#pragma once`; PascalCase file names; `namespace HexCoord`, `HexModel`, `HexXml`, `HexRules`,
  `HexSearch`, `HexEngine`, `HexRecord`, `HexView`, `HexQt`, `Trc`, `Pgg`, `Ds`, `Ddat`.
- Braces on every `if`/`else`; `throw`, never `assert`; exhaustive `switch` with no `default`; Yoda
  literals; trailing-`P` predicate names; explicit `return;` at the end of void functions.
- `.clang-format` (2-space, column 100) is authoritative.

## Engine rules that are easy to get wrong
- Coordinates are the author's ABC system (`hexcoord`), re-implemented from `panj\hexmap\libsrc\tricoord.*`.
  Never introduce cube/axial/offset "hacks"; offset (col,row) exists only at the sheet-grid boundary.
- Printed hex ids are the identity in every document; `hexcoord::Grid` maps id ↔ ABC.
- PRNG: `std::mt19937_64` per named stream, seeded `mixSeed(seed, tag)`; generators passed by reference;
  every roll/draw is an event. Any container whose iteration reaches a PRNG is insertion-ordered
  (`std::vector`/`std::map`), never `unordered_*`.
- `Board`, `RuleSet`, `Roster`, policies are immutable and shared `const`; only `Position`, streams,
  scratch and log are per session. No globals, no `static` mutable state, no `thread_local`.
- Adjudicators are pure functions; mid-resolution choices become `PendingDecision`, never a blocking call.
- Goldens are never hand-edited: `tools/bless-goldens.ps1`, then review by `git diff`, commit cites
  `refactor` / `bugfix` / `rules <id>`.
- Any change to an `.xsd` is a review gate: propose it in `PLAN.md` ("XSD proposals awaiting review")
  and wait for Ben.

## Workflow (Fable coordinator + ≤ 5 workers)
- Contracts before code: compiling interface headers, `uml/*.puml`, `doc/.../test-lists/*.md`.
- Each worker owns one `tasks/NN-slug.md` (see `tasks/README.md`), updates its `resume:` line after every
  green build, and reports there — not in chat prose.
- The coordinator rewrites `PLAN.md` RESUME HERE before every delegation and after every hand-off, and
  commits at every milestone. After a crash: PLAN.md → in-flight task files → `git status`.
- Open design decisions are `TODO(decide)` in code and listed in `PLAN.md`; never pick silently.
- PlantUML rendering needs Java + `plantuml.jar` (`$env:PLANTUML_JAR`); `tools/render-uml.ps1`.
