# Palette Selector — Rethinking the Foundations

*Design notes, 2026-06-20. A verbose record of the discussion that led from
"the unified algorithm is wrong" to "enumerate composable schemes." Nothing here
is decided yet — it's material to think about.*

---

## 1. The pivot

The conclusion reached: **the single-algorithm approach is wrong.** One algorithm
that tries to complete any partial palette keeps colliding with cases that are
extremely common in real games:

- **Go** — pale beige board, **black + white** stones.
- **Reversi** — dark green board, **white + black** pieces.
- **Chess / checkers** — light board, light + dark pieces.
- **Maps / Risk** — pale ground (beige, pale green, pale blue) with **vivid red
  and blue**.

The decision: **enumerate two or three (probably more) schemes for designing
palettes, and let the user mix-and-match**, instead of forcing one scheme to
cover every case.

## 2. Why one algorithm cannot cover these — the distinction axis

The deep reason the cases conflict: **they don't separate the two players the
same way.**

| Game | Board | Pieces | Players told apart by |
|---|---|---|---|
| Go | pale beige | black + white | **value** (lightness) |
| Reversi | dark green | black + white | **value** |
| Chess / checkers | light | light + dark | **value** |
| Maps / Risk | pale beige/green/blue | vivid red + blue | **hue** |

Two different *distinction axes*:

- **Value-distinguished** players (Go, Reversi, chess): the two pieces differ in
  **lightness**. The piece↔piece value gap *is the whole point*; figure-ground
  (piece vs board) is deliberately loose — white stones on a pale beige board are
  genuinely low-contrast, and that's accepted (stones read by their black/white
  difference, helped by outline/shadow).
- **Hue-distinguished** players (maps, Risk): the two pieces differ in **hue**,
  at similar lightness, on a pale ground. Here figure-ground *is* the binding
  constraint and piece↔piece luminance doesn't matter.

The current `palette_core` hard-coded the **hue** axis. DESIGN.md §2 literally
states: *"hue difference alone distinguishes P1 from P2 … the piece↔piece
luminance gap is not a constraint."* That is exactly **backwards** for the
value-distinguished games, which are the most common ones. No single model can
sit on both sides of that split — which is the friction we kept hitting.

## 3. How we got here (the findings that forced the pivot)

Two earlier investigations set this up; they're worth remembering because they
rule out "just patch the current model":

1. **Brightness round-trip is geometrically impossible for extreme boards.**
   In WCAG shifted-luminance ℓ = L + 0.05, the matched forward/reverse pair is
   - forward (Mode 1, straddle): ℓ(P1) = C·ℓ(B), ℓ(P2) = ℓ(B)/C  (C = contrast floor)
   - reverse: ℓ(B) = √(ℓ(P1)·ℓ(P2))  (geometric mean)
   - these commute exactly: √(C·ℓB · ℓB/C) = ℓB.
   But only inside the band ℓ(B) ∈ [0.05·C, 1.05/C] → for C = 3, **L_B ∈ [0.10,
   0.30]**. A near-white board (beige, L ≈ 0.98) has **no room for a brighter
   opponent above it**, so the forward can't straddle and nothing recovers it.
   This is geometry, not a bug. → *Round-trip should never have been a contract.*

2. **The RYB harmony space is awkward.** It gives artist's complements (red↔green)
   but is asymmetric and behaves oddly near neutrals (the pale-yellow triad
   surprise). We replaced ArtColors' fitted inverse with a true numerical inverse
   of the forward cube so hue at least round-trips, but the asymmetry remains —
   RYB's only real payoff is the painter's-wheel complement feel.

## 4. The new foundation: schemes built from composable components

Rather than choose "the one right color space / algorithm," **let the user pick a
named scheme**; each scheme is a small, predictable, one-directional function.
"Mix-and-match" = **pick a scheme → provide whatever colors you've fixed → it
completes the rest** under that scheme's rule. No universal cube, no impossible
inversions, no round-trip contract.

The key idea for designing "probably more" schemes: **decompose a scheme into
orthogonal components, then a scheme is just one choice per component.** New
schemes = new combinations.

### Candidate components (the things to mix-and-match)

1. **Distinction axis** — how the two players differ: *value* | *hue* | *chroma*.
2. **Anchor / what's fixed** — *board* | *one piece* | *two pieces* | *seed only*.
3. **Board role** — *neutral field* (loose figure-ground) | *figure-ground ground*
   (must meet a floor) | *derived for contrast*.
4. **Piece lightness policy** — *value extremes* (black/white) | *symmetric
   straddle about the board* | *both on one side* | *equal mid*.
5. **Chroma policy** — *achromatic pieces* | *vivid pieces* | *pale (low-chroma)
   board* | *match input*.
6. **Contrast model** — which contrasts are *binding* vs *reported*: piece↔piece,
   piece↔board; and the floor value.
7. **Harmony relation** (optional) — *none* | *complement* | *split* | *triad* |
   *analogous* | *tetrad*, and the **space** it's reasoned in.
8. **Color space for reasoning** — *sRGB/HSL* | *OKLCh (perceptual, recommended)*
   | *RYB (artist)*.
9. **Gamut handling** — chroma reduction on output to stay in sRGB.
10. **Diagnostics / warnings** — which failure conditions to surface.

### The four candidate schemes, instantiated across the components

| Component | A. Value-contrast | B. Hue-contrast | C. Harmonic | D. Manual + diagnostics |
|---|---|---|---|---|
| Distinction axis | value | hue | hue (or mixed) | user-defined |
| Anchor | board (field) | board (pale) or one piece | seed or board | all three fixed |
| Board role | neutral field, loose FG | pale ground, FG floor binds | derived | user-set |
| Piece lightness | value extremes (black/white) | similar mid | derived for contrast | user |
| Chroma | achromatic pieces | vivid pieces, pale board | harmonious | user |
| Contrast model | piece↔piece binds; FG reported | FG floor binds; pieces by hue | FG floor | report only |
| Harmony | none | optional among pieces | explicit (comp/split/triad) | n/a |
| Space | luminance (sRGB fine) | OKLCh or RYB | OKLCh | n/a |
| Examples | Go, Reversi, chess | maps, Risk | "make it pleasing" | validator |

### More schemes worth considering (to seed "probably more")

- **Team/brand-anchored** — pieces fixed to brand/team colors (pieces are the
  anchor), derive a board that suits them.
- **Accessibility-max** — ignore aesthetics, maximize the minimum of all the
  binding contrasts (this is the existing minimax, foregrounded as its own mode).
- **Monochrome value-ramp** — single hue, three lightnesses (board + two pieces).
- **Tinted-neutral / wood** — warm-neutral board + value-extreme pieces (a Go
  variant that makes the board warmth explicit).
- **Analogous-board + complementary-pieces**, etc. — once the components exist,
  these fall out as combinations.

## 5. What's salvageable from the current build

Most of it — it gets *reorganized under a scheme selector*, not thrown away:

- **WCAG contrast / relative luminance** (`legibility.*`) — keep as-is.
- **Minimax background luminance** (`minimax.*`) — keep; it's exactly the
  "Accessibility-max" / derive-board-for-a-value-pair component.
- **Harmony angle helpers** (`harmony.*`) — keep for the hue/harmonic schemes.
- **Conversion** (`conversion.*`) — keep the true-inverse machinery if we keep
  RYB; or swap the reasoning space to OKLCh (then this becomes the sRGB↔OKLab
  transforms instead, which are exact and avoid the cube pathologies).
- **Qt UI shell** (`palette_widgets`, `palette_gui`) — keep; the mode toggle
  becomes a **scheme selector**, the rest of the panel (swatches, board preview,
  diagnostics, warnings) stays.

## 6. Open questions to think about

- Which schemes make the v1 set (the four above? which "more"?).
- The natural **anchor** per scheme (board-anchored is simplest and fits most
  board games; pieces-anchored matters for brand/team colors).
- Is the **component list** (§4) the right decomposition? Are there missing
  components? Which are truly orthogonal vs entangled?
- **Color space**: commit to OKLCh for the hue/harmonic schemes (clean,
  invertible, perceptual) or keep RYB for the painter's feel? (Value schemes
  barely care — they're about lightness.)
- Should piece↔piece contrast be a *first-class* constraint (it must be, for the
  value schemes — the current model omitted it)?
- How much should schemes share vs. be fully independent functions?
