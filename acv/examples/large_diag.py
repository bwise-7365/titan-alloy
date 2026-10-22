"""
large_diag.py
----------------------------------------------------------------------
Asks where the cells missed in Section 12 are lost.

Two things could account for them.  Forward selection might never reach
a cell, in which case no choice of M could recover it and the fault
lies with the search.  Or forward selection might reach it while the
criterion settles on a smaller pattern, in which case the search is
adequate and the fault lies with Phi.

The two are told apart by asking, for every cell of the true matrix
that the procedure left empty, whether the column appears anywhere in
the chain of subsets that forward selection produced for that row.
----------------------------------------------------------------------
"""

import numpy as np
import large as L


def main(seed=20260726, case="block"):
    A_block, A_scatter, X, E = L.build(seed)
    A_true = A_block if case == "block" else A_scatter
    Y = A_true @ X + E

    tables = [L.row_table(X, Y, i) for i in range(L.N)]
    A = L.refine(X, Y, L.search(X, Y))

    hat = A != 0.0
    tru = A_true != 0.0
    reached_missed, unreached_missed = [], []
    kchosen, ktrue = [], []

    depth_vs_size = []          # (depth in the greedy chain, size chosen)
    exact_at_true_size = 0

    for i in range(L.N):
        cols = tables[i][3]
        kmax = max(k for k in range(L.KMAX + 1) if cols[k] is not None)
        deepest = list(cols[kmax])
        ki = int(hat[i].sum())
        kt = int(tru[i].sum())
        kchosen.append(ki)
        ktrue.append(kt)
        # is the true subset of this row the greedy subset of its own size?
        if kt <= kmax and set(cols[kt]) == set(np.where(tru[i])[0]):
            exact_at_true_size += 1
        for j in np.where(tru[i] & ~hat[i])[0]:
            if j in deepest:
                reached_missed.append(abs(A_true[i, j]))
                depth_vs_size.append((deepest.index(j) + 1, ki))
            else:
                unreached_missed.append(abs(A_true[i, j]))

    got = np.abs(A_true[tru & hat])
    rm = np.array(reached_missed)
    um = np.array(unreached_missed)
    kc = np.array(kchosen)
    kt = np.array(ktrue)

    lines = [f"seed {seed}, case {case}, KMAX = {L.KMAX}", ""]
    lines.append(f"true cells {int(tru.sum())}   recovered {int((tru&hat).sum())}"
                 f"   missed {int((tru&~hat).sum())}"
                 f"   falsely occupied {int((hat&~tru).sum())}")
    lines.append("")
    lines.append("of the missed cells:")
    lines.append(f"  reached by forward selection but left out by the "
                 f"criterion : {rm.size}")
    lines.append(f"  never reached by forward selection                    "
                 f"     : {um.size}")
    lines.append("")
    lines.append("true magnitude of a cell:")
    lines.append(f"  recovered            n={got.size:5d}  "
                 f"median {np.median(got):.3f}  "
                 f"fraction below 0.60 {np.mean(got < 0.60):.3f}")
    if rm.size:
        lines.append(f"  missed, reached      n={rm.size:5d}  "
                     f"median {np.median(rm):.3f}  "
                     f"fraction below 0.60 {np.mean(rm < 0.60):.3f}")
    if um.size:
        lines.append(f"  missed, not reached  n={um.size:5d}  "
                     f"median {np.median(um):.3f}  "
                     f"fraction below 0.60 {np.mean(um < 0.60):.3f}")
    lines.append("")
    lines.append(f"cells in a row: true mean {kt.mean():.2f} max {kt.max()}"
                 f"   chosen mean {kc.mean():.2f} max {kc.max()}")
    lines.append(f"rows whose true count reaches the limit {L.KMAX}: "
                 f"{int(np.sum(kt >= L.KMAX))}")
    lines.append("")
    lines.append("the question that matters: is the true subset of a row the "
                 "subset")
    lines.append("forward selection reaches at that row's own size?")
    lines.append(f"  rows where it is     : {exact_at_true_size} of {L.N}")
    lines.append(f"  rows where it is not : {L.N - exact_at_true_size}")
    if depth_vs_size:
        d = np.array([a for a, _ in depth_vs_size])
        s = np.array([b for _, b in depth_vs_size])
        lines.append("")
        lines.append("for each missed cell, where forward selection put it "
                     "in the chain,")
        lines.append("against the number of cells the criterion gave that "
                     "row:")
        lines.append(f"  admitted at step  median {np.median(d):.0f}  "
                     f"range {d.min()}-{d.max()}")
        lines.append(f"  row size chosen   median {np.median(s):.0f}  "
                     f"range {s.min()}-{s.max()}")
        lines.append(f"  cells admitted later than the size chosen: "
                     f"{int(np.sum(d > s))} of {d.size}")

    text = "\n".join(lines)
    print(text)
    with open("large_diag.txt", "w") as f:
        f.write(text + "\n")


if __name__ == "__main__":
    main()
