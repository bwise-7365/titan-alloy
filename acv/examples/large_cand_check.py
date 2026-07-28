"""
large_cand_check.py
----------------------------------------------------------------------
The experiment of large_lasso_phi.py gave the same answer as forward
selection, which is not what was expected: the adaptive lasso recovers
this matrix exactly, so its path ought to contain the true pattern, and
Phi prefers the true pattern by a wide margin.

Either the candidate list does not contain the true subset of each row,
or it does and the dynamic programme is not selecting it.  This script
settles which, by asking of every row whether its true subset appears
among the candidates offered, and whether it survives the test that its
coefficients lie inside the range of the prior.
----------------------------------------------------------------------
"""

import numpy as np
import large as L
import large_lasso_phi as P


def main(seed=20260726, case="block"):
    A_block, A_scatter, X, E = L.build(seed)
    A_true = A_block if case == "block" else A_scatter
    Y = A_true @ X + E

    in_greedy = in_path = in_either = 0
    feasible = 0
    kept = 0
    oversize = 0

    for i in range(L.N):
        y = Y[i]
        true_cols = tuple(sorted(np.nonzero(A_true[i])[0]))
        k = len(true_cols)
        if k > L.KMAX:
            oversize += 1
            continue

        g = L.greedy_row(X.T, y)
        gset = {len(c): tuple(sorted(c)) for c in g}
        pth, Zs, nrm = P.plain_path_supports(X, y)

        ing = gset.get(k) == true_cols
        inp = true_cols in [tuple(sorted(c)) for c in pth.get(k, [])]
        in_greedy += ing
        in_path += inp
        in_either += (ing or inp)

        s = P.score_support(X, y, true_cols)
        if s is not None:
            feasible += 1
            # would it survive the cut to NCAND, ranked by residual?
            subs = set(pth.get(k, [])) | ({gset[k]} if k in gset else set())
            subs.add(true_cols)
            scored = []
            for c in subs:
                sc = P.score_support(X, y, c)
                if sc is not None:
                    scored.append((sc[0], tuple(sorted(c))))
            scored.sort()
            if true_cols in [c for _, c in scored[:P.NCAND]]:
                kept += 1

    lines = [f"seed {seed}, case {case}, rows {L.N}", ""]
    lines.append(f"true subset is the greedy subset of its own size : "
                 f"{in_greedy}")
    lines.append(f"true subset is visited by the plain lasso path   : "
                 f"{in_path}")
    lines.append(f"true subset available from either source         : "
                 f"{in_either}")
    lines.append(f"true subset passes the coefficient-range test    : "
                 f"{feasible}")
    lines.append(f"true subset survives the cut to {P.NCAND} candidates      : "
                 f"{kept}")
    if oversize:
        lines.append(f"rows whose true size exceeds KMAX: {oversize}")

    text = "\n".join(lines)
    print(text)
    with open("large_cand_check.txt", "w") as f:
        f.write(text + "\n")


if __name__ == "__main__":
    main()
