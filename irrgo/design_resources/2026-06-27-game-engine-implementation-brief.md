# Implementation Brief: Simultaneous-Move Stochastic-Game Engine

**Purpose.** This document is a self-contained handoff specification. It records the
design that emerged from a prior discussion so that a fresh implementer (human or
another Claude instance, with no access to that conversation) can build the engine.
It states the game model, the settled design decisions and their rationale, the
algorithm, the architecture, the language-specific targets, and the deferred
questions. It deliberately does **not** specify the concrete game rules (the
transition dynamics): those are the author's design and will be supplied as a
plug-in generative model. Do not invent or substitute game mechanics.

---

## 1. Scope and deliverables

Two implementations of the *same* engine concept:

1. **Java 17** --- one **specific game** (rules supplied by the author), with a
   Swing GUI. Build with Ant/NetBeans or IntelliJ. Must run unmodified on Windows
   and Debian Linux.
2. **C++ 20** --- a **reusable library** that hosts several example games, with a
   Qt 6 GUI for demos. Build with CMake. Must run unmodified on Windows
   (Visual Studio) and Debian Linux (KDevelop).

Both compute, for a given game state, the **top-level mixed strategies for both
players** from a single centralized engine.

---

## 2. The game model

### 2.1 Class of game

A two-player **zero-sum stochastic (Markov) game with simultaneous moves, a known
chance kernel, and otherwise perfect information**. Formally it is the result of
the following transformation applied to a perfect-information game: both players
submit moves simultaneously; a known chance device adjudicates the joint
submission; the submitted moves **and** the updated state are then revealed before
the next turn.

Key structural facts the implementer must rely on:

- **Perfect monitoring (reveal version).** Because submitted moves and the new
  state are revealed each turn, the entire history is common knowledge. There are
  **no nontrivial information sets**. Do **not** build counterfactual-regret /
  information-set machinery; per-state matrix-game backups suffice.
- **Simultaneity forces mixed strategies.** Even with full information about the
  past, the concurrent move is unknown at decision time, so optimal play is in
  general a *mixed* strategy at each state. A deterministic best response is
  exploitable. Every backup must solve a matrix game, never take a plain max.
- **Known generative model.** The chance device's distribution is known and
  sampleable. This is a *planning* problem, not a learning problem: the engine
  may call a simulator `sample(state, moveA, moveB, rng)` as many times as the
  time budget allows.

### 2.2 Moves

- Moves are **plain labels**, not parameterized actions (no `placePiece(r,c)`;
  just `M01..M12`).
- The move set is **fixed** (the same set is available in every state).
- Count is small: **5–20** moves per player.
- **v1:** both players share an identical move set.
- **Later:** the two players' move sets may differ, but will always remain plain
  labels of similar small cardinality. Design the engine for `moveCountA` and
  `moveCountB` separately even though v1 sets them equal.
- **Legality of a joint move** is handled *inside the transition* (e.g., a move
  that the chance device cannot legally apply becomes a pass), **not** by pruning
  the action set. The engine always forms the full `moveCountA x moveCountB`
  joint-action matrix.

### 2.3 State

- Large and complex: on the order of **1500 board locations** and **~100 pieces**;
  state updates are **expensive**.
- Transitions are **stochastic**.
- The state space is astronomically large (loosely, ~1500! reachable
  configurations) and **positions effectively never recur**. Consequence:
  **transposition tables and memoization are useless** and must not be relied on.
  The binding resource is the **total number of transition evaluations**, not the
  number of distinct states.

### 2.4 Outcomes and utility

- The **basic outcome** is a scalar `x` (zero-sum, from A's perspective): A
  receives `x`, B receives `-x`.
- **v1:** risk-neutral zero-sum. A's utility is `x`, B's is `-x`. The per-node
  matrix game is zero-sum and is solved by minimax (one LP yields both sides'
  strategies and the unique value).
- **Later:** each side applies a **concave CARA utility** `u(x) = 1 - exp(-a*x)`
  (constant absolute risk aversion; coefficient `a`). A's payoff is `u(x)`, B's is
  `u(-x)`. This makes the game **general-sum in utility** (the payoffs no longer
  sum to a constant), which changes the per-node solve and the equilibrium-
  selection requirements (see Section 8). Keep the utility behind an interface
  from the start so v1 and the CARA version share all other code.

---

## 3. Settled design decisions (decision log)

Each entry is a decision already made; do not re-litigate without reason.

1. **Algorithm family: sparse-sampling online planning with matrix-game (Nash)
   backups.** This is the published method of Kearns, Mansour, and Singh, *Fast
   Planning in Stochastic Games* (building on Kearns, Mansour, Ng sparse
   sampling). It is the correct family precisely because the state space is huge,
   positions never recur, and only a generative model is available: its per-state
   running time has **no dependence on state-space size** and depends
   exponentially on the horizon.
2. **No transposition table / no memoization.** Justified by 2.3.
3. **Perfect monitoring; no information-set / CFR machinery.** Justified by 2.1.
4. **Centralized engine computes both players' strategies.** Matches the KMS
   "common copy" model (the result is a centralized near-Nash / correlated
   solution). For zero-sum v1 both strategies come from the single minimax solve.
5. **Exploitability is acceptable for the demo.** Strong head-to-head play with a
   simple selection rule is fine for v1; a low-exploitability path is recorded as
   an upgrade (Section 5), not required now.
6. **Finite horizon (fixed depth).** Keep the search finite-horizon (e.g., depth
   3, configurable). Do **not** switch to infinite-horizon discounted value
   iteration for the general-sum/CARA version: that is known not to converge in
   the general-sum case (KMS counterexample). Finite horizon always terminates.
7. **Apply utility at the leaves, average in utility space.** Because risk
   aversion lives only in the *distribution* of outcomes, the engine must compute
   `mean_i u(x_i)` over sampled leaf outcomes, never `u(mean_i x_i)`. (Recall
   `E[u(X)] != u(E[X])`.) This is a correctness requirement for the CARA version
   and must be respected by the leaf/backup code from the outset.

---

## 4. The core algorithm

### 4.1 Author's baseline scheme (v1 target)

At the current state, recursively to a fixed depth `D`:

1. Form the `mA x mB` joint-action matrix (`mA = moveCountA`, etc.).
2. For each joint action `(a, b)`, draw `N` sampled successors via the generative
   model. Recurse on each successor to depth `D`.
3. At a leaf (terminal state, or depth cutoff), obtain the basic outcome `x`
   (terminal value, or the static evaluator), then map it through the utility to
   `(uA, uB)`.
4. Back up: at each internal node, the matrix entry `(a,b)` is the **average over
   its N sampled children** of the child node values `(vA, vB)`; the node's value
   and the node's mixed strategies are the solution of the resulting matrix game
   (Section 6).
5. **Anytime loop:** repeat to accumulate more samples until the time budget is
   spent (see the caveat in 4.3).
6. Return the **root** mixed strategies `(sigmaA, sigmaB)`. The game loop / umpire
   samples one actual move per side from these.

### 4.2 Language-neutral pseudocode

```
function PLAN(state):
    repeat until time budget exhausted:
        accumulate ESTIMATE(state, depth = D) into root statistics
    (payoffA, payoffB) := root averaged matrices
    return SELECTOR.solve(payoffA, payoffB)        # (sigmaA, sigmaB, vA, vB)

function ESTIMATE(state, depth):
    if isTerminal(state):
        x := terminalOutcome(state)
        return (utilityA(x), utilityB(x))
    if depth == 0:
        x := evaluate(state)                        # static heuristic, basic outcome
        return (utilityA(x), utilityB(x))
    for a in 0..mA-1:
        for b in 0..mB-1:
            accA := 0 ; accB := 0
            for k in 1..N:
                s' := sampleTransition(state, a, b, rng)   # EXPENSIVE
                (cA, cB) := ESTIMATE(s', depth - 1)
                accA += cA ; accB += cB
            payoffA[a][b] := accA / N
            payoffB[a][b] := accB / N
    (sigmaA, sigmaB, vA, vB) := SELECTOR.solve(payoffA, payoffB)
    return (vA, vB)
```

For zero-sum v1, `payoffB = -payoffA`, `SELECTOR` is the minimax LP, and
`vB = -vA`.

### 4.3 Cost and the one caveat to honor

The fully expanded fixed-depth tree has leaf count on the order of
`(mA * mB * N)^D`. With `m = 12`, that is `(144 N)^D`. Two consequences:

- **Decouple "samples for the chance expectation" from "tree branching."** In the
  naive scheme `N` does both jobs, so raising `N` inflates the whole tree
  cubically at depth 3. Prefer the MCTS-style anytime behavior: keep the
  per-cell sample count modest and add **independent root trajectories** that
  accumulate into the root statistics, rather than re-inflating every subtree.
- **Budget in transitions.** Expose the per-move transition budget as the primary
  control; depth `D`, per-cell `N`, and trajectory count are derived from it.

### 4.4 Output contract

`PLAN` returns `(sigmaA, sigmaB)`, each a probability vector over that player's
moves, plus the node value(s). The engine does **not** itself commit a move; the
caller samples from `sigmaA` and `sigmaB`.

---

## 5. Known refinements and upgrade path (not required for v1)

Record these so the v1 baseline is built to be extended, not replaced:

- **Adaptive allocation.** Uniform `N` per cell wastes samples on dominated joint
  actions. Upgrades, in increasing sophistication and sharing the same backup:
  Adaptive Multi-stage Sampling (AMS; Chang, Fu, Hu, Marcus 2005); Forward Search
  Sparse Sampling (FSSS; Walsh, Goschin, Littman 2010); and Simultaneous-Move
  MCTS with matrix-game backups (selective, incremental depth). Keep the
  allocation policy behind an interface so it can be swapped.
- **Lower exploitability.** For demo strength, decoupled-UCT-style selection is
  fine. For least-exploitable play, use regret matching / Exp3 / Online Outcome
  Sampling as the per-node rule. Same backup object; different selection policy.
- **Estimator bias.** The min-max (or Nash) of *estimated* payoffs is a biased
  estimate of the true value (optimization bias over noisy entries). Mitigate
  with larger `N` at shallow nodes, or by regularizing the solve (a small logit
  temperature smooths the selection and the value; see Section 8).
- **Parallelism.** Transitions are independent. Parallelize trajectory generation
  (root parallelization) and/or per-cell sampling. This converts cores directly
  into lower variance or deeper reach and is the highest-leverage engineering
  step given expensive transitions.

---

## 6. The matrix-game solver / Nash selector

A single interface, with the zero-sum case as the v1 implementation.

- **Input:** `payoffA[mA][mB]`, `payoffB[mA][mB]`.
- **Output:** `sigmaA[mA]`, `sigmaB[mB]`, `vA`, `vB`.

**v1 (zero-sum minimax).** `payoffB = -payoffA`. Solve the matrix game by linear
programming; `vA` is the game value, `vB = -vA`; `sigmaA` and `sigmaB` are the
optimal mixed strategies (primal and dual). The matrices are tiny (<= 400
entries), so any correct LP is adequate; a small dense simplex implemented in the
library, or a vetted LP dependency, both suffice. This minimax solve **is** the
Nash selection function for the zero-sum case --- no selection ambiguity exists.

**Later (general-sum / CARA).** `payoffA + payoffB != const`. The node is a
genuine bimatrix game with possibly several non-equivalent equilibria, so a
**Nash selection function** is required and must be applied **identically at every
node** (the KMS coherence condition). See Section 8 for which selector to use.

---

## 7. Architecture and shared abstractions

The same conceptual interfaces appear in both languages. Keep the **planner
generic over the game model**; keep the **game model, utility, leaf evaluator,
selector, and RNG** as separate injectable components.

### 7.1 Components

- `GameModel<State>` --- the generative model: `moveCountA`, `moveCountB`,
  `isTerminal`, `terminalOutcome` (basic `x`), `evaluate` (static leaf heuristic,
  basic `x`), and `sampleTransition(state, moveA, moveB, rng) -> State`. The
  concrete game rules (including the chance device, e.g. random serialization of
  the two moves with an illegal move treated as a pass, and termination on mutual
  explicit pass) live **entirely inside `sampleTransition` / `isTerminal`** and
  are author-supplied.
- `Utility` --- `utilityA(x)`, `utilityB(x)`. v1: identity / negation. CARA:
  `1 - exp(-a x)` and `1 - exp(a x)`.
- `NashSelector` --- `solve(payoffA, payoffB) -> (sigmaA, sigmaB, vA, vB)`.
- `AllocationPolicy` (optional seam) --- decides how samples are spread across
  cells; v1 = uniform `N`.
- `Planner<State>` --- owns depth, budget, references to the above, and the RNG;
  exposes `plan(state) -> (sigmaA, sigmaB, vA, vB)`.

### 7.2 RNG discipline

Thread the RNG **explicitly** (no global mutable RNG); seedable for reproducible
runs. Java: `RandomGenerator` / `SplittableRandom` (split per worker). C++:
`std::mt19937_64` with per-thread seeded streams. This matches the preference for
referential transparency and is required for safe parallelism.

### 7.3 State representation --- an explicit decision for the implementer

`sampleTransition` should behave as a **pure function** (input state unchanged,
new state returned), both for clarity and for lock-free parallelism. The tension
is the expensive ~1500-location state: full deep copies per node are costly.
Resolve it with **immutable state plus structural sharing / copy-on-write** so
that a transition allocates only the changed portion. Avoid an in-place
make-move/undo-move design unless profiling forces it: it reintroduces shared
mutable state, defeats parallelism, and is the kind of side-effecting code to be
avoided here. Flag the final choice in the code; do not silently pick the mutable
route for a quick speed-up.

### 7.4 Failure handling

Fail fast. An out-of-range move index, an empty strategy vector, a non-finite
payoff, or an `N <= 0` is a programming error and must raise immediately, not be
silently clamped or defaulted. Silent default substitution is prohibited because
it hides the origin of bugs. (In Java specifically, do not paper over missing
keys with `Map.getOrDefault` / `merge` except when building an initial
collection.)

### 7.5 Interface sketches (starting points, not final)

Java 17:

```java
public interface GameModel<S> {
    int moveCountA();
    int moveCountB();
    boolean isTerminal(S state);
    double terminalOutcome(S state);                 // basic outcome x, A's view
    double evaluate(S state);                         // static leaf heuristic, basic x
    S sampleTransition(S state, int moveA, int moveB, java.util.random.RandomGenerator rng);
}

public interface Utility {
    double utilityA(double basicOutcome);
    double utilityB(double basicOutcome);
}

public interface NashSelector {
    SolveResult solve(double[][] payoffA, double[][] payoffB);
}

public record SolveResult(double[] sigmaA, double[] sigmaB, double valueA, double valueB) { }
```

C++ 20 (runtime-polymorphic form for the multi-example library; a template form is
an alternative if per-transition virtual dispatch proves costly):

```cpp
template <class State>
class GameModel {
public:
    virtual int moveCountA() const = 0;
    virtual int moveCountB() const = 0;
    virtual bool isTerminal(const State& s) const = 0;
    virtual double terminalOutcome(const State& s) const = 0;   // basic outcome x
    virtual double evaluate(const State& s) const = 0;          // static leaf heuristic
    virtual State sampleTransition(const State& s, int moveA, int moveB,
                                   std::mt19937_64& rng) const = 0;
    virtual ~GameModel() = default;
};

struct SolveResult {
    std::vector<double> sigmaA;
    std::vector<double> sigmaB;
    double valueA;
    double valueB;
};

class NashSelector {
public:
    virtual SolveResult solve(const std::vector<std::vector<double>>& payoffA,
                              const std::vector<std::vector<double>>& payoffB) const = 0;
    virtual ~NashSelector() = default;
};
```

---

## 8. The general-sum / CARA extension (future, design now)

When the CARA utility is switched on the game becomes general-sum in utility. The
following are required; they are recorded here so v1 leaves room for them.

- **Why selection is needed.** A zero-sum-in-dollars game transformed by a
  *strictly concave* utility is **not** strategically equivalent to a zero-sum
  game (no affine relation exists between the two players' payoffs), so the
  zero-sum guarantees --- unique value, interchangeable equilibria --- do not
  carry over. Per-node bimatrix games may have several inequivalent equilibria.
- **The governing simplification.** With `a` small the game sits just off the
  zero-sum manifold. By upper-hemicontinuity, multiplicity is **local and rare**:
  at nodes whose underlying zero-sum game has a unique optimal pair (the generic
  case) the perturbed node has a single nearby equilibrium and selection is moot.
  Selection only bites at **degenerate** nodes (ties, symmetry, a continuum of
  zero-sum optima). So: detect degeneracy; apply a principled tie-break only
  there.
- **Recommended selector for this setting.** A tiered rule:
  1. Default to the **security / minimax** solution (each side's maxmin of its own
     matrix). It is unique generically, coincides with the truth as `a -> 0`, and
     costs one LP.
  2. At detected-degenerate nodes, break ties by **risk dominance** --- the
     criterion that aligns with the modeled risk aversion and with the absence of
     any coordination channel. For symmetric 2x2 reductions it is closed form
     (the action optimal under a uniform prior over the opponent). 
  3. For noise robustness (the matrices are Monte Carlo estimates), optionally
     realize the tie-break as a **small fixed-temperature logit equilibrium**,
     which is continuous in the payoffs and approximates risk-dominant selection
     without an expensive homotopy.
- **Do not** default to payoff dominance: it assumes coordination the game does
  not support and conflicts with the deliberate risk aversion.
- **Apply the same selector at every node** (KMS coherence) and **apply utility at
  the leaves, averaging in utility space** (Section 3, item 7).
- **Symmetry.** When the node game is symmetric, the selector must respect
  symmetry; ad-hoc index-order tie-breaks must not be used, as they break the
  value's symmetry.

---

## 9. Open questions / deferred to the author

These are intentionally unspecified here and must be supplied or decided during
implementation. Do not fill them with plausible defaults.

1. **Concrete game rules.** The dynamics inside `sampleTransition`, the exact
   chance device, the legality/pass rule, and the termination condition are the
   author's design. Obtain them before coding the specific Java game.
2. **Static leaf evaluator.** The quality of `evaluate` dominates demo strength
   under expensive, non-reusable transitions. Its design (features, whether it
   returns a point estimate or mean/variance for the CARA version) is open.
3. **Transition budget and latency target.** Sets depth, `N`, and trajectory
   count. Needs a number from the author.
4. **State representation.** Confirm the immutable-with-structural-sharing choice
   against the actual state type and update cost (Section 7.3).
5. **LP dependency vs in-house simplex** for the zero-sum solver, and the
   bimatrix solver + selection implementation for the CARA version.
6. **Whether v2 makes the move sets or the game asymmetric**, which affects only
   the selector's symmetry handling, not the interfaces (already separated into
   A/B move counts).

---

## 10. References (verified)

- Kearns, Mansour, Singh, *Fast Planning in Stochastic Games*, UAI 2000 ---
  sparse sampling with matrix-game (Nash) backups; Nash selection function;
  finite- vs infinite-horizon note. https://arxiv.org/pdf/1301.3867
- Kearns, Mansour, Ng, *A Sparse Sampling Algorithm for Near-Optimal Planning in
  Large MDPs*, Machine Learning 49 (2002).
  https://www.cis.upenn.edu/~mkearns/papers/sparsesampling-journal.pdf
- Chang, Fu, Hu, Marcus, *An Adaptive Sampling Algorithm for Solving Markov
  Decision Processes*, Operations Research 53(1), 2005 (AMS; precursor to UCT).
- Walsh, Goschin, Littman, *Integrating Sample-Based Planning and Model-Based
  Reinforcement Learning*, AAAI 2010 (FSSS).
  https://cdn.aaai.org/ojs/7556/7556-13-11084-1-2-20201228.pdf
- Bosansky, Lisy, Lanctot, Cermak, Winands, *Algorithms for computing strategies
  in two-player simultaneous move games*, Artificial Intelligence 237 (2016) ---
  exact and online methods; matrix-game backups; convergence.
  https://www.sciencedirect.com/science/article/pii/S0004370216300285
- Lisy, Kovarik, Lanctot, Bosansky, *Convergence of MCTS in Simultaneous Move
  Games*, NeurIPS 2013.
  https://proceedings.neurips.cc/paper/2013/hash/1579779b98ce9edb98dd85606f2c119d-Abstract.html
- Shapley, *Stochastic Games*, PNAS 39 (1953) --- value iteration; zero-sum
  convergence.
- Littman, *Markov games as a framework for multi-agent reinforcement learning*,
  ICML 1994 --- minimax-Q (model-free analogue, if ever needed).
- Adler, Daskalakis, Papadimitriou, *A Note on Strictly Competitive Games*, WINE
  2009 --- strictly competitive iff affine variant of zero-sum (why CARA breaks
  the zero-sum guarantees). https://people.csail.mit.edu/costis/SCG_1.pdf
- Harsanyi, Selten, *A General Theory of Equilibrium Selection in Games*, MIT
  Press 1988 --- risk dominance, payoff dominance, tracing procedure.
  Summary with the 2x2 closed form: https://en.wikipedia.org/wiki/Risk_dominance
- Carlsson, van Damme, *Global Games and Equilibrium Selection*, Econometrica
  61(5), 1993 --- noise-perturbation foundation for risk-dominant selection.
  http://www.dklevine.com/archive/refs4122247000000001088.pdf
- McKelvey, Palfrey, *Quantal Response Equilibria for Normal Form Games*, GEB 10
  (1995); Turocy, *A dynamic homotopy interpretation of the logistic QRE
  correspondence*, GEB 51 (2005) --- logit selection; implemented in Gambit,
  http://www.gambit-project.org
