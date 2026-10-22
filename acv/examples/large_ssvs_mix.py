"""
large_ssvs_mix.py
----------------------------------------------------------------------
Checks whether the sampler used in Section 12 has been run long enough.

Forty thousand indicators are drawn at every sweep, and eight thousand
sweeps is not obviously sufficient for them.  If it is not, the
comparison would charge the sampler for a shortage of computing rather
than for anything about the method.

Two chains are run from different random numbers at each length.  If
the eight thousand sweeps used in Section 12 are enough, the two chains
should agree with each other and with the longer runs.
----------------------------------------------------------------------
"""

import numpy as np
import large as L

LENGTHS = [(4000, 1000), (8000, 2000), (20000, 5000)]
CHAINS = (11, 22)
SEED = 20260726


def main(case="block", lengths=None, out="large_ssvs_mix.txt"):
    global LENGTHS
    if lengths is not None:
        LENGTHS = lengths
    return _run(case, out)


def _run(case, out):
    A_block, A_scatter, X, E = L.build(SEED)
    A_true = A_block if case == "block" else A_scatter
    Y = A_true @ X + E
    tru = A_true != 0.0

    lines = [f"seed {SEED}, case {case}, true cells {int(tru.sum())}", "",
             f"{'draws':>7} {'burn':>6} {'chain':>6} {'M':>6} {'corr':>6}"
             f" {'false':>6} {'miss':>6} {'frob':>8}"]
    keep = {}
    for draws, burn in LENGTHS:
        for c in CHAINS:
            pip = L.ssvs(X, Y, c, draws=draws, burn=burn)
            keep[(draws, c)] = pip
            m = pip >= 0.5
            s = L.score(L.refit(X, Y, m), m, A_true, X, Y)
            lines.append(f"{draws:7d} {burn:6d} {c:6d} {s['M']:6d}"
                         f" {s['correct']:6d} {s['false']:6d}"
                         f" {s['missed']:6d} {s['frob']:8.2f}")
        a, b = keep[(draws, CHAINS[0])], keep[(draws, CHAINS[1])]
        lines.append(f"        chains differ on "
                     f"{int(np.sum((a >= 0.5) != (b >= 0.5)))} cells of "
                     f"{L.q}; largest gap in inclusion probability "
                     f"{float(np.max(np.abs(a - b))):.3f}")
    text = "\n".join(lines)
    print(text)
    with open(out, "w") as f:
        f.write(text + "\n")


if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        # a single length, given as draws:burn, written to its own file
        d, b = (int(v) for v in sys.argv[1].split(":"))
        main(lengths=[(d, b)], out=f"large_ssvs_mix_{d}.txt")
    else:
        main()
