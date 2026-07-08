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

### GP5 result inventory (what "results available" means per file)

- **glra4B**: the `.gms` comments record per-solver shortfalls
  (PATH/NLPEC: N000 13.977, N007 3.256, N019 2.440, N024 4.373) and
  objective values (527.556 PATH/NLPEC; 529.171 KNITRO; MILES violating
  the ton-mile limit). Gate: visolver's solve of the parsed model
  reproduces the PATH/NLPEC shortfall pattern and objective within a
  stated tolerance.
- **alloceff**: the file records that NPLEC agrees with Ben's saoeJNrn
  (Josephy-Newton) answer at alpha = 1.0, and the repo carries the
  verified SAOE reference equilibrium. Gate: parsed model reaches the
  reference equilibrium E (roster mechanism as in `saoe_chain_test`).
- **deploy_v09**: no listing numbers in-file, but the file records that
  PATH solves at aSm = 0.25 (and fails at 0.35+). Gate: parsed model
  converges at aSm = 0.25 to a feasible equilibrium; the specific
  equilibrium goes to Ben for roster verification (deploy_v07 precedent).
- **forcepkg_ln, pewem01**: no recorded numbers in-file. Gate: solve to
  convergence; Display-style output handed to Ben for plausibility (or
  exact GAMS listings if Ben supplies them — standing data request).

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
