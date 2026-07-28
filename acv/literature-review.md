# Prior work bearing on `acv-preliminary.tex`

A survey of the open literature, made to establish which results in the paper are
already published and which appear to be new, together with a numerical comparison
against the two methods a reader is most likely to name. Every link below was checked
and resolved at the time of writing. Where a publisher blocks automated access, an
open copy or a stable repository record is given instead.

A LaTeX version of this document, with the same content and a formal bibliography, is
[literature-review.tex](literature-review.tex).

## 1. How the search was conducted

The paper was decomposed into ten technical elements, and each was searched
separately rather than searching for the paper as a whole. The elements are the
model, the two bounded priors, the pattern prior, the hyper-prior on the occupancy
rate, the integrated criterion, the search procedure, the underdetermined regime, the
recovery of block arrangement, and the AutoClass framing. Searching them together
returns nothing, because no single publication contains the same combination.
Searching them separately returns a great deal.

## 2. Verdict at a glance

| Element in the paper | Status | Nearest prior work |
|---|---|---|
| The model $Y = AX + \varepsilon$ with $A$ sparse and the pattern unknown | Well established | George, Sun and Ni (2008); Basu and Michailidis (2015) |
| Each row a separate subset-selection problem | Well established | Standard decomposability; Silander and Myllymäki (2006) |
| Bounded log-uniform prior on $\lvert A_{ij} \rvert$ | Known device, different name | The modified Jeffreys prior |
| Failure of the improper $1/\lvert a \rvert$ prior for comparing patterns | Textbook | Bartlett–Lindley paradox |
| Bounded log-uniform prior on $\sigma$; the divergence as $R \to 0$ | Same textbook result, second instance | As above |
| Bernoulli pattern prior with Beta hyper-prior | Well established | Scott and Berger (2010) |
| The increment $\log((q-M-1+\beta)/(M+\alpha))$ growing only as $\log \beta$ | Consistent with published theory | Scott and Berger (2010) |
| Exponent $(n-M)/2$ and the Occam volume term | Textbook | Schwarz (1978); MacKay (1992); Kass and Raftery (1995) |
| Maximising within a pattern, integrating across patterns | Common practice | Kass and Raftery (1995) |
| Fixed-$\sigma$ dynamic programming over rows | Not found in this form | Nearest is decomposable-score dynamic programming |
| Recovering a block arrangement without imposing one | Actively published, by other means | Gudmundsson and Brownlees (2021) and four others |
| AutoClass as the parent formalism for this problem | Not found | Cheeseman and Stutz (1996) themselves |
| Attribution of residual over-fitting to the truncation at $a_{\min}$ | Not found | — |

## 3. The parts that are already well known

State these as received results in the talk. Presenting any of them as new will draw
a correction.

### 3.1 The model and the estimation problem

With $X$ known and $A$ unknown, the $i$th row of $A$ is an ordinary linear regression
of the $i$th row of $Y$ on the rows of $X$, with $T$ observations and $N$ candidate
coefficients. The whole problem is $N$ independent subset-selection problems sharing
a scale. This is the most heavily worked problem in modern statistics.

The Bayesian treatment by indicator variables goes back to George and McCulloch
(1993) and was carried to vector autoregressions by George, Sun and Ni (2008), which
is the single closest published antecedent to the present formulation. That paper
places a two-component normal prior on each coefficient, a Bernoulli prior on each
indicator, and searches by Markov chain Monte Carlo. The differences from the present
paper are the shape of the coefficient prior, the use of a maximum rather than a
sample, and the search. The structure of the argument is the same.

- George, Sun and Ni (2008), *Bayesian stochastic search for VAR model restrictions*,
  Journal of Econometrics 142(1), 553–580.
  [Author copy (PDF)](https://faculty.wharton.upenn.edu/wp-content/uploads/2012/04/GeorgeSunNi-JE2008.pdf)
- Basu and Michailidis (2015), *Regularized estimation in sparse high-dimensional
  time series models*, Annals of Statistics 43(4), 1535–1567.
  [Project Euclid](https://projecteuclid.org/euclid.aos/1434546214)
- Nicholson, Bien and Matteson, *VARX-L: structured regularization for large vector
  autoregressions*. [arXiv:1508.07497](https://arxiv.org/abs/1508.07497)

The same problem is standard in signal processing, where $X$ is called a dictionary
and the several columns are called multiple measurement vectors. The Bayesian
treatment there is called sparse Bayesian learning.

- Zhang and Rao, *Sparse signal recovery with temporally correlated source vectors
  using sparse Bayesian learning*. [arXiv:1102.3949](https://arxiv.org/abs/1102.3949)
- Ament and Gomes (2021), *Sparse Bayesian learning via stepwise regression*.
  [PMLR v139 (PDF)](https://proceedings.mlr.press/v139/ament21a/ament21a.pdf)

### 3.2 Why the improper priors fail

Section 4.2 shows that $1/\lvert a \rvert$ over $(0,\infty)$ leaves the posterior
without a maximum, and Section 5.1 shows that $1/\sigma$ over $(0,\infty)$ makes an
exact fit infinitely good. Both are instances of one known result: an improper prior
on a parameter that appears in some candidate models and not in others leaves the
comparison undefined, because the missing normalising constant does not cancel. This
is the Bartlett–Lindley paradox, and it is in every treatment of Bayesian model
selection. The derivations in the paper are correct and are worth keeping for the
reader, but they should be introduced as instances of a known difficulty, not as
discoveries.

- Llorente, Martino, Delgado and López-Santiago, *On the safe use of prior densities
  for Bayesian model selection*. [arXiv:2206.05210](https://arxiv.org/abs/2206.05210)
- Kass and Raftery (1995), *Bayes factors*, JASA 90(430), 773–795.
  [Author copy (PDF)](https://www.stat.washington.edu/raftery/Research/PDF/kass1995.pdf)
- *A parsimonious tour of Bayesian model uncertainty*, which surveys the paradox and
  the standard remedies. [arXiv:1902.05539](https://arxiv.org/abs/1902.05539)

### 3.3 The bounded log-uniform prior

Truncating $1/\lvert a \rvert$ to $[a_{\min}, a_{\max}]$ and dividing by
$\log(a_{\max}/a_{\min})$ is a known construction. In the astronomical literature it
is called the modified Jeffreys prior and is used for exactly the reason given in
Section 4.2: the scale-free form is the right shape but is not a density, and the
model comparison needs a density. The paper's observation that $L_A$ is unchanged by
a rescaling of units is the standard motivation.

- Feroz, Balan and Hobson, *Detecting extrasolar planets from stellar radial
  velocities using Bayesian evidence*, which sets out the modified Jeffreys prior and
  the reason for it. [arXiv:1012.5129](https://arxiv.org/abs/1012.5129)
- Liang, Paulo, Molina, Clyde and Berger, *Mixtures of g-priors for Bayesian variable
  selection*, for the alternative and more common remedy.
  [Author copy (PDF)](https://www2.stat.duke.edu/~berger/papers/g-priors.pdf)

One qualification. Although the device is standard, the criterion that follows from
it — a term $\sum \log \lvert A_{ij} \rvert$ that grows as coefficients grow — is
rarely written out, because most authors use a normal or a Laplace prior, whose
corresponding term shrinks as coefficients grow. I did not find the criterion
displayed in the form of Equation (7.1) anywhere. That is a point about exposition
and not about method, and it should be claimed no more strongly than that.

### 3.4 The Beta hyper-prior and multiplicity

This is the closest match in the entire survey, and it is a close one. Scott and
Berger (2010) is the standard reference for placing a Beta prior on the occupancy
rate of a Bernoulli pattern prior and studying what that does to the number of
coefficients selected. The paper's result in Section 6.2 — that the cost of one more
occupied cell is $\log((q-M-1+\beta)/(M+\alpha))$, and that this grows only as
$\log \beta$ — is the log-odds increment of the beta-binomial distribution. It is
correct, it is a good thing to display, and it is consistent with the published
theory rather than contrary to it.

The claim to avoid is that the hyper-prior was found to be unable to control
over-fitting. What Scott and Berger establish is the complementary and more precise
statement: the Beta prior supplies multiplicity correction, which is a different
thing from the Occam penalty that comes from integrating over the coefficients, and
neither substitutes for the other. The paper reaches the same conclusion by a direct
calculation. Cite them and say so.

- Scott and Berger (2010), *Bayes and empirical-Bayes multiplicity adjustment in the
  variable-selection problem*, Annals of Statistics 38(5), 2587–2619.
  [DOI](https://doi.org/10.1214/10-AOS792) ·
  [arXiv:1011.2333](https://arxiv.org/abs/1011.2333)

### 3.5 The integrated criterion

Section 9 integrates the coefficients out by a Laplace approximation, obtains
$\tfrac{1}{2}\sum \log \lvert G_i \rvert - (M/2)\log 2\pi$, and finds the residual
exponent falling from $n/2$ to $(n-M)/2$. Both are textbook. The exponent
$(n-M)/2$ is the ordinary residual degrees of freedom of a normal linear model with
the scale integrated out, and the determinant term is the Occam factor. Retaining the
constant terms rather than dropping them, as the Schwarz criterion does, is the only
departure, and it is a departure toward the exact calculation rather than away.

- Schwarz (1978), *Estimating the dimension of a model*.
  [Project Euclid](https://projecteuclid.org/journals/annals-of-statistics/volume-6/issue-2/Estimating-the-Dimension-of-a-Model/10.1214/aos/1176344136.full)
- MacKay (1992), *Bayesian interpolation*, Neural Computation 4(3), 415–447, which is
  the standard source for the Occam factor as a ratio of volumes.
  [CaltechAUTHORS record](https://authors.library.caltech.edu/records/r7qgh-q6g10) ·
  [MacKay's papers](https://www.inference.org.uk/mackay/PhD.html)
- Kass and Raftery (1995), as above.

The related information-theoretic account, in which
$\sum \log \lvert A_{ij} \rvert$ and the Occam term appear as code lengths, is the
minimum description length literature. It is worth one sentence in the paper, because
a listener from that tradition will recognise the criterion.

- Wallace, *Statistical and Inductive Inference by Minimum Message Length*.
  [Springer](https://link.springer.com/book/10.1007/0-387-27656-4)
- Dhillon, Foster and Ungar (2011), *Minimum description length penalization for group
  and multi-task sparse learning*, JMLR 12.
  [JMLR](https://jmlr.csail.mit.edu/papers/v12/dhillon11a.html)

### 3.6 The search

Enumerating subsets per row, then combining rows, exploits the fact that the score is
a sum of per-row terms once the shared parameters are fixed. That property is called
decomposability, and it is the same property that makes exact structure learning of
Bayesian networks possible by dynamic programming.

- Silander and Myllymäki (2006), *A simple approach for finding the globally optimal
  Bayesian network structure*. [arXiv:1206.6875](https://arxiv.org/abs/1206.6875)
- *Bayesian selection of best subsets via hybrid search*, Computational Statistics.
  [Springer](https://link.springer.com/article/10.1007/s00180-020-00996-y)
- Kowal, *Bayesian subset selection and variable importance*, JMLR 23.
  [JMLR (PDF)](https://www.jmlr.org/papers/volume23/21-0403/21-0403.pdf)

The specific device in Section 10.2 — conditioning on a grid of $\sigma$ values so
that a criterion which is not separable becomes separable, running the dynamic
program at each grid point, and scoring the resulting candidates exactly — I did not
find published. It is a small idea and it is a natural one, so absence from the
search is weak evidence. Describe it, do not claim it.

## 4. The two methods a reader will name

Two alternatives come up whenever this problem is described, and they are the two
the paper is measured against in Section 5. Both are given here in the notation of
the paper, so that one row of the matrix is a regression of the $T$-vector $y$ on
the $T \times N$ matrix $Z = X'$ with the $N$-vector $a$ to be found.

### 4.1 The lasso

The lasso replaces least squares by least squares with a penalty proportional to the
sum of the absolute values of the coefficients:

$$\hat{a}(\lambda) = \arg\min_a \tfrac{1}{2}\|y - Za\|^2 + \lambda \sum_j |a_j|$$

Two facts account for its position. The first is that the absolute value has a
corner at the origin, and a penalty with a corner drives coefficients to exactly
zero rather than merely close to it. Selection and estimation are therefore done in
one step, and no search over patterns is required at all — which is precisely the
difficulty Section 11 of the paper leaves open. The second is that the problem is
convex, so it solves reliably and quickly at sizes far beyond anything in the paper.
Together these make it the default tool for this regime.

Its relation to the present work is closer than it looks. The minimiser is the
maximum-posterior estimate under a double-exponential prior on each coefficient,
with $\lambda$ playing the part of the rate. It is therefore an estimate of exactly
the kind the paper uses, differing in the prior rather than in the principle. The
differences are that the double-exponential is peaked at zero rather than bounded
away from it, and that there is no separate indicator for occupancy: a cell is empty
when the penalty happens to drive it to zero, not because a discrete choice was made
about it.

Three properties matter for the comparison. The penalty must be supplied, and
cross-validation, the usual answer, is aimed at prediction rather than at recovering
the pattern; it is known to select too many cells, and with six observations in a
row there is very little to cross-validate upon. The surviving coefficients are
pulled toward zero, so refitting by least squares on the selected pattern is normal
practice. And the lasso recovers the correct pattern only when the columns of $Z$
satisfy a condition relating the occupied columns to the empty ones, which cannot be
checked without knowing the answer.

The adaptive lasso addresses the third point by penalising each coefficient in
inverse proportion to a first-pass estimate of its size, so that large coefficients
are penalised little and small ones a great deal. It is the strongest variant that
can fairly be asked for here.

- Tibshirani (1996), *Regression shrinkage and selection via the lasso*, JRSS-B
  58(1), 267–288.
  [PDF](https://webdoc.agsci.colostate.edu/koontz/arec-econ535/papers/Tibshirani%20%28JRSS-B%201996%29.pdf)
- Park and Casella (2008), *The Bayesian lasso*, JASA 103(482), 681–686, for the
  double-exponential reading.
  [PDF](https://people.eecs.berkeley.edu/~jordan/courses/260-spring09/other-readings/park-casella.pdf)
- Zhao and Yu (2006), *On model selection consistency of lasso*, JMLR 7, 2541–2563,
  for the condition on the design.
  [JMLR (PDF)](https://www.jmlr.org/papers/volume7/zhao06a/zhao06a.pdf)
- Zou (2006), *The adaptive lasso and its oracle properties*, JASA 101(476),
  1418–1429. [PDF](https://pages.stat.wisc.edu/~shao/stat992/zou2006.pdf)

### 4.2 Stochastic search variable selection

SSVS attaches an indicator $\gamma_{ij}$ to each cell and makes the prior on the
coefficient depend upon it:

$$A_{ij} \mid \gamma_{ij}=0 \sim N(0, \tau_0^2), \qquad
  A_{ij} \mid \gamma_{ij}=1 \sim N(0, \tau_1^2), \qquad
  \gamma_{ij} \sim \mathrm{Bernoulli}(p)$$

with $\tau_0$ small and $\tau_1$ large. The essential device is that neither
component is exactly zero. Because every coefficient always has a value, every
conditional distribution in the model is a standard one, and the whole posterior can
be explored by drawing in turn the coefficients, the indicators, the occupancy rate
$p$, and the error variance. No move that changes the number of parameters is ever
needed, which is what makes the sampler practical.

The output is not a single pattern but a sample of them. The proportion of draws in
which a cell is occupied is its posterior inclusion probability, and the usual
summary is the median probability model, which takes every cell whose probability is
at least one half. That choice is not arbitrary; it is optimal for prediction under
stated conditions.

The resemblance to the paper is very close. The pattern prior is the same Bernoulli
form, the hyper-prior on $p$ is the same Beta, one error scale serves the whole
matrix, and the hierarchy is arranged in the same order. Three things differ. The
coefficient prior is a pair of normal densities rather than a bounded log-uniform.
The pattern is sampled rather than maximised, so the answer is a set of
probabilities rather than one matrix. And no integral is performed in closed form.

The two widths $\tau_0$ and $\tau_1$ play exactly the part played by $a_{\min}$ and
$a_{\max}$ in the paper. They are not determined by the data, the analyst must
supply them, and the answer moves when they move.

- George, Sun and Ni (2008), as above, for the vector autoregression case.
- Barbieri and Berger (2004), *Optimal predictive model selection*, Annals of
  Statistics 32(3), 870–897, for the median probability model.
  [Project Euclid](https://projecteuclid.org/journals/annals-of-statistics/volume-32/issue-3/Optimal-predictive-model-selection/10.1214/009053604000000238.full)

### 4.3 What size of problem SSVS has actually been applied to

This question was asked because the sampler failed to converge at N = 200, and it
matters a great deal whether that is a defect of our implementation or a property of
the method at that size. The published record is unambiguous on one point: **no
application of SSVS at anything approaching forty thousand coefficients was found.**

Every figure below was read out of the paper itself, not from an abstract or a
summary.

| Study | Model | Coefficients | Data | MCMC draws |
|---|---|---|---|---|
| George, Sun & Ni (2008) — application | 7-variable VAR, 12 lags | **595** in Φ, 28 in Ψ | monthly, 1969–80 (144 obs) and 1981–2005 (296 obs) | 20,000 after 10,000 burn-in |
| George, Sun & Ni (2008) — simulations | 4-variable VAR, 2 lags | ~36 | simulated | 50,000 after 10,000 burn-in |
| Korobilis (2013) | 3-equation VAR, 168 right-hand-side variables per equation | **504** | monthly, 1960:1–2003:12 | 150,000 after 50,000 burn-in |
| Chan, *Large Bayesian VARs* | 20-variable VAR (SSVS among the priors compared) | — | quarterly, 1964Q1–2015Q4 | — |

Two things stand out.

**George, Sun and Ni describe 595 coefficients as a large number of parameters**, and
give that as their reason for the number of MCMC cycles they ran. Our problem has
40,000 — sixty-seven times as many. Korobilis, whose model has 504 coefficients,
notes that the number of candidate models is $2^{168}$ in each equation and runs
150,000 iterations.

**Sweeps per coefficient differ by two to three orders of magnitude.** George, Sun and
Ni ran about 34 draws per coefficient; Korobilis about 298. The runs reported in
Section 5 gave 0.2 draws per coefficient at 8,000 sweeps and 0.5 at 20,000. By the
crudest yardstick available, the sampler was given a fraction of a per cent of the
computing that the published applications used.

### 4.4 Whether slow mixing is expected

It is, and there is theory saying so. Yang, Wainwright and Jordan (2016) prove that
posterior concentration — the statistical property that makes the spike-and-slab
posterior correct — **does not imply rapid mixing** of the MCMC algorithm used to
explore it. They obtain a mixing time linear in the number of covariates, up to a
logarithmic factor, only after introducing a truncated sparsity prior and analysing a
particular Metropolis–Hastings algorithm; and they distinguish explicitly between
rapid mixing, where the mixing time grows polynomially, and slow mixing, where it
grows exponentially.

That is the theoretical setting for what we observed. The plain Gibbs sampler of
George and McCulloch is not the algorithm for which rapid mixing has been proved, and
a correct posterior is no guarantee that it can be reached.

There is also a body of methods work that exists precisely because the plain sampler
does not scale — Biswas, Mackey and Meng (2022) reduce the per-iteration cost from
order $n^2p$; and in genomics, where the predictor counts are far larger, the field
moved to variational Bayes and evolutionary stochastic search because MCMC was too
slow for the size of modern SNP chips.

### 4.5 What this means for the comparison

Stated fairly:

- Our implementation is the textbook George–McCulloch sampler. Nothing found suggests
  it is wrong. It reproduced the N = 9 results to the last bit after the Cholesky
  change, and at N = 9 it tied the method of the paper exactly.
- It is, however, the *plain* sampler, and the published applications are 60 to 80
  times smaller while using 100 to 1000 times more sweeps per coefficient.
- Therefore **"SSVS fails at N = 200" is not a defensible claim.** The defensible claim
  is narrower: the plain Gibbs sampler, run for as many sweeps as are affordable here,
  has not converged at this size — which is consistent both with published practice
  and with the theory.
- A blocked sampler, the scalable variant of Biswas and co-authors, or the
  truncated-prior Metropolis–Hastings scheme of Yang and co-authors might do far
  better. None of them has been tried here.

**Sources**

- George, Sun and Ni (2008), as above.
  [Author copy (PDF)](https://faculty.wharton.upenn.edu/wp-content/uploads/2012/04/GeorgeSunNi-JE2008.pdf)
- Korobilis (2013), *VAR forecasting using Bayesian variable selection*, Journal of
  Applied Econometrics 28(2), 204–230.
  [Working paper (PDF)](https://mpra.ub.uni-muenchen.de/21122/1/ssvs.pdf) ·
  [RePEc record](https://ideas.repec.org/a/wly/japmet/v28y2013i2p204-230.html)
- Chan, *Large Bayesian vector autoregressions*.
  [Author copy (PDF)](https://www.joshuachan.org/papers/large_BVAR.pdf)
- Yang, Wainwright and Jordan (2016), *On the computational complexity of
  high-dimensional Bayesian variable selection*, Annals of Statistics 44(6),
  2497–2532.
  [Project Euclid (PDF)](https://projecteuclid.org/journals/annals-of-statistics/volume-44/issue-6/On-the-computational-complexity-of-high-dimensional-Bayesian-variable-selection/10.1214/15-AOS1417.pdf)
- Biswas, Mackey and Meng (2022), *Scalable spike-and-slab*.
  [arXiv:2204.01668](https://arxiv.org/abs/2204.01668)

## 5. The comparison on the illustrations of the paper

Both methods were run on the two illustrations of the paper, over the same forty
seeds, with the same matrices, the same inputs and the same measurement errors. The
comparison is paired at the level of the seed and at the level of the case, giving
eighty paired comparisons. The script is
[`examples/compete.py`](examples/compete.py); the output is `examples/compete.txt`.

Four variants of the lasso were run, because one would not settle the matter.
`lasso-cv` chooses the penalty by leave-one-out cross-validation, which is what an
analyst without a noise estimate would do. `lasso-bic` chooses it by the Schwarz
criterion **given the true error scale**, which no analyst would have. `alasso-bic`
is the adaptive lasso, reweighted from the second. `lasso-best` chooses the penalty
row by row by counting pattern errors against the truth and taking the penalty that
makes them fewest — this requires the answer, so it is not a method but a bound on
what any choice of penalty could achieve. All four have their coefficients refitted
by least squares on the selected pattern, so they are charged for the pattern rather
than for the shrinkage.

SSVS was run with $\tau_0 = 0.02$, $\tau_1 = 1.5$, twenty thousand draws of which
the first five thousand were discarded. Those widths bear the same relation to the
true magnitudes, drawn log-uniformly on $[0.40, 3.00]$, that the bounds $[0.20,
5.0]$ of the paper do, so neither method is given better information than the other.

### 5.1 What the forty realisations show

| case | method | correct of 27 | false | missed | exact | median σ̂ |
|---|---|---|---|---|---|---|
| block | **mple** | **26.20** | **1.57** | **0.80** | **14 of 40** | 0.091 |
| block | lasso-cv | 20.85 | 18.43 | 6.15 | 0 of 40 | 0.614 |
| block | lasso-bic | 23.70 | 18.27 | 3.30 | 0 of 40 | 0.065 |
| block | alasso-bic | 23.32 | 8.28 | 3.67 | 0 of 40 | 0.092 |
| block | lasso-best | 19.27 | 1.68 | 7.72 | 0 of 40 | 0.746 |
| block | **ssvs** | **26.32** | **1.50** | **0.68** | **10 of 40** | 0.092 |
| scatter | **mple** | **26.40** | **1.40** | **0.60** | **17 of 40** | 0.089 |
| scatter | lasso-cv | 21.73 | 19.30 | 5.28 | 0 of 40 | 0.430 |
| scatter | lasso-bic | 23.98 | 17.00 | 3.02 | 0 of 40 | 0.065 |
| scatter | alasso-bic | 23.57 | 7.85 | 3.42 | 2 of 40 | 0.096 |
| scatter | lasso-best | 19.68 | 2.35 | 7.33 | 0 of 40 | 0.696 |
| scatter | **ssvs** | **26.00** | **1.12** | **1.00** | **17 of 40** | 0.094 |

The two `mple` lines reproduce the table of Section 10.5 of the paper exactly, which
confirms that the procedure compared against is the procedure the paper describes.

Counting a pattern error as a cell wrongly occupied or wrongly empty, the eighty
paired comparisons fall out as follows:

| method | worse than mple | better | equal | two-sided sign test |
|---|---|---|---|---|
| lasso-cv | 80 | 0 | 0 | $p = 2 \times 10^{-24}$ |
| lasso-bic | 80 | 0 | 0 | $p = 2 \times 10^{-24}$ |
| alasso-bic | 77 | 2 | 1 | $p = 1 \times 10^{-20}$ |
| lasso-best | 79 | 0 | 1 | $p = 3 \times 10^{-24}$ |
| ssvs | 24 | 24 | 32 | $p = 1.00$ |

### 5.2 The lasso is not competitive here

No variant comes close, and the variant given the answer does not come close either.
The best of them, the adaptive lasso, occupies more than eight cells that should be
empty and misses nearly four that should be occupied, against a little over two
errors in total for the paper's procedure. Averaged over the eighty comparisons the
total number of pattern errors is 11.61 for the adaptive lasso and 9.54 for the
oracle variant, against 2.19 for the paper.

The reason is not obscure. With six observations and nine candidates in each row,
the condition under which the lasso recovers the correct pattern (Zhao and Yu) is
unlikely to hold, and a single continuous penalty must serve two purposes at once:
large enough to empty six cells, small enough to retain three. The oracle variant
shows that no penalty does both, since even with the answer in hand it must trade
nearly eight missed cells for its low false count. Cross-validation, aimed at
prediction, fails in the opposite direction and occupies about nineteen cells too
many.

**This closes the objection** that the paper compares itself only against the
minimum-norm pseudo-inverse. It does not, now, and the comparison is not close.

### 5.3 SSVS is a dead heat

This is the more important result and it must be reported plainly. Against SSVS the
paper's procedure wins 24 of the 80 comparisons, loses 24, and ties 32. The sign
test gives $p = 1.00$, which is as even as the arithmetic permits. Mean pattern
errors are 2.19 for the paper against 2.15 for the sampler. Exact recoveries are 31
of 80 against 27, and a test of that difference on the twelve discordant cases gives
$p = 0.388$. The estimated error scale is 0.0900 against 0.0925, with the truth at
0.1000.

The honest summary is that the criterion developed in the paper attains the accuracy
of the established method at this size, and does not exceed it.

Two differences are real. The first is the worst case. The sampler goes wrong less
often — it exceeds five pattern errors in 6 of the 80 comparisons against 11 for the
paper's procedure — but when it does go wrong it goes further: its worst trial
misplaces **21** cells against a worst of **10**. The second is that the paper's
procedure is
deterministic — it evaluates a closed-form criterion on candidates from a dynamic
programme and returns the same answer every time, whereas the sampler returns a
distribution and can differ between chains on the same data.

### 5.4 Both methods depend on widths the analyst supplies

The most natural objection to the paper is that $a_{\min}$ and $a_{\max}$ are not
determined by the data. The same objection applies with at least equal force to the
alternative. The two widths were varied over four settings, two chains each, on seed
20260726 (`examples/ssvs_sensitivity.py`):

| $\tau_0$ | $\tau_1$ | ratio | block | scatter |
|---|---|---|---|---|
| 0.005 | 3.00 | 600 | $M = 2$, 25 missed | $M = 7$ or 27, chain-dependent |
| 0.020 | 1.50 | 75 | 27 correct, 0 or 1 false | exact |
| 0.050 | 1.00 | 20 | exact | exact |
| 0.100 | 0.60 | 6 | $M = 81$, every cell | $M = 81$, every cell |

Outside a fairly narrow band the method fails, and it fails in both directions. When
the wide component is made wider the model collapses to nearly empty — that is the
Bartlett–Lindley effect of Section 3.2 acting inside the sampler itself, since
spreading the prior over a larger range lowers the marginal likelihood of every
occupied cell. When the narrow component is made wider it ceases to represent an
empty cell at all, and every cell is occupied.

This does not excuse the paper. It does establish that the requirement is a property
of the problem rather than of the paper's formulation, and that the paper's bounds
have an advantage of form: they are stated in the units of the coefficients, as the
smallest and largest magnitude worth calling non-zero, which is a quantity an
analyst can be asked about. A pair of prior standard deviations whose effect is not
monotone is harder to elicit.

## 6. Block arrangement recovered from data

This is the section of the work most exposed to correction, and it deserves care.

The premise given for the search was that the global vector autoregression fixes a
block arrangement in advance, so that recovering one from data would be of interest.
That premise is sound, and it is also the premise of a live and growing literature.
Several groups have published methods that infer group or block membership from the
data instead of assuming it, and they do so with consistency proofs and at
cross-sections far larger than nine.

- Gudmundsson and Brownlees (2021), *Detecting groups in large vector
  autoregressions*, Journal of Econometrics 225(1), 2–26. This introduces the
  stochastic block vector autoregression and a spectral clustering procedure that
  consistently recovers the latent groups.
  [Barcelona School of Economics record](https://bse.eu/research/publications/detecting-groups-large-vector-autoregressions) ·
  [EconPapers](https://econpapers.repec.org/RePEc:eee:econom:v:225:y:2021:i:1:p:2-26)
- Brownlees, Gudmundsson and Lugosi (2017), *Sparse estimation of huge networks with
  a block-wise structure*, Econometrics Journal 20(3), S61–S85.
  [RePEc record](https://ideas.repec.org/a/wly/emjrnl/v20y2017i3ps61-s85.html)
- Billio, Casarin and Rossini (2019), *Bayesian nonparametric sparse VAR models*,
  Journal of Econometrics 212(1), 97–115. A nonparametric prior clusters the
  coefficients into groups without the groups being specified.
  [arXiv:1608.02740](https://arxiv.org/abs/1608.02740)
- *Learning bi-clustered vector autoregressive models*, ECML PKDD 2012. Sparse
  learning with a nonparametric bi-clustered prior, by blocked Gibbs sampling.
  [Springer](https://link.springer.com/chapter/10.1007/978-3-642-33486-3_47)
- Kim and Baek (2026), *Latent community paths in VAR-type models via dynamic
  directed spectral co-clustering*, which allows the block membership to change over
  time. [arXiv:2604.12563](https://arxiv.org/abs/2604.12563)

Plainly stated: the ability to recover a block arrangement from data, without
imposing it, is not new and is not rare. Any claim of novelty on that ground will be
corrected, probably by whoever in the audience follows the econometrics literature.

There is, however, a real and defensible distinction, and it is worth making
precisely. Every method listed above contains machinery for grouping — a block model,
a clustering prior, a spectral step. Each is built to find groups. The method in this
paper contains no such machinery at all. It treats every cell of the matrix alike and
fits each row separately, and the block arrangement, when there is one, emerges as a
by-product of selecting cells one at a time.

The paper already says the honest thing about this, in Section 10.6 and in the
forty-trial comparison: block and scattered arrangements are recovered equally well,
the sign test gives $p = 0.345$, and no difference should be expected, since the
prior treats all cells alike. That result is the right thing to present. It is
evidence that no block assumption is needed for this kind of recovery, which is a
statement about the block assumption rather than a claim of a new capability. Framed
that way it is defensible. Framed as recovery of block structure it is not.

One suggestion. The methods above are aimed at a different regime, with many series
and long samples, where the aim is to summarise a large system by a few groups. The
present method is aimed at the regime where the sample is shorter than the
cross-section and the ordinary estimate does not exist at all. Saying which regime
the work addresses will forestall most of the objection.

## 7. The AutoClass framing

Cheeseman and Stutz applied the same hierarchical construction — a discrete structural
choice, parameters within the structure, and an approximated marginal likelihood to
choose among structures — to classification. The paper carries that construction to
the selection of a sparsity pattern in a linear model. I found no publication that
makes this extension or cites AutoClass in a sparse-regression setting. The
lineage is a matter of presentation rather than of method, and the underlying
approximation is the ordinary Laplace one, so the framing is legitimate but should be
offered as a way of organising the argument, not as a technical contribution.

- Cheeseman and Stutz, *Bayesian classification (AutoClass): theory and results*.
  [Internet Archive](https://archive.org/details/nasa_techdoc_19920075544) ·
  [NASA Technical Reports Server](https://ntrs.nasa.gov/citations/19920075544)

## 8. What appears to be new

Four items survived the search. None is a new method. Each is small, checkable, and
safe to state.

**The explicit criterion.** Equations (7.1) and (9.4), with the constants retained and
with the $\sum \log \lvert A_{ij} \rvert$ term that follows from a bounded log-uniform
prior, are not displayed in this form in anything found. The ingredients are all
standard; the assembled and normalised criterion is not in print.

**The diagnosis of the residual over-fitting.** Section 10.6 attributes the surviving
falsely occupied cells to the Laplace step, on the evidence that they cluster at small
magnitudes — median $0.138$ against $1.171$ for the correct cells, and a third below
$0.10$ against fewer than one in two hundred — which is where the approximation
ignores the truncation at $a_{\min}$ and treats $\prod 1/\lvert A_{ij} \rvert$ as
constant. I found no comparable attribution in the literature. It is a modest,
verifiable, and genuinely useful observation, and it is the strongest thing in the
paper to present as new.

**The deterministic treatment throughout.** Nearly all of the Bayesian work cited
above samples. This work maximises and integrates in closed form, with a deterministic
search. Deterministic Bayesian subset selection exists, but not, so far as the search
shows, with this criterion or in this regime.

**The fixed-$\sigma$ dynamic program.** As discussed in Section 3.6 above. Weak, but
not found.

To that should be added something that is not a result but is worth stating: the
paper is self-contained. Every constant is normalised, every integral is verified in
Maxima, and every number a reader needs to reproduce the illustrations is printed in
the appendix. Very little of the cited literature can be reproduced from the paper
alone. That is a real service to a reader and it is safe to say so.

## 9. What a colleague is most likely to raise

Anticipating these will be worth more than any of the above.

**There is no comparison against the lasso.** This was the most likely objection.
Section 5 answers it, and answers it comfortably. It should be answered in the paper
as well, by a table, and not left to be raised from the floor.

**The criterion attains but does not exceed SSVS.** Somebody will ask what the paper
buys. The answer is not that it is more accurate, because it is not. The answer is
that it is deterministic, that its worst case over eighty trials was less than half
as bad, and that it produces a criterion which can be examined term by term, so the
source of a failure can be identified — Section 10.6 does exactly that, and no
sampler yields such an account. Saying this plainly is much stronger than claiming an
advantage the numbers do not support.

**The bounds must be supplied by the analyst.** The criterion depends on $a_{\min}$,
$a_{\max}$, $s_0$ and $s_1$, and Section 10.6 shows that lowering $a_{\min}$ from
$0.20$ to $0.05$ raises the falsely occupied cells from $1.49$ to $5.90$. That is a
genuine sensitivity, and the paper is candid about it. It is now known that the
alternative is at least as sensitive to its own two widths and fails in both
directions outside a narrow band, which makes the objection one about the problem
rather than about the paper.

**The scale of the illustration.** Nine by nine, six observations. The cited
literature works at hundreds of series. Say at the outset that this is the smallest
problem that exhibits the phenomenon and that the illustration is chosen so the reader
can check every number, and the point will not be pressed. Add that the search of
Section 10.2 enumerates subsets, and that nothing in the paper establishes how the
method behaves when $q$ is forty thousand.

**Approximate rather than exact marginal likelihood.** The Laplace step is an
approximation, and Section 10.6 shows it is the one that costs something. This is
already acknowledged in the paper.

## 10. Summary judgement

Nothing in the paper duplicates a specific published result outright, and nothing in
it is a mistaken statement of what is known. But the great majority of its technical
content is standard material assembled carefully, and two of its findings — the
failure of the improper priors and the behaviour of the Beta hyper-prior — restate
results that are in the standard references. Presented as an exposition of how a
maximum-posterior criterion for a sparsity pattern is built from first principles,
with everything normalised and verified, the work is sound and useful and will draw
no correction. Presented as a new method for recovering sparse or block-structured
matrices, it will.

The numerical comparison sharpens this. Against the lasso, in four variants, two of
which are given information no analyst would have, the method wins 79 or 80 of 80
paired trials. Against SSVS it wins 24, loses 24, and ties 32. The first result is
worth presenting and closes the most likely objection. The second is worth presenting
because it is true, and because presenting it removes any suggestion that the
comparison was arranged to flatter.

Three things are worth putting forward: the diagnosis of where the residual
over-fitting comes from, which was not found in the literature; the demonstration
that no grouping mechanism is required to recover a grouped arrangement; and the
observation that a deterministic criterion, evaluated in closed form, matches a
Markov chain method at this size with a smaller worst case. All three are modest, all
three are supported by the numbers in hand, and all three will hold up.
