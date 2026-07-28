"""
ssvs_sensitivity.py
----------------------------------------------------------------------
Checks how much of the performance of stochastic search variable
selection, reported in compete.py, depends on the two widths that the
analyst must supply.

The two-component prior needs a narrow width for cells to be treated as
empty and a wide width for cells to be treated as occupied.  These play
the same part as the bounds a_min and a_max in the paper: they are not
determined by the data, and the answer moves when they move.  This
script shows by how much.

Two chains are run at each setting, with different random numbers, so
that a difference between settings can be told from the variability of
the sampler itself.
----------------------------------------------------------------------
"""

import numpy as np

from sweep import N, build, logdets
from compete import ssvs, refit, score

SETTINGS = [(0.005, 3.0), (0.02, 1.5), (0.05, 1.0), (0.10, 0.6)]
SEED = 20260726
CHAINS = (11, 22)


def main():
    A_block, A_scatter, X, E = build(SEED)
    logdets(X)

    lines = [f"seed {SEED}, two chains at each setting",
             "",
             f"{'case':<8} {'tau0':>6} {'tau1':>6} {'chain':>6} {'M':>4}"
             f" {'corr':>5} {'false':>6} {'miss':>5} {'frob':>7}"]

    for name, A_true in (("block", A_block), ("scatter", A_scatter)):
        Y = A_true @ X + E
        for tau0, tau1 in SETTINGS:
            for c in CHAINS:
                pip = ssvs(X, Y, c, tau0=tau0, tau1=tau1)
                mask = pip >= 0.5
                A_ref, _ = refit(X, Y, mask)
                s = score(A_ref, mask, A_true, X, Y)
                lines.append(f"{name:<8} {tau0:6.3f} {tau1:6.2f} {c:6d}"
                             f" {s['M']:4d} {s['correct']:5d} {s['false']:6d}"
                             f" {s['missed']:5d} {s['frob']:7.3f}")
        lines.append("")

    text = "\n".join(lines)
    print(text)
    with open("ssvs_sensitivity.txt", "w") as f:
        f.write(text + "\n")


if __name__ == "__main__":
    main()
