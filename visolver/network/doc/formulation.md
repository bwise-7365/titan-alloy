<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
# Formulation of the flow-planning problem (task A1)

Formal statement of the problem in `../../flow-planning-problem.txt`, edge-case
conventions, structural lemmas (proof sketches here; full proofs in A3 / report
Part III), and the audit of the greedy-planner specification. Math is written
in LaTeX notation for direct reuse in the report.

## 1. Data

- Node set $V$, $|V| = M$ (typically $20 \le M \le 200$).
- $C_i \ge 0$: supply capacity at node $i$ (tons).
- $D_i \ge 0$: demand at node $i$ (tons).
- $P_i > 0$: priority of meeting demand at $i$ (defined where $D_i > 0$).
- $c_{ij} > 0$: per-unit cost (miles) of moving stuff $i \to j$, for every
  ordered pair including $i = j$; asymmetric ($c_{ij} \ne c_{ji}$, differing
  1-10%); $c_{ii}$ small (1-5), off-diagonal typically 100-2000.
- $L > 0$: ton-mile budget.

Derived sets: sources $V_C = \{i : C_i > 0\}$, sinks $V_D = \{i : D_i > 0\}$.

## 2. Decision variables

- $S_i \in [0, C_i]$: supply node $i$ injects into the network.
- $R_i \ge 0$: resupply delivered to node $i$.
- $f_{ij} \ge 0$: flow shipped directly $i \to j$ (diagonal $f_{ii}$ allowed:
  self-supply).

Count: $2M + M^2$ as stated in the spec (5,040 at $M = 70$). Conventions in
section 4 fix many of these to zero a priori.

## 3. Constraints

As literally specified:

$$\textstyle\sum_i f_{ij} + S_j \;=\; R_j + \sum_k f_{jk} \qquad \forall j \in V \tag{balance}$$
$$\textstyle\sum_{ij} c_{ij} f_{ij} \;\le\; L \tag{budget}$$
$$0 \le S_i \le C_i, \qquad R_i \ge 0, \qquad f_{ij} \ge 0.$$

### Finding F1 (spec inconsistency): the balance as written makes self-supply FREE

In (balance), $f_{jj}$ appears once on each side and cancels. Worse, a node
with $C_j > 0, D_j > 0$ can set $S_j = R_j > 0$ with **no flow at all**: the
balance holds, no ton-miles are consumed, and $c_{jj}$ never binds. This
contradicts the stated intent ("very cheap, but **not free**... $c_{ii}$ can
be small... but not zero").

The greedy planner itself reveals the intended semantics: it serves
self-supply as an arc flow $f_{nn}$ at cost $c_{nn}$ (step (2) selects
$m = n$ when $c_{nn}$ is cheapest). So supply must **leave via arcs** and
resupply must **arrive via arcs**.

**Adopted fix (one extra inequality per node).** Model each node as split
into a supply port and a delivery port with a nonnegative through-flow
$T_j \ge 0$ (transshipment). Eliminating $T_j$ gives, equivalently, the
balance above **plus**

$$R_j \;\le\; \textstyle\sum_i f_{ij} \qquad \forall j \tag{delivery}$$

(equivalently $S_j \le \sum_k f_{jk}$; given (balance) each implies the
other). Under (delivery), self-supply is forced through $f_{jj}$ at cost
$c_{jj}$, transshipment remains legal ($T_j = \sum_i f_{ij} - R_j \ge 0$),
and the model matches both the prose and the greedy algorithm.

## 4. Objective and edge conventions

$$\min \;\; \theta(R) \;=\; \sum_{i \in V_D} P_i \left(\frac{D_i - R_i}{D_i}\right)^{\!2}$$

- **$D_i = 0$ nodes:** the spec's summand is undefined there ($0/0$); the sum
  runs over $V_D$ only, and we fix $R_i = 0$ for $i \notin V_D$ (delivering
  to a no-demand node never helps and only creates degenerate optima).
- **$C_i = 0$ nodes:** $S_i = 0$ is forced by $S_i \le C_i$.
- $\theta$ is a function of $R$ alone; it is **strictly convex in
  $R_{V_D}$** with diagonal Hessian $\partial^2\theta/\partial R_i^2 =
  2P_i/D_i^2 > 0$. $f$ and $S$ enter only through linear constraints.

**Classification.** Linearly constrained, convex (in all variables; strictly
convex in $R$) quadratic program. Feasible ($S = R = f = 0$ works), and the
feasible set is compact (budget + $c_{ij} > 0$ bound $f$; (delivery) and
(balance) then bound $R$; $S \le C$).

## 5. Structural lemmas (proofs in A3 / Part III)

- **L1 (existence).** A minimizer exists: continuous objective, nonempty
  compact feasible set.
- **L2 (unique $R^*$).** The optimal resupply vector is unique: $\theta$ is
  strictly convex in $R_{V_D}$ and the feasible region projects to a convex
  set in $R$-space. The optimal $(S^*, f^*)$ need **not** be unique
  (degenerate routings); if a determinate plan is wanted, add a secondary
  tie-break (e.g. minimize total ton-miles among optimal plans).
- **L3 (no overshoot).** Every optimum has $R_i \le D_i$ on $V_D$: if
  $R_i > D_i$, scaling down some delivering flow strictly decreases
  $\theta$ and relaxes (budget) — so overshoot is never optimal, despite the
  quadratic penalizing it symmetrically.
- **L4 (monotone value).** The optimal value is nonincreasing in $L$ and
  bounded below by the budget-unconstrained rationing optimum (section 7).

## 6. Greedy-planner audit

The algorithm is correct as described, with the following notes (numbered for
reference):

- **G1 (mis-type, harmless).** "If $MR < \sum_i C_i$, then we really can meet
  all the demand": the condition is equivalent to $\sum D_i < \sum C_i$
  (since $MR$ is the min of the two sums), which does imply meetable demand —
  but the boundary case $\sum D_i = \sum C_i$ (demand exactly meetable) fails
  the test and falls to the rationing branch. Harmless: rationing then yields
  $\lambda = 0$, $R_i = D_i$. Cleaner statement: "if $\sum D_i \le \sum C_i$."
- **G2 (rationing formula verified; clamp is load-bearing).** KKT of
  $\min \sum P_i((D_i - R_i)/D_i)^2$ s.t. $\sum R_i \le MR$ gives
  $R_i = D_i - \tfrac{\mu}{2} D_i^2 / P_i$; the spec's $\lambda$ absorbs the
  factor 2 — **correct**. The interior formula requires $R_i \ge 0$; when
  some $R_i$ clamps to 0, $\lambda$ must be re-solved over the remaining
  nodes (the quadratic-knapsack pegging step). With the stated ranges the weights
  $w_i = D_i^2/P_i$ span a factor of up to 250, so clamping is a **live
  case**, not a corner case — worth confirming the Java implementation
  iterates the clamp.
- **G3 (termination verified).** The 0.01% scale-down ($RD \leftarrow 0.9999
  \, RD$) guarantees $\sum RD < \sum RC$, so step (2) always finds a source
  with $RC_m > 0$. Each iteration sets $RD_n$ or $RC_m$ to exact zero (the
  subtraction of $q = \min$ is exact in floating point), no $(n, m)$ pair
  repeats, so the loop runs at most $|V_D| + |V_C|$ iterations.
- **G4 (observation).** Phase 2 ignores $P_i$. Harmless for delivered
  amounts (all rationed demand is fully served since $\sum RD < \sum RC$),
  but the resulting ton-miles — and hence $L$ — do not reflect priorities.
- **G5 (swap move verified).** Per-unit saving of the exchange is
  $(c_{ij} + c_{mn}) - (c_{in} + c_{mj})$; the move preserves every node's
  balance, and since cost is linear in $x$ the maximal step
  $x = \min(f_{ij}, f_{mn})$ is optimal for an improving swap. This is the
  classic transportation-problem 2-exchange. Complexity counts
  ($O(N^4)$ brute / $O(N^3)$ busiest-node) are right.
- **G6 (important: direction of the greedy comparison).** The greedy plan
  delivers the **full** rationed targets, i.e. it (nearly) achieves the
  budget-unconstrained optimum of $\theta$ — while spending 100% of its
  ton-miles, of which $L$ is only 80%. So under the actual budget the greedy
  plan is **infeasible**, and its objective is a **lower** bound, not an
  upper bound. "Optimal never worse than greedy" is a valid check only at
  $L = L_{\text{greedy}}$ (where the greedy plan is feasible). See section 7
  for the checks that are valid at 80%.

## 7. Validation bounds (the sandwich)

Let $\theta_{\text{ration}}$ = optimal value of the Phase-1 rationing problem
(budget-unconstrained lower bound), $\theta_{\text{greedy}}$ = greedy plan's
objective ($\approx \theta_{\text{ration}}$, exactly: computed with the
0.9999-scaled targets), $L_g$ = greedy plan's ton-miles, and
$\theta^*(L)$ = optimal value at budget $L$. Then every solver run can be
checked mechanically against:

1. $\theta^*(L) \ge \theta_{\text{ration}}$ for every $L$;
2. $\theta^*(L)$ is nonincreasing in $L$ (L4);
3. $\theta^*(L_g) \le \theta_{\text{greedy}}$ (greedy is feasible there);
4. $\theta^*(0.8\,L_g) \ge \theta_{\text{greedy}}$ up to the 0.9999 scaling
   slack (tighter budget cannot beat the unconstrained optimum);
5. KKT residual of the returned $z$ below tolerance (squared-norm
   convention, per visolver).

## 8. Rulings (confirmed at gate 1, 2026-07-03)

1. **F1 fix adopted**: (delivery) $R_j \le \sum_i f_{ij}$ is part of the
   official model. Self-supply routes through $f_{jj}$ at cost $c_{jj}$.
2. **$R_i = 0$ at $D_i = 0$ nodes** adopted; objective sums over $V_D$ only.
3. **G6 adopted**: the greedy comparison runs at $L = L_g$; at $0.8\,L_g$ the
   section-7 sandwich is the check.
4. **Tie-break adopted**: the user has observed the flow degeneracy in
   practice; a small min-ton-mile tie-breaker will be included (exact form —
   tiny linear ton-mile term $\varepsilon \sum c_{ij} f_{ij}$ vs. two-phase
   lexicographic solve — decided in A3, with a perturbation bound on the
   objective).
5. **G2 confirmed by the user**: the Java rationing does assume all $R_i$
   interior, solves $\lambda$, and must **exclude negative $R_i$ and
   re-solve** (possibly several rounds, rarely more than one in practice).
   This iterative clamp is part of the official Phase-1 algorithm and will be
   implemented in B2.

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
