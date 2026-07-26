"""
final.py
----------------------------------------------------------------------
Produces the numbers and the LaTeX tables for Section 10 of the
preliminary document.

Settings differ from the first trial in one respect.  The floor on the
coefficient magnitudes is set at 0.20 and the true magnitudes are drawn
from [0.40, 3.00], so that the floor lies strictly below the smallest
coefficient present.  A floor set at the edge of the truth rejects
borderline coefficients that the measurement error pushes below it,
which was the cause of the missed cells in the first trial.

Everything else follows sweep.py, whose functions are used directly.
----------------------------------------------------------------------
"""

import numpy as np
from math import log
import sweep as S

# ----------------------------------------------------------- settings

S.AMIN, S.AMAX = 0.20, 5.0
S.LA = log(S.AMAX / S.AMIN)
S.S0, S.S1 = 0.02, 10.0
S.LSIG = log(S.S1 / S.S0)
S.MAGLO, S.MAGHI = 0.40, 3.00
S.SIGMA_TRUE = 0.10
S.ALPHA, S.BETA = 1.0, 1.0

SEED = 20260726                       # the first seed, not a selected one
NSEEDS = 40
SEEDS = [SEED] + [SEED + 1000 * i for i in range(1, NSEEDS)]

N, T, q, n = S.N, S.T, S.q, S.n


# ------------------------------------------------------------ helpers


def mat_tex(A, caption):
    rows = []
    for i in range(N):
        cells = ["$\\cdot$" if A[i, j] == 0.0 else f"${A[i, j]:.2f}$"
                 for j in range(N)]
        rows.append("  " + " & ".join(cells) + " \\\\")
    return ("\\begin{center}\n\\footnotesize\n\\begin{tabular}{"
            + "r" * N + "}\n" + "\n".join(rows)
            + "\n\\end{tabular}\n\\end{center}\n"
            + f"% {caption}\n\n")


def run_seed(seed):
    A_block, A_scatter, X, E = S.build(seed)
    slds = S.logdets(X)
    out = {}
    for name, A_true in (("block", A_block), ("scatter", A_scatter)):
        Y = A_true @ X + E
        res = S.search(X, Y, slds)
        A_hat = S.refine(X, Y, res[1])
        At = np.zeros((N, N))
        for i in range(N):
            c = np.where(A_true[i] != 0.0)[0]
            b, *_ = np.linalg.lstsq(X[c, :].T, Y[i], rcond=None)
            At[i, c] = b
        hat, tru = A_hat != 0.0, A_true != 0.0
        M = int(np.sum(hat))
        R = float(np.sum((Y - A_hat @ X) ** 2))
        out[name] = dict(
            A_true=A_true, A_hat=A_hat, X=X, Y=Y,
            M=M,
            correct=int(np.sum(hat & tru)),
            false=int(np.sum(hat & ~tru)),
            missed=int(np.sum(~hat & tru)),
            phi_hat=S.phi(X, Y, A_hat),
            phi_true=S.phi(X, Y, At),
            sigma=float(np.sqrt(R / max(n - M, 1))),
            frob=float(np.sqrt(np.sum((A_hat - A_true) ** 2))))
    return out


# --------------------------------------------------------------- main

if __name__ == "__main__":
    lines, tex = [], []

    lines.append(f"N={N} T={T} q={q} n={n}")
    lines.append(f"amin={S.AMIN} amax={S.AMAX} L_A={S.LA:.4f}")
    lines.append(f"s0={S.S0} s1={S.S1} L_sigma={S.LSIG:.4f}")
    lines.append(f"true magnitudes log-uniform on [{S.MAGLO}, {S.MAGHI}]")
    lines.append(f"sigma_true={S.SIGMA_TRUE}  alpha={S.ALPHA} beta={S.BETA}")
    lines.append(f"per-row limit = {S.CAP}   seed shown = {SEED}")

    r = run_seed(SEED)
    for name in ("block", "scatter"):
        d = r[name]
        lines.append(f"\n--- {name}, seed {SEED} ---")
        lines.append(f"M={d['M']}  correct={d['correct']}  false={d['false']}"
                     f"  missed={d['missed']}")
        lines.append(f"Phi(recovered)={d['phi_hat']:.3f}   "
                     f"Phi(true pattern)={d['phi_true']:.3f}")
        lines.append(f"sigma={d['sigma']:.4f} (true {S.SIGMA_TRUE})   "
                     f"Frobenius error={d['frob']:.4f}")
        tex.append(mat_tex(d["A_true"], f"true A, {name}"))
        tex.append(mat_tex(d["A_hat"], f"recovered A, {name}"))

    # least squares contrast, on the block case
    X, Y = r["block"]["X"], r["block"]["Y"]
    G = X @ X.T
    rank = int(np.linalg.matrix_rank(G))
    sv = np.linalg.svd(G, compute_uv=False)
    A_pinv = Y @ X.T @ np.linalg.pinv(G)
    lines.append(f"\n--- unrestricted least squares, block case ---")
    lines.append(f"X is {N} by {T}; X X' is {N} by {N} of rank {rank}")
    lines.append(f"smallest three singular values: {sv[-3]:.2e}, "
                 f"{sv[-2]:.2e}, {sv[-1]:.2e}")
    lines.append(f"minimum-norm solution: {int(np.sum(np.abs(A_pinv)>1e-10))}"
                 f" of {q} cells non-zero, residual "
                 f"{float(np.sum((Y - A_pinv @ X)**2)):.2e}")
    lines.append(f"Frobenius error against the truth: "
                 f"{np.sqrt(np.sum((A_pinv - r['block']['A_true'])**2)):.4f}")
    tex.append(mat_tex(np.round(A_pinv, 2), "minimum-norm solution, block"))

    # the sweep
    agg = {"block": [], "scatter": []}
    for s in SEEDS:
        d = run_seed(s)
        for nm in ("block", "scatter"):
            agg[nm].append(d[nm])
    lines.append(f"\n--- {NSEEDS} realisations ---")
    rowtex = []
    for nm in ("block", "scatter"):
        rs = agg[nm]
        c = np.mean([x["correct"] for x in rs])
        f_ = np.mean([x["false"] for x in rs])
        mi = np.mean([x["missed"] for x in rs])
        ex = sum(1 for x in rs if x["false"] == 0 and x["missed"] == 0)
        sg = np.median([x["sigma"] for x in rs])
        lines.append(f"  {nm:<8} correct {c:5.2f}/27  false {f_:5.2f}"
                     f"  missed {mi:5.2f}  exact {ex:2d}/{NSEEDS}"
                     f"  median sigma {sg:.3f}")
        rowtex.append(f"{nm} & ${c:.2f}$ & ${f_:.2f}$ & ${mi:.2f}$ & "
                      f"${ex}$ of ${NSEEDS}$ & ${sg:.3f}$ \\\\")
    err = lambda x: x["false"] + x["missed"]
    worse = sum(1 for a, b in zip(agg["block"], agg["scatter"])
                if err(a) > err(b))
    better = sum(1 for a, b in zip(agg["block"], agg["scatter"])
                 if err(a) < err(b))
    lines.append(f"  paired within seed: block worse {worse}, "
                 f"block better {better}, equal {NSEEDS-worse-better}")

    tex.append("\\begin{center}\n\\begin{tabular}{lccccc}\n"
               "case & correct of 27 & false & missed & exact & "
               "median $\\hat\\sigma$ \\\\\n\\hline\n"
               + "\n".join(rowtex) + "\n\\end{tabular}\n\\end{center}\n")
    tex.append(f"% paired: block worse {worse}, better {better}, "
               f"equal {NSEEDS-worse-better}\n")

    # ---- the two claims of Section 10.6 about the coefficient floor ----
    #
    # Section 10.6 states that lowering the floor from 0.20 to 0.05 raises
    # the count of falsely occupied cells from about 1.5 to about 6, and
    # that those extra cells sit at small magnitudes.  Both are produced
    # here, over the same seeds, so that no number in the document rests
    # on a calculation that is not repeated by this script.
    lines.append(f"\n--- the coefficient floor, over the same {NSEEDS} seeds ---")
    keep_amin, keep_la = S.AMIN, S.LA
    for floor in (0.20, 0.05):
        S.AMIN = floor
        S.LA = log(S.AMAX / floor)
        tp, fp, fcount = [], [], []
        for s in SEEDS:
            d = run_seed(s)
            for nm in ("block", "scatter"):
                A, At = d[nm]["A_hat"], d[nm]["A_true"]
                hat, tru = A != 0.0, At != 0.0
                tp += list(np.abs(A[hat & tru]))
                fp += list(np.abs(A[hat & ~tru]))
                fcount.append(int(np.sum(hat & ~tru)))
        tp, fp = np.array(tp), np.array(fp)
        lines.append(f"  floor {floor:.2f}:  falsely occupied cells per matrix"
                     f" {np.mean(fcount):.2f}")
        lines.append(f"    correctly occupied: {len(tp):4d} cells, "
                     f"median |A| {np.median(tp):.3f}, "
                     f"fraction below 0.10 {np.mean(tp < 0.10):.2f}")
        lines.append(f"    falsely   occupied: {len(fp):4d} cells, "
                     f"median |A| {np.median(fp):.3f}, "
                     f"fraction below 0.10 {np.mean(fp < 0.10):.2f}")
    S.AMIN, S.LA = keep_amin, keep_la

    text = "\n".join(lines)
    print(text)
    open("final.txt", "w").write(text + "\n")
    # One file per table, each \input by acv-preliminary.tex.
    names = ["tab_block_true", "tab_block_hat", "tab_scatter_true",
             "tab_scatter_hat", "tab_pinv", "tab_sweep"]
    assert len(names) == len(tex), (len(names), len(tex))
    for nm, body in zip(names, tex):
        open(nm + ".tex", "w").write(body)
