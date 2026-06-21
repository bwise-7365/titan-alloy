# Palette redesign — restart handoff (for Claude)

Resume point for the IrrGo palette selector after a multi-day gap. Read this
before touching `palette_core` / `palette_widgets`. Paired with the verbose human
notes: `doc/2026-06-20-palette-redesign-notes.md`.

## TL;DR / decision state
- **DECIDED (direction):** abandon the single unified algorithm. Build a small set
  of **named schemes** the user picks from ("mix-and-match"); each scheme is a
  tiny one-directional completion function composed from orthogonal **components**.
- **NOT DECIDED yet:** the exact scheme set, the per-scheme anchor, and whether to
  switch the harmony reasoning space from RYB to OKLCh. The user rejected an
  AskUserQuestion that tried to lock these — they want to reflect first. **Do not
  re-ask immediately; wait for direction, or propose a concrete DESIGN v2 draft.**
- **DO NOT** try to fix the old unified model further (round-trip, brightness).
  Those are dead ends — see "settled facts" below.

## Settled facts (do NOT re-derive or re-litigate)
1. **Distinction axis is the organizing insight.** Games separate the two players
   by **value** (Go/Reversi/chess: black+white pieces) or by **hue** (maps/Risk:
   red+blue). The old core hard-coded *hue* (DESIGN.md §2 assumes piece↔piece
   luminance gap is not a constraint) — backwards for the value games, which are
   the most common. Piece↔piece contrast must become a first-class constraint.
2. **Brightness round-trip is geometrically impossible for extreme boards.**
   In ℓ = L+0.05: forward straddle ℓ(P1)=C·ℓB, ℓ(P2)=ℓB/C; reverse ℓB=√(ℓP1·ℓP2);
   commute exactly but only for ℓB∈[0.05C, 1.05/C] → L_B∈[0.10,0.30] at C=3.
   Near-white/near-black boards have no room on one side. Round-trip is therefore
   NOT a product contract; at most a banded sanity check.
3. **RYB cube**: forward = ArtColors "artist wheel" corners (ported, in
   `conversion.cpp`); inverse was replaced by a true LM numerical inverse of the
   forward so hue round-trips. RYB's only payoff is painter's complements
   (red↔green); it's asymmetric and odd near neutrals. OKLCh is the cleaner
   alternative for hue/harmonic schemes (exact inverse, perceptual, no cube
   pathologies). Value schemes barely use hue, so they don't care.

## Component model (the thing to design next)
A scheme = one choice per component:
1. distinction axis: value | hue | chroma
2. anchor: board | one piece | two pieces | seed
3. board role: neutral field (loose FG) | FG ground (floor binds) | derived
4. piece lightness: value extremes | symmetric straddle | both-one-side | equal mid
5. chroma: achromatic pieces | vivid pieces | pale board | match input
6. contrast model: which of {piece↔piece, piece↔board} bind vs report; floor value
7. harmony relation: none | complement | split | triad | analogous | tetrad (+space)
8. reasoning space: sRGB/HSL | OKLCh | RYB
9. gamut handling: chroma reduction on output
10. diagnostics/warnings: which conditions to flag

### Candidate schemes (instantiations)
- **A. Value-contrast** (Go/Reversi/chess): axis=value; anchor=board(field);
  pieces=value extremes (black/white) or chosen dark/light; piece↔piece binds,
  FG reported. Reuses `minimax` only when deriving a board for a value pair.
- **B. Hue-contrast** (maps/Risk): axis=hue; anchor=board(pale) or one piece;
  vivid pieces at similar mid lightness; FG floor binds. ≈ today's Mode 1/2 minus
  minimax-centering. Reuses `harmony` + realize-at-lightness.
- **C. Harmonic** (optional): explicit harmony in OKLCh.
- **D. Manual + diagnostics** (optional, cheap): user sets all three; only compute
  contrasts + warnings.
- More to consider: team/brand-anchored (pieces anchor), accessibility-max (the
  minimax foregrounded), monochrome value-ramp, tinted-neutral/wood.

## Current repo state
- `palette_core/` builds + tests pass (MSVC). Implemented: `legibility`,
  `minimax`, `conversion` (RYB + true inverse), `harmony`, `modes` (the OLD
  unified Mode 1/2/3 — now considered legacy / to be reorganized), tests incl.
  `test_roundtrip` (hue ≤10°, passes; brightness intentionally not asserted).
- `palette_widgets/` + `palette_gui/` build + run (Qt 6.8.3 MSVC). UI: explicit
  3-mode toggle, native QColorDialog input, harmony combo (**Triad default**),
  spread checkbox, board preview (3:2 + two circles), RYB wheel (display-only),
  diagnostics, warnings. Default board beige RGB(255,255,221).
- Qt platform plugin: `palette_gui.exe` needs `platforms/qwindows.dll` beside it
  (different build subdir than irrgo_gui); user handles the copy.
- `main_gui.cpp`: window 500x400.

## What to keep vs replace when rebuilding on schemes
- KEEP as-is: `legibility.*`, `minimax.*`, `harmony.*`, the Qt shell.
- KEEP or swap-to-OKLCh: `conversion.*` (keep true-inverse if staying RYB; else
  replace with sRGB↔OKLab).
- REORGANIZE: `modes_*` → per-scheme completion functions; the UI "mode toggle"
  → "scheme selector"; add piece↔piece contrast as a first-class constraint.

## Conventions (unchanged)
- Copyright first AND last line: `// Copyright Ben Paul Wise. All Rights Reserved.`
  (CMake uses `#`). `#pragma once` in widgets / include guards in core, members
  trailing `_`, braces on every if/else, surface bad input (no silent fallback).
- C++20, cross-platform (Win + Debian); `palette_core` no Qt, generator-agnostic.
  User builds/runs himself — do NOT run builds to "verify."

## Concrete next steps (in order)
1. Confirm scheme set + per-scheme anchor + space (RYB vs OKLCh) with the user.
2. Draft DESIGN v2 around the component model (§"Component model") replacing the
   single-algorithm framing of the current DESIGN.md.
3. Add piece↔piece contrast to the legibility layer as a first-class constraint.
4. Implement schemes A and B first (cover Go/Reversi/chess + maps/Risk); wire the
   UI mode toggle into a scheme selector.
5. Demote round-trip to a banded sanity check (assert only L_B∈[0.10,0.30]).
