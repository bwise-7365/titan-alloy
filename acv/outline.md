# Outline for the successor document

Key ideas extracted from `acv-preliminary.tex` (frozen), sorted into a narrative
order. Each key is one sentence. Two are marked **[new]**: they are true of the work
and were said in discussion, but are nowhere stated in the frozen paper, and their
absence is the main reason a reader cannot tell how the story ends.

## 1. The problem, and why the natural priors will not do

1. We observe paired vectors, believe the output is a fixed linear function of the
   input plus error, and want the matrix — expecting most of its cells to be zero
   without knowing which.
2. Choosing which cells are occupied and choosing their values is one problem and not
   two, because how many values there are depends on which cells are occupied.
3. The natural prior for a coefficient magnitude is scale-free — as likely between
   0.01 and 0.1 as between 10 and 100 — which written as a density is proportional to
   1/|a|.
4. That density has no normalising constant, its integral diverging at zero and again
   at infinity.
5. Used as it stands it fails twice over: the posterior has no maximum, since
   log|A_ij| falls without bound as a coefficient approaches zero; and patterns of
   different size cannot be compared, since an undetermined constant enters as its
   Mth power and any conclusion about sparsity can be produced by choosing it.
6. The general rule is that an unnormalised density is safe only when it contributes
   the same number of factors to every candidate, and here that number is M, which is
   what we are trying to determine.
7. The remedy keeps the logarithmic statement but confines it to a range the analyst
   can state, and the invariance that recommended 1/|a| survives, since a change of
   units leaves log(a_max/a_min) unchanged.
8. The error scale needs the same treatment for the same reason: 1/σ over the whole
   half-line makes an exact fit infinitely good, and bounding it makes a perfect fit
   merely very good.

## 2. Building the criterion

9. Bayes's theorem lets us work with the numerator alone, since the denominator is
   the same number for every candidate matrix and could not be evaluated anyway.
10. The occupancy rate is removed by integrating against a Beta hyper-prior, leaving a
    term that depends on the pattern only through its size.
11. The charge for one more occupied cell has a closed form and depends on the
    hyper-prior only through its logarithm, so a prior merely sceptical of dense
    matrices changes almost nothing and anything that must be prevented has to be
    prevented elsewhere.
12. Maximising over the coefficients records the height of the posterior and
    disregards the width of the region of good fit, so a criterion built that way
    occupies as many cells as it is permitted to.
13. Comparing patterns of different size therefore has to be done between integrals
    rather than between heights, which changes the exponent on the residual from n/2
    to (n−M)/2 and adds terms measuring how narrow the region of good fit has become.
14. A coefficient the data determine sharply raises the criterion while one the data
    barely constrain adds little, and that is the accounting for freedom which the
    maximised form omits.
15. The maximum of a density is a fragile quantity — it moves under a change of
    variable, and for a monotone density it sits at an endpoint whatever the
    parameters are — so wherever a conclusion rests on comparing regions rather than
    locating a point, the integral is the quantity with a stable meaning.
16. Every algebraic step in the derivation was verified in Maxima.

## 3. Choosing among patterns: the search

17. A matrix of order N admits 2^(N²) patterns — about 10^24 at nine variables and
    2^40000 at two hundred — so they cannot be examined one by one.
18. Before the occupancy rate and the error scale are removed the rows are separate
    problems, joined only through those two shared quantities, and that separation is
    the whole of the hierarchical structure and the natural starting point for a
    search.
19. Holding the error scale fixed makes the criterion separable across rows, so a
    dynamic programme allocates cells among rows exactly and one pass gives the best
    allocation for every total at once.
20. At nine variables every admissible subset of every row can be enumerated, but at
    two hundred the true row occupancy of thirteen would need 10^20 subsets per row,
    which at measured speed is some seventeen thousand million years.
21. Forward selection replaces enumeration at a cost growing as N rather than as
    N-choose-k, preparing two hundred rows in about a second, and the criterion can be
    accumulated through the dynamic programme because it depends on a pattern only
    through its size and three sums taken over rows.

## 4. What the criterion achieves

22. At nine variables and six observations it recovers about 26 of the 27 occupied
    cells, in a setting where the unrestricted least-squares estimate does not exist
    at all because X X' has rank at most T.
23. It is indifferent to arrangement, recovering a block-structured matrix neither
    better nor worse than a scattered one, which is what declining to impose structure
    means.
24. At nine variables no variant of the lasso is competitive, losing seventy-nine or
    eighty of eighty paired trials, because one continuous penalty must be large
    enough to empty six cells and small enough to keep three; against stochastic
    search variable selection the result is an exact tie.
25. At two hundred variables with eighty quarters the per-row problem is easier than
    the small illustration, eighty observations carrying thirteen coefficients against
    six carrying three, and the whole of the difficulty lies in the search.
26. Given candidates from a penalty path rather than from forward selection, the
    criterion recovers the matrix **exactly** — every occupied cell of some 2570, none
    falsely occupied — in every trial and both arrangements.
27. **[new]** This matches the adaptive lasso's exact recovery, but as a Bayesian
    procedure with an interpretable criterion rather than a penalty chosen by a rule
    of thumb.
28. **[new]** And it retains what the lasso cannot offer: a number that says how much
    better one pattern is than another.
29. That the criterion was never the difficulty is shown directly: with the weaker
    search it left two dozen cells out, yet scored the true pattern better by more
    than eleven thousand natural units, so it would have taken the truth had the
    search offered it.
30. Nothing an analyst lacks enters the procedure, since cross-validation supplies the
    weights that shape the candidate list and needs no error scale, while the
    criterion makes every selection.

## 5. What is not established

31. The test used is the strictest available — each of forty thousand decisions right
    or wrong, with no credit for magnitude and none for nearly recovering the
    arrangement — and by almost any other measure the same results would read better.
32. No proof is offered that the criterion occupies more cells than the truth or
    fewer on average, two sizes having been examined and fallen on opposite sides.
33. One occupancy rate and one error scale serve the whole matrix, whereas the
    AutoClass formalism shares parameters within classes, and letting them vary by row
    or by group is the natural next step.
34. The inputs are taken to be exact, and error in the inputs as well as the outputs
    is a different and harder problem.
35. Every number needed to reproduce the illustrations is printed in the document
    itself.

---

## Note on the arc

The frozen paper has all of 1 to 26 and 29 to 35 somewhere in it, but they arrive in
the order they were discovered rather than the order a reader needs them, and 27 and
28 are missing entirely. The consequence is that the paper ends on the mechanics of a
sampler that did not converge, so a reader reaches the last page without ever being
told whether the method worked.

The proposed order fixes that by making Section 4 answer one question — *does it
work* — and answer it in this sequence: it works at small scale where nothing else
does; it works at realistic scale; it equals the best competitor; it is a criterion
and not a rule of thumb; and the failures it did show were failures of search, which
have been repaired.

## Where the sampler material went — settled

The failure of stochastic search variable selection to converge at two hundred
variables is deliberately absent from the arc above. It was a large part of the work —
four run lengths, two chains at each, and a literature search establishing that the
largest published application of the method carries 595 coefficients against forty
thousand here — but it says more about the state of that literature than about this
criterion, and carrying it in the results is part of what buries the verdict.

**It now lives in `literature-review.md` and `literature-review.tex`**, alongside the
published problem sizes and the theory on mixing, which is where a reader who wants it
will look for it. The successor document should refer to it in one sentence and no
more, along these lines:

> The Bayesian method closest to this one could not be assessed at this size. The
> sampler had not converged after sixty thousand sweeps, and the largest published
> application of it carries 595 coefficients against forty thousand here; the evidence
> is set out in the accompanying review.

That sentence belongs in Section 4, immediately after key 24 records the exact tie at
nine variables, so that the reader learns why the comparison stops there and is not
left wondering. It is a statement about a fixed amount of computing and not about the
method, and it should be worded so that nobody can read it as a claim that the method
fails.
