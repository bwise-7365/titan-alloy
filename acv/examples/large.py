"""
large.py
----------------------------------------------------------------------
The synthetic study at the size of a real econometric problem: twenty
countries of ten variables each, so that A is 200 by 200 and holds
forty thousand cells.

The arrangement is the one an economist described.  Each country is a
solid ten by ten block, and each ordered pair of countries is joined by
one or two cells.  That gives about 2570 occupied cells, or one cell in
fifteen.  There are eighty quarters of data, so T = 80 and the row of
each equation has eighty observations for about thirteen coefficients.
Per row the problem is therefore easier than the nine by nine
illustration, where six observations carried three coefficients.  What
is harder is the search.

Three departures from examples/compete.py are forced by the size, and
each is described in Section 12 of the document.

  1.  The pattern of each row is chosen by forward selection rather
      than by enumerating subsets.  Enumeration needs C(200, 13) fits
      per row, which is 10^20, and no cap small enough to be affordable
      is large enough to hold the truth.

  2.  The integral over sigma is taken in the closed form of
      Section 6.3 rather than by quadrature.  With n = 16000 the
      integrand is concentrated in a range of width about 0.006 in
      log sigma, which a grid of two thousand points over a range of
      six does not resolve.

  3.  The criterion is accumulated through the dynamic programme rather
      than recomputed on materialised patterns.  Phi depends on a
      pattern only through M and three sums taken over rows, so those
      sums are carried alongside the cost and Phi is evaluated for
      every pair of sigma and M without any pattern being built.

The cross-validation folds and the matrix inverse are as amended in
compete.py.
----------------------------------------------------------------------
"""

import sys
import numpy as np
from math import log, sqrt, lgamma, inf, pi
from scipy.special import betaln, gammainc
from scipy.linalg import cho_factor, cho_solve, solve_triangular

# ------------------------------------------------------------ constants

NCOUNTRY, NVAR = 20, 10
N = NCOUNTRY * NVAR                     # 200
T = 80                                  # twenty years of quarters
q = N * N                               # 40000
n = N * T                               # 16000

AMIN, AMAX = 0.20, 5.0
LA = log(AMAX / AMIN)

S0, S1 = 0.02, 10.0

SIGMA_TRUE = 0.10
MAGLO, MAGHI = 0.40, 3.00

# The occupied fraction is about 2570 in 40000, which is close to 2/29.
# A Beta with mean 2/29 has alpha = 2 and beta = 27; the density is then
# proportional to p (1-p)^26.
ALPHA, BETA = 2.0, 27.0

KMAX = 25                               # largest number of cells in a row
MMAX = KMAX * N
NSIGMA = 240

# lasso
NLAM = 60
NFOLDS = 5
LAMRATIO = 1.0e-3
CDITERS = 1000
CDTOL = 1.0e-9

# ssvs
SSVS_TAU0 = 0.02
SSVS_TAU1 = 1.50
SSVS_DRAWS = 8000
SSVS_BURN = 2000
SSVS_A0 = 0.001
SSVS_B0 = 0.001


# ----------------------------------------------------------- generation


def build(seed):
    """The block-and-link matrix, a scattered control, and the data.

    The scattered matrix carries the same number of cells in each row as
    the block matrix, placed at random columns, so that the two differ
    in arrangement alone.  Both are driven by the same inputs and the
    same errors within a seed, so the comparison is paired.
    """
    rng = np.random.default_rng(seed)

    def draw(m):
        mags = np.exp(rng.uniform(log(MAGLO), log(MAGHI), size=m))
        return np.round(mags * rng.choice([-1.0, 1.0], size=m), 2)

    A_block = np.zeros((N, N))

    # the solid diagonal blocks
    for c in range(NCOUNTRY):
        lo = c * NVAR
        A_block[lo:lo + NVAR, lo:lo + NVAR] = draw(NVAR * NVAR).reshape(NVAR,
                                                                       NVAR)
    # one or two cells joining each ordered pair of countries
    for c in range(NCOUNTRY):
        for d in range(NCOUNTRY):
            if c == d:
                continue
            k = int(rng.integers(1, 3))
            r = rng.integers(0, NVAR, size=k) + c * NVAR
            s = rng.integers(0, NVAR, size=k) + d * NVAR
            for a, b in zip(r, s):
                if A_block[a, b] == 0.0:
                    A_block[a, b] = draw(1)[0]

    # the same row counts, placed at random
    A_scatter = np.zeros((N, N))
    for i in range(N):
        vals = A_block[i][A_block[i] != 0.0]
        cols = rng.choice(N, size=vals.size, replace=False)
        A_scatter[i, cols] = vals

    X = rng.standard_normal((N, T))
    E = rng.standard_normal((N, T)) * SIGMA_TRUE
    return A_block, A_scatter, X, E


# ------------------------------------------------------------ criterion


def log_pattern(M):
    return -betaln(M + ALPHA, q - M + BETA) + betaln(ALPHA, BETA)


def log_sigma_integral(R, M):
    """Logarithm of the integral of sigma^(M-n-1) exp(-R/2 sigma^2) over
    the interval [s0, s1], in the closed form of Section 6.3.

    Substituting v = 1/sigma^2 turns the integral into an incomplete
    gamma function.  Quadrature was used at n = 54 and is not safe at
    n = 16000, where the integrand occupies a range of width about
    0.006 in log sigma.  Both arguments may be arrays.
    """
    R = np.asarray(R, dtype=float)
    M = np.asarray(M, dtype=float)
    a = 0.5 * (n - M)
    x0 = R / (2.0 * S0 * S0)
    x1 = R / (2.0 * S1 * S1)
    span = gammainc(a, x0) - gammainc(a, x1)
    span = np.maximum(span, 1.0e-300)
    lg = np.vectorize(lgamma)
    return -log(2.0) + a * np.log(2.0 / R) + lg(a) + np.log(span)


def phi_parts(M, R, g, sld):
    """Phi from the number of cells and the three sums taken over rows.

    Phi depends on a pattern only through these four quantities, which
    is what allows the dynamic programme to carry them and evaluate the
    criterion without ever building the pattern.
    """
    return (log_pattern(M) + M * log(2.0 * LA) + g + 0.5 * sld
            - 0.5 * M * log(2.0 * pi) - log_sigma_integral(R, M))


# ------------------------------------------------------- row selection


def greedy_row(Z, y):
    """Forward selection for one row, one cell at a time.

    At each step the column admitted is the one that reduces the
    residual sum of squares by the most, which is the exact forward
    step and not an approximation to it.  Holding an orthonormal basis
    of the columns already admitted makes the reduction available for
    every candidate at once: for a candidate j the reduction is the
    square of its inner product with the residual, divided by the
    squared length of the part of that column orthogonal to the basis.

    The selection does not depend on sigma, so it is done once and
    serves the whole grid.  This is where exactness is given up: the
    subset of size k is the one forward selection reaches, not the best
    subset of size k.
    """
    Tn, Nn = Z.shape
    Q = np.zeros((Tn, KMAX))
    chosen = []
    taken = np.zeros(Nn, dtype=bool)
    r = y.copy()
    sq = np.sum(Z * Z, axis=0)
    proj = np.zeros(Nn)                 # squared length inside the basis
    out = []
    for k in range(KMAX):
        perp = sq - proj
        num = Z.T @ r
        live = (perp > 1.0e-10) & ~taken
        gain = np.full(Nn, -inf)
        np.divide(num * num, perp, out=gain, where=live)
        j = int(np.argmax(gain))
        if not np.isfinite(gain[j]) or gain[j] <= 0.0:
            break
        w = Z[:, j] - Q[:, :k] @ (Q[:, :k].T @ Z[:, j])
        nw = float(np.linalg.norm(w))
        if nw < 1.0e-8:
            break
        Q[:, k] = w / nw
        proj = proj + (Q[:, k] @ Z) ** 2
        r = r - Q[:, k] * float(Q[:, k] @ y)
        taken[j] = True
        chosen.append(j)
        out.append(list(chosen))
    return out


def row_table(X, Y, i):
    """The quantities the dynamic programme needs, for one row and every
    size from zero to KMAX.

    A size whose least-squares coefficients fall outside the range of
    the prior is marked infeasible, exactly as in the small study.
    """
    Z = X.T
    y = Y[i]
    paths = greedy_row(Z, y)
    K = len(paths)
    R = np.full(KMAX + 1, inf)
    g = np.full(KMAX + 1, inf)
    sld = np.zeros(KMAX + 1)
    cols = [None] * (KMAX + 1)
    coef = [None] * (KMAX + 1)

    R[0] = float(y @ y)
    g[0] = 0.0
    cols[0] = []
    coef[0] = np.zeros(0)

    for k in range(1, K + 1):
        s = paths[k - 1]
        Xs = X[s, :]
        c, *_ = np.linalg.lstsq(Xs.T, y, rcond=None)
        res = y - Xs.T @ c
        R[k] = float(res @ res)
        a = np.abs(c)
        if np.all(a >= AMIN) and np.all(a <= AMAX):
            g[k] = float(np.sum(np.log(a)))
        sld[k] = float(np.linalg.slogdet(Xs @ Xs.T)[1])
        cols[k] = list(s)
        coef[k] = c
    return R, g, sld, cols, coef


# ------------------------------------------------------------- the search


def search(X, Y, verbose=False, want_value=False):
    """Choose the pattern by minimising Phi.

    For a fixed sigma the criterion separates over rows, and the
    dynamic programme finds, for every total M, the allocation of cells
    to rows that minimises the separable part.  Carried alongside the
    cost are the three sums Phi needs, so that Phi itself is evaluated
    for every pair of sigma and M directly.  Only the winning allocation
    is ever turned into a matrix.
    """
    tables = [row_table(X, Y, i) for i in range(N)]
    Rt = np.array([t[0] for t in tables])           # N by KMAX+1
    gt = np.array([t[1] for t in tables])
    st = np.array([t[2] for t in tables])
    ok = np.isfinite(Rt) & np.isfinite(gt)

    sizes = np.arange(KMAX + 1)
    Ms = np.arange(MMAX + 1)
    best = None

    for sigma in np.exp(np.linspace(log(S0), log(S1), NSIGMA)):
        inv2s2 = 1.0 / (2.0 * sigma * sigma)
        half = 0.5 * log(2.0 * pi * sigma * sigma)
        cost = np.where(ok, Rt * inv2s2 + gt + 0.5 * st - sizes * half, inf)

        dp = np.full(MMAX + 1, inf)
        aR = np.zeros(MMAX + 1)
        ag = np.zeros(MMAX + 1)
        aS = np.zeros(MMAX + 1)
        dp[0] = 0.0
        choice = np.full((N, MMAX + 1), -1, dtype=np.int8)

        for i in range(N):
            new = np.full(MMAX + 1, inf)
            nR = np.zeros(MMAX + 1)
            ng = np.zeros(MMAX + 1)
            nS = np.zeros(MMAX + 1)
            for k in range(KMAX + 1):
                c = cost[i, k]
                if not np.isfinite(c):
                    continue
                cand = dp[:MMAX + 1 - k] + c
                tgt = new[k:]
                better = cand < tgt
                if not better.any():
                    continue
                tgt[better] = cand[better]
                nR[k:][better] = aR[:MMAX + 1 - k][better] + Rt[i, k]
                ng[k:][better] = ag[:MMAX + 1 - k][better] + gt[i, k]
                nS[k:][better] = aS[:MMAX + 1 - k][better] + st[i, k]
                choice[i, k:][better] = k
            dp, aR, ag, aS = new, nR, ng, nS

        live = np.isfinite(dp) & (aR > 0.0)
        if not live.any():
            continue
        val = np.full(MMAX + 1, inf)
        val[live] = phi_parts(Ms[live], aR[live], ag[live], aS[live])
        m = int(np.argmin(val))
        if np.isfinite(val[m]) and (best is None or val[m] < best[0]):
            best = (float(val[m]), m, choice.copy(), float(aR[m]))

    if best is None:
        return None
    if want_value:
        return best[0], best[1], best[3]
    _, M, choice, _ = best
    A = np.zeros((N, N))
    MM = M
    for i in range(N - 1, -1, -1):
        k = int(choice[i, MM])
        if k < 0:
            return None
        if k:
            A[i, tables[i][3][k]] = tables[i][4][k]
        MM -= k
    return A


def refine(X, Y, A, iters=40):
    """Move the coefficients to the stationary point, with the guard of
    the small study against the iteration running away."""
    A = A.copy()
    for _ in range(iters):
        R = float(np.sum((Y - A @ X) ** 2))
        prev = A.copy()
        for i in range(N):
            c = np.where(A[i] != 0.0)[0]
            if c.size == 0:
                continue
            Xs = X[c, :]
            try:
                A[i, c] = np.linalg.solve(Xs @ Xs.T,
                                          Xs @ Y[i] - (R / n) / A[i, c])
            except np.linalg.LinAlgError:
                pass
        if not np.all(np.isfinite(A)):
            return prev
        nz = np.abs(A[A != 0.0])
        if nz.size and (np.any(nz < AMIN) or np.any(nz > AMAX)):
            return prev
        if np.max(np.abs(A - prev)) < 1e-12:
            break
    return A


# ---------------------------------------------------------------- lasso


def standardise(Z):
    nrm = np.sqrt(np.sum(Z * Z, axis=0))
    nrm[nrm == 0.0] = 1.0
    return Z / nrm, nrm


def lasso_path(Z, y, lams):
    """Coordinate descent along the whole path, warm started downwards.

    Sweeping all two hundred coordinates every time is wasteful, since
    only a few tens of them are ever away from zero.  After each full
    sweep the descent is confined to the coordinates currently away
    from zero until they settle, and a further full sweep then checks
    whether any of the rest should enter.  The rule is the one used by
    the standard implementations; it reaches the same minimum, since the
    problem is convex and the final sweep is over all coordinates.
    """
    p = Z.shape[1]
    zz = np.sum(Z * Z, axis=0)
    a = np.zeros(p)
    out = np.empty((len(lams), p))
    r = y.copy()

    def sweep(idx, lam):
        biggest = 0.0
        for j in idx:
            aj = a[j]
            rho = float(Z[:, j] @ r) + zz[j] * aj
            new = np.sign(rho) * max(abs(rho) - lam, 0.0) / zz[j]
            if new != aj:
                r[:] -= Z[:, j] * (new - aj)
                a[j] = new
                biggest = max(biggest, abs(new - aj))
        return biggest

    allj = range(p)
    for kk, lam in enumerate(lams):
        for _ in range(CDITERS):
            if sweep(allj, lam) < CDTOL:
                break
            act = np.nonzero(a)[0]
            for _ in range(CDITERS):
                if sweep(act, lam) < CDTOL:
                    break
        out[kk] = a
    return out


def lasso_row_variants(X, y, a_true_row):
    """The four variants of Section 11, for one row."""
    Z = X.T
    Zs, nrm = standardise(Z)
    lam_max = float(np.max(np.abs(Zs.T @ y)))
    lams = np.exp(np.linspace(log(lam_max), log(lam_max * LAMRATIO), NLAM))
    path = lasso_path(Zs, y, lams)

    cv = np.zeros(NLAM)
    for fold in np.array_split(np.arange(T), min(NFOLDS, T)):
        keep = np.setdiff1d(np.arange(T), fold)
        co = lasso_path(Zs[keep, :], y[keep], lams)
        cv += np.sum((co @ Zs[fold, :].T - y[fold]) ** 2, axis=1)

    resid = y[None, :] - path @ Zs.T
    bic = (np.sum(resid * resid, axis=1) / SIGMA_TRUE ** 2
           + np.sum(path != 0.0, axis=1) * log(n))

    tru = a_true_row != 0.0
    hit = path != 0.0
    errs = np.sum(hit & ~tru, axis=1) + np.sum(~hit & tru, axis=1)

    out = {"lasso-cv": path[int(np.argmin(cv))] / nrm,
           "lasso-bic": path[int(np.argmin(bic))] / nrm,
           "lasso-best": path[int(np.argmin(errs))] / nrm}

    w = 1.0 / np.maximum(np.abs(out["lasso-bic"]), 0.05)
    Zw = Zs / w
    lam_w = float(np.max(np.abs(Zw.T @ y)))
    lw = np.exp(np.linspace(log(lam_w), log(lam_w * LAMRATIO), NLAM))
    pw = lasso_path(Zw, y, lw) / w
    resid = y[None, :] - pw @ Zs.T
    bw = (np.sum(resid * resid, axis=1) / SIGMA_TRUE ** 2
          + np.sum(pw != 0.0, axis=1) * log(n))
    out["alasso-bic"] = pw[int(np.argmin(bw))] / nrm
    return out


# ----------------------------------------------------------------- ssvs


def ssvs(X, Y, seed, tau0=SSVS_TAU0, tau1=SSVS_TAU1,
         draws=SSVS_DRAWS, burn=SSVS_BURN):
    """As in compete.py, with the row drawn from its Cholesky factor
    rather than from an explicit inverse.

    Forming the inverse is unnecessary: the mean solves one triangular
    system and the deviation is a second, so the draw costs a factor of
    three less than the inverse alone.
    """
    rng = np.random.default_rng(seed)
    Z = X.T
    ZtZ = Z.T @ Z
    Zty = Z.T @ Y.T                                 # N by N, column i per row
    A = np.zeros((N, N))
    gam = np.zeros((N, N), dtype=bool)
    sig2 = float(SIGMA_TRUE ** 2)
    p = 0.1
    pip = np.zeros((N, N))
    kept = 0
    v0, v1 = tau0 * tau0, tau1 * tau1
    hlr = 0.5 * log(v0 / v1)

    for it in range(draws):
        for i in range(N):
            dinv = np.where(gam[i], 1.0 / v1, 1.0 / v0)
            L = np.linalg.cholesky(ZtZ / sig2 + np.diag(dinv))
            mu = cho_solve((L, True), Zty[:, i] / sig2)
            z = rng.standard_normal(N)
            A[i] = mu + solve_triangular(L, z, trans='T', lower=True)

        lodds = (log(p) - log(1.0 - p) + hlr
                 + 0.5 * A * A * (1.0 / v0 - 1.0 / v1))
        gam = rng.random((N, N)) < 1.0 / (1.0 + np.exp(np.clip(-lodds,
                                                              -500, 500)))
        M = int(np.sum(gam))
        p = float(rng.beta(ALPHA + M, BETA + q - M))
        p = min(max(p, 1.0e-9), 1.0 - 1.0e-9)

        Rm = Y - A @ X
        R = float(np.sum(Rm * Rm))
        sig2 = float(1.0 / rng.gamma(SSVS_A0 + 0.5 * n,
                                     1.0 / (SSVS_B0 + 0.5 * R)))
        if it >= burn:
            pip += gam
            kept += 1
    return pip / kept


# -------------------------------------------------------------- scoring


def refit(X, Y, mask):
    A = np.zeros((N, N))
    for i in range(N):
        c = np.where(mask[i])[0]
        if c.size == 0:
            continue
        Xs = X[c, :]
        co, *_ = np.linalg.lstsq(Xs.T, Y[i], rcond=None)
        A[i, c] = co
    return A


def score(A, mask, A_true, X, Y):
    tru = A_true != 0.0
    M = int(np.sum(mask))
    R = float(np.sum((Y - A @ X) ** 2))
    return dict(M=M,
                correct=int(np.sum(mask & tru)),
                false=int(np.sum(mask & ~tru)),
                missed=int(np.sum(~mask & tru)),
                sigma=float(sqrt(R / max(n - M, 1))),
                frob=float(sqrt(np.sum((A - A_true) ** 2))))


LASSOS = ["lasso-cv", "lasso-bic", "alasso-bic", "lasso-best"]
METHODS = ["mple"] + LASSOS + ["ssvs"]


def one_case(X, Y, A_true, seed, methods):
    import time
    out, times = {}, {}

    if "mple" in methods:
        t0 = time.time()
        A = search(X, Y)
        if A is not None:
            A = refine(X, Y, A)
            out["mple"] = score(A, A != 0.0, A_true, X, Y)
        times["mple"] = time.time() - t0

    if any(m in methods for m in LASSOS):
        t0 = time.time()
        raw = {t: np.zeros((N, N)) for t in LASSOS}
        for i in range(N):
            vs = lasso_row_variants(X, Y[i], A_true[i])
            for t in LASSOS:
                raw[t][i] = vs[t]
        for t in LASSOS:
            if t not in methods:
                continue
            m = raw[t] != 0.0
            out[t] = score(refit(X, Y, m), m, A_true, X, Y)
        times["lasso"] = time.time() - t0

    if "ssvs" in methods:
        t0 = time.time()
        m = ssvs(X, Y, seed) >= 0.5
        out["ssvs"] = score(refit(X, Y, m), m, A_true, X, Y)
        times["ssvs"] = time.time() - t0

    return out, times


def main(seeds, cases, methods, outfile="large.txt"):
    rows = []
    print(f"N={N} T={T} q={q} n={n} KMAX={KMAX} "
          f"alpha={ALPHA} beta={BETA}")
    print(f"{'seed':>10} {'case':<8} {'method':<11} {'M':>5} {'corr':>5}"
          f" {'false':>6} {'miss':>5} {'sigma':>7} {'frob':>8}")
    for s in seeds:
        A_block, A_scatter, X, E = build(s)
        truth = {"block": A_block, "scatter": A_scatter}
        for case in cases:
            A_true = truth[case]
            Y = A_true @ X + E
            res, times = one_case(X, Y, A_true, s, methods)
            print(f"# seed {s} {case}: true cells {int(np.sum(A_true != 0))}"
                  f"   timings " +
                  "  ".join(f"{k} {v:.1f}s" for k, v in times.items()))
            for m in METHODS:
                if m not in res:
                    continue
                r = dict(res[m])
                r.update(seed=s, case=case, method=m)
                rows.append(r)
                print(f"{s:>10} {case:<8} {m:<11} {r['M']:5d} {r['correct']:5d}"
                      f" {r['false']:6d} {r['missed']:5d} {r['sigma']:7.4f}"
                      f" {r['frob']:8.2f}")
            sys.stdout.flush()

    with open(outfile, "w") as f:
        f.write(f"N={N} T={T} q={q} n={n} KMAX={KMAX} sigma_true={SIGMA_TRUE}"
                f" alpha={ALPHA} beta={BETA}\n")
        f.write(f"ssvs tau0={SSVS_TAU0} tau1={SSVS_TAU1} "
                f"draws={SSVS_DRAWS} burn={SSVS_BURN}\n")
        f.write(f"lasso nlam={NLAM} folds={NFOLDS}\n\n")
        f.write(f"{'seed':>10} {'case':<8} {'method':<11} {'M':>5} {'corr':>5}"
                f" {'false':>6} {'miss':>5} {'sigma':>7} {'frob':>8}\n")
        for r in rows:
            f.write(f"{r['seed']:>10} {r['case']:<8} {r['method']:<11} "
                    f"{r['M']:5d} {r['correct']:5d} {r['false']:6d} "
                    f"{r['missed']:5d} {r['sigma']:7.4f} {r['frob']:8.2f}\n")
    return rows


LABEL = {"mple": "this paper",
         "lasso-cv": "lasso, cross-validated",
         "lasso-bic": "lasso, Schwarz, true $\\sigma$",
         "alasso-bic": "adaptive lasso, Schwarz",
         "lasso-best": "lasso, penalty from the answer",
         "ssvs": "SSVS"}


def read_rows(path):
    rows = []
    for ln in open(path):
        p = ln.split()
        if len(p) == 9 and p[0].isdigit():
            rows.append(dict(seed=int(p[0]), case=p[1], method=p[2],
                             M=int(p[3]), correct=int(p[4]), false=int(p[5]),
                             missed=int(p[6]), sigma=float(p[7]),
                             frob=float(p[8])))
    return rows


def write_table(rows, path, methods=None):
    """The summary table for the large study, in the form of Section 12."""
    ms = methods or [m for m in METHODS
                     if any(r["method"] == m for r in rows)]
    cases = [c for c in ("block", "scatter")
             if any(r["case"] == c for r in rows)]
    out = ["\\begin{center}", "\\footnotesize",
           "\\begin{tabular}{llccccccc}",
           "case & method & trials & occupied & correct of & false & missed"
           " & median $\\hat\\sigma$ & median error \\\\", "\\hline"]
    for case in cases:
        if case != cases[0]:
            out.append("\\hline")
        for m in ms:
            rs = [r for r in rows if r["case"] == case and r["method"] == m]
            if not rs:
                continue
            f = lambda k: np.mean([r[k] for r in rs])
            # correct plus missed is the number of cells the truth holds
            true = f("correct") + f("missed")
            out.append(f"{case} & {LABEL[m]} & ${len(rs)}$ & "
                       f"${f('M'):.0f}$ & "
                       f"${f('correct'):.0f}$ of ${true:.0f}$ & "
                       f"${f('false'):.0f}$ & "
                       f"${f('missed'):.0f}$ & "
                       f"${np.median([r['sigma'] for r in rs]):.3f}$ & "
                       f"${np.median([r['frob'] for r in rs]):.2f}$ \\\\")
    out += ["\\end{tabular}", "\\end{center}", ""]
    with open(path, "w") as fh:
        fh.write("\n".join(out))


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--tables":
        write_table(read_rows("large.txt"), "tab_large.tex")
        write_table(read_rows("large_mple.txt"), "tab_large_mple.tex",
                    methods=["mple"])
        print("tab_large.tex and tab_large_mple.tex written")
        sys.exit(0)
    nseeds = int(sys.argv[1]) if len(sys.argv) > 1 else 3
    cases = (sys.argv[2].split(",") if len(sys.argv) > 2
             else ["block", "scatter"])
    methods = (sys.argv[3].split(",") if len(sys.argv) > 3 else METHODS)
    outfile = sys.argv[4] if len(sys.argv) > 4 else "large.txt"
    start = int(sys.argv[5]) if len(sys.argv) > 5 else 0
    main([20260726 + 1000 * i for i in range(start, start + nseeds)],
         cases, methods, outfile)
