<!-- Copyright Ben Paul Wise. All Rights Reserved. -->

# GAMS-subset census and implementation estimate (2026-07-08)

**Trademark and warranty disclaimer.** "GAMS" is a registered trademark of
GAMS Development Corporation. This work is not endorsed or certified by
GAMS Development Corporation. The subset of the GMS parsed by this code is
incompatible with most of the GAMS modeling language. The software is
provided without warranty of any kind, express or implied, including
without limitation for any particular purpose. The provider makes no
guarantees about its performance, accuracy, or suitability for any
specific application. Every mention of GAMS in this document refers to
that limited, incompatible subset.

Input to the pending decision recorded in `2026-07-06-engine-plan.md`
("Deferred: problem input format"): a text-based way for people to specify
problems, including solver choice. The candidate weighed here is a bespoke
parser for exactly the GAMS constructs used by six example files; the
competing option A (Pyomo/AMPL front-end producing `.nl`, read via the
BSD-licensed AMPL/MP library) is summarized at the end for comparison.

## Corpus

Six files, all in `doc/`:

| File | Lines | Model |
|---|---|---|
| `forcepkg_ln.gms` | 115 | force-package selection, affine MCP (an LCP) |
| `alloceff01cm.gms` | 163 | exertion-of-influence Nash game (SAOE family) |
| `glra4B.gms` + `glra4B.inc` | 211 + 254 | global logistics as resource allocation |
| `pewem01.gms` | 228 | partial-equilibrium energy market |
| `deploy_v09.gms` | 498 | PPD softplus-salvo interdiction game |

## Construct census

| Construct | forcepkg | alloceff | glra4B(+inc) | pewem | deploy_v09 |
|---|---|---|---|---|---|
| `Set` (1-D lists), `Alias` | yes | yes + alias | yes + 5-way alias | yes | yes + alias |
| `Scalar` / `Parameter` with `/ key value /` data | yes | yes | yes (newline-separated pairs) | yes (incl. `k1 .k2 = v` 2-D form) | yes |
| `Table` (2-D, dense) | 3 | 1 | 3 (30x30, no `+` continuation) | none | 4 |
| Assignments `p(i) = expr;` (eager; post-solve uses `.L`) | yes | yes | yes | yes | heavy |
| `Positive Variable` / free `Variable` | pos only | pos only | pos only | pos only | both |
| `.L` / `.UP` attribute assignment | none | `.L`, `.up` | `.L` | `.l` | `.L`, `.UP` |
| Equations `=e=` / `=g=` | `=g=` | both | `=g=` | `=g=` | both |
| `sum` incl. multi-index `sum((i,j),...)`, alias indices | yes | yes | yes | yes | nested |
| Intrinsics | sqrt | sqrt, round | `**` | `**` | exp, log, sqr, card, max |
| `Model /eq.var, .../` MCP pairing | yes | yes | yes | yes | 24 pairs |
| `Option MCP = X;`, `Solve ... using MCP` | MILES | NLPEC | MILES | MILES | PATH |
| `Display` | yes | yes | yes | yes | yes |
| `$` directives | `$ONSYMLIST` | same | + `$include` | same | + `$macro` x15, nested |

Grammar details the corpus requires: identifiers are CASE-INSENSITIVE
(`C_B_Red_prob` vs `C_B_Red_Prob`, `Vr` vs `VR`, `.l` vs `.L`); descriptions
may be quoted or bare words; data-list separators are comma OR newline;
`Model` pair lists separate by comma OR newline; comments are `*` in
column 1.

## What is absent (the headline)

Across all six files: no `$(...)` conditional evaluation in any rule, no
`loop`/`if`/`while`, no dynamic sets, no `ord`/lag/lead, no sparse tables,
no table `+` continuation, no put-files — and **no objective functions at
all**. Every model is a pure MCP with hand-derived stationarity conditions.
That removes the two most expensive subsystems of a general front-end:
automatic differentiation and NLP/QP objective handling. The parser only
ever EVALUATES expressions; the FD Jacobian and the engine catalog do the
rest.

## Mapping onto VINCP

- `Model /eq.var/` maps one-to-one onto `VIModel`: free variables paired
  with `=e=` rows form the `H` block; positive variables form the `G`
  block. Pairing semantics derive from the VARIABLE'S KIND, not the
  relational operator (alloceff pairs `=e=` rows with positive variables;
  the mixed-complementarity reading `0 <= z perp F >= 0` is still correct);
  the operator gets a consistency check only.
- Variable packing: free block first, then nonnegative, matching
  `makeMixedProjector`. Initial `.L` assignments become `z0` (the corpus
  comments stress interior starts).
- Instance scale is comfortable: glra4B, the largest, expands to ~1,950
  unknowns.

## Work plan and estimate

Components, in cost order:

1. Lexer + recursive-descent parser -> AST: statements, expressions,
   `/ ... /` data blocks, dense `Table` blocks ("row label + K values" —
   every table in the corpus is fully dense), case-insensitive symbol
   handling, `$ONSYMLIST` ignored, `$include` spliced, and `$macro`
   textual expansion with arguments (deploy_v09 builds its combat model
   from 15 nested macros; all bodies are parenthesized, so parse-time
   expansion is faithful). ~1,200-1,500 lines.
2. Symbol table + eager evaluator: sets, aliases, keyed parameter arrays,
   indexed assignment loops, intrinsics {exp, log, sqrt, sqr, max, round,
   card, `**`}. ~500-800 lines.
3. Instance builder: expand equations over domains, pack `z`, pair per the
   `Model` block with dimension checks, emit `H`/`G` as AST evaluators.
   ~300-500 lines.
4. Post-solve: write `.L` back, run reporting assignments, print
   `Display` — needed for parity checks against GAMS listings.

Anything outside this footprint throws (the house throw-don't-substitute
stance): the subset parser fails loudly on the sixth file's novelty rather
than parsing it wrongly.

**Total: ~2,500-3,500 lines including tests, four gates:**

- **GP1** — grammar + AST, round-trip tests on all six files.
- **GP2** — evaluator + data + `$include`/`$macro`, checked against
  hand-computed values.
- **GP3** — `VIModel` builder, end-to-end on `forcepkg_ln` (affine — an
  LCP, the ideal smoke test).
- **GP4** — acceptance parity: alloceff against the SAOE reference,
  deploy_v09 against its PATH-verified listing (in-repo relatives:
  `saoe_test`, `test/gams_deploy_test.cpp`).

Days-scale per gate under the build-run-review loop: **roughly a week of
gated sessions**, not several weeks.

## Open decisions (Ben)

1. **`.UP` bounds** (alloceff, deploy_v09). Both files' own comments say
   the bounds are redundant at solutions and exist for numerical
   protection. `VIModel`'s `K` is the orthant — no boxes. First pass:
   parse, surface on the built model, warn. Box-`K` later via the
   `Projector` seam for the projection engines if ever needed; the
   semismooth path would need a bounded NCP function (real extension).
2. **`Option MCP = MILES/PATH/NLPEC`** — foreign solver names. Proposal:
   engine choice comes from the caller/CLI (default `solveModelAuto`),
   the statement is echoed-and-ignored, and an extension key (e.g.
   `Option MCP = VISOLVER_SSN;`) can carry an explicit engine choice.

## Comparison hook: option A (for the pending decision)

Option A (Pyomo or licensed AMPL -> `.nl` -> visolver via the AMPL/MP
reader, which has a first-class `OnComplementarity` callback): ~days of
integration work, PATH byte-parity for free, but model authors must run
Python or own AMPL. The bespoke subset parser above: ~a week of gated
sessions plus a permanent maintenance surface, but fully self-contained
(one executable, text in, solution out) and the input language is exactly
the GAMS dialect the existing model corpus is already written in — the
six files run UNMODIFIED. The two are not exclusive: the internal problem
representation (gate GP3's output) is the same thing an `.nl` handler
would target, so starting with either leaves the other open.

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
