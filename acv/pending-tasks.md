# Pending tasks

Open work on the sparse-matrix problem. Last revised 2026-07-27.

## Closed

### 2. Lasso path as candidate generator, Φ as judge — **DONE, and it works**

`examples/large_lasso_phi.py`, results in `examples/large_lasso_phi.txt`, table in
`examples/tab_lasso_phi.tex`, written up as Section 12.6 of `acv-preliminary.tex`.

Over three seeds and both arrangements at N = 200, T = 80, about 2569 true cells of
40,000:

| candidates from | correct | false | missed | σ̂ | Φ |
|---|---|---|---|---|---|
| forward selection | 2547 of 2569 | 7 | 22 | 0.239 | — |
| and the plain path | 2547 of 2569 | 8 | 22 | 0.239 | +5095 |
| **and the adaptive path** | **2569 of 2569** | **0** | **0** | **0.101** | **−6398** |

Exact recovery in all six runs. The Φ of −6398 is the value measured at the *true*
pattern in `large_phi_check.txt`, so the criterion found the pattern it had preferred
all along.

Two things learned, both worth keeping:

- **The plain lasso path is a poor generator.** It visits the true subset of a row
  only 19 times in 200, against 194 for forward selection
  (`examples/large_cand_check.txt`). Adding it changes almost nothing.
- **The adaptive path works only if its weights come from a fit that is too dense.**
  Weighting by the answer Φ has just given fails completely: a cell that answer
  leaves empty receives the largest weight the rule allows and can never return.
  Weighting by a cross-validated fit — about 11,000 cells, containing every true one
  — is what makes it exact. Cross-validation needs no σ and is used only to weight,
  never to select, so nothing an analyst lacks enters. Φ makes every selection.

### 1. Fix the row search so it can exchange a column — **superseded, not done**

The plan was to let forward selection substitute a column rather than only truncate,
so that the six rows in 200 where it admits a wrong column would stop costing
twenty-four cells and inflating σ̂ to 0.25. Task 2 solved the same problem by leaving
forward selection alone and giving Φ a better list to choose from.

Reopen only if the cost of task 2 proves unacceptable and a cheap exact search is
wanted instead. The diagnosis is preserved in Section 12.5 of the paper: Φ prefers
the true pattern by 11,448 natural units; forward selection reaches the true subset
at the right size for 194 of 200 rows; in the other six the wrong column's fitted
coefficient falls below `a_min`, which makes that subset inadmissible, so the row is
truncated rather than corrected. The true subset itself passes the range test in all
200 rows.

## Open

### 3. Reduce the cost of the candidate generation

Forward selection prepares the matrix in about a second and the whole procedure runs
in sixteen. The cross-validated weighting takes some forty minutes, being five
penalty paths for each of two hundred rows. There is no obstacle of principle to
reducing this. Obvious avenues: fewer folds; a cheaper first-pass estimate for the
weights than cross-validation; a coarser penalty grid for the weighting pass only,
since it decides only the weights and not the answer; and a compiled coordinate
descent in place of the pure-Python one.

### 4. Handle the truncation at `a_min` properly — carried over from the small study

The Laplace step of Section 9.1 ignores the truncation at `a_min` and treats
∏ 1/|A_ij| as constant where it is steepest. Both errors overstate the integral for
small coefficients, which is where the spurious cells appeared at N = 9. Section
10.6 names this as the first improvement to attempt at that size. It is not what
limits the answer at N = 200.

### 5. Open questions about the criterion itself, named in Section 12.8

Whether $E[\hat{M}] - M$ is positive, negative or zero in general; how it behaves as
T grows with q fixed; and whether the pattern is recovered in the limit. Two sizes
have been examined and they fell on opposite sides. No proof is available for any of
it.
