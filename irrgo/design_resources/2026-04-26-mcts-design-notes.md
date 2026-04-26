# Multithreaded MCTS Design — Handoff Notes

**Date:** 2026-04-26
**Project:** C++ MCTS engine, target 8 cores, board game of the user's own
design (similar to Go but fundamentally different — Go-specific concepts and
code patterns must not be substituted).
**Purpose of this document:** Capture the state of a design discussion so a
follow-on session (in CLion via Claude Code, JetBrains AI Assistant, or any
other client) can resume without re-deriving anything.

---

## Measured profile

- Single-threaded engine reaches **100–110 terminal nodes/second**.
- One terminal node corresponds to one full MCTS iteration
  (select → expand → simulate → backpropagate), so roughly **9–10 ms per
  iteration**.
- The rollout (playout from leaf to terminal) dominates this; tree-descent
  and backpropagation are almost certainly under 100 µs combined.
- Conclusion: this is a **rollout-dominated profile**. Mutex contention on
  tree nodes will be a small fraction of total cost regardless of locking
  scheme.

## Designs surveyed

Three serious candidates from the parallel-MCTS literature, all validated at
the 4–16 thread scale on shared-memory hardware. Leaf parallelization
(multiple rollouts from the same leaf) is the fourth member of the classic
Chaslot–Winands–van den Herik (2008) taxonomy, but it has lost in essentially
every head-to-head since and was excluded from consideration.

### 1. Root parallelization (independent trees, vote/sum aggregation)

Each thread runs a complete sequential MCTS with its own tree. At decision
time, aggregate by summing visit counts or by majority-voting root moves.
Threads share nothing during search.

- **Pros:** Trivially correct, embarrassingly parallel, excellent NUMA
  behavior. Strong baseline at low core counts. No concurrency machinery.
- **Cons:** Each tree is smaller than a single shared tree of equivalent
  total compute. Mid-search information not shared, so threads can make
  correlated mistakes. Strength-speedup tends to plateau by 8–16 threads.
- **Verdict for this case:** Weakened by the rollout-dominated profile.
  Its main argument was avoiding contention, but contention is already
  negligible at 10 ms per iteration.

### 2. Lock-free tree parallelization (Enzenberger & Müller, Fuego)

A single shared tree; threads concurrently descend, expand, simulate, and
back-propagate. Atomic word-sized writes on x86's TSO memory model are used,
and rare non-atomic updates of two adjacent fields (visit count, win count)
are accepted as statistically dominated by the millions of correct ones.

- **Pros:** Specifically validated at 7 threads on a dual-quad Xeon E5420
  (8 cores) in Fuego on 9×9 and 19×19 Go.
- **Cons:** Correctness arguments depend on x86/x86-64 strong memory
  ordering. C++20 `std::atomic` with appropriate memory orders reproduces
  this on x86 and AMD64. ARM64 (Apple Silicon, Raspberry Pi) would require
  explicit acquire/release fences. The "ignore rare bad updates" argument is
  uncomfortable to verify formally.
- **Verdict for this case:** Overkill given the contention level. Worth
  considering only if profiling later shows mutex acquisition as a
  measurable fraction of throughput.

### 3. Tree parallelization with virtual loss (AlphaGo lineage)

Shared tree as in (2), but during selection a thread temporarily marks
visited nodes with a "virtual loss" — pretends an in-flight simulation will
lose — to deflect other threads from following the same UCB1-best path. The
loss is reversed when the real result back-propagates.

- **Pros:** Segal (2010) showed scaling to 64+ threads with virtual loss
  versus ~8 without. With 8 threads at 10 ms per rollout, ~80 ms of in-flight
  rollouts overlap at any moment; virtual loss directly addresses the
  pathology of all eight threads committing to the same path before any
  visit-count update is visible.
- **Cons:** Virtual loss is a known approximation that perturbs the UCB1
  selection rule. Mirsoleimani et al. (2017) showed it can degrade
  performance starting at 4 workers in a high-energy-physics MCTS
  application. Liu et al. (2020) introduced WU-UCT (uses on-going-simulation
  count rather than a fake loss) and BU-UCT (regret-bounded), which are more
  recent but less battle-tested.
- **Verdict for this case:** Strengthened by the rollout-dominated profile.

## Selected design

**Tree parallelization with virtual loss, using straightforward mutexes.**

Implementation outline:

- Per-node `std::mutex` (or `std::shared_mutex` if reads vastly outnumber
  writes after profiling).
- `std::atomic<int>` for visit count, win count, and the virtual-loss
  counter.
- Standard 4-phase MCTS loop per thread, with virtual loss applied during
  selection and reversed at backpropagation.
- Cross-platform: builds on Windows and Debian Linux without
  platform-specific code (the user's standing requirement).
- C++20, CMake build, runs on KDevelop (Linux) and Visual Studio (Windows).

Rationale: at 9–10 ms per iteration with sub-millisecond tree work, even a
single coarse mutex around the entire tree would cost under 5% throughput.
Per-node `std::mutex` is therefore comfortably below that. Lock-free
machinery is unnecessary at this contention level and adds correctness risk
on non-x86 targets without buying meaningful throughput.

Realistic expectation: **6–7× speedup on 8 cores**. Migrate to the lock-free
design later if and only if profiling shows mutex acquisition as a
non-trivial fraction of total time.

## Open question (single-thread headroom)

100 iterations/sec is on the slow end of the empirical distribution for
board-game MCTS engines. Two possibilities, not mutually exclusive:

1. The rollout policy is intentionally heavy (heuristic-rich, NN-evaluated,
   or simulating a complex domain), in which case 10 ms is reasonable and
   parallelization is the right lever.
2. There may be single-threaded headroom in the rollout itself: state
   representation, move generation, copy-versus-undo, allocator pressure,
   branch prediction in the inner loop.

Single-thread improvements compound multiplicatively with parallel speedup.
A 5× single-thread win combined with 7× parallel speedup yields 35× overall.
**Decision needed:** profile the rollout for single-thread headroom before
threading work, or after?

## References

- Chaslot, Winands & van den Herik (2008), *Parallel Monte-Carlo Tree
  Search*: <https://dke.maastrichtuniversity.nl/m.winands/documents/multithreadedMCTS2.pdf>
- Enzenberger & Müller (2009/2010), *A Lock-Free Multithreaded Monte-Carlo
  Tree Search Algorithm*: <https://webdocs.cs.ualberta.ca/~mmueller/ps/enzenberger-mueller-acg12.pdf>
- Segal (2010), *On the Scalability of Parallel UCT*:
  <https://app.dimensions.ai/details/publication/pub.1021283639>
- Mirsoleimani, Plaat, van den Herik & Vermaseren (2017), *An Analysis of
  Virtual Loss in Parallel MCTS*:
  <https://liacs.leidenuniv.nl/~plaata1/papers/paper_ICAART17.pdf>
- Mirsoleimani (2019), PhD thesis *Structured Parallel Programming for Monte
  Carlo Tree Search* (Leiden) — most thorough single source on shared-memory
  MCTS:
  <https://liacs.leidenuniv.nl/~plaata1/theses/AliMirsoleimani.pdf>
- Steinmetz & Gini (2020), *More Trees or Larger Trees: Parallelizing Monte
  Carlo Tree Search*:
  <https://www-users.cse.umn.edu/~gini/publications/papers/Steinmetz2020TG.pdf>
- Liu et al. (2020), *Watch the Unobserved: A Simple Approach to
  Parallelizing Monte Carlo Tree Search* (WU-UCT, ICLR 2020):
  <http://starai.cs.ucla.edu/papers/LiuDRLW20.pdf>

## Constraints to honor in any follow-up session

- C++20 with CMake; cross-platform Windows + Debian Linux.
- The game is similar to Go but fundamentally different — Go-specific
  concepts and code (patterns, group-detection idioms, ko rules, etc.) must
  not be substituted.
- Two-dimensional hex coordinate system is also of the user's own design;
  do not substitute cubic coordinates or other common hex-grid conventions.
- Code style: curly braces around all `if`/`else` clauses even single-line.
  No silent default substitution on errors. Avoid programming by side-effect
  except where it is the explicit goal (I/O). Functions kept under a few
  hundred lines.
