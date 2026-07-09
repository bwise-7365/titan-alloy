<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
# Literature basis (task A2)

Annotated bibliography, organized by the claim in our approach that each
source supports. Together with the sources already in
`../../2026-07-01-logistics-qp-handoff.md`, these are the citations for
report Part III. Conclusion at the end.

## Claim 1 — Transshipment reduces to transportation via shortest paths

The foundation of our preprocessing (task A3/C1): with per-unit arc costs and
free transshipment, an optimal plan may be assumed to route along shortest
paths, so only source-to-sink shipments at all-pairs shortest distances need
be decided.

- A. Orden, "The Transhipment Problem," *Management Science* **2**(3):
  276-285, 1956. The original statement that the transshipment extension of
  the transportation problem collapses back to a transportation problem over
  optimal (shortest) linked paths.
  <https://pubsonline.informs.org/doi/abs/10.1287/mnsc.2.3.276>
- R. K. Ahuja, T. L. Magnanti, J. B. Orlin, *Network Flows: Theory,
  Algorithms, and Applications*, Prentice Hall, 1993. Standard modern
  treatment (transshipment-to-transportation transformation; flow
  decomposition, used in our reduction lemma proof).

## Claim 2 — Projection-contraction converges on monotone linear VIs

The convergence guarantee behind `bsHe94b`, which does our single affine
solve.

- B. He, "A new method for a class of linear variational inequalities,"
  *Mathematical Programming* **66**:137-144, 1994. The method `bsHe94b`
  implements; global convergence for monotone (PSD) linear VIs.
  <https://link.springer.com/article/10.1007/BF01581141>
- B. He, "A projection and contraction method for a class of linear
  complementarity problems and its application in convex quadratic
  programming," *Applied Mathematics and Optimization* **25**:247-262, 1992.
  **Direct precedent for our exact pipeline**: convex QP -> LCP ->
  projection-contraction. Strengthens Part III considerably.
- D. Han, "A generalized projection method for variational inequalities" /
  self-adaptive variant (2006) — already implemented as `dHan06`; kept as the
  alternative inner solver for cross-checking (`han_vs_he` pattern).

## Claim 3 — KKT of a convex QP is a monotone mixed LCP

Justifies handing the KKT system to a monotone-LVI solver (task C2/C4).

- R. W. Cottle, J.-S. Pang, R. E. Stone, *The Linear Complementarity
  Problem*, Academic Press, 1992 (SIAM Classics reprint 2009). QP-KKT to
  LCP construction; the KKT matrix of a convex QP is bisymmetric positive
  semidefinite, hence the LCP is monotone.
- F. Facchinei, J.-S. Pang, *Finite-Dimensional Variational Inequalities and
  Complementarity Problems*, Springer, 2003. General VI/CP reference
  (existence, monotonicity taxonomy); already the codebase's theoretical
  backbone.

## Claim 4 — Separable convex-quadratic min-cost flow is strongly polynomial

Context and the matrix-free contingency route (if scale ever outgrows the
dense reduced QP).

- L. A. Vegh, "A Strongly Polynomial Algorithm for a Class of Minimum-Cost
  Flow Problems with Separable Convex Objectives," *SIAM Journal on
  Computing* **45**(5), 2016 (arXiv:1110.4882). Covers our objective class
  (convex quadratic arc/node costs).
  <https://epubs.siam.org/doi/10.1137/140978296>

## Claim 5 — One budget side-constraint: Lagrangian dual on a scalar

Grounds the other contingency (dualize the ton-mile row, 1-D search on the
multiplier) and the shadow-price interpretation of the budget multiplier.

- "Network flow problems with one side constraint: A comparison of three
  solution methods," *Computers & Operations Research* **15**(4), 1988.
  Compares primal partitioning, Lagrangian relaxation + subgradient, and
  warm-started LP; relevant conclusion: Lagrangian handling of the single
  side row is effective.
  <https://www.sciencedirect.com/science/article/abs/pii/0305054888900226>
- M. Holzhauser, S. O. Krumke, C. Thielen, "Budget-constrained minimum cost
  flows," *Journal of Combinatorial Optimization*, 2016. Modern treatment of
  exactly our budget-row structure.
  <https://www.researchgate.net/publication/277576254_Budget-constrained_minimum_cost_flows>

## Claim 6 — Rationing subproblem is a continuous quadratic knapsack problem

Phase-1 of the greedy planner (and our validation lower bound
$\theta_{\text{ration}}$) is a very simple example of the CONTINUOUS QUADRATIC
KNAPSACK PROBLEM — the quadratic case of the continuous, separable, convex
resource-allocation problem (minimize a weighted quadratic shortfall subject to
one budget constraint and box bounds). Its closed form
$R_i = D_i - \lambda D_i^2/P_i$ with the exclude-negatives-and-resolve loop is
the standard finite PEGGING (variable-fixing) algorithm. (This is NOT the
information-theory water-filling problem — that maximizes $\sum \log$ capacity
and has the additive $p_i=(\mu-n_i)_+$ form; only the "raise one multiplier
until the budget binds" skeleton is shared, and that skeleton is generic to
every singly-constrained separable convex allocation.)

- M. Patriksson, "A survey on the continuous nonlinear resource allocation
  problem", *European Journal of Operational Research* 185(1):1–46, 2008.
- M. Patriksson, C. Strömberg, "Algorithms for the continuous nonlinear
  resource allocation problem — new implementations and numerical studies",
  *EJOR* 243(3):703–722, 2015 (arXiv:1501.07035); the pegging algorithms.
- K. Kiwiel, "On linear-time algorithms for the continuous quadratic knapsack
  problem" / the EJOR 2013 quadratic-knapsack library.

## Not used (recorded decisions)

- OSQP (arXiv:1711.08013): capable, but an external dependency we do not
  need after the reduction (gate-0 decision).
- Dafermos 1980 / Nagurney: user-equilibrium VI literature — background
  only; this problem is system-optimal (per the handoff).

## Conclusion

No source found that suggests a better approach than the planned one, and
two strengthen it: Orden 1956 makes the reduction classical (not novel — good
for confidence, still worth a clean proof in our setting because of the
budget row and the (delivery) constraint), and He 1992 is a direct precedent
for solving convex QPs by projection-contraction on the KKT LCP. The plan
stands unchanged.

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
