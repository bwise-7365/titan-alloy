<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
# Fleet extension: multi-vehicle, multi-asset flow planning (task G1)

Formal statement of the fleet-planning generalization of the flow-planning
problem (`formulation.md`), its edge-case conventions, the structural lemmas
the fleet greedy planner relies on, and the recorded design decisions. Math is
written in LaTeX notation for direct reuse in the report. This model is for
CAPABILITY SCOPING — mapping what can plausibly move where within a planning
horizon; the assignment of actual vehicles to delivery schedules is a
different application's job.

Scope of the implementation this document governs: data model, validation,
random generation, feasibility checking, and the greedy planner ONLY. The
optimal-solver pipeline (`reduction` / `flowlcp` / `flowplan` / `flownewton` /
`oracle`) remains single-commodity and untouched. This also resolves the
"vehicle modeling fork" flagged in `../../2026-07-01-logistics-qp-handoff.md`:
vehicles carry MIXED assets under a joint weight + area capacity per link,
and every quantity (including vehicle counts) is continuous, so the model
stays a linearly constrained convex program rather than a packing problem.

## 1. Data

Nodes and geometry:

- Node set $V$, $|V| = M$.
- $d_{ij} > 0$: distance in miles of the directed link $i \to j$, for every
  ordered pair INCLUDING $i = j$; asymmetric allowed. One matrix shared by all
  vehicle types (types differ in speed, capacity, and count — not in the map).
  The diagonal $d_{ii}$ is small but positive: self-supply is cheap, not free,
  exactly as $c_{ii}$ in the base model.
- Generated instances (G7, user decision 2026-07-07): $d_{ij} =
  \max(\|p_i - p_j\|, 1) \cdot (1 + \varepsilon_{ij})$ with $\varepsilon_{ij}
  \sim U[0, 0.05]$ drawn independently PER DIRECTION — bare Euclidean
  separation of the placed coordinates, almost symmetric
  ($d_{ij}/d_{ji} \in [1/1.05, 1.05]$), with no handling floor and no
  per-mile scale (unlike the base model's cost matrix, which is NOT reused);
  $d_{ii} \sim U[1, 5]$. The 1-mile minimum separation guards the
  measure-zero coincident-placement case.

Asset types $a \in A$, $|A| \ge 1$ (the "goods" of the handoff document):

- $w_a \ge 0$: weight of one unit of asset $a$ (tons/unit).
- $s_a \ge 0$: cargo area of one unit of asset $a$ (sqft/unit).
- Requirement $w_a + s_a > 0$ (Finding G-F1 below).

Per-(node, asset) data — the scalar node data of the base model, one column
per asset:

- $C_{ia} \ge 0$: supply capacity of asset $a$ at node $i$ (units).
- $D_{ia} \ge 0$: demand for asset $a$ at node $i$ (units).
- $P_{ia} > 0$: priority of meeting demand $(i, a)$ (defined where
  $D_{ia} > 0$). Priorities live on the (node, asset) pair: a node may value
  one asset above another.

Vehicle types $k \in K$, $|K| \ge 1$:

- $T_k \ge 0$: weight capacity of one vehicle of type $k$ (tons/vehicle).
- $A_k \ge 0$: cargo-area capacity of one vehicle (sqft/vehicle).
  Requirement $T_k + A_k > 0$; pure carriers ($T_k = 0$ or $A_k = 0$) are
  legal (Finding G-F1).
- $N_k \ge 0$: number of vehicles available — REAL, not integer (fractional
  fleets express partial availability over the horizon).
- $v_k > 0$: speed (miles/hour).

Horizon:

- $H > 0$: planning horizon (hours).

Derived per-type budget (data, not calibrated — contrast the base model's
greedy-calibrated $L$):

$$B_k \;=\; N_k \, v_k \, H \qquad \text{(vehicle-miles of type } k\text{)}.$$

The ton-mile capacity of type $k$ is $T_k B_k$, recovering the user's
$(\text{tons/vehicle})(\#\text{vehicles})(\text{mph})(\text{hours})$ product;
the model works in vehicle-miles because area capacity rides on the same
vehicle flow.

Derived sets: per-asset sources $V_C^a = \{i : C_{ia} > 0\}$ and sinks
$V_D^a = \{i : D_{ia} > 0\}$.

## 2. Decision variables

All continuous and non-negative:

- $x^a_{ij} \ge 0$: units of asset $a$ shipped directly $i \to j$ (diagonal
  $x^a_{ii}$ allowed: self-supply, as $f_{ii}$ in the base model).
- $u^k_{ij} \ge 0$: vehicles of type $k$ traversing $i \to j$ over the
  horizon, LOADED OR EMPTY (fractional).
- $S_{ia} \in [0, C_{ia}]$: supply of asset $a$ injected at node $i$.
- $R_{ia} \ge 0$: resupply of asset $a$ delivered to node $i$.

Count: $(|A| + |K|) M^2 + 2 |A| M$.

## 3. Constraints

Per-asset flow structure — the base model's balance + delivery (F1 fix),
independently for each asset $a$:

$$\textstyle\sum_i x^a_{ij} + S_{ja} \;=\; R_{ja} + \sum_l x^a_{jl}
  \qquad \forall j, a \tag{asset balance}$$
$$R_{ja} \;\le\; \textstyle\sum_i x^a_{ij} \qquad \forall j, a \tag{delivery}$$

Link capacity — cargo on a link must fit, by weight and by area, in the
vehicles allocated to that link:

$$\textstyle\sum_a w_a \, x^a_{ij} \;\le\; \sum_k T_k \, u^k_{ij}
  \qquad \forall i, j \tag{link weight}$$
$$\textstyle\sum_a s_a \, x^a_{ij} \;\le\; \sum_k A_k \, u^k_{ij}
  \qquad \forall i, j \tag{link area}$$

Vehicle circulation — vehicles of each type conserve at every node; empty
repositioning (deadheading) is how a vehicle gets back:

$$\textstyle\sum_i u^k_{ij} \;=\; \sum_l u^k_{jl}
  \qquad \forall j, k \tag{circulation}$$

Fleet budget — total miles run by type $k$, loaded AND empty, within the
horizon's vehicle-miles:

$$\textstyle\sum_{ij} d_{ij} \, u^k_{ij} \;\le\; B_k
  \qquad \forall k \tag{budget}$$

Bounds: $0 \le S_{ia} \le C_{ia}$, $0 \le R_{ia} \le D_{ia}$ with
$R_{ia} = 0$ where $D_{ia} = 0$, $x^a_{ij} \ge 0$, $u^k_{ij} \ge 0$.

## 4. Objective and edge conventions

$$\min \;\; \theta(R) \;=\; \sum_{a} \sum_{i \in V_D^a}
  P_{ia} \left(\frac{D_{ia} - R_{ia}}{D_{ia}}\right)^{\!2}$$

- The sum runs over demand cells only; $R_{ia} = 0$ is fixed at
  $D_{ia} = 0$ cells (same convention, per asset, as the base model).
- $\theta$ is separable across (node, asset) cells and strictly convex in
  $R$ on the demand cells (diagonal Hessian $2 P_{ia} / D_{ia}^2 > 0$).
- All constraints are linear, so the full problem remains a linearly
  constrained convex QP — the handoff document's "clean QP" outcome survives
  the joint-capacity vehicle fork because vehicle counts are continuous.
- Feasible ($x = u = S = R = 0$ works) and compact (budget + $d_{ij} > 0$
  bound $u$; link weight/area + $w_a + s_a > 0$ bound $x$; delivery and
  balance then bound $R$ and $S$).

## 5. Findings and adopted decisions

### Finding G-F1: degenerate assets and vehicles

An asset with $w_a = s_a = 0$ occupies no vehicle at all: any quantity moves
with $u = 0$ and the link constraints never bind — and the greedy planner's
per-vehicle unit capacity (G-F5) divides by zero. REJECTED at validation.
A vehicle type with $T_k = A_k = 0$ can carry nothing and only pads the data:
also rejected. PURE carriers are legal — a type with $T_k > 0, A_k = 0$
(dense freight only) simply cannot carry any asset with $s_a > 0$; the
planner skips such (asset, type) pairings rather than dividing by zero.

### Finding G-F2: the diagonal is constrained too

Link weight/area and the budget apply to $i = j$ as well. Self-supply
$x^a_{ii}$ must ride on vehicles $u^k_{ii}$ paying $d_{ii}$ miles each — the
per-asset restatement of "cheap, not free" (F1 in `formulation.md`). A
diagonal traversal starts and ends at the same node, so (circulation) at
node $i$ is satisfied by $u^k_{ii}$ alone: it adds one to row $i$ and column
$i$ of $u^k$ simultaneously. No deadhead leg exists or is needed.

### Finding G-F3: deadheading is charged

(circulation) alone would allow "free" repositioning if the budget counted
only loaded miles. Adopted semantics (user decision, 2026-07-07): EMPTY miles
consume budget identically to loaded miles — $\sum_{ij} d_{ij} u^k_{ij}$
counts every traversal. An out-and-back service of a link $i \to n$ therefore
costs $d_{in} + d_{ni}$ vehicle-miles per vehicle, not $d_{in}$.

### Finding G-F4: budgets are data; calibration inverts into a scale hint

The base greedy CALIBRATES the ton-mile limit ($L \approx 0.8 \times$ its own
usage). Here the fleet — hence every $B_k$ — is given, so the analogous
advisory runs the other way: the fleet greedy reports
$$\text{fleetScaleHint} \;=\; \max_k \; \frac{\text{miles type } k
  \text{ would use with unlimited budgets}}{B_k},$$
the factor by which the fleet would have to grow for this heuristic to serve
every rationed target ($\le 1$ means the given fleet already sufficed).
Types with $B_k = 0$ are excluded from the unlimited pass (a type with no
vehicles does not exist operationally); a remainder no positive-budget type
can carry is structurally unservable and does not inflate the hint.

### Finding G-F5: conservative per-type loading

Define the unit capacity of vehicle type $k$ for asset $a$:
$$\kappa_{ak} \;=\; \min\!\left(\frac{T_k}{w_a}\Big|_{w_a > 0},\;
  \frac{A_k}{s_a}\Big|_{s_a > 0}\right), \qquad
  \kappa_{ak} = 0 \text{ if a needed capacity is } 0,$$
(each ratio present only when its denominator is positive; $w_a + s_a > 0$
guarantees at least one is). Allocating $u = q / \kappa_{ak}$ vehicles for
$q$ units of asset $a$ satisfies BOTH link constraints for that type's share:
$w_a q \le T_k u$ and $s_a q \le A_k u$ by construction. Summing over types
preserves both inequalities. This is CONSERVATIVE: pairing a weight-bound
asset with an area-bound asset in the same vehicles could carry more per
vehicle-mile. Accepted as heuristic slack — the optimizer (future work)
exploits mixed loading; the greedy does not.

### Finding G-F6: greedy service order now matters

In the base greedy every rationed target is ultimately served (capacity
suffices by construction after the scale-down), so service ORDER affected
only ton-miles used, not who got served. With finite budgets the loop can run
dry mid-plan, so order allocates scarcity. Adopted selection rule: serve the
(node, asset) cell with the largest remaining priority-weighted fractional
shortfall
$$\text{score}(n, a) \;=\; P_{na} \left(\frac{\text{remTarget}_{na}}
  {D_{na}}\right)^{\!2},$$
i.e. the objective decrease available from fully serving the cell.
Dimensionless, so assets measured in different units compare fairly, and it
generalizes "largest remaining target" (to which it reduces at equal
priorities and demands). Sources are chosen by cheapest ROUND TRIP
$d_{in} + d_{ni}$ ($d_{ii}$ on the diagonal), since the deadhead return is
charged (G-F3).

## 6. Lemma FL1 (per-asset rationing separability)

**Claim.** Phase-1 rationing — minimize $\theta(R)$ subject only to the
per-asset aggregate caps $\sum_i R_{ia} \le \min(\sum_i D_{ia},
\sum_i C_{ia})$ and $0 \le R_{ia} \le D_{ia}$ — decomposes exactly into $|A|$
independent single-asset continuous quadratic-knapsack (`scqkp`) problems, each
identical in form to the base model's `rationTargets`.

**Proof.** $\theta$ is a sum over assets of terms involving only column $a$
of $R$; each aggregate cap involves only column $a$; the box bounds are
per-cell. The feasible set is a product over $a$ and the objective is
separable over $a$, so the minimum is attained column-wise. Each column
problem is the base model's rationing problem verbatim (demand $D_{\cdot a}$,
priority $P_{\cdot a}$, meetable $\min(\sum D_{\cdot a}, \sum C_{\cdot a})$).
$\square$

Phase 1 ignores transport (link capacity, circulation, budgets) exactly as
the base phase 1 ignores the ton-mile budget: the targets are the
transport-unconstrained optimum, an infeasibility-tolerant upper aspiration,
and the per-column clamp-loop termination argument (active set strictly
shrinks; interior targets sum to the positive meetable) transfers unchanged.
Assets never compete for SUPPLY — $C$ and $D$ are per-asset — only for
transport, which phase 1 deliberately does not see.

## 7. Lemma FL2 (out-and-back circulation)

**Claim.** A plan built exclusively from moves of the form "add $\delta$
vehicles of type $k$ to BOTH $u^k_{in}$ and $u^k_{ni}$" (off-diagonal,
$i \ne n$) and "add $\delta$ to $u^k_{ii}$" (diagonal) satisfies
(circulation) exactly, for every prefix of moves.

**Proof.** (circulation) at node $j$ says row sum equals column sum of
$u^k$ at $j$. An off-diagonal move adds $\delta$ to row $i$ (via $u^k_{in}$)
and to column $i$ (via $u^k_{ni}$), and adds $\delta$ to row $n$ (via
$u^k_{ni}$) and to column $n$ (via $u^k_{in}$): both nodes' row/column sums
move together. A diagonal move adds $\delta$ to row $i$ and column $i$ at
once. Other nodes are untouched. Equality is preserved move by move, and the
constructed $u^k$ is exactly SYMMETRIC as a stored matrix. $\square$

The fleet greedy uses only these moves, so its plans have circulation
violation exactly $0$, not merely within tolerance — PROVIDED the checker
evaluates the residual as the sum of per-arc differences
$\sum_i (u_{ij} - u_{ji})$, each term exactly zero for symmetric $u$.
(Summing a whole row and a whole column separately is the same quantity
mathematically, but a vectorized contiguous-column reduction and a strided
row reduction associate differently and leave ulp-level residue even on
identical values; `checkFleetPlan` therefore uses the per-arc form.)

## 8. The fleet greedy planner

Phase 1: `rationFleetTargets` — Lemma FL1 column-wise quadratic-knapsack
rationing, reusing the base `scqkp` core.

Phase 2 (loop; targets scaled by $1 - 10^{-4}$ per asset so a source with
capacity always exists for a servable cell):

1. Pick the unserved, still-servable cell $(n, a)$ maximizing the G-F6 score;
   stop when none remain.
2. Pick the source $i$ with $C$-remaining capacity in asset $a$ minimizing
   the round trip $\rho = d_{in} + d_{ni}$ (or $d_{ii}$).
3. Transport availability: over types with remaining budget and
   $\kappa_{ak} > 0$, best $\kappa$ first (ties: lower type index, for
   determinism), each type $k$ can move
   $(\text{remBudget}_k / \rho)\,\kappa_{ak}$ units. If the total is zero,
   mark $(n, a)$ unservable — budget exhaustion is source-independent — and
   continue.
4. Ship $q = \min(\text{remTarget}_{na}, \text{remCap}_{ia},
   \text{transportMax})$, spilling across types best-$\kappa$-first;
   each type's vehicles are added out-and-back (Lemma FL2). Whichever
   resource binds is assigned EXACTLY zero (never decremented past it) — the
   base greedy's exactness discipline extended to budgets, preventing
   floating-point sliver iterations.

Termination: every pass zeroes a target cell ($\le M|A|$), zeroes a capacity
cell ($\le M|A|$), drains the last capable budget ($\le |K|$ drain events),
or marks a cell unservable ($\le M|A|$); hence at most $3 M |A| + |K|$
passes. The plan is feasible by construction: circulation exactly (FL2), link
weight/area per type share (G-F5), budgets by the availability bound,
per-asset balance/delivery by the same aggregation as the base greedy
(supplied = row sums, resupply = column sums, per asset).

The result reports per-type miles used, utilization $\in [0, 1]$, unserved
remainder per cell, the shortfall objective, and the G-F4 fleetScaleHint.

## 9. The fleet optimizer: reduced conservative QP (task G5)

The optimizer solves the fleet problem to certified optimality under the
SAME conservative conventions the greedy planner uses, so that the
rationing bound, the optimizer, and the greedy bound one model:
$\theta_{\text{ration}} \le \theta^\ast \le \theta_{\text{greedy}}$.

### Finding G-F7: conservative model, not the full link-coupled QP

The full model of section 3 couples assets per link through the aggregate
weight and area constraints (mixed loading: one vehicle may carry a
weight-bound asset and an area-bound asset together) and routes vehicles
as a circulation. Its variable count is $(|A| + |K|) M^2 + 2|A|M$ —
roughly 35{,}000 at the 70-node default — and its KKT system exceeds what
the library's dense engines can factor. DECISION (user, 2026-07-07): the
optimizer adopts the conservative conventions G-F5 (per-asset loading
$\kappa_{ak}$) and G-F3/FL2 (out-and-back deadheading, charged), under
which vehicles reduce to $|K|$ budget rows and the problem collapses to
per-asset shipments. The full link-coupled QP is recorded as future work
requiring sparse solver machinery (ledger G5i). Consequence: the
optimizer's $\theta^\ast$ is an upper bound on the full model's optimum
(the conservative feasible set is a subset), and it is the exact optimum
of the model the greedy heuristically attacks.

### Data and reduction (Lemma FL3)

Shortest routes are computed once on the shared distance matrix
(Floyd–Warshall, the base pipeline's `computeShortestRoutes`), giving
$\hat d_{ij}$ and, on the diagonal, the cheapest at-least-one-arc
self-route. **Lemma FL3 (per-asset shortest-route reduction).** Under the
conservative conventions, some optimal plan routes every unit of every
asset along a shortest route from its source to its sink: transport
consumption is $\rho_{pk} = \hat\rho_p / \kappa_{ak}$ vehicle-miles per
unit, where $\hat\rho_p = \hat d_{st} + \hat d_{ts}$ is the round-trip
reduced distance of pair $p = (s, t)$ for asset $a$, and re-routing any
unit off a shortest route weakly increases every budget row it touches
while changing nothing else. *Proof sketch:* mirror of the base model's
R1 with cost replaced by round-trip distance; the budget rows are the
only transport constraints, and they are monotone in route length.
$\square$

Per asset $a$: sources $V_C^a$, sinks $V_D^a$, kept pairs (optionally
screened per asset by the base k-cheapest / gap rules); per pair and
CAPABLE type ($\kappa_{ak} > 0$) one variable
$y_{pk} \ge 0$ (units of asset $a(p)$ on pair $p$ carried by type $k$).
Incapable combinations get no variable. Delivered per demand cell:
$R_{ta} = \sum_{p \to (t,a)} \sum_k y_{pk}$.

### The reduced QP

$$\min \;\; \sum_{(t,a)} P_{ta}\left(\frac{D_{ta} - R_{ta}}{D_{ta}}\right)^2
  \;+\; \varepsilon \sum_{p,k} \rho_{pk}\, y_{pk}$$

subject to, with multipliers in parentheses:

$$\textstyle\sum_{p \text{ from } (s,a)} \sum_k y_{pk} \;\le\; C_{sa}
  \qquad (\mu_{sa} \ge 0, \text{ one per supply cell}) $$
$$\textstyle\sum_{p} \rho_{pk}\, y_{pk} \;\le\; B_k
  \qquad (\lambda_k \ge 0, \; k = 1..|K| \text{ — Finding G-F8: } |K|
  \text{ budget rows generalize the base model's scalar}) $$
$$y \ge 0.$$

No explicit $R \le D$ row: at $R_{ta} = D_{ta}$ the objective gradient in
any $y$ feeding that cell is $+\varepsilon \rho_{pk} > 0$, so overshoot
strictly worsens the objective and never occurs at an optimum (the base
model's C4 argument, per cell). The tie-break
$\varepsilon = 10^{-8} \cdot (\sum_{(t,a)} P_{ta}) / (\sum_k B_k)$
(the R4 analog, nondimensionally consistent: gain rows scale like $P$,
price rows like $\rho$) selects among degenerate optima the plan with
fewer vehicle-miles and resolves type ties toward efficient types.

### KKT as a monotone pure NCP

$z = [\,y \mid \mu \mid \lambda\,] \ge 0$, $0 \le Mz + q \perp z \ge 0$,
with $Q_{ta} = 2 P_{ta} / D_{ta}^2$:

- $y$-rows: $G_y = Q_{ta}(R_{ta} - D_{ta}) + \mu_{sa}
  + (\lambda_k + \varepsilon)\rho_{pk}$, i.e.\ $M$ carries a rank-one
  block $Q_{ta} \mathbf{1}\mathbf{1}^T$ over each demand CELL's variables
  (all its (source, type) combinations, stored contiguously —
  "cell-major", the analog of the base LCP's sink-major layout), a $+1$
  into the pair's $\mu_{sa}$ column, and $+\rho_{pk}$ into its
  $\lambda_k$ column; $q_y = -Q_{ta} D_{ta} + \varepsilon \rho_{pk}
  = -2P_{ta}/D_{ta} + \varepsilon\rho_{pk}$.
- $\mu$-rows: $G_\mu = C_{sa} - \sum y$ (skew $-1$ entries);
  $q_\mu = C_{sa}$.
- $\lambda$-rows: $G_\lambda = B_k - \sum_p \rho_{pk} y_{pk}$ (skew
  $-\rho$ entries); $q_\lambda = B_k$.

The symmetric part of $M$ is block-diagonal — one PSD rank-one block per
demand cell, zeros elsewhere; the $\mu$ and $\lambda$ borders are skew
and cancel — so $M$ is positive semidefinite plus skew: MONOTONE, and
every VINCP engine applies. Dimension: numPairVariables + numSupplyCells
+ $|K|$. The derivation is machine-verified in
`doc/fleet-mcp-check.mac` (task G5b), which the C++ assembly cites by
check number.

### Lemma FL4 (path out-and-back circulation) and unpack

**Claim.** Allocating, for each pair $p$ and type $k$, $y_{pk}/\kappa_{ak}$
vehicles along every arc of the outbound shortest route AND every arc of
the reverse shortest route yields $u^k$ satisfying (circulation) exactly,
the link weight/area constraints per type share, and total per-type miles
$\sum_p \rho_{pk} y_{pk} \le B_k$. *Proof:* each vehicle enters and
leaves every interior node of its route once (pass-through), and leaves
and re-enters its source endpoint once; sums of such loops balance at
every node exactly (FL2 extended to multi-arc loops). Link capacity per
type share follows from G-F5; the mileage identity is the definition of
$\hat\rho_p$. $\square$

The unpacker walks each pair's outbound route accumulating
$x^a$, and both routes accumulating $u^k$, then sets $S$/$R$ as row/column
aggregates; `checkFleetPlan` must report zero (circulation exactly, the
rest to accumulation tolerance).

### Finding G-F9: purification cap for optimal fleet plans

Consolidating pivots on an optimal fleet plan must not spend new budget:
across $|K|$ coupled budget rows, a spending pivot changes the vehicle
reallocation nonlocally. DECISION (user, 2026-07-07): purification runs
per asset with the ton-mile cap set to that asset's round-trip unit-miles
AT ENTRY. Slack starts at zero, so only pivots with non-negative
round-trip saving are accepted initially; savings may be re-spent, but no
asset's usage ever exceeds its entry level, so the final vehicle rebuild
is feasible within the original budgets (the proportional-reuse existence
argument of `fleetswap.hpp`).

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
