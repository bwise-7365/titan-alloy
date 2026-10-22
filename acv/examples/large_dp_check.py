"""
large_dp_check.py
----------------------------------------------------------------------
The diagnostic in large_diag.py and the criterion check in
large_phi_check.py disagree.  The first says the true subset of a row
is reached by forward selection at that row's own size for 194 rows of
200.  The second says the criterion prefers the true pattern by more
than eleven thousand natural units.  If both are so, the search ought
to have found something very close to the truth and did not.

This script asks where the discrepancy lies.  It evaluates, at the true
error scale, the cost the dynamic programme is minimising, the value of
Phi the search records, and the value of Phi at the allocation that
matches the true row counts.
----------------------------------------------------------------------
"""

import numpy as np
from math import log, pi
import large as L


def main(seed=20260726, case="block"):
    A_block, A_scatter, X, E = L.build(seed)
    A_true = A_block if case == "block" else A_scatter
    Y = A_true @ X + E
    tru = A_true != 0.0

    tables = [L.row_table(X, Y, i) for i in range(L.N)]
    Rt = np.array([t[0] for t in tables])
    gt = np.array([t[1] for t in tables])
    st = np.array([t[2] for t in tables])
    ok = np.isfinite(Rt) & np.isfinite(gt)

    ktrue = tru.sum(axis=1)
    lines = [f"seed {seed}, case {case}", ""]

    # Is the true row size even admissible?
    adm = np.array([ok[i, ktrue[i]] if ktrue[i] <= L.KMAX else False
                    for i in range(L.N)])
    lines.append(f"rows whose true size is admissible in the row table: "
                 f"{int(adm.sum())} of {L.N}")
    bad = np.where(~adm)[0]
    if bad.size:
        lines.append(f"  first few inadmissible rows and why:")
        for i in bad[:6]:
            k = ktrue[i]
            why = ("size beyond the limit" if k > L.KMAX else
                   "residual not finite" if not np.isfinite(Rt[i, k]) else
                   "a coefficient falls outside the range of the prior")
            lines.append(f"    row {i:3d} true size {k:2d}: {why}")

    # Phi at the allocation that matches the true row counts
    M = int(ktrue.sum())
    R = float(sum(Rt[i, ktrue[i]] for i in range(L.N) if adm[i]))
    g = float(sum(gt[i, ktrue[i]] for i in range(L.N) if adm[i]))
    s = float(sum(st[i, ktrue[i]] for i in range(L.N) if adm[i]))
    lines.append("")
    lines.append(f"allocation matching the true row counts: M={M} "
                 f"R={R:.2f}  Phi={float(L.phi_parts(M, R, g, s)):.2f}")

    # What the search actually records
    best = L.search(X, Y, want_value=True)
    lines.append(f"search records                        : M={best[1]} "
                 f"R={best[2]:.2f}  Phi={best[0]:.2f}")

    text = "\n".join(lines)
    print(text)
    with open("large_dp_check.txt", "w") as f:
        f.write(text + "\n")


if __name__ == "__main__":
    main()
