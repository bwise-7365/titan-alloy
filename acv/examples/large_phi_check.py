"""
large_phi_check.py
----------------------------------------------------------------------
The decisive question raised by Section 12: when the procedure leaves
out two dozen weak cells, is that the search failing to find the true
pattern, or the criterion preferring a pattern that is not the true one?

The two are told apart by evaluating Phi at the true pattern and at the
recovered one.  If Phi is smaller at the truth, the search is at fault.
If Phi is smaller at the recovered pattern, the search did its work and
the criterion is answering a question we did not intend to ask.

For completeness Phi is also evaluated at the pattern the adaptive
lasso returns, which in this study is the true pattern exactly.
----------------------------------------------------------------------
"""

import numpy as np
import large as L


def phi_of(X, Y, A):
    """Phi for a given matrix, from the four quantities it depends on."""
    M = int(np.sum(A != 0.0))
    R = float(np.sum((Y - A @ X) ** 2))
    nz = np.abs(A[A != 0.0])
    if nz.size and (np.any(nz < L.AMIN) or np.any(nz > L.AMAX)):
        return float("inf"), M, R, "outside the range of the prior"
    g = float(np.sum(np.log(nz)))
    sld = 0.0
    for i in range(L.N):
        c = np.where(A[i] != 0.0)[0]
        if c.size:
            Xs = X[c, :]
            sld += float(np.linalg.slogdet(Xs @ Xs.T)[1])
    return float(L.phi_parts(M, R, g, sld)), M, R, ""


def main(seed=20260726, case="block"):
    A_block, A_scatter, X, E = L.build(seed)
    A_true = A_block if case == "block" else A_scatter
    Y = A_true @ X + E

    # the true pattern, with coefficients fitted rather than assumed
    A_fit = L.refit(X, Y, A_true != 0.0)

    A_hat = L.refine(X, Y, L.search(X, Y))

    lines = [f"seed {seed}, case {case}", ""]
    for tag, A in (("true pattern, least squares", A_fit),
                   ("true pattern, true values", A_true),
                   ("recovered pattern", A_hat)):
        v, M, R, note = phi_of(X, Y, A)
        lines.append(f"{tag:<28} M={M:5d}  R={R:10.2f}  Phi={v:14.2f}"
                     f"  {note}")

    v_true = phi_of(X, Y, A_fit)[0]
    v_hat = phi_of(X, Y, A_hat)[0]
    lines.append("")
    if v_hat < v_true:
        lines.append(f"The criterion prefers the recovered pattern to the "
                     f"truth by {v_true - v_hat:.2f} natural units.")
        lines.append("The search is not at fault.  The criterion is.")
    else:
        lines.append(f"The criterion prefers the truth by "
                     f"{v_hat - v_true:.2f} natural units, which the search "
                     f"failed to reach.")

    text = "\n".join(lines)
    print(text)
    with open("large_phi_check.txt", "w") as f:
        f.write(text + "\n")


if __name__ == "__main__":
    main()
