"""
large_lasso_phi.py
----------------------------------------------------------------------
Task 2 of pending-tasks.md: keep Phi as the judge and take the
candidate patterns from a penalty path instead of from forward
selection.

The reasoning is in Section 12 of the document.  At two hundred
variables the adaptive lasso recovers the matrix exactly, while forward
selection leaves about two dozen cells out; and Phi, evaluated at the
two answers, prefers the lasso's by some eleven thousand natural units.
The criterion and the lasso therefore agree, and the only thing between
them is the list of patterns the criterion is asked to choose from.

Nothing here uses the true error scale, the true pattern, or any other
quantity an analyst would not have.  The penalty path is computed for
its own sake; which point of it to take is decided by Phi and not by
cross-validation or by the Schwarz criterion.

Three lists of candidates are compared.

  greedy        the forward-selection chain of large.py, as a control

  path          that chain together with the supports the plain lasso
                path visits

  adaptive      those, together with the supports visited by an
                adaptive path whose weights come from the answer Phi
                gave on the previous list

The dynamic programme is the one already in use.  It is given several
candidate subsets of each size for each row rather than one, and at
each sigma it takes whichever of them the row cost prefers, which is
what the enumeration of Section 10 did at nine variables.
----------------------------------------------------------------------
"""

import sys
import time
import numpy as np
from math import log, inf, pi
import large as L

NCAND = 6                       # candidate subsets kept for each row and size


# ------------------------------------------------------------ candidates


def supports_from_path(path, nrm):
    """The distinct supports a penalty path visits, by size.

    A path is indexed by penalty, not by size: it may pass through the
    same support at several penalties and may skip a size entirely.
    """
    out = {}
    for row in path:
        cols = tuple(np.nonzero(row / nrm)[0])
        k = len(cols)
        if 0 < k <= L.KMAX:
            out.setdefault(k, [])
            if cols not in out[k]:
                out[k].append(cols)
    return out


def plain_path_supports(X, y):
    Z = X.T
    Zs, nrm = L.standardise(Z)
    lam_max = float(np.max(np.abs(Zs.T @ y)))
    lams = np.exp(np.linspace(log(lam_max), log(lam_max * L.LAMRATIO),
                              L.NLAM))
    return supports_from_path(L.lasso_path(Zs, y, lams), nrm), Zs, nrm


def adaptive_path_supports(X, y, init, Zs, nrm):
    """The adaptive path, weighted by a previous estimate of the row."""
    w = 1.0 / np.maximum(np.abs(init), 0.05)
    Zw = Zs / w
    lam_max = float(np.max(np.abs(Zw.T @ y)))
    lams = np.exp(np.linspace(log(lam_max), log(lam_max * L.LAMRATIO),
                              L.NLAM))
    return supports_from_path(L.lasso_path(Zw, y, lams) / w, nrm)


def cv_fit(X, y, Zs, nrm):
    """The plain lasso fit whose penalty cross-validation chooses.

    Cross-validation is used here only to weight the adaptive path, not
    to select anything.  It over-selects heavily, which is exactly what
    is wanted: every cell that ought to be occupied is present, with a
    coefficient large enough to earn a small weight, while the cells
    that ought to be empty carry small coefficients and large weights.
    """
    lam_max = float(np.max(np.abs(Zs.T @ y)))
    lams = np.exp(np.linspace(log(lam_max), log(lam_max * L.LAMRATIO),
                              L.NLAM))
    cv = np.zeros(L.NLAM)
    for fold in np.array_split(np.arange(L.T), min(L.NFOLDS, L.T)):
        pk = np.setdiff1d(np.arange(L.T), fold)
        co = L.lasso_path(Zs[pk, :], y[pk], lams)
        cv += np.sum((co @ Zs[fold, :].T - y[fold]) ** 2, axis=1)
    path = L.lasso_path(Zs, y, lams)
    return path[int(np.argmin(cv))] / nrm


def greedy_supports(X, y):
    out = {}
    for chain in L.greedy_row(X.T, y):
        out.setdefault(len(chain), []).append(tuple(sorted(chain)))
    return out


def score_support(X, y, cols):
    """The three quantities the criterion needs, for one subset."""
    Xs = X[list(cols), :]
    c, *_ = np.linalg.lstsq(Xs.T, y, rcond=None)
    res = y - Xs.T @ c
    a = np.abs(c)
    if np.any(a < L.AMIN) or np.any(a > L.AMAX):
        return None
    return (float(res @ res), float(np.sum(np.log(a))),
            float(np.linalg.slogdet(Xs @ Xs.T)[1]), c)


def build_tables(X, Y, extra=None):
    """Row tables holding several candidates for each size.

    `extra` supplies, for each row, a further dictionary of supports to
    merge with the ones this function generates.
    """
    Rt = np.full((L.N, L.KMAX + 1, NCAND), inf)
    gt = np.full((L.N, L.KMAX + 1, NCAND), inf)
    st = np.zeros((L.N, L.KMAX + 1, NCAND))
    store = [[[None] * NCAND for _ in range(L.KMAX + 1)] for _ in range(L.N)]
    keep = []

    for i in range(L.N):
        y = Y[i]
        cand = greedy_supports(X, y)
        path, Zs, nrm = plain_path_supports(X, y)
        keep.append((Zs, nrm))
        for k, v in path.items():
            cand.setdefault(k, [])
            for c in v:
                if c not in cand[k]:
                    cand[k].append(c)
        if extra is not None:
            for k, v in extra[i].items():
                cand.setdefault(k, [])
                for c in v:
                    if c not in cand[k]:
                        cand[k].append(c)

        Rt[i, 0, 0] = float(y @ y)
        gt[i, 0, 0] = 0.0
        store[i][0][0] = (tuple(), np.zeros(0))
        for k, subs in cand.items():
            # Cheapest first, so that the NCAND kept are the plausible ones.
            scored = []
            for c in subs:
                s = score_support(X, y, c)
                if s is not None:
                    scored.append((s[0], c, s))
            scored.sort(key=lambda t: t[0])
            for j, (_, c, s) in enumerate(scored[:NCAND]):
                Rt[i, k, j], gt[i, k, j], st[i, k, j] = s[0], s[1], s[2]
                store[i][k][j] = (c, s[3])
    return Rt, gt, st, store, keep


# ------------------------------------------------------------- the search


def search(Rt, gt, st, store):
    """As in large.py, but choosing among candidates of each size."""
    sizes = np.arange(L.KMAX + 1)[:, None]
    Ms = np.arange(L.MMAX + 1)
    best = None

    for sig_i, sigma in enumerate(np.exp(np.linspace(log(L.S0), log(L.S1),
                                                     L.NSIGMA))):
        inv2s2 = 1.0 / (2.0 * sigma * sigma)
        half = 0.5 * log(2.0 * pi * sigma * sigma)
        full = Rt * inv2s2 + gt + 0.5 * st - sizes * half   # N, K+1, NCAND
        pick = np.argmin(np.where(np.isfinite(full), full, inf), axis=2)
        cost = np.take_along_axis(full, pick[:, :, None], axis=2)[:, :, 0]

        dp = np.full(L.MMAX + 1, inf)
        aR = np.zeros(L.MMAX + 1)
        ag = np.zeros(L.MMAX + 1)
        aS = np.zeros(L.MMAX + 1)
        dp[0] = 0.0
        choice = np.full((L.N, L.MMAX + 1), -1, dtype=np.int8)

        for i in range(L.N):
            new = np.full(L.MMAX + 1, inf)
            nR = np.zeros(L.MMAX + 1)
            ng = np.zeros(L.MMAX + 1)
            nS = np.zeros(L.MMAX + 1)
            for k in range(L.KMAX + 1):
                c = cost[i, k]
                if not np.isfinite(c):
                    continue
                j = pick[i, k]
                cand = dp[:L.MMAX + 1 - k] + c
                tgt = new[k:]
                better = cand < tgt
                if not better.any():
                    continue
                tgt[better] = cand[better]
                nR[k:][better] = aR[:L.MMAX + 1 - k][better] + Rt[i, k, j]
                ng[k:][better] = ag[:L.MMAX + 1 - k][better] + gt[i, k, j]
                nS[k:][better] = aS[:L.MMAX + 1 - k][better] + st[i, k, j]
                choice[i, k:][better] = k
            dp, aR, ag, aS = new, nR, ng, nS

        live = np.isfinite(dp) & (aR > 0.0)
        if not live.any():
            continue
        val = np.full(L.MMAX + 1, inf)
        val[live] = L.phi_parts(Ms[live], aR[live], ag[live], aS[live])
        m = int(np.argmin(val))
        if np.isfinite(val[m]) and (best is None or val[m] < best[0]):
            best = (float(val[m]), m, choice.copy(), pick.copy())

    if best is None:
        return None, None
    value, M, choice, pick = best
    A = np.zeros((L.N, L.N))
    MM = M
    for i in range(L.N - 1, -1, -1):
        k = int(choice[i, MM])
        if k < 0:
            return None, None
        if k:
            cols, coef = store[i][k][pick[i, k]]
            A[i, list(cols)] = coef
        MM -= k
    return A, value


# ------------------------------------------------------------------ main


def run_case(X, Y, A_true, lines):
    t0 = time.time()
    Rt, gt, st, store, keep = build_tables(X, Y)
    A, v = search(Rt, gt, st, store)
    A = L.refine(X, Y, A)
    s = L.score(A, A != 0.0, A_true, X, Y)
    lines.append(f"  path      M={s['M']:5d} correct {s['correct']:5d} "
                 f"false {s['false']:4d} missed {s['missed']:4d} "
                 f"sigma {s['sigma']:.4f} error {s['frob']:6.2f} "
                 f"Phi {v:10.1f}  [{time.time() - t0:.0f}s]")

    # An adaptive path, weighted by a fit that contains the cells we are
    # trying to recover.
    #
    # Weighting by the answer Phi has just given does not work, and the
    # reason is worth recording: a cell that answer leaves empty is
    # given the largest weight the rule allows, so the adaptive path can
    # never bring it back.  The weights must come from a fit that is too
    # dense rather than too sparse.  Cross-validation supplies one, and
    # it needs no estimate of the error scale, so nothing an analyst
    # lacks enters here.
    t0 = time.time()
    extra = []
    for i in range(L.N):
        Zs, nrm = keep[i]
        init = cv_fit(X, Y[i], Zs, nrm)
        extra.append(adaptive_path_supports(X, Y[i], init, Zs, nrm))
    Rt, gt, st, store, _ = build_tables(X, Y, extra=extra)
    A2, v2 = search(Rt, gt, st, store)
    A2 = L.refine(X, Y, A2)
    s2 = L.score(A2, A2 != 0.0, A_true, X, Y)
    lines.append(f"  adaptive  M={s2['M']:5d} correct {s2['correct']:5d} "
                 f"false {s2['false']:4d} missed {s2['missed']:4d} "
                 f"sigma {s2['sigma']:.4f} error {s2['frob']:6.2f} "
                 f"Phi {v2:10.1f}  [{time.time() - t0:.0f}s]")
    return s, s2


def main(nseeds=2, cases=("block", "scatter"), start=0):
    lines = [f"N={L.N} T={L.T} n={L.n} q={L.q}  candidates from the penalty "
             f"path, Phi as judge", ""]
    for si in range(start, start + nseeds):
        seed = 20260726 + 1000 * si
        A_block, A_scatter, X, E = L.build(seed)
        for case in cases:
            A_true = A_block if case == "block" else A_scatter
            Y = A_true @ X + E
            lines.append(f"seed {seed}, {case}, true cells "
                         f"{int(np.sum(A_true != 0))}")
            t0 = time.time()
            Ag = L.refine(X, Y, L.search(X, Y))
            sg = L.score(Ag, Ag != 0.0, A_true, X, Y)
            lines.append(f"  greedy    M={sg['M']:5d} correct "
                         f"{sg['correct']:5d} false {sg['false']:4d} "
                         f"missed {sg['missed']:4d} sigma {sg['sigma']:.4f} "
                         f"error {sg['frob']:6.2f}"
                         f"{'':17}[{time.time() - t0:.0f}s]")
            run_case(X, Y, A_true, lines)
            lines.append("")
            print("\n".join(lines[-5:]))
            sys.stdout.flush()
    text = "\n".join(lines)
    with open("large_lasso_phi.txt", "w") as f:
        f.write(text + "\n")


if __name__ == "__main__":
    ns = int(sys.argv[1]) if len(sys.argv) > 1 else 2
    cs = sys.argv[2].split(",") if len(sys.argv) > 2 else ["block", "scatter"]
    st = int(sys.argv[3]) if len(sys.argv) > 3 else 0
    main(ns, cs, st)
