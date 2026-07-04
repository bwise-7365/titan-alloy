<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
# Handoff: Logistics network QP (system-optimal distribution planning)

*For the next Claude instance. Read before starting. Records a new problem the user
is bringing to the `visolver` / `VINCP` codebase: the classification and
recommendations already agreed, the generalization coming next, and a staged-plan
skeleton to flesh out with the user. Planning only — no code written for it yet.*

## Relationship to the existing codebase
- `visolver` / `VINCP` solves VIs / NCPs (`dHan06`, `bsHe94b` inner LVI solvers;
  `solveVI` Josephy-Newton outer loop; the SAOE Nash-equilibrium demo/test). See
  `CLAUDE.md`.
- This new problem is a **system-optimal convex QP** (one planner), **NOT** a
  game / equilibrium. Do NOT conflate with SAOE (a non-monotone game). It may or
  may not be solved with the VINCP tools — see recommendations.

## The current problem (single commodity, single vehicle)
Central planner chooses flows `f_ij` on a sparse network (~100 nodes; ~500
feasible directed links out of ~10,000 possible — most links are infeasible,
beyond vehicle range without an intermediate stop).
- `S_i = sum_j f_ij` (supply out of node i); `R_i = sum_j f_ji` (resupply into i).
- **Objective:** `min  sum_i P_i * ((D_i - R_i)/D_i)^2` — weighted squared
  *fractional shortfall* (`P_i` priority, `D_i` demand). Separable and strictly
  convex in `R_i`; diagonal Hessian `Q_ii = 2 P_i / D_i^2`.
- **Constraints (all linear):**
  - flow balance (`=`) at each node  [exact semantics: confirm — see open questions];
  - supply cap `S_i <= Cap_i`;
  - non-negativity `f_ij >= 0`;
  - ONE global coupling row: ton-mile budget `sum_ij c_ij f_ij <= B` (`c_ij` ~
    distance). "Not enough ton-miles to move everything."

### Classification & key structure
- **Sparse convex QP, system-optimal.** Used routes have very different costs
  (NOT equalized) — the user was explicit this is not a user equilibrium.
- Cost touches flows only via node aggregates `R_i` (and `S_i`) => flow-space
  Hessian is low-rank. Introduce `R_i` / `S_i` as auxiliary node variables tied by
  the sparse incidence (2 nonzeros/column) => Hessian becomes **diagonal**, all
  coupling linear.
- Optimal `R*` is unique (strictly convex in R); the flow pattern `f` may be
  non-unique (degenerate) => add a lexicographic tie-break (e.g. min ton-miles) if
  a determinate `f` is needed.
- Exactly ONE coupling constraint (the budget); everything else is network-local.
  This is the exploitable feature.

### Recommendations already agreed (single-commodity)
1. **Lagrangian on the budget (best structure exploit).** Dualize only the
   ton-mile row with scalar `lambda >= 0` => inner problem is a
   separable-convex-cost **min-cost flow** (network-structured, fast; strongly
   polynomial for convex-quadratic costs, Vegh). 1-D bisection/Newton on `lambda`
   to hit `B` (usage is monotone in lambda). `lambda*` = shadow price
   (weighted-shortfall reduction per ton-mile).
2. **OSQP (robust off-the-shelf).** Genuine convex QP; budget row + caps + balance
   + `f>=0` go in directly; sparse KKT factored once + warm-start across the many
   per-instance solves.
3. **Reuse VINCP via KKT -> monotone affine mixed LCP.** Free block = balance
   multipliers; non-negative block = flows + cap multipliers + (scalar) budget
   multiplier; solve with `bsHe94b` (factor `(M+I)` once). Monotone => converges
   cleanly (unlike SAOE). Viable reuse, but OSQP / network decomposition are more
   natural for a plain convex QP.

### Data structures
- Edge list for the graph; sparse node-arc incidence for balance and for defining
  `R_i` / `S_i`.
- `Eigen::SparseMatrix<double>` (CSC); `SimplicialLDLT` factored once, reused.
- At this size (~500 flows + ~2M multipliers) dense would also be fine; sparsity
  matters as it scales or as you loop over instances.

## The coming generalization (likely; plan tomorrow)
User expects to add:
- **Multiple vehicle types** (3-5).
- **Multiple goods** (2-8).
- **Per-(vehicle, good) carrying limits** => multiple cost matrices.
- **Multiple interacting resource budgets** (ton-miles, gallon-miles, ...) =>
  several coupling constraints, not just one.
- Objective: each good at each location assessed quadratically, **separately** from
  other locations  [confirm: also separate from other goods].

### Implications
- Variables become `f_ij^g` per good g (and possibly per vehicle v, or vehicles are
  a shared capacity resource) => count multiplies by (#goods x maybe #vehicles);
  still sparse.
- **Multiple coupling constraints** => the elegant 1-D dual (single lambda) becomes
  a small vector of resource prices (a handful) => low-dimensional dual (dual
  ascent / subgradient / few-dim root find); still very tractable. Or solve the
  larger convex QP directly (OSQP).
- Still a **convex QP** with a separable convex-quadratic objective across
  (location, good) => diagonal Hessian in the aggregates. Convexity / monotonicity
  preserved => all three approaches still apply, scaled up.
- **Vehicle modeling fork (confirm):** do vehicles carry mixed goods under a joint
  capacity (=> per-vehicle-leg multi-good packing coupling, combinatorial, harder),
  or is "vehicle type" just another named resource budget (ton-miles / gallon-miles
  per type, which keeps it a clean QP)?

## Staged plan skeleton (flesh out with the user tomorrow)
- **Stage 0 (current):** single good, single vehicle, single ton-mile budget.
  Implement + validate on a small hand-checkable instance.
- **Stage 1:** multiple interacting resource budgets (still single good/vehicle) =>
  vector dual / multi-row QP.
- **Stage 2:** multiple goods (separable objective; shared resource budgets).
- **Stage 3:** multiple vehicle types + per-(vehicle, good) capacity (resolve the
  vehicle fork first).
Each stage: fix data structures, pick a solver (Lagrangian-network vs OSQP vs
VINCP-LCP), define a small validation instance, add a test.

## Open questions to resolve tomorrow
- Exact "flow balance at each node" semantics (transshipment conservation? supply
  injection + inflow = outflow + consumption? how `S_i`, `R_i`, `Cap_i`, `D_i`
  relate).
- Are goods coupled at a node (shared handling/storage) or fully separable?
- Vehicle modeling: shared resource budgets vs per-vehicle packing capacity.
- Is a determinate `f` needed, or is `R*` enough? (degeneracy tie-break.)
- Build target: OSQP dependency acceptable? keep everything in the VINCP codebase?
  a custom Lagrangian-network solver? (Repeated per-instance solves + possible JNI
  context favor factor-once / warm-start.)
- Full-scale sizes (nodes, links, goods, vehicles) => whether sparsity/decomposition
  is essential vs nice-to-have.

## Sources
- Vegh, "A Strongly Polynomial Algorithm for Minimum-Cost Flow with Separable
  Convex Objectives," arXiv:1110.4882 / SIAM J. Comput. — convex-quadratic
  min-cost-flow inner solver.
- "Convex Network Flows," arXiv:2404.00765 — convex-cost network-flow framework +
  duality.
- OSQP, arXiv:1711.08013 — sparse convex QP, factor-caching + warm-start.
- Background only (NOT applicable — this is system-optimal, not user equilibrium):
  Dafermos 1980; Nagurney, *Network Economics* (those cover asymmetric-cost user
  equilibria / VIs).

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
