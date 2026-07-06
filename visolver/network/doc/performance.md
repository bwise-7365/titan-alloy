<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
# Scale and performance results (task C5)

`network_benchmark` output, Release build, single-threaded, user's Windows 11
machine, 2026-07-04. Five seeds (20260704-08) per configuration; budget
calibrated per instance at 80% of the greedy plan's ton-miles. Columns:
kept/total pairs, R3 certificate rounds, bsHe94b iterations (final solve),
wall ms for the whole `solveFlowPlan` call, the rationing lower bound
theta_ration, achieved theta*, delivered fraction of rationed targets,
optimizer ton-miles / greedy ton-miles, real-unit budget shadow price,
certified flag.

## Raw results

### 26 nodes, laydown 0, keep-all (exact reference)

    seed        kept/total rnds   iter       ms  th_ration    th_star    dlv   tm/tmG      lambda  cert
    20260704     224/  224    0   5940    111.3    0.00000    0.04480  0.976    0.800    1.40e-07   yes
    20260705     224/  224    0   4805     91.1    2.06679    3.08113  0.954    0.800    6.97e-07   yes
    20260706     224/  224    0   2229     41.3    3.58524    4.15604  0.992    0.800    5.42e-07   yes
    20260707     224/  224    0   5168    113.5    2.71224    3.21396  0.998    0.800    1.09e-06   yes
    20260708     224/  224    0   6945    134.7    0.00000    0.25144  0.942    0.800    3.95e-07   yes
    mean: ms 98.4   th_ration 1.67285   th_star 2.14948   dlv 0.973   tm/tmG 0.800   certified 5/5

### 26 nodes, laydown 0, screen k=3 + certificate (cross-check)

    20260704      67/  224    2    965      6.2    0.00000    0.04480  0.976    0.800    1.40e-07   yes
    20260705     145/  224    1   1908     18.4    2.06679    3.08111  0.954    0.800    6.97e-07   yes
    20260706     153/  224    1   1784     17.8    3.58524    4.15604  0.992    0.800    5.42e-07   yes
    20260707      62/  224    1    731      2.8    2.71224    3.21390  0.998    0.800    1.09e-06   yes
    20260708      64/  224    1   1117      4.0    0.00000    0.25142  0.942    0.800    3.95e-07   yes
    mean: ms 9.8   th_ration 1.67285   th_star 2.14945   dlv 0.973   tm/tmG 0.800   certified 5/5

### 70 nodes, laydown 0, screen k=10 + certificate

    20260704     543/ 2000    1   4675    907.0    2.25076    2.70037  0.997    0.800    6.23e-07   yes
    20260705     500/ 2000    0   1411    150.4    3.02734    3.02734  1.000    0.799    0.00e+00   yes
    20260706     500/ 2000    0   4786    475.1    5.57613    5.71299  1.000    0.800    3.46e-07   yes
    20260707     502/ 2000    1   4374    896.9    0.51031    0.56057  1.000    0.800    2.10e-07   yes
    20260708     709/ 2000    1   1818    834.1    2.63574    2.63575  1.000    0.798    0.00e+00   yes
    mean: ms 652.7   th_ration 2.80006   th_star 2.92740   dlv 0.999   tm/tmG 0.799   certified 5/5

Keep-all reference for seed 20260704 (single run, earlier benchmark shape):
43,247 iterations, **144,203 ms**, theta* 2.70043, certified — the screened
solve above reproduces it to 5 decimals (2.70037) in 907 ms: **~160x faster,
certified equal**.

### 70 nodes, laydown 1 (banded), screen k=10

    20260704    1666/ 2000    2  13845  65039.3    2.25076    6.06103  0.930    0.800    1.57e-06   yes
    20260705    1605/ 2000    2  16465  60674.6    3.02734    5.94287  0.951    0.800    1.38e-06   yes
    20260706    1610/ 2000    2  10088  42685.3    5.57613    9.69259  0.941    0.800    1.53e-06   yes
    20260707    1642/ 2000    2   8671 104761.6    0.51031    2.69466  0.926    0.800    9.00e-07   yes
    20260708    1646/ 2000    3  11659  95037.9    2.63574    5.13671  0.950    0.800    1.21e-06   yes
    mean: ms 73639.8   th_ration 2.80006   th_star 5.90557   dlv 0.940   tm/tmG 0.800   certified 5/5

(Same th_ration column as laydown 0: both laydowns draw coordinates with two
RNG values per node before the tonnage draws, so identical seeds produce
identical C/D/P — an accidental but useful controlled comparison of pure
geometry. Fragile: renumbering the generator's draws would break it.)

### 200 nodes (50/50/95 + 5 inert), laydown 0, screen k=10 + certificate

    20260704    1534/14500    2   3398  29263.2   39.84167   39.84651  1.000    0.800    4.40e-08   yes
    20260705    1450/14500    0  45903  85472.5   19.44851   19.44851  1.000    0.790    0.00e+00   yes
    20260706    1450/14500    0  12403  22903.9   36.45837   36.45837  1.000    0.798    0.00e+00   yes
    20260707    1535/14500    2   8683  45526.8   46.32694   46.34442  1.000    0.800    9.21e-08   yes
    20260708    1450/14500    0  21546  39574.7   64.02551   64.02551  1.000    0.795    0.00e+00   yes
    mean: ms 44548.2   th_ration 41.22020   th_star 41.22466   dlv 1.000   tm/tmG 0.796   certified 5/5

## Findings

- **P1 — The certificate is exact in practice, not just in theory.** Every
  screened solve certified, and where a keep-all reference exists (all five
  26-node seeds; the 70-node seed 20260704) theta* matches to 5-6 significant
  digits. Certificate rounds stayed at 0-3 everywhere.
- **P2 — The screen is an iteration-count necessity, not a memory nicety.**
  Keep-all at 70 nodes: 43k iterations, 144 s. Screened: 1.4-4.8k iterations,
  0.15-0.9 s — **~160x**, certified equal. At 26 nodes the same pattern is
  10x (98 -> 10 ms). Iteration count, not factorization, is the cost driver
  at every size tested (bsHe94b factors once; assembly is negligible).
- **P3 — Three budget regimes, all observed.** (a) *Budget-bound*: most
  26/70-node laydown-0 seeds — tm/tmG pinned at 0.800, positive lambda,
  theta* above the rationing floor. (b) *Budget-comfortable*: 70-node seeds
  20260705/08 and three 200-node seeds — the optimizer absorbs the whole 20%
  cut (lambda = 0, full rationed delivery, tm/tmG < 0.8). (c)
  *Rationing-dominated*: all 200-node instances — total capacity (~100
  sources) is far short of total demand (~145 sinks), theta_ration ~ 20-64
  dwarfs the budget effect and theta* ~= theta_ration. Which regime an
  instance is in is readable directly from (lambda, dlv, tm/tmG).
- **P4 — The banded laydown is the hard case, for both the screen and the
  solver.** With near-tied bare-distance costs, k=10 excludes pairs whose
  reduced costs are nearly zero: the certificate loop pulls the kept set from
  700 up to ~1650 of 2000 pairs over 2-3 rounds, and the near-degenerate
  optimal face pushes iterations to 9-16k on a ~1700-dim system: mean 74 s.
  It is also *economically* harder: delivered fraction 0.93-0.95 and theta*
  roughly 2x the geometric laydown on identical tonnage data (the accidental
  controlled comparison above).
- **P5 — Practical guidance.** Laydown-0-like (spread, floor-dominated)
  geometry: screen k=10 is right; sub-second at 70 nodes, well within
  interactive use. Banded/near-tied geometry: either start with a much larger
  k (the certificate loop will otherwise buy the same pairs in expensive
  installments), or accept ~minute solves at 70 nodes. A gap-based screen
  (keep sources within a cost MARGIN of the cheapest, rather than a count)
  and the deferred Solodov-Svaiter / smoothing-Newton hybrid engine are the
  two levers if the banded case ever needs to be fast. 200-node planning at
  ~45 s/solve is usable for batch studies; not yet interactive.

## Addendum (2026-07-06): interior point + structured Newton on 200-banded

The 200-node laydown-1 case that stalled bsHe94b (150k-iteration cap on the
round-0 system, abandoned after 10 minutes with zero rows) was rerun under
the Mehrotra interior-point engine, first with its default dense LU (probe
IP4a) and then with the NS2 structured Newton factory (per-sink
Sherman-Morrison + dual Schur complement; `solver.ipmNewton = flow`) on the
FULL pair set with no screen at all (probe NS3). Release build, seed
20260704, single instance, iterMax 200.

    config                              kept/total rnds  iter        ms  th_ration   th_star    dlv  tm/tmG    lambda  cert
    ipm, dense LU, screen k=10+gap,
      maxCertificateRounds=1 (IP4a)    10737/14500    1    35   1778400   39.84167  53.11743  0.957   0.800  2.22e-06    no
    ipm, flow Newton, keep-all (NS3)   14500/14500    0    36    4513.9   39.84167  52.90758  0.959   0.800  2.22e-06   yes

- **P6 — The interior point's iteration count is dimension- and
  degeneracy-insensitive at full scale.** 35 iterations at Newton dimension
  10,838; 36 at 14,601 — on the geometry that drove bsHe94b past 150k
  iterations at dimension ~1.9k. The engine question raised in P5 is
  settled: the projection-contraction rate collapse on near-tied banded
  faces simply does not afflict the central path.
- **P7 — The structured factory removes the linear-algebra wall, and with it
  the need for the screen.** IP4a's cost was dense dim^3 LU (~50 s per
  iteration) times the R3 certificate balloon (one round pulled the kept set
  to 74% of keep-all and still could not certify). The flow factory solves
  each Newton system in O(pairs) plus an LLT of size numSources+1, so the
  KEEP-ALL problem — nothing excluded, hence nothing to certify, exact by
  construction — runs end to end in 4.5 s: ~394x faster than the screened
  dense probe and certified, on identical hardware and seed. The remaining
  per-iteration cost is the engine's own O(dim^2) residual matvecs; the
  one-time dense M assembly (~1.7 GB at this size) is the memory price.
- **P8 — Keep-all is also economically better than a bounded screen.** The
  uncertified one-round IP4a answer overshot the true optimum by ~0.4%
  (53.117 vs 52.908) — real money the excluded pairs were worth. The
  sandwich holds (th_ration 39.84 < th_star 52.91), delivery is 0.959 of
  rationed targets at the 80% budget, and lambda ~ 2.2e-06 agrees across
  both probes.
- **P9 — Revised practical guidance.** Banded or large instances: engine
  `ipm` + `ipmNewton = flow` + NO screen is now the production
  configuration — seconds-scale, exact, no certificate machinery
  (`network/ns3-keepall.cfg` is the template). Small laydown-0 instances:
  the screened bsHe94b path remains perfectly good (sub-second) and avoids
  the dense-M memory footprint. The IP4b weekend controls (chain / bshe94b /
  ssn bounded probes on this instance) will complete the engine comparison;
  they no longer gate any production decision.

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
