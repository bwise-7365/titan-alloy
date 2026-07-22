# Palette selector — design specification

A reusable C++ library that completes a three-color game palette: one **board
background** `B` plus two **opposing-player piece colors** `P1`, `P2`. Three
entry modes depending on what the user has already fixed. Harmony is reasoned on
the **RYB color wheel**; legibility is reasoned on **sRGB relative luminance**.
Color-vision-deficiency handling is explicitly **out of scope** (normal
trichromatic vision assumed).

---

## 1. Two decoupled layers

Keep these in separate coordinate systems; never conflate them.

**Harmony layer (angular, on the RYB wheel).** A color carries a hue angle
`h_ryb ∈ [0,360)`. All harmony operations are angular on `h_ryb`:
- complement: `+180°`
- split-complement: `+180° ± α` (default α = 30°)
- triad: `±120°`
- analogous: `±α`
- tetrad: the two diagonals.

**Legibility layer (luminance, on rendered sRGB).** Contrast is computed only
after rendering to sRGB. WCAG relative luminance:

```
linearize(c)   = (c <= 0.04045) ? c/12.92 : ((c+0.055)/1.055)^2.4   // per channel, c in [0,1]
L(srgb)        = 0.2126*linearize(R) + 0.7152*linearize(G) + 0.0722*linearize(B)
contrast(a,b)  = (max(L_a,L_b)+0.05) / (min(L_a,L_b)+0.05)            // in [1,21]
```

**Bridge: RYB↔sRGB.** Use Gossett–Chen trilinear interpolation over an RYB cube
whose 8 corners hold Itten-consistent sRGB values. `rybToSrgb` is the forward map.
`srgbToRyb` (needed because Modes 2 & 3 receive user-picked sRGB colors and must
recover their RYB hue for angular harmony) is the harder inverse; ArtColors fits
the inverse cube numerically. Do **not** guess the corner constants — port them
from the references in §7.

**Why this decoupling matters:** harmony fixes only *hue*; legibility is driven
by *luminance*; these are nearly orthogonal, so hues come from harmony and
value/chroma remain free for contrast. The only coupling is gamut: a saturated
hue caps its luminance range (saturated blue can't be light; saturated yellow
can't be dark). Neutral (zero chroma) spans the full luminance range — hence the
universal fallback.

---

## 2. Legibility model (the key simplification)

With CVD out of scope, **hue difference alone distinguishes `P1` from `P2`** in
normal vision. So the piece↔piece luminance gap is *not* a constraint. The
binding legibility constraints are the two figure-ground ratios
`contrast(B,P1)` and `contrast(B,P2)`. Default floor: `3.0` (UI-component grade).

Design heuristics:
- Background should be **low chroma** so the higher-chroma pieces advance and the
  board recedes (chroma contrast reinforces figure-ground).
- The figure (pieces) should be more chromatic than the ground (board).

---

## 3. Background-luminance minimax (closed form)

Selecting a background against two fixed pieces is a 1-D minimax over background
luminance. Let `ℓ_i = L_i + 0.05` be shifted luminances, `ℓ1 ≤ ℓ2`, and `β` the
background's. Maximize `g(β) = min(contrast(β,ℓ1), contrast(β,ℓ2))`. Three
candidate optima:

| candidate | background | objective value |
|---|---|---|
| darkest  | `β = 0.05` | `ℓ1 / 0.05`          (binds on darker piece) |
| lightest | `β = 1.05` | `1.05 / ℓ2`          (binds on lighter piece) |
| interior | `β* = √(ℓ1·ℓ2)` (geometric mean) | `√(ℓ2/ℓ1)`   (the two contrasts equalize) |

Global optimum = max of the three:
- pieces **clustered** in luminance (`ℓ2/ℓ1` small) → an extreme wins; choose
  black iff `ℓ1·ℓ2 > 0.0525`, else white.
- pieces **spread** (`ℓ2/ℓ1` large) → the interior geometric-mean gray wins.
  (Black + white pieces land here: `β* ≈ 0.23` → a mid gray, the chess/beige case.)

`minimaxBackgroundLuminance(l1, l2)` returns the target `L = β* − 0.05`; the
background is then realized at the chosen hue/chroma closest to that luminance,
dropping chroma toward neutral if the harmonic hue can't reach the target.

This is microsecond-cheap; no solver needed.

---

## 4. The three modes

All three are **pure** functions returning a `Palette` (chosen colors +
diagnostics + warnings).

### Mode 1 — `fromBackground(B, template, constraints)`
`B` fixes `h_B`, `L_B`.
- Piece hues from a template **anchored on B** giving harmony-with-board plus
  mutual distinction: split-complement with B as base (`h_B+180°±30°`, 60° apart)
  or triad (`h_B±120°`). **Avoid analogous-to-B** (pieces would blend into the
  board hue).
- Piece luminances on the far side of the luminance midpoint from `L_B`
  (B dark → both pieces light; B light → both dark; B mid → split one lighter /
  one darker so each clears the floor — bonus luminance distinction). Reduce
  piece chroma only as needed to reach target luminance, staying more chromatic
  than B.

### Mode 2 — `fromOnePiece(P1, template, constraints)`
`P1` fixes `h1`, `L1`.
- Choose `P2`'s hue as the harmony partner of `P1` (the `template` decides:
  complement `h1+180°`, a split arm, or a triad partner — see TODO below).
- Then run the Mode-3 logic on `{P1, P2}` for the background. Complements ⇒
  neutral background is natural.

### Mode 3 — `fromTwoPieces(P1, P2, constraints)`
`P1`, `P2` fixed by user (may be disharmonious). Background = §3 minimax for
luminance; hue completes the pieces' scheme or goes neutral; chroma low.

---

## 5. Least-bad accommodation

The user may hand in an infeasible point. Key asymmetry: **legibility is always
satisfiable** (a free-luminance neutral background can hit `β*` for any two
pieces), while **harmony is the soft constraint** that degrades. So the library
never returns an illegible board; it relaxes harmony and emits `Warning`s. These
are valid results, not errors — do not throw, do not silently fix.

| failure | detection | response |
|---|---|---|
| pieces too close in hue (would-be opponents nearly same color), similar luminance | hue gap < threshold AND small ΔL | return best figure-ground background; `Warning::PiecesHueTooClose`; suggest minimal correction = the smaller of {spread piece luminance, widen hue gap} (luminance nudge usually least intrusive) |
| both pieces mid-luminance | `ℓ1 ≈ ℓ2 ≈ √0.0525` | return best candidate; `Warning::PiecesLuminanceMidband`; suggest spreading piece luminances |
| pieces at opposite luminance extremes | `ℓ2/ℓ1` large | NOT a failure — return geometric-mean gray |
| clashing hue pair, fine luminance | no recognized scheme among the two hues | neutralize background (chroma→0); `Warning::NoHarmonicBackground`; optionally suggest nudging one piece to nearest harmonic angle |

Minimal-correction suggestions are a projection of the user's piece colors onto
the nearest feasible set under a chosen metric (TODO: which metric).

---

## 6. API sketch (`palette_core`, no Qt)

Adjust names to taste; types are small immutable value structs.

```cpp
namespace palette {

struct Srgb   { double r, g, b; };          // each in [0,1]
struct Ryb    { double r, y, b; };          // RYB coords in [0,1]
struct HsvRyb { double hueDeg, sat, val; }; // hue measured on the RYB wheel

enum class Harmony { Complement, SplitComplement, Triad, Analogous, Tetrad };

struct Constraints {
    double minPieceBgContrast      = 3.0;
    double splitComplementAlphaDeg = 30.0;
    bool   allowPieceLuminanceSpread = true;   // TODO(decide) default
    // hue-too-close threshold, metric choice, etc.
};

enum class Warning {
    PiecesHueTooClose,
    PiecesLuminanceMidband,
    NoHarmonicBackground
};

struct Diagnostics {
    double contrastBgP1, contrastBgP2, contrastP1P2;
    Harmony templateUsed;
    double backgroundTargetLuminance;
};

struct Palette {
    Srgb background, piece1, piece2;
    std::vector<Warning> warnings;
    Diagnostics diagnostics;
};

// pure primitives
double relativeLuminance(Srgb c);
double contrastRatio(Srgb a, Srgb b);
Srgb   rybToSrgb(Ryb v);
Ryb    srgbToRyb(Srgb v);                       // inverse fit; see References
double minimaxBackgroundLuminance(double l1, double l2);  // returns target L

// the three modes (pure)
Palette fromBackground(Srgb bg,    Harmony t, Constraints c);
Palette fromOnePiece  (Srgb piece, Harmony t, Constraints c);
Palette fromTwoPieces (Srgb p1, Srgb p2,       Constraints c);

} // namespace palette
```

`palette_widgets`: an interactive `RybColorWheel : public QWidget` that paints the
wheel and emits a signal on selection, plus a controller that calls the pure core
and updates swatches. The wheel widget holds **no** business logic. `QColor`
converts to/from `Srgb` only at this boundary.

### Suggested tests (write before the widget)
- `relativeLuminance` / `contrastRatio` against known pairs (black/white = 21:1;
  a mid pair sanity-checked by hand).
- `minimaxBackgroundLuminance`: black+white pieces ⇒ `β* ≈ 0.23`; two clustered
  bright pieces ⇒ black background chosen; verify the `0.0525` boundary.
- round-trip `srgbToRyb(rybToSrgb(x)) ≈ x` within tolerance (document the
  tolerance; the inverse is approximate).
- each mode meets the contrast floor or emits the correct `Warning`.

---

## 7. References (verified)

- Gossett & Chen, *Paint Inspired Color Mixing and Compositing for Visualization*
  (IEEE InfoVis 2004) — the RYB↔RGB trilinear cube:
  http://vis.computer.org/vis2004/DVD/infovis/papers/gossett.pdf
- ProfJski/ArtColors — RYB↔RGB + triadic/split-complement/tetradic harmony,
  including the numerically-fit inverse (C/RayLib, but the math ports directly to
  C++): https://github.com/ProfJski/ArtColors
- WCAG 2.x relative luminance & contrast ratio (W3C):
  https://www.w3.org/WAI/WCAG21/Techniques/general/G18
- RYB vs RGB/CMY complementary pairs (EPFL): https://graphsearch.epfl.ch/concept/405803
- Color-harmony schemes (open-access): https://arxiv.org/pdf/2310.00791

---

## 8. Open decisions — `TODO(decide)`

1. **Default `Harmony` template** for the engine (drives Mode 2 opponent choice
   and Mode 1 piece-pair generation): Complement (max opposition), SplitComplement
   (softest, cleanest background slot), or Triad (most balanced).
2. **Piece luminance policy**: default the two pieces to equal luminance (hue
   carries distinction) or allow spread? (`allowPieceLuminanceSpread`.)
3. **Contrast target**: keep the `3.0` figure-ground floor or a different target.
4. **Hue-too-close threshold** and the **projection metric** for minimal-correction
   suggestions (RYB-wheel degrees vs perceptual ΔE in CIELAB).
5. **srgbToRyb inverse**: acceptable round-trip tolerance, and whether to port
   ArtColors' fitted cube or solve the inverse directly.
6. Namespace / CMake target names.
