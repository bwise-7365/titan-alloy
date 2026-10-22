"""
sweep.py
----------------------------------------------------------------------
Repeats the two illustrations of recover.py over many random seeds.

A single realisation cannot distinguish a property of block structure
from an accident of one draw.  This script settles it by repeating the
whole construction over many seeds.

For each seed the coefficient values, the inputs X, the measurement
errors, and the scattered pattern are all redrawn.  The block pattern is
fixed by construction, as three diagonal blocks of size three; only its
values change.  Both cases of a given seed share the same X and the same
errors, so within a seed the comparison is paired.

The procedure is the one settled on in the document: the pattern is
chosen by minimising Phi, equation (9.7), and the coefficient values are
then moved to the stationary point of equation (9.3).

The generation of the data reproduces recover.py call for call, so that
seed 20260726 gives the numbers already reported.  That agreement is
checked at the end.
----------------------------------------------------------------------
"""

import numpy as np
from itertools import combinations
from math import log, lgamma, inf
from scipy.special import betaln

N, T = 9, 6
q = N * N
n = N * T

AMIN, AMAX = 0.20, 5.0
LA = log(AMAX / AMIN)

S0, S1 = 0.02, 10.0
LSIG = log(S1 / S0)

SIGMA_TRUE = 0.10

# Range from which the true coefficient magnitudes are drawn.  It must
# lie strictly inside [AMIN, AMAX]; a floor set at the edge of the truth
# rejects borderline coefficients that noise pushes below it.
MAGLO, MAGHI = 0.40, 3.00
CAP = T - 1                     # largest number of occupied cells in a row
MMAX = CAP * N
ALPHA, BETA = 1.0, 1.0          # uniform hyper-prior on p
NSIGMA = 240

# Subsets of columns, restricted to the sizes the procedure allows.
SUBSETS = [tuple(c) for k in range(CAP + 1) for c in combinations(range(N), k)]
SIZES = np.array([len(s) for s in SUBSETS])
BY_SIZE = [np.where(SIZES == k)[0] for k in range(CAP + 1)]


def log_pattern(M):
    return -betaln(M + ALPHA, q - M + BETA) + betaln(ALPHA, BETA)


def log_sigma_integral(R, M):
    """log of the integral of sigma^(M-n-1) exp(-R/2 sigma^2) on [s0,s1]."""
    u = np.linspace(log(S0), log(S1), 2001)
    lg = -(n - M) * u - 0.5 * R * np.exp(-2.0 * u)
    top = lg.max()
    return top + log(np.trapezoid(np.exp(lg - top), u))


# ------------------------------------------------------------ generation


def build(seed):
    """Reproduces the data generation of recover.py, call for call."""
    rng = np.random.default_rng(seed)

    mags = np.exp(rng.uniform(log(MAGLO), log(MAGHI), size=27))
    signs = rng.choice([-1.0, 1.0], size=27)
    vals = np.round(mags * signs, 2)

    A_block = np.zeros((N, N))
    k = 0
    for b in range(3):
        for i in range(3 * b, 3 * b + 3):
            for j in range(3 * b, 3 * b + 3):
                A_block[i, j] = vals[k]
                k += 1

    while True:
        A_scatter = np.zeros((N, N))
        ok = True
        for i in range(N):
            cols = np.sort(rng.choice(N, size=3, replace=False))
            block_cols = np.arange(3 * (i // 3), 3 * (i // 3) + 3)
            if np.array_equal(cols, block_cols):
                ok = False
                break
            A_scatter[i, cols] = A_block[i, A_block[i] != 0.0]
        if ok:
            break

    X = rng.standard_normal((N, T))
    E = rng.standard_normal((N, T)) * SIGMA_TRUE
    return A_block, A_scatter, X, E


# --------------------------------------------------------------- fitting


def logdets(X):
    """log|X_S X_S'| for every subset, which depends on X alone."""
    out = np.zeros(len(SUBSETS))
    for idx, sub in enumerate(SUBSETS):
        if sub:
            Xs = X[list(sub), :]
            out[idx] = float(np.linalg.slogdet(Xs @ Xs.T)[1])
    return out


def row_table(X, y):
    """Least-squares fit of every subset to one row."""
    Rs = np.empty(len(SUBSETS))
    gs = np.empty(len(SUBSETS))
    feas = np.zeros(len(SUBSETS), dtype=bool)
    coefs = [None] * len(SUBSETS)
    for idx, sub in enumerate(SUBSETS):
        if not sub:
            Rs[idx] = float(y @ y)
            gs[idx] = 0.0
            feas[idx] = True
            coefs[idx] = np.zeros(0)
            continue
        Xs = X[list(sub), :]
        c, *_ = np.linalg.lstsq(Xs.T, y, rcond=None)
        r = y - Xs.T @ c
        Rs[idx] = float(r @ r)
        a = np.abs(c)
        if np.all(a >= AMIN) and np.all(a <= AMAX):
            feas[idx] = True
            gs[idx] = float(np.sum(np.log(a)))
        else:
            gs[idx] = inf
        coefs[idx] = c
    return Rs, gs, feas, coefs


def search(X, Y, slds):
    """Choose the pattern by minimising Phi.

    Phi does not separate by row, because of log(sum_i R_i) and the
    total M.  Holding sigma fixed removes the first coupling; dynamic
    programming over the rows then gives, for each total M, the exact
    minimiser of the separable form.  Those patterns are the candidates,
    and Phi is evaluated on each.
    """
    tables = [row_table(X, Y[i]) for i in range(N)]
    candidates = set()

    for sigma in np.exp(np.linspace(log(S0), log(S1), NSIGMA)):
        inv2s2 = 1.0 / (2.0 * sigma * sigma)
        half = 0.5 * log(2.0 * np.pi * sigma * sigma)

        cost_k = np.full((N, CAP + 1), inf)
        idx_k = np.full((N, CAP + 1), -1, dtype=int)
        for i, (Rs, gs, feas, _) in enumerate(tables):
            c = np.where(feas, Rs * inv2s2 + gs + 0.5 * slds - SIZES * half, inf)
            for k in range(CAP + 1):
                sel = BY_SIZE[k]
                j = sel[np.argmin(c[sel])]
                if np.isfinite(c[j]):
                    cost_k[i, k] = c[j]
                    idx_k[i, k] = j

        dp = np.full(MMAX + 1, inf)
        dp[0] = 0.0
        choice = np.full((N, MMAX + 1), -1, dtype=int)
        for i in range(N):
            new = np.full(MMAX + 1, inf)
            for k in range(CAP + 1):
                if not np.isfinite(cost_k[i, k]):
                    continue
                cand = dp[:MMAX + 1 - k] + cost_k[i, k]
                tgt = new[k:]
                better = cand < tgt
                tgt[better] = cand[better]
                choice[i, k:][better] = k
            dp = new

        for M in range(MMAX + 1):
            if not np.isfinite(dp[M]):
                continue
            alloc, MM = [], M
            for i in range(N - 1, -1, -1):
                k = choice[i, MM]
                if k < 0:
                    alloc = None
                    break
                alloc.append(k)
                MM -= k
            if alloc is None or MM != 0:
                continue
            alloc.reverse()
            # Carry the subsets the dynamic programme actually chose at
            # this sigma, not merely how many cells it gave each row.
            candidates.add(tuple(int(idx_k[i, alloc[i]]) for i in range(N)))

    best = None
    for picks in candidates:
        A = np.zeros((N, N))
        for i, j in enumerate(picks):
            sub = SUBSETS[j]
            if sub:
                A[i, list(sub)] = tables[i][3][j]
        val = phi(X, Y, A)
        if best is None or val < best[0]:
            best = (val, A)
    return best


def phi(X, Y, A, slds=None):
    Rm = Y - A @ X
    R = float(np.sum(Rm * Rm))
    nz = A[A != 0.0]
    M = int(nz.size)
    a = np.abs(nz)
    if M and (np.any(a < AMIN) or np.any(a > AMAX)):
        return inf
    sld = 0.0
    for i in range(N):
        cols = np.where(A[i] != 0.0)[0]
        if cols.size:
            Xs = X[cols, :]
            sld += float(np.linalg.slogdet(Xs @ Xs.T)[1])
    return (log_pattern(M) + M * log(2.0 * LA) + float(np.sum(np.log(a)))
            + 0.5 * sld - 0.5 * M * log(2.0 * np.pi)
            - log_sigma_integral(R, M))


def refine(X, Y, A, iters=60):
    A = A.copy()
    for _ in range(iters):
        R = float(np.sum((Y - A @ X) ** 2))
        prev = A.copy()
        for i in range(N):
            cols = np.where(A[i] != 0.0)[0]
            if cols.size == 0:
                continue
            Xs = X[cols, :]
            a = A[i, cols]
            try:
                A[i, cols] = np.linalg.solve(Xs @ Xs.T, Xs @ Y[i] - (R / n) / a)
            except np.linalg.LinAlgError:
                pass
        if not np.all(np.isfinite(A)):
            return prev
        nz = np.abs(A[A != 0.0])
        if nz.size and (np.any(nz < AMIN) or np.any(nz > AMAX)):
            # The iteration divides by the coefficient, so it can run away
            # when a coefficient is small.  Equation (9.3) shifts the
            # least-squares point by a quantity of order R/n; a step that
            # leaves the range of the prior is not that shift, and the
            # least-squares values are kept instead.
            return prev
        if np.max(np.abs(A - prev)) < 1e-12:
            break
    return A


def analyse(X, Y, A_true, slds):
    res = search(X, Y, slds)
    if res is None:
        return None
    A = refine(X, Y, res[1])
    hat = A != 0.0
    tru = A_true != 0.0
    M = int(np.sum(hat))
    R = float(np.sum((Y - A @ X) ** 2))
    return dict(M=M,
                correct=int(np.sum(hat & tru)),
                false=int(np.sum(hat & ~tru)),
                missed=int(np.sum(~hat & tru)),
                sigma=float(np.sqrt(R / max(n - M, 1))),
                frob=float(np.sqrt(np.sum((A - A_true) ** 2))))


# ------------------------------------------------------------------ main

if __name__ == "__main__":
    import sys

    nseeds = int(sys.argv[1]) if len(sys.argv) > 1 else 40
    seeds = [20260726] + [20260726 + 1000 * i for i in range(1, nseeds)]

    rows = []
    print(f"{'seed':>10}  {'case':<8} {'M':>3} {'corr':>5} {'false':>6}"
          f" {'miss':>5} {'sigma':>7} {'frob':>7}")
    for s in seeds:
        A_block, A_scatter, X, E = build(s)
        slds = logdets(X)
        for name, A_true in (("block", A_block), ("scatter", A_scatter)):
            Y = A_true @ X + E
            r = analyse(X, Y, A_true, slds)
            r.update(seed=s, case=name)
            rows.append(r)
            print(f"{s:>10}  {name:<8} {r['M']:3d} {r['correct']:5d}"
                  f" {r['false']:6d} {r['missed']:5d} {r['sigma']:7.4f}"
                  f" {r['frob']:7.3f}")

    def summarise(case):
        rs = [r for r in rows if r["case"] == case]
        corr = np.array([r["correct"] for r in rs])
        fal = np.array([r["false"] for r in rs])
        mis = np.array([r["missed"] for r in rs])
        exact = int(np.sum((fal == 0) & (mis == 0)))
        return (f"  {case:<8} n={len(rs):3d}  correct {corr.mean():5.2f} of 27"
                f"   false {fal.mean():5.2f}   missed {mis.mean():5.2f}"
                f"   exact recoveries {exact:3d} ({100*exact/len(rs):.0f}%)")

    print("\n---------------- summary ----------------")
    print(summarise("block"))
    print(summarise("scatter"))

    b = {r["seed"]: r for r in rows if r["case"] == "block"}
    s_ = {r["seed"]: r for r in rows if r["case"] == "scatter"}
    err = lambda r: r["false"] + r["missed"]
    worse = sum(1 for k in b if err(b[k]) > err(s_[k]))
    better = sum(1 for k in b if err(b[k]) < err(s_[k]))
    same = len(b) - worse - better
    print(f"\n  paired within seed: block worse {worse}, "
          f"block better {better}, equal {same}")

    r0b, r0s = b[20260726], s_[20260726]
    print(f"\n  seed 20260726 check against recover.py:")
    print(f"    block   M={r0b['M']} correct={r0b['correct']} "
          f"false={r0b['false']} missed={r0b['missed']}"
          f"   (expected 28 / 27 / 1 / 0)")
    print(f"    scatter M={r0s['M']} correct={r0s['correct']} "
          f"false={r0s['false']} missed={r0s['missed']}"
          f"   (expected 27 / 27 / 0 / 0)")

    with open("sweep.txt", "w") as f:
        f.write(f"seeds={len(seeds)} N={N} T={T} sigma_true={SIGMA_TRUE}\n")
        for r in rows:
            f.write(f"{r['seed']} {r['case']} {r['M']} {r['correct']} "
                    f"{r['false']} {r['missed']} {r['sigma']:.4f} "
                    f"{r['frob']:.4f}\n")
