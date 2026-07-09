<!-- Copyright Ben Paul Wise. All Rights Reserved. -->

# GAMS front-end plan: gates and the GP1 step list (2026-07-08)

**Trademark and warranty disclaimer.** "GAMS" is a registered trademark of
GAMS Development Corporation. This work is not endorsed or certified by
GAMS Development Corporation. The subset of the GMS parsed by this code is
incompatible with most of the GAMS modeling language. The software is
provided without warranty of any kind, express or implied, including
without limitation for any particular purpose. The provider makes no
guarantees about its performance, accuracy, or suitability for any
specific application. Every mention of GAMS in this document refers to
that limited, incompatible subset.

Plan of record for the GAMS-subset front-end (the census and estimate are
in `2026-07-08-gams-subset-census.md`; the pending-decision record is the
status board's "Deferred: problem input format" section — now decided:
proceed, staged, starting at GP1). Under option C the AMPL/MP `.nl` reader
remains a later, separate integration; GP3's internal representation is
the shared target either way.

## Gate board

| Gate | What | Exit criterion |
|------|------|----------------|
| GP1 | Grammar + AST + canonical echo | All six corpus files parse; census-count assertions green; round-trip idempotence green; loud-failure cases throw |
| GP2 | Symbol table + eager evaluator + data (`$include`/`$macro` already at GP1) | Derived parameters match hand-computed values per file |
| GP3 | `VIModel` builder: index expansion, packing, MCP pairing, `z0` from `.L` | End-to-end on `forcepkg_ln` (affine — an LCP): parse -> build -> solve -> converged |
| GP4 | Acceptance parity vs in-repo references | alloceff reproduces the SAOE reference (`saoe_test` / `gams_alloceff_test` relatives); deploy family checked against `gams_deploy_test`'s verified transcription |
| GP5 | **Result reproduction (Ben's added gate)**: reproduce recorded results for every corpus file that has them | See inventory below |

### GP5 dispositions (2026-07-09, probe measurements in)

- **forcepkg**: ctest-gated, MATCHES the MILES listing levels exactly
  (`GmsSolve.ForcepkgReproducesGamsLevels`).
- **pewem**: ctest-gated on the degeneracy-robust aggregates, MATCHES
  (`GmsSolve.PewemReproducesGamsAggregates`).
- **alloceff**: ctest-gated on the AGGREGATE equilibrium, MATCHES PATH
  (gamma + nfv + sigma to listing rounding; solved in 2.06 s). The
  actor-level attribution (beta, eff) differs and is deliberately
  ungated: aggregative-game multiplicity — per-option totals pinned,
  individual splits not (`GmsSolve.AlloceffReproducesPathAggregates`).
- **glra4B**: MATCHES, verified by probe (not ctest — the run is ~8.5 min,
  dominated by the 489 s exact-M assembly). Affinity exact (2.6e-15);
  interior point converged in 16 iterations / 5.1 s; the PATH shortfall
  quartet reproduced to four decimals; TotalDlvrd 535 with 28,180 of the
  50,000 ton-mile budget used. NOTE: the `.gms` comments' "PATH delivered
  527.556" is from an EARLIER configuration (facp 145 / ~26-27.5k
  budgets); with this file's data, total supply is exactly 535 and PATH's
  own shortfalls imply delivered 535.01 — the two references agree.
- **deploy_v09**: BLOCKED ON PERFORMANCE, not correctness (Ben,
  2026-07-09). Parse/eval/build/parity are all verified; the semismooth
  probe (20 iterations, 24 min) thrashed the plateau exactly as every
  standalone engine did on deploy_v07 — this family needs the
  alternating chain, and at the interpreted-F cost (0.0294 s/F, ~53 s
  per FD Jacobian) a chain run is hours. Measured levers if ever needed:
  AD on the Expr trees (exact sparse Jacobians), a tape/CSE evaluator
  (glra4B's F spends its 0.211 s recomputing one 900-term sum 900
  times). Ben's decision of record: PRODUCTION problems get hand-built
  C++ matrix structures + data files, one per problem; this front end
  serves as the reference/prototyping path.

### GP5 result inventory (what "results available" means per file)

Ben supplied the GAMS solution listings on 2026-07-08; they live beside
the models: **`doc/forcepkg_ln.solve.lst`** (MILES, Model Status 1
Optimal, full variable levels — e.g. fs = (0, 471.718, 1819.606,
855.367, 0), beta = (0, 0, 0.696, 0.169, 0, 0.444)),
**`doc/pewem01.solve.lst`** (MILES, Optimal, full levels), and
**`doc/deploy_v09.solve.lst`** (PATH, Optimal, full levels). These are
the authoritative expected results for their models.

- **forcepkg_ln**: reproduce the `forcepkg_ln.solve.lst` levels (fs,
  beta) within a stated tolerance. The model is an LP-complementarity
  LCP, so degenerate alternatives are possible — compare the level
  vectors first and fall back to objective/support comparison only if a
  legitimately different vertex appears.
- **pewem01**: reproduce the `pewem01.solve.lst` levels (prices p,
  quantities, rents) within tolerance; same degenerate-alternative
  caveat.
- **deploy_v09**: PATH's equilibrium in `deploy_v09.solve.lst` at
  aSm = 0.25 is the reference; multiple Nash equilibria are expected in
  this family (deploy_v07 found two), so the gate is: converge feasibly,
  compare against the PATH levels, and send any DIFFERENT equilibrium to
  Ben for roster verification (deploy_v07 precedent) rather than failing
  outright.
- **glra4B**: `doc/glra4B.solve.lst` exists but CAUTION — its own solve
  summary reads MILES, SOLVER STATUS 2 Iteration Interrupt, MODEL STATUS
  6 Intermediate Infeasible, TotalDlvrd 535: it is the run the `.gms`
  comments flag as violating the ton-mile limit, NOT a clean optimum.
  The `.gms` comments' PATH/NLPEC values remain the better target:
  shortfalls N000 13.977, N007 3.256, N019 2.440, N024 4.373 and
  delivered 527.556. Ben expects the solution to be unique; if it turns
  out not to be, the gate is: FEASIBLE (budget respected) with the
  delivered/objective value near the listed PATH/NLPEC figures. The
  `.lst` serves as a detail record only (its ValAchieved is 14321.925 on
  the infeasible point).
- **alloceff01cm** (the model called "alloceff" in Ben's Octave work):
  `doc/alloceff01cm.solve.lst` (PATH, Model Status 1 Optimal, full
  levels) is the expected solution per Ben (2026-07-08). Historical
  note from the `.gms` comments: PATH's equilibrium did not always match
  the NLPEC/saoeJNrn one and multiple equilibria exist, so a converged
  DIFFERENT equilibrium goes to Ben for verification rather than
  failing outright; the in-repo SAOE reference (`saoe_chain_test`)
  remains a secondary cross-check.

## GP1 — step list

Scope rule: GP1 parses and represents; no evaluation, no index expansion,
no `VIModel`, no solver dispatch (those are GP2-GP4). Exactly the
censused grammar; anything else throws with `file:line:col`.

1. **Module skeleton.** New top-level `gams/` mirroring `network/`:
   `gams/include/`, `gams/lib/`, `gams/test/`, own CMakeLists, static lib
   `vincpgms` at `/W4`-as-errors. Standard library only (no Eigen, no
   `vincp` link at this gate). Namespace `VINCP::Gms`; house style
   throughout. Files: `gmstoken.hpp`, `gmslexer.{hpp,cpp}`, `gmsast.hpp`,
   `gmsparser.{hpp,cpp}`, `gmsecho.{hpp,cpp}`.
2. **Lexer.** Tokens: identifiers (original spelling + lower-cased key —
   case-insensitivity is corpus-load-bearing), numbers incl. scientific,
   quoted strings (`'` and `"`), operators, `..`, `=e=/=g=/=l=`, `=`,
   NEWLINE (significant only in list contexts). `*` in column 1 =
   comment. `\r` and BOM tolerated (Windows files).
3. **Directives + `$macro`.** `$ONSYMLIST` ignored; `$include` spliced
   (path relative to the including file); `$macro` captured, then a
   token-level expansion pass substitutes arguments and recursively
   expands nested macros (depth guard). Any other `$` throws.
4. **AST.** Value-semantic structs + `std::variant<...> Statement`;
   one `Expr` tree (Number / SymbolRef / AttrRef / Unary / Binary /
   Call / Sum). Defaulted deep equality; no source positions in AST
   nodes (parse errors carry positions from tokens; semantic positions
   are GP2's concern). Declaration statements hold ITEM LISTS (Scalars /
   Parameters / Variables / Equations blocks declare many symbols);
   descriptions may be quoted or bare-to-end-of-line; data lists accept
   comma OR newline separators; table rows must be dense.
5. **Canonical echo.** `gmsecho` prints an AST to canonical text
   (quoted descriptions, full-paren expressions, `%.17g` numbers).
   Round-trip criterion is IDEMPOTENCE: `parse(echo(parse(F))) ==
   parse(F)` — byte identity is impossible (comments, alignment).
6. **Tests (`gms_parse_test`, GoogleTest).**
   - Census-as-assertions: per-file construct counts (statements by
     kind, declaration items, equation defs by relation, model pairs,
     display items) — the census table becomes EXPECT_EQs.
   - Round-trip idempotence on all six files.
   - Macro tests: synthetic nested-macro expansion against a hand-built
     AST; deploy_v09's macro table has exactly 15 entries.
   - Case-insensitivity: keys are lower-cased in the AST.
   - Loud failures: unsupported `$` directive, `$(...)` condition,
     `loop`, sparse table row — each throws `std::invalid_argument`.
   - Corpus files referenced in place under `doc/` (reuse over
     duplicate) via a compile-time path constant.
7. **CMake.** Root gains `add_subdirectory(gams)`; `gms_parse_test`
   registered via `gtest_discover_tests`; aggregate `run_gams_tests`;
   `gms_parse_test` added to `run_all_tests`. CLion: CMake reload
   required (new directory + targets).
8. **Docs.** CLAUDE.md gains a short `gams/` layer paragraph; status
   board and this plan updated at gate close.

**GP1 exit criteria:** (a) all six files parse; (b) all test classes
above green; (c) Ben's build+run (`ctest -R GmsParse` plus the full
suite; the new expected total test count stated in the handoff so a
stale build is detectable); (d) stop for review + token-usage report
before GP2.

Corpus correction found while deriving the test expectations: deploy_v09's
`Model interdict` has **24** pairs (10 cost-benefit + 14 slackness),
not the 26 the census table said; the census document is corrected in
the same commit as this plan.

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
