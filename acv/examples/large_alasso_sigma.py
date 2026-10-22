"""
large_alasso_sigma.py
----------------------------------------------------------------------
The adaptive lasso recovered the matrix of Section 12 exactly, but it
was scored by the Schwarz criterion using the true error scale, which
no analyst would have.  This script repeats it with the error scale
estimated from the data, which is the only form in which the result
could ever be used.

The procedure is the ordinary plug-in iteration.

  1.  An initial error scale is obtained without reference to sigma at
      all, by choosing the penalty of the plain lasso by five-fold
      cross-validation and refitting by least squares on the cells it
      selects.

  2.  Given the current estimate, the penalty of the plain lasso is
      chosen for each row by the Schwarz criterion, which supplies the
      weights of the adaptive lasso.

  3.  The adaptive path is computed with those weights and its penalty
      chosen by the same criterion.  The selected cells are refitted by
      least squares over the whole matrix and the error scale
      re-estimated as R / (n - M).

  4.  Steps 2 and 3 are repeated until the pattern stops changing.

The Schwarz criterion is a sum over rows, so it separates, and each row
may take its own penalty while the error scale is shared.  To show that
the fixed point does not depend on where the iteration starts, one case
is also run from a deliberately large initial value.
----------------------------------------------------------------------
"""

import sys
import numpy as np
from math import log
import large as L

ITERS = 5


def plain_path(X, y):
    """The lasso path for one row, with the scaling undone.

    The path itself does not depend on the error scale; only the choice
    of a point on it does.  It is therefore computed once and reused at
    every iteration.
    """
    Z = X.T
    Zs, nrm = L.standardise(Z)
    lam_max = float(np.max(np.abs(Zs.T @ y)))
    lams = np.exp(np.linspace(log(lam_max), log(lam_max * L.LAMRATIO),
                              L.NLAM))
    return L.lasso_path(Zs, y, lams), Zs, nrm


def bic_pick(path, Zs, y, sigma):
    resid = y[None, :] - path @ Zs.T
    bic = (np.sum(resid * resid, axis=1) / (sigma * sigma)
           + np.sum(path != 0.0, axis=1) * log(L.n))
    return int(np.argmin(bic))


def cv_pick(Zs, y, lams):
    cv = np.zeros(L.NLAM)
    for fold in np.array_split(np.arange(L.T), min(L.NFOLDS, L.T)):
        keep = np.setdiff1d(np.arange(L.T), fold)
        co = L.lasso_path(Zs[keep, :], y[keep], lams)
        cv += np.sum((co @ Zs[fold, :].T - y[fold]) ** 2, axis=1)
    return int(np.argmin(cv))


def sigma_from(X, Y, mask):
    A = L.refit(X, Y, mask)
    R = float(np.sum((Y - A @ X) ** 2))
    M = int(np.sum(mask))
    return float(np.sqrt(R / max(L.n - M, 1))), A, M


def run(X, Y, A_true, sigma0=None, lines=None):
    paths, Zss, nrms, lamss = [], [], [], []
    for i in range(L.N):
        p, Zs, nrm = plain_path(X, Y[i])
        paths.append(p)
        Zss.append(Zs)
        nrms.append(nrm)
        lam_max = float(np.max(np.abs(Zs.T @ Y[i])))
        lamss.append(np.exp(np.linspace(log(lam_max),
                                        log(lam_max * L.LAMRATIO), L.NLAM)))

    if sigma0 is None:
        mask = np.zeros((L.N, L.N), dtype=bool)
        for i in range(L.N):
            k = cv_pick(Zss[i], Y[i], lamss[i])
            mask[i] = (paths[i][k] / nrms[i]) != 0.0
        sigma, _, M = sigma_from(X, Y, mask)
        lines.append(f"  start: cross-validation gives M = {M}, "
                     f"sigma = {sigma:.4f}")
    else:
        sigma = sigma0
        lines.append(f"  start: sigma set to {sigma:.4f} by hand")

    prev = None
    for it in range(ITERS):
        mask = np.zeros((L.N, L.N), dtype=bool)
        for i in range(L.N):
            k = bic_pick(paths[i], Zss[i], Y[i], sigma)
            init = np.abs(paths[i][k] / nrms[i])
            w = 1.0 / np.maximum(init, 0.05)
            Zw = Zss[i] / w
            lw_max = float(np.max(np.abs(Zw.T @ Y[i])))
            lw = np.exp(np.linspace(log(lw_max),
                                    log(lw_max * L.LAMRATIO), L.NLAM))
            pw = L.lasso_path(Zw, Y[i], lw) / w
            kk = bic_pick(pw, Zss[i], Y[i], sigma)
            mask[i] = (pw[kk] / nrms[i]) != 0.0
        sigma, A, M = sigma_from(X, Y, mask)
        s = L.score(A, mask, A_true, X, Y)
        lines.append(f"  pass {it + 1}: M = {M:5d}  correct {s['correct']:5d}"
                     f"  false {s['false']:5d}  missed {s['missed']:5d}"
                     f"  sigma {sigma:.4f}  error {s['frob']:.2f}")
        if prev is not None and np.array_equal(prev, mask):
            lines.append("  the pattern has stopped changing")
            break
        prev = mask.copy()
    return s, sigma


def main(nseeds=2, cases=("block", "scatter")):
    lines = [f"N={L.N} T={L.T} n={L.n}  adaptive lasso with the error scale "
             f"estimated", f"true error scale {L.SIGMA_TRUE}", ""]
    for si in range(nseeds):
        seed = 20260726 + 1000 * si
        A_block, A_scatter, X, E = L.build(seed)
        for case in cases:
            A_true = A_block if case == "block" else A_scatter
            Y = A_true @ X + E
            lines.append(f"seed {seed}, {case}, true cells "
                         f"{int(np.sum(A_true != 0))}")
            run(X, Y, A_true, None, lines)
            if si == 0 and case == "block":
                lines.append("  the same case started from a large value, to "
                             "show the fixed point does not depend on it")
                run(X, Y, A_true, 1.0, lines)
            lines.append("")
            print("\n".join(lines[-8:]))
            sys.stdout.flush()
    text = "\n".join(lines)
    with open("large_alasso_sigma.txt", "w") as f:
        f.write(text + "\n")


if __name__ == "__main__":
    main(int(sys.argv[1]) if len(sys.argv) > 1 else 2)
