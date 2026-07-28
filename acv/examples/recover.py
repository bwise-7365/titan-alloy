"""
recover.py
----------------------------------------------------------------------
Two numerical illustrations for the acv preliminary document.

Case A: the occupied cells of A form three diagonal blocks of size 3.
Case B: the same coefficient values, with the occupied columns of each
        row permuted, so that no structure remains.

Both cases use the same inputs X and the same measurement errors, so
the only difference between them is where the coefficients sit.

Nothing in the prior or in the search refers to blocks.  The prior
treats all N*N cells alike.

The criterion minimised is equation (7.2) of the document:

    F(A) = log C(q,M) + M log(2 L_A) + sum log|A_ij| - log I(R)

with I(R) the integral over sigma on [s0, s1], equation (6.7).
----------------------------------------------------------------------
"""

import numpy as np
from itertools import combinations
from math import log, lgamma, inf
from scipy.special import gammainc, betaln

# ----------------------------------------------------------------- setup

N, T = 9, 6
q = N * N
n = N * T
m = n / 2.0

AMIN, AMAX = 0.20, 5.0
LA = log(AMAX / AMIN)

S0, S1 = 0.02, 10.0
LSIG = log(S1 / S0)

# Largest number of occupied cells allowed in one row.  N means no
# restriction.  A row with T or more occupied cells reproduces its data
# exactly, so restricting the count below T excludes exact fits by
# construction.  Both settings are examined below.
CAP = N

SIGMA_TRUE = 0.10
SEED = 20260726

rng = np.random.default_rng(SEED)

# ------------------------------------------------- the two true matrices

# 27 coefficient magnitudes, log-uniform on [0.4, 3.0], with random signs.
# The range lies strictly inside [AMIN, AMAX]; see Section 10.1.
mags = np.exp(rng.uniform(log(0.4), log(3.0), size=27))
signs = rng.choice([-1.0, 1.0], size=27)
vals = np.round(mags * signs, 2)

A_block = np.zeros((N, N))
k = 0
for b in range(3):                       # three diagonal blocks of size 3
    for i in range(3 * b, 3 * b + 3):
        for j in range(3 * b, 3 * b + 3):
            A_block[i, j] = vals[k]
            k += 1

# Case B: same three values in each row, moved to three columns chosen at
# random.  Redrawn until no row keeps its block columns.
while True:
    A_scatter = np.zeros((N, N))
    cols_of_row = []
    ok = True
    for i in range(N):
        cols = np.sort(rng.choice(N, size=3, replace=False))
        block_cols = np.arange(3 * (i // 3), 3 * (i // 3) + 3)
        if np.array_equal(cols, block_cols):
            ok = False
            break
        cols_of_row.append(cols)
        A_scatter[i, cols] = A_block[i, A_block[i] != 0.0]
    if ok:
        break

# --------------------------------------------------------- the data

X = rng.standard_normal((N, T))
E = rng.standard_normal((N, T)) * SIGMA_TRUE

Y_block = A_block @ X + E
Y_scatter = A_scatter @ X + E

# ------------------------------------------------------- the criterion


def set_sigma_bounds(s0, s1):
    global S0, S1, LSIG
    S0, S1 = s0, s1
    LSIG = log(S1 / S0)


def log_I(R):
    """log of the integral over sigma on [s0,s1], equation (6.7).

    For very small R the regularised incomplete gamma functions
    underflow, so the limiting form (6.9) is used instead.
    """
    x0 = R / (2.0 * S0 * S0)
    x1 = R / (2.0 * S1 * S1)
    if x0 < 1e-6:
        # I(R) -> (s0^-n - s1^-n) / (n L_sigma)
        return -log(n * LSIG) - n * log(S0)
    d = gammainc(m, x0) - gammainc(m, x1)
    if d <= 0.0:
        return -log(n * LSIG) - n * log(S0)
    return -log(2.0 * LSIG) + m * log(2.0 / R) + lgamma(m) + log(d)


def lchoose(a, b):
    return lgamma(a + 1) - lgamma(b + 1) - lgamma(a - b + 1)


# Hyper-prior on p.  ALPHA = BETA = 1 is the uniform prior of the
# document.  A Beta prior with mean ALPHA/(ALPHA+BETA) below one half
# expresses an expectation of sparsity without excluding dense matrices.
ALPHA, BETA = 1.0, 1.0


def log_pattern(M):
    """Minus the log of the integral over p of the pattern prior.

    With p ~ Beta(ALPHA, BETA) the integral is
        B(M+ALPHA, q-M+BETA) / B(ALPHA, BETA),
    which reduces to 1/((q+1) C(q,M)) when ALPHA = BETA = 1.
    """
    return -betaln(M + ALPHA, q - M + BETA) + betaln(ALPHA, BETA)


def criterion(R, M, sum_log_abs):
    """F, equation (7.2), with the pattern term generalised."""
    return log_pattern(M) + M * log(2.0 * LA) + sum_log_abs - log_I(R)


# ------------------------------------------- per-row, per-subset fitting

ALL_SUBSETS = [tuple(c) for kk in range(N + 1) for c in combinations(range(N), kk)]


def row_table(y_row):
    """For one row of Y, fit every subset of columns by least squares.

    Returns arrays over subsets: size, residual sum of squares, the sum
    of log|coefficient|, and a feasibility flag.  A subset is infeasible
    when a fitted coefficient falls outside [AMIN, AMAX] in magnitude,
    since the prior gives such a matrix no weight.  The corresponding
    smaller subset is considered separately, so nothing is lost.
    """
    sizes = np.empty(len(ALL_SUBSETS), dtype=int)
    Rs = np.empty(len(ALL_SUBSETS))
    gs = np.empty(len(ALL_SUBSETS))
    feas = np.zeros(len(ALL_SUBSETS), dtype=bool)
    coefs = [None] * len(ALL_SUBSETS)

    for idx, sub in enumerate(ALL_SUBSETS):
        sizes[idx] = len(sub)
        if len(sub) == 0:
            Rs[idx] = float(y_row @ y_row)
            gs[idx] = 0.0
            feas[idx] = True
            coefs[idx] = np.zeros(0)
            continue
        Xs = X[list(sub), :]                      # k by T
        c, *_ = np.linalg.lstsq(Xs.T, y_row, rcond=None)
        r = y_row - Xs.T @ c
        Rs[idx] = float(r @ r)
        a = np.abs(c)
        if np.all(a >= AMIN) and np.all(a <= AMAX):
            feas[idx] = True
            gs[idx] = float(np.sum(np.log(a)))
        else:
            gs[idx] = inf
        coefs[idx] = c
    return sizes, Rs, gs, feas, coefs


# --------------------------------------------------------- the search
#
# F couples the rows through log(sum_i R_i) and through the total M, so
# it does not separate.  Holding sigma fixed removes the first coupling:
# the joint posterior of (A, sigma), with p integrated out, contains
#
#     sum_i [ R_i/(2 sigma^2) + sum_j log|A_ij| ]
#         + (n+1) log sigma + M log(2 L_A) + log C(q,M)
#
# which separates by row once sigma and the allocation of M among rows
# are fixed.  For each sigma on a grid, dynamic programming over the
# rows gives the exact best pattern for every total M.  Those patterns
# form the candidate set, and F is then evaluated on each one.


def search(Y):
    tables = [row_table(Y[i]) for i in range(N)]
    candidates = {}

    for sigma in np.exp(np.linspace(log(S0), log(S1), 240)):
        inv2s2 = 1.0 / (2.0 * sigma * sigma)

        # best subset of each size, for each row, at this sigma
        best_cost = np.full((N, N + 1), inf)
        best_idx = np.full((N, N + 1), -1, dtype=int)
        for i, (sizes, Rs, gs, feas, _) in enumerate(tables):
            cost = np.where(feas, Rs * inv2s2 + gs, inf)
            for kk in range(CAP + 1):
                sel = np.where(sizes == kk)[0]
                j = sel[np.argmin(cost[sel])]
                if np.isfinite(cost[j]):
                    best_cost[i, kk] = cost[j]
                    best_idx[i, kk] = j

        # dynamic programming over rows, indexed by total M
        MMAX = N * N
        dp = np.full((N + 1, MMAX + 1), inf)
        choice = np.full((N + 1, MMAX + 1), -1, dtype=int)
        dp[0, 0] = 0.0
        for i in range(N):
            for M in range(MMAX + 1):
                if not np.isfinite(dp[i, M]):
                    continue
                for kk in range(CAP + 1):
                    if not np.isfinite(best_cost[i, kk]):
                        continue
                    v = dp[i, M] + best_cost[i, kk]
                    if v < dp[i + 1, M + kk]:
                        dp[i + 1, M + kk] = v
                        choice[i + 1, M + kk] = kk

        for M in range(MMAX + 1):
            if not np.isfinite(dp[N, M]):
                continue
            # recover the allocation
            alloc, MM = [], M
            for i in range(N, 0, -1):
                kk = choice[i, MM]
                alloc.append(kk)
                MM -= kk
            alloc.reverse()
            # Key by the subsets actually chosen, not by how many cells
            # each row was given.  Keying by the allocation alone admits
            # only the first sigma that produces it and leaves part of
            # the candidate set unexamined.
            key = tuple(int(best_idx[i, alloc[i]]) for i in range(N))
            if key not in candidates:
                candidates[key] = [
                    tables[i][4][best_idx[i, alloc[i]]] for i in range(N)
                ], [ALL_SUBSETS[best_idx[i, alloc[i]]] for i in range(N)]

    # evaluate the exact criterion on every candidate
    best = None
    for key, (coefs, subs) in candidates.items():
        A = np.zeros((N, N))
        for i in range(N):
            if len(subs[i]):
                A[i, list(subs[i])] = coefs[i]
        F, R, M = evaluate(A, Y)
        if best is None or F < best[0]:
            best = (F, A, R, M)
    return best, len(candidates)


def evaluate(A, Y, check_bounds=True):
    """F, R and M for a completed matrix.

    A matrix with a coefficient outside [AMIN, AMAX] lies outside the
    support of the prior (6.4) and has no posterior weight at all, so F
    is infinite there.  The reference patterns reported below are scored
    under the same rule as the candidates, which requires this check.
    """
    Rm = Y - A @ X
    R = float(np.sum(Rm * Rm))
    nz = A[A != 0.0]
    M = int(nz.size)
    a = np.abs(nz)
    if check_bounds and M and (np.any(a < AMIN) or np.any(a > AMAX)):
        return inf, R, M
    return criterion(R, M, float(np.sum(np.log(a)))), R, M


def fit_pattern(A_pattern, Y):
    """Least-squares coefficients on the occupied cells of A_pattern."""
    A = np.zeros((N, N))
    for i in range(N):
        cols = np.where(A_pattern[i] != 0.0)[0]
        if cols.size == 0:
            continue
        c, *_ = np.linalg.lstsq(X[cols, :].T, Y[i], rcond=None)
        A[i, cols] = c
    return A


def confusion(A_hat, A_true):
    hat = A_hat != 0.0
    tru = A_true != 0.0
    return (int(np.sum(hat & tru)), int(np.sum(hat & ~tru)), int(np.sum(~hat & tru)))


# ------------------------------------------------------------- reporting


def matrix_tex(A, label):
    rows = []
    for i in range(N):
        cells = [("$\\cdot$" if A[i, j] == 0.0 else f"${A[i, j]:.2f}$") for j in range(N)]
        rows.append(" & ".join(cells) + " \\\\")
    body = "\n".join(rows)
    return (
        "\\begin{center}\n\\begin{tabular}{" + "r" * N + "}\n"
        + body + "\n\\end{tabular}\n\\end{center}\n"
        + f"% {label}\n"
    )


def report(name, A_true, Y, out):
    out.append(f"\n================ {name} ================")
    A_true_fit = fit_pattern(A_true, Y)
    F_true, R_true, M_true = evaluate(A_true_fit, Y)

    (F_hat, A_hat, R_hat, M_hat), ncand = search(Y)
    hit, false_pos, missed = confusion(A_hat, A_true)

    # reference patterns
    A_empty = np.zeros((N, N))
    F_empty, R_empty, _ = evaluate(A_empty, Y)

    A_sat = np.zeros((N, N))                    # six cells per row: exact fit
    for i in range(N):
        A_sat[i, :T] = 1.0
    A_sat = fit_pattern(A_sat, Y)
    F_sat, R_sat, M_sat = evaluate(A_sat, Y)

    A_full = fit_pattern(np.ones((N, N)), Y)
    F_full, R_full, M_full = evaluate(A_full, Y)

    out.append(f"candidate patterns examined : {ncand}")
    out.append(f"true pattern      M={M_true:3d}  R={R_true:12.5g}  F={F_true:12.4f}")
    out.append(f"recovered pattern M={M_hat:3d}  R={R_hat:12.5g}  F={F_hat:12.4f}")
    out.append(f"empty matrix      M={0:3d}  R={R_empty:12.5g}  F={F_empty:12.4f}")
    out.append(f"saturated (exact) M={M_sat:3d}  R={R_sat:12.5g}  F={F_sat:12.4f}")
    out.append(f"all cells         M={M_full:3d}  R={R_full:12.5g}  F={F_full:12.4f}")
    out.append(f"cells correctly occupied : {hit} of {M_true}")
    out.append(f"cells falsely occupied   : {false_pos}")
    out.append(f"cells missed             : {missed}")
    out.append(f"sigma implied by R_hat   : {np.sqrt(R_hat/(n+1)):.4f}"
               f"   (true {SIGMA_TRUE})")
    out.append(f"p implied by M_hat       : {(M_hat+1)/(q+2):.4f}"
               f"   (true {M_true/q:.4f})")
    err = np.sqrt(np.sum((A_hat - A_true) ** 2))
    out.append(f"Frobenius error |A_hat - A_true| : {err:.4f}")
    return A_hat, F_true, F_hat


def least_squares_contrast(Y, A_true, out):
    """Contrast with the unrestricted least-squares estimate.

    For Y = A X with Y and X of shape N by T, minimising the sum of
    squared residuals gives the normal equations Y X^T = A X X^T, hence

        A = (Y X^T) (X X^T)^{-1}.

    Only one of the two possible comparisons can be made here, and the
    reason is the shape of X.  Since X X^T = sum_t x_t x_t^T is a sum of
    T matrices of rank one, its rank is at most T whatever the values in
    X.  When T >= N and X has full row rank the inverse exists and the
    formula above is the least-squares estimate; that is the ordinary
    situation, and a comparison against it is meaningful.  These examples
    use N = 9 and T = 6, so the rank is at most 6 out of 9 and the matrix
    is singular for every X, not merely for unlucky ones.  The
    least-squares estimate does not exist, and no comparison against it
    is possible.

    What can be compared is the minimum-norm solution obtained from the
    pseudo-inverse.  That is a different estimator: among the matrices
    that reproduce the data exactly it selects the one of smallest
    Frobenius norm.  It is reported below to show what the unrestricted
    calculation yields when it yields anything at all.  A comparison
    against genuine least squares requires a separate run with T >= N.
    """
    out.append("\n================ unrestricted least squares ================")
    G = X @ X.T
    out.append(f"X is {N} by {T};  X X^T is {N} by {N} with rank "
               f"{np.linalg.matrix_rank(G)}")
    sv = np.linalg.svd(G, compute_uv=False)
    out.append(f"smallest three singular values of X X^T : "
               f"{sv[-3]:.3e}, {sv[-2]:.3e}, {sv[-1]:.3e}")
    A_pinv = Y @ X.T @ np.linalg.pinv(G)
    nz = int(np.sum(np.abs(A_pinv) > 1e-10))
    err = np.sqrt(np.sum((A_pinv - A_true) ** 2))
    R = float(np.sum((Y - A_pinv @ X) ** 2))
    out.append(f"minimum-norm solution: non-zero cells {nz} of {q}, "
               f"residual sum of squares {R:.3e}")
    out.append(f"Frobenius error |A_pinv - A_true| : {err:.4f}")
    return A_pinv


# ------------------------------------------------------------------ main

# ------------------------------------------ the alternative criterion
#
# The sweep above shows that maximising over the coefficients does not
# account for the freedom a larger pattern carries.  Integrating over
# them instead, by Laplace's method about the least-squares point,
# multiplies the result by (2 pi sigma^2)^(M/2) prod |G_i|^(-1/2), where
# G_i = X_{S_i} X_{S_i}^T for row i.  The power of sigma then becomes
# M - n - 1 rather than -n - 1, so the exponent attached to the residual
# sum of squares falls from n/2 to (n-M)/2.  That is the accounting for
# degrees of freedom absent from (7.2).
#
# This is NOT the criterion of the document.  It is computed here so
# that the two may be compared on the same data.


def log_sigma_integral(R, M):
    """log of the integral of sigma^(M-n-1) exp(-R/2 sigma^2) on [s0,s1].

    Evaluated on a grid in u = log sigma, where the integral becomes
    the integral of exp(-(n-M) u - R exp(-2u)/2) du.  Done in log space
    so that it is stable for every M, including M >= n.
    """
    u = np.linspace(log(S0), log(S1), 4001)
    lg = -(n - M) * u - 0.5 * R * np.exp(-2.0 * u)
    top = lg.max()
    return top + log(np.trapezoid(np.exp(lg - top), u))


def criterion_marginal(R, M, sum_log_abs, sum_log_det):
    return (log_pattern(M) + M * log(2.0 * LA) + sum_log_abs
            + 0.5 * sum_log_det - 0.5 * M * log(2.0 * np.pi)
            - log_sigma_integral(R, M))


def evaluate_marginal(A, Y):
    Rm = Y - A @ X
    R = float(np.sum(Rm * Rm))
    nz = A[A != 0.0]
    M = int(nz.size)
    a = np.abs(nz)
    if M and (np.any(a < AMIN) or np.any(a > AMAX)):
        return inf, R, M
    sld = 0.0
    for i in range(N):
        cols = np.where(A[i] != 0.0)[0]
        if cols.size:
            G = X[cols, :] @ X[cols, :].T
            sld += float(np.linalg.slogdet(G)[1])
    return criterion_marginal(R, M, float(np.sum(np.log(a))), sld), R, M


def search_marginal(Y):
    tables = []
    for i in range(N):
        sizes, Rs, gs, feas, coefs = row_table(Y[i])
        slds = np.zeros(len(ALL_SUBSETS))
        for idx, sub in enumerate(ALL_SUBSETS):
            if len(sub):
                G = X[list(sub), :] @ X[list(sub), :].T
                slds[idx] = float(np.linalg.slogdet(G)[1])
        tables.append((sizes, Rs, gs, feas, coefs, slds))

    candidates = {}
    for sigma in np.exp(np.linspace(log(S0), log(S1), 240)):
        inv2s2 = 1.0 / (2.0 * sigma * sigma)
        half_log = 0.5 * log(2.0 * np.pi * sigma * sigma)
        best_cost = np.full((N, N + 1), inf)
        best_idx = np.full((N, N + 1), -1, dtype=int)
        for i, (sizes, Rs, gs, feas, _, slds) in enumerate(tables):
            cost = np.where(feas,
                            Rs * inv2s2 + gs + 0.5 * slds - sizes * half_log,
                            inf)
            for kk in range(CAP + 1):
                sel = np.where(sizes == kk)[0]
                j = sel[np.argmin(cost[sel])]
                if np.isfinite(cost[j]):
                    best_cost[i, kk] = cost[j]
                    best_idx[i, kk] = j

        MMAX = N * N
        dp = np.full((N + 1, MMAX + 1), inf)
        choice = np.full((N + 1, MMAX + 1), -1, dtype=int)
        dp[0, 0] = 0.0
        for i in range(N):
            for M in range(MMAX + 1):
                if not np.isfinite(dp[i, M]):
                    continue
                for kk in range(CAP + 1):
                    if not np.isfinite(best_cost[i, kk]):
                        continue
                    v = dp[i, M] + best_cost[i, kk]
                    if v < dp[i + 1, M + kk]:
                        dp[i + 1, M + kk] = v
                        choice[i + 1, M + kk] = kk
        for M in range(MMAX + 1):
            if not np.isfinite(dp[N, M]):
                continue
            alloc, MM = [], M
            for i in range(N, 0, -1):
                kk = choice[i, MM]
                alloc.append(kk)
                MM -= kk
            alloc.reverse()
            key = tuple(int(best_idx[i, alloc[i]]) for i in range(N))
            if key not in candidates:
                candidates[key] = [ALL_SUBSETS[best_idx[i, alloc[i]]]
                                   for i in range(N)], \
                                  [tables[i][4][best_idx[i, alloc[i]]]
                                   for i in range(N)]

    best = None
    for subs, coefs in candidates.values():
        A = np.zeros((N, N))
        for i in range(N):
            if len(subs[i]):
                A[i, list(subs[i])] = coefs[i]
        F, R, M = evaluate_marginal(A, Y)
        if best is None or F < best[0]:
            best = (F, A, R, M)
    return best


def refine_stationary(A, Y, iters=60):
    """Move the coefficients from least squares to the stationary point.

    Equation (9.3) of the document requires, for every occupied cell,

        e_ij = R / (n A_ij),      e_ij = sum_t (Y_it - Z_it) X_jt.

    For row i with occupied columns S_i this reads

        G_i a = X_{S_i} y_i - (R/n) * (1/a)

    componentwise in the last term.  R involves every row, so the
    iteration below sweeps the rows and recomputes R each time, starting
    from the least-squares solution.  The departure is of order R/n and
    the iteration converges quickly.
    """
    A = A.copy()
    for _ in range(iters):
        R = float(np.sum((Y - A @ X) ** 2))
        prev = A.copy()
        for i in range(N):
            cols = np.where(A[i] != 0.0)[0]
            if cols.size == 0:
                continue
            Xs = X[cols, :]
            G = Xs @ Xs.T
            a = A[i, cols]
            rhs = Xs @ Y[i] - (R / n) / a
            try:
                A[i, cols] = np.linalg.solve(G, rhs)
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


def final_run(out):
    """The procedure of the document: pattern by (9.7), values by (9.3)."""
    global CAP
    out.append("\n============ option 2: pattern by Phi, values by (9.3) ======")
    CAP = T - 1
    set_sigma_bounds(0.02, 10.0)
    for name, A_true, Y in (("block", A_block, Y_block),
                            ("scatter", A_scatter, Y_scatter)):
        Fm_hat, A_ls, _, M_hat = search_marginal(Y)
        A_st = refine_stationary(A_ls, Y)
        shift = float(np.max(np.abs(A_st - A_ls)))
        hit, fp, miss = confusion(A_st, A_true)
        R = float(np.sum((Y - A_st @ X) ** 2))
        out.append(f"  {name:<9} M={M_hat:3d}  correct {hit:3d}  false {fp:3d}"
                   f"  missed {miss:3d}   Phi={Fm_hat:9.3f}")
        out.append(f"            largest move from least squares: {shift:.5f}"
                   f"   (R/n = {R/n:.5f})")
        out.append(f"            Frobenius error against the truth: "
                   f"{np.sqrt(np.sum((A_st - A_true)**2)):.4f}")
        out.append(f"            sigma from R/(n-M): "
                   f"{np.sqrt(R/max(n-M_hat,1)):.4f}  (true {SIGMA_TRUE})")
    CAP = N
    set_sigma_bounds(0.02, 10.0)


def compare_criteria(out):
    """The two criteria, under three hyper-priors on p.

    The Laplace approximation behind the integrated criterion requires
    G_i = X_{S_i} X_{S_i}^T to be non-singular, which fails once a row
    has more than T occupied cells.  Both criteria are therefore run
    with the per-row count restricted to T-1, so that the comparison is
    made on the same set of patterns.

    Mean sparsity ALPHA/(ALPHA+BETA):  1/2, 1/6, 1/51.
    """
    global ALPHA, BETA, CAP
    out.append("\n=========== the two criteria, three priors on p ===========")
    out.append("  E[p]     criterion    case      M_hat  correct  false"
               "  missed     F(true)      F(hat)")
    CAP = T - 1
    for a, b, lab in ((1.0, 1.0, "1/2"), (1.0, 5.0, "1/6"), (1.0, 50.0, "1/51")):
        ALPHA, BETA = a, b
        for name, A_true, Y in (("block", A_block, Y_block),
                                ("scatter", A_scatter, Y_scatter)):
            F_true, _, _ = evaluate(fit_pattern(A_true, Y), Y)
            (F_hat, A_hat, _, M_hat), _ = search(Y)
            hit, fp, miss = confusion(A_hat, A_true)
            out.append(f"  {lab:<8} maximised    {name:<9} {M_hat:5d}"
                       f"  {hit:7d} {fp:6d} {miss:7d}"
                       f"  {F_true:10.3f}  {F_hat:10.3f}")

            Fm_true, _, _ = evaluate_marginal(fit_pattern(A_true, Y), Y)
            Fm_hat, Am_hat, _, Mm_hat = search_marginal(Y)
            hit, fp, miss = confusion(Am_hat, A_true)
            out.append(f"  {lab:<8} integrated   {name:<9} {Mm_hat:5d}"
                       f"  {hit:7d} {fp:6d} {miss:7d}"
                       f"  {Fm_true:10.3f}  {Fm_hat:10.3f}")
    ALPHA, BETA = 1.0, 1.0
    CAP = N


def variants(out):
    """Diagnostic sweep over the noise floor and the per-row restriction.

    The first run of these examples preferred patterns that reproduce the
    data exactly.  Two candidate explanations are separated here.  The
    first is that s0 was set far below any credible measurement error, so
    that the bound on the sigma integral, while finite, is enormous.  The
    second is that maximising a density over the coefficients, rather
    than integrating over them, fails to charge for the extra freedom the
    larger patterns carry.  Restricting each row to fewer than T occupied
    cells removes exact fits by construction and isolates the second.
    """
    global CAP
    out.append("\n================ diagnostic sweep ================")
    out.append("  s0     cap   case        M_hat  correct  false  missed"
               "     F(true)      F(hat)")
    for s0 in (0.001, 0.02, 0.05):
        for cap in (N, T - 1):
            set_sigma_bounds(s0, S1)
            CAP = cap
            for name, A_true, Y in (("block", A_block, Y_block),
                                    ("scatter", A_scatter, Y_scatter)):
                F_true, _, _ = evaluate(fit_pattern(A_true, Y), Y)
                (F_hat, A_hat, _, M_hat), _ = search(Y)
                hit, fp, miss = confusion(A_hat, A_true)
                out.append(f"  {s0:<6} {cap:<5} {name:<10} {M_hat:5d}"
                           f"  {hit:7d} {fp:6d} {miss:7d}"
                           f"  {F_true:10.3f}  {F_hat:10.3f}")
    CAP = N
    set_sigma_bounds(0.02, 10.0)


if __name__ == "__main__":
    out = []
    out.append(f"N={N} T={T} q={q} n={n} sigma_true={SIGMA_TRUE} seed={SEED}")
    out.append(f"amin={AMIN} amax={AMAX} L_A={LA:.5f}")
    out.append(f"s0={S0} s1={S1} L_sigma={LSIG:.5f}")

    Ahat_b, Ftrue_b, Fhat_b = report("CASE A  block structure", A_block, Y_block, out)
    Ahat_s, Ftrue_s, Fhat_s = report("CASE B  scattered", A_scatter, Y_scatter, out)
    A_pinv = least_squares_contrast(Y_block, A_block, out)
    variants(out)
    set_sigma_bounds(0.02, 10.0)
    compare_criteria(out)
    set_sigma_bounds(0.02, 10.0)
    final_run(out)

    text = "\n".join(out)
    print(text)
    with open("results.txt", "w") as f:
        f.write(text + "\n")

    # The tables printed in the document are produced by final.py.
