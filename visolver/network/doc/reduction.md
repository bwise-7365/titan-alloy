<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
# Reduction and pruning, with proofs (task A3)

Builds on `formulation.md` (definitions, F1 fix, conventions). Establishes:
the shortest-path reduction (Lemma R1), exact arc pruning (Proposition R2),
certified source-sink pair pruning (Proposition R3), the tie-break form and
its perturbation bound (Proposition R4), and complexity accounting. These
statements and proofs go to report Part III essentially verbatim.

## 1. Movement distances

Let $d^0_{mn}$ be the ordinary all-pairs shortest-path distance on the
complete digraph with weights $c_{ij} > 0$ (zero diagonal, Floyd-Warshall).
Because supply must leave and resupply must arrive **via arcs** (the
(delivery) constraint), the relevant cost of moving one ton from supply at
$m$ to delivery at $n$ uses at least one arc:

$$\hat d_{mn} = \begin{cases} d^0_{mn}, & m \ne n,\\[2pt]
\min\bigl(c_{nn},\; \min_{k \ne n} (c_{nk} + d^0_{kn})\bigr), & m = n. \end{cases}$$

(The diagonal correction: a self-supplied ton either uses the self-arc
$c_{nn}$ or a round trip; typically $\hat d_{nn} = c_{nn}$.)

## 2. The reduced problem

Variables $t_{mn} \ge 0$ for $(m, n) \in V_C \times V_D$ (tons shipped from
source $m$ to sink $n$, routed along a shortest walk):

$$(P_\varepsilon)\qquad \min_{t \ge 0}\;\; \sum_{n \in V_D} P_n
\Bigl(\tfrac{D_n - R_n(t)}{D_n}\Bigr)^{\!2} \;+\; \varepsilon \sum_{m,n} \hat d_{mn} t_{mn}$$
$$\text{s.t.}\quad R_n(t) = \sum_m t_{mn}; \qquad \sum_n t_{mn} \le C_m \;\; (m \in V_C);
\qquad \sum_{m,n} \hat d_{mn} t_{mn} \le L.$$

Sizes: $|V_C|\,|V_D|$ variables (the 70-node profile: $40 \times 50 = 2000$,
versus $5040$), $|V_C| + 1$ inequality rows, diagonal objective Hessian in
$R$.

### Lemma R1 (exact reduction)

*The optimal value of $(P_0)$ equals the optimal value of the full problem
of `formulation.md`, and any feasible $t$ expands to a feasible full plan
with the same $R$ and no greater budget usage.*

**Proof.** (reduce) Split each node $j$ into a supply port $u_j^{\text{out}}$
and a delivery port $u_j^{\text{in}}$; arcs
$u_i^{\text{out}} \to u_j^{\text{in}}$ with flow $f_{ij}$ and cost $c_{ij}$;
free transship arcs $u_j^{\text{in}} \to u_j^{\text{out}}$ with flow
$T_j = \sum_i f_{ij} - R_j$, which is $\ge 0$ exactly by (delivery).
The two port balances hold by (balance) and the definition of $T_j$, and
summing (balance) over $j$ gives $\sum_j S_j = \sum_j R_j$. So $(f, T)$ is a
conserved flow with supplies $S$ and demands $R$, and by the flow
decomposition theorem [Ahuja-Magnanti-Orlin, Thm 3.5] it decomposes into
path flows $u_m^{\text{out}} \leadsto u_n^{\text{in}}$ (with $S_m > 0$, so
$m \in V_C$; and $R_n > 0$, so $n \in V_D$ by the $R_i = 0$ convention) plus
cycle flows. Every cycle contains at least one original arc (transship arcs
alone form no cycle), so cycles have strictly positive cost; every
$m \leadsto n$ path uses $\ge 1$ original arc and costs $\ge \hat d_{mn}$.
Set $t_{mn}$ = total path flow $m \leadsto n$. Then
$R_n = \sum_m t_{mn}$, $\sum_n t_{mn} \le S_m \le C_m$, and
$\sum \hat d_{mn} t_{mn} \le \sum_{ij} c_{ij} f_{ij} \le L$: $t$ is feasible
for $(P_0)$ with the same objective.
(expand) Conversely, route each $t_{mn} > 0$ along a fixed shortest walk
realizing $\hat d_{mn}$; accumulate arc flows $f$, set
$S_m = \sum_n t_{mn}$, $R_n = \sum_m t_{mn}$. Port balances hold by
construction, (delivery) holds since every delivered ton arrives on an arc,
and the budget usage is exactly $\sum \hat d_{mn} t_{mn} \le L$. $\square$

The expansion in (expand) is the **unpacker** (task C2): follow the
Floyd-Warshall predecessor matrix; the diagonal correction stores its own
one- or two-leg route.

## 3. Arc pruning (exact)

### Proposition R2

*Every arc $(i, j)$ with $c_{ij} > d^0_{ij}$ (a "dominated" arc) is unused
by the canonical expansion, and any optimal plan using such an arc can be
strictly improved in budget usage with $R$ unchanged. Keeping only arcs with
$c_{ij} = d^0_{ij}$ (and the diagonal routes of section 1) loses nothing.*

**Proof.** Subpath optimality: an arc lying on a cost-minimal walk must
itself be a cost-minimal route between its endpoints; if
$c_{ij} > d^0_{ij}$, substituting the shorter $i \leadsto j$ path for the
arc strictly reduces the walk's cost, so no shortest walk uses the arc, and
a plan shipping on it wastes $(c_{ij} - d^0_{ij})$ ton-miles per ton, freed
by rerouting (feasibility is preserved: only arc flows change). $\square$

This answers "which links are provably not part of any plan even remotely
near optimal" at the arc level. With near-metric costs many arcs survive R2;
the deeper cut is R1 (variables collapse from $M^2 + 2M$ to
$|V_C||V_D|$) and R3 below.

## 4. Source-sink pair pruning (certified)

Screen, then certify. For each sink $n$, keep its $k$ cheapest sources
$K(n) \subset V_C$ by $\hat d_{mn}$ (default $k = 10$); solve $(P_\varepsilon)$
restricted to kept pairs; then check optimality of the full problem a
posteriori.

### Proposition R3 (exactness certificate)

*Let $t^\star$ (with multipliers $\lambda^\star \ge 0$ on the budget and
$\mu_m^\star \ge 0$ on the capacities) solve the restricted problem. If for
every excluded pair $(m, n)$*

$$\frac{2 P_n}{D_n}\cdot\frac{D_n - R_n^\star}{D_n} \;\le\; (\lambda^\star + \varepsilon)\, \hat d_{mn} + \mu_m^\star ,$$

*then $t^\star$ (padded with zeros) is optimal for the unrestricted
$(P_\varepsilon)$. Any violated pair is added to $K(n)$ and the problem
re-solved; this terminates in finitely many rounds at the exact optimum.*

**Proof sketch.** The displayed inequality is exactly dual feasibility
(nonnegative reduced cost) of the excluded column at the restricted
solution: the left side is $-\partial \theta / \partial R_n$ at $R^\star$,
the marginal objective gain of one more delivered ton at $n$; the right side
is its marginal price through the budget, tie-break, and capacity rows. KKT
of a convex QP is sufficient, so satisfying it for all columns certifies
global optimality. Each failed round permanently adds a column; columns are
finite. (This is column generation with a convex master; standard.) $\square$

**A priori screen.** From the same stationarity relation, any pair carrying
flow at an optimum satisfies
$\hat d_{mn} \le \dfrac{2 P_n}{\lambda^\star D_n}$ (when the budget binds,
$\lambda^\star > 0$) — expensive pairs are priced out by the budget's shadow
price. A cheap capacity-relaxed 1-D dual pre-pass gives an estimate
$\hat\lambda$ to size $k$; correctness never depends on the estimate, only
speed does (R3 certifies).

## 5. Tie-break: form and perturbation bound

Gate-1 ruling: a small tie-breaker is included. **Adopted form:** the linear
term $\varepsilon \sum \hat d_{mn} t_{mn}$ in $(P_\varepsilon)$ — one solve,
no second phase, and it selects minimal-ton-mile plans among near-optimal
ones (the user-observed flow degeneracy is exactly ties in this linear
direction).

### Proposition R4 (perturbation bound)

*Let $\theta^\star$ be the optimal shortfall of $(P_0)$ and $t^\varepsilon$
solve $(P_\varepsilon)$. Then*
$$\theta(t^\varepsilon) \;\le\; \theta^\star + \varepsilon L .$$

**Proof.** For an optimal $t^0$ of $(P_0)$:
$\theta(t^\varepsilon) \le \theta(t^\varepsilon) + \varepsilon \sum \hat d\, t^\varepsilon
\le \theta(t^0) + \varepsilon \sum \hat d\, t^0 \le \theta^\star + \varepsilon L$,
using optimality of $t^\varepsilon$ for $(P_\varepsilon)$ and the budget row.
$\square$

**Choice of $\varepsilon$.** Objective scale: $\theta \le \sum_n P_n$
(attained at $R = 0$). Taking a shortfall tolerance
$\delta_\theta = 10^{-8} \sum_n P_n$ gives
$\varepsilon = \delta_\theta / L$ — the tie-break costs at most $10^{-8}$ of
full-scale shortfall. Named constant in code; revisit only if `bsHe94b`
tolerances are tightened past it.

## 6. Complexity and sizing

| Step | Cost | 70 nodes (40x50) | 200 nodes (100x100, worst) |
|------|------|------------------|-----------------------------|
| Floyd-Warshall + predecessors | $O(M^3)$ time, $O(M^2)$ mem | ~3.4e5 relax steps | ~8e6 relax steps |
| Diagonal correction | $O(M^2)$ | trivial | trivial |
| Reduced variables $t$ | $|V_C||V_D|$ | 2,000 | 10,000 |
| LCP dimension (t + caps + budget) | — | 2,041 | 10,101 |
| Dense $(M{+}I)$ memory | $8 n^2$ B | 33 MB | 816 MB |
| Dense factorization | $O(n^3)$ | ~3e9 flops (sub-second) | ~3.4e11 flops (minutes) |
| With R3 pruning, $k{=}10$ | — | (not needed) | dim ~1,101; 10 MB; trivial |
| Unpacker (path expansion) | $O(\text{pairs} \cdot M)$ | trivial | trivial |

Conclusions: at the 70-node profile the reduced problem fits visolver's
dense `bsHe94b` **without** pair pruning (R3 still used as a speed switch);
at the 200-node extreme R3 is what keeps it dense-feasible — the certificate
loop makes that exact, not approximate. The original full formulation
(5,040-25,4M-entry Jacobians) is never formed.

## 7. Handoff to C1/C2

- C1 implements: Floyd-Warshall with predecessor matrix, diagonal
  correction, R2 arc filter, $K(n)$ screen, and instance-level stats
  (surviving arcs, kept pairs).
- C2 implements: KKT of $(P_\varepsilon)$ as a **pure NCP** ($R$ substituted
  out; unknowns $t$, $\mu$, $\lambda$; all complementarity, no free block —
  `projectNonnegative` suffices), plus the R3 certificate check and the
  unpacker. Packing layout documented there.

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
