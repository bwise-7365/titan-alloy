
# Research Plan: Five-Year EMA Trading Experiments with CARA Utility

## 1. Objective

Evaluate a simple long-only, state-based stock trading rule over five-year historical periods, using an exhaustive grid of buy and sell exponential-average parameters. Assess candidate rules using **five-year dollar profit or loss** and CARA utility. First generate and inspect the simulated outcomes; only afterward explore Bayesian predictive models and candidate distribution families, without presupposing an analytical form or finite variance. Use approximately two-thirds of stocks for model selection and reserve the remaining third for a final out-of-sample evaluation.

**Unit of experiment:** one stock, one calendar-year start, one five-year horizon, one pair of EMA parameters. Each experiment is financially independent of all other experiments. No shared portfolio, capital budget, or compounding between experiments is assumed.

## 2. Experimental calendar and observations

- Start a new experiment on the first trading day of each calendar year for which a complete five-year follow-up is available. Initial position is a zero balance. 
- Each experiment ends at the close of the last trading day before its fifth anniversary (a precise calendar convention to be fixed in code and documented).
- An experiment uses only the specified set of stocks in the portfolio of training stocks (described below). Experiments may overlap, and the same set of stocks is used in successive annual starts.
- Five-year windows started one year apart share roughly four of five years of market history. Their **returns are correlated**, and the degree of statistical dependence must be estimated rather than assumed to equal 0.8. The 80% figure describes approximate calendar overlap, not necessarily return correlation.
- Pre-start history may be used solely to initialize EMA state. No trades or cash flows from that history are carried into the experiment.
- Do not treat all stock-window observations as independent in inference. Stocks also share market-wide shocks and sector exposures.

## 3. Trading rule

Let P_t be the split-adjusted closing price on trading day t. For K in {10,...,100}, compute the K-day  exponential moving average as follows. 

A_t = ((K-1) A_(t-1) + 2*P_t)/(K +1).

The smoothing parameter is the conventional technical-analysis 2/(K+1). Use a documented warm-up (preferably at least 500 trading days) before the first experiment start and exact average thereafter to the end of the last experiment. The closing price of day t is included in A_t.

At each close, evaluate the **state-based** rule :

- If flat and P_t > A_t for an unheld stock using the M day average, signal a buy.
- If holding shares and P_t < A_t for a held stock using the N-day average, signal a sale.
- Otherwise do nothing; equality causes no action.

Only one position in a stock can be open in an experiment at a time, though multiple stocks in the training set can be held simultaneously. A fresh $10,000 purchase is made on **every  buy**, including re-entries, regardless of prior realized gains or losses. Position size is not tied to any cash balance or to another experiment. This means the strategy is a sequence of fixed-notional trades, not a self-financing portfolio. Record the maximum capital committed and gross purchase notional as diagnostics.

## 4. Execution, accounting, and terminal payoff

A signal is determined using information available through close to. Because stocks are not bought or sold instantly, we use a conservative execution convention. For the deliberately conservative retrospective execution convention:

- Buy execution price: B_t = 1.002 max(P_t, O_(t+1)).
- Sell execution price: S_t = 0.998 min(P_t, O_(t+1)).

Here O_(t+1) is the next session's split-adjusted opening price. The 20 basis points are charged on each side, through the effective prices above. This is a conservative *ex-post valuation convention*, not an executable instruction, since the worse of two prices is known only afterward. Include a secondary sensitivity run using next-open-only execution plus the same 20-bp charge.

For each completed buy/sell pair, with purchased shares q_j = floor(10000/B_j), realized P&L is q_j(S_j-B_j). **The experiment starts with zero cumulative P&L, not a zero-cash, self-financing brokerage account.** There is no constraint that a purchase must be financed from previous sales; the $10,000 trade notional is an exogenous per-entry assumption.

At the five-year end, any open position must be valued consistently. Recommended convention: force liquidation at the final eligible close with a 20-bp sell cost (rather than using a next-day opening outside the five-year window). Report this terminal-close exception explicitly; alternatively, end signals a day earlier and use a within-window next opening. Apply the same rule to all parameter combinations.

Define total five-year payoff of the (M, N) strategy applied to all stocks in the portfolio as

X_(y,M,N) = sum of realized trade P&Ls, including terminal liquidation, summed over stocks, and start year y. 

Apply CARA utility **once to this total over all stocks in the training set**, not separately to individual trades, to individual stocks, or to annual returns. A run with no purchases of any stocks has X=0. Do not impose a maximum on total five-year gains; repeated $10,000 entries can accumulate large positive P&L. Do not impose a maximum on total five-year losses; repeated $10,000 entries can accumulate large negative P&L. Track dividends as cash receipts while shares are held, or state explicitly that the initial analysis omits them; dividends should ideally be included in the principal specification. Split-adjusted prices and share quantities must be consistent with the dividend records.

Because every entry deploys another $10,000 and proceeds are not recycled into a common cash account, X can reflect several sequential trades of several stocks. For a fully paid long-only position, each entry's downside is bounded by its purchase notional, but the **total five-year loss** can exceed $10,000 if multiple entries lose money. No borrowing cost is modeled initially; the economic meaning of repeated external funding should be reported as a limitation.

## 5. CARA calibration and certainty equivalent

Use U(X) = -exp(-a X). Set a by requiring a $10,000 loss to cause twice the utility decrement of the utility increment from a $10,000 gain:

U(0)-U(-10000) = 2[U(10000)-U(0)].

This gives a = ln(2)/10000 ≈ 6.93147 × 10^-5 dollars^-1.

When a predictive distribution has been justified from the observed outcomes, and its exponential moment exists, calculate

CE = -(1/a) ln E[exp(-a X)].

The objective is to maximize the posterior predictive CE, rather than average return, CAGR, or Sharpe ratio. Because each new entry is fixed at $10,000, the CARA calibration measures risk-averse  assessment total dollar gains or  losses from a sequence of trades over five years. It is not equivalent to the risk or gains of investing a single $10,000 stake once.

Use numerically stable log-sum-exp computations for empirical or simulated expected exponential utility. Report mean payoff, median, lower quantiles, probability of a negative payoff, and CE together. Compare with always-cash (X=0) and a carefully defined buy-and-hold benchmark (one $10,000 purchase at the beginning, liquidation at the end). Because we use a training portfolio, use the index of the training portfolio to buy and hold. Note that buy-and-hold uses a single entry while the strategy can use multiple entries; this is an intentionally simple benchmark, not equalized capital exposure.

## 6. Stock universe and validation design

Construct the stock universe without selecting on subsequent survival where possible; retain delisted securities and corporate-action histories when available. Free price feeds may not permit this, so record the resulting survivorship limitation. Preserve source provenance, timestamps, split/dividend actions, and missing-data flags.

Split **stocks**, not individual overlapping runs, into a training set of two portfolios, each approximately 1/3 of the stocks, and 1/3 holdout set, preferably stratified by sector, historical coverage, and liquidity. All windows of a given stock remain in the same partition. Fix and save the random seed and the stock IDs. Do not use holdout results to choose parameters, priors, data-cleaning exceptions, or risk-aversion calibration.

Thus, for the forty-five year period from 1980 to 2025, there would be 90 values of  X(y, M, N): two for each year from the two training portfolios. The final evaluation of strategy (M, N) is the CE of those 90 values. 

This holdout tests transfer across stocks but not across time, because both partitions share market regimes. Add decade-by-decade diagnostics and, if feasible, a separate forward-time validation. The final one-third stock holdout should be evaluated once after the methodology is frozen.

## 7. Empirical-first statistical and Bayesian analysis

**Do not assume any analytical payoff distribution before running the simulations.** The payoffs of a strategy are determined solely by the stocks and the (M, N) parameters. The only random choice is the partition into two training and one hold out portfolios. The first deliverable is the complete collection of realized five-year P&Ls for every annual start, and (M,N) pair. (Note again that this is the P&L of the (M, N) strategy applied to the entire portfolio for 5 years, not a single stock or single year). Preserve all observed losses exactly; do not winsorize, clip, censor, cap, discard, or stop a run because it performs very well or very badly. Never introduce a maximum cumulative loss or maximum gain as a modeling convenience. Runs terminate at the prescribed five-year endpoint, except for genuine data or explicitly modeled feasibility constraints, which must be reported rather than silently filtered.

Recommended sequence:

1. **Generate outcomes before fitting distributions to them.** Run and verify the deterministic C++ engine. Retain individual P&Ls, trade histories, dates, and stock identifiers, including the most favorable and most adverse outcomes. Compute empirical utility with stable log-sum-exp arithmetic; no parametric likelihood is required for this descriptive step.
2. **Inspect empirical structure.** Examine the full distribution, extreme negative and positive observations, quantiles, empirical survival functions, year/sector patterns, and dependence between stocks and overlapping windows. Report the worst outcomes and their underlying trade sequences. Examine whether sample variance and empirical exponential-utility estimates are stable as the sample grows.
3. **Explore distribution families only after seeing the data.** Consider nonparametric, semiparametric, finite-variance, and infinite-variance candidates if warranted by the evidence. Do not exclude a family merely because its variance is infinite, and do not force a Gaussian, lognormal, Student-t, or other form at the outset. Compare tail fit, posterior predictive checks, and sensitivity to thresholds and sample coverage. A finite sample generally cannot establish whether the population variance is finite or infinite; report uncertainty instead of declaring a definitive tail class.
4. **Represent dependence.** Five-year windows begun one year apart overlap by approximately 80% in time, but their payoff correlation is not necessarily 0.8. Stocks share market and sector shocks. Use dependence-aware resampling for exploratory uncertainty and, when justified, a Bayesian hierarchical or other predictive model that captures common years, stocks, and overlapping windows. Do not treat stock-year observations as independent.
5. **Check existence of CARA expectations, without modifying losses to make them exist.** For a candidate predictive law, CE = -(1/a) log E[exp(-aX)] is finite only if the negative exponential moment exists. Some infinite-variance distributions have finite exponential moments; others do not. If the expectation diverges, report the implied CE as negative infinity or undefined under that model, and investigate whether the conclusion is supported by the data. **Do not truncate the negative tail or impose a loss cap merely to obtain a finite CE.** Distinguish the always-finite empirical-sample CE from claims about an unobserved population tail.
6. **Select on training stocks only.** Compare posterior predictive utility or CE across all 8,281 parameter pairs, including uncertainty and sensitivity to tail assumptions. Prefer robust parameter regions over isolated optima. Keep the one-third stock holdout untouched until the analysis and selection rules are frozen; then report its full outcome distribution and realized utility without reoptimization.

A Bayesian posterior does not remove selection bias from searching 8,281 rules. The choice of predictive model and priors is itself part of the research and must be disclosed. The purpose of tail modeling is to expose possible severe losses, not to make unfavorable strategies look safer.

## 8. Optimized C++ implementation

For each stock, load contiguous arrays of adjusted open, high, low, close, corporate actions, volume, and trading dates. Precompute 91 EMA series (K=10,...,100) and corresponding Boolean buy/sell conditions. Then run a compact two-state (flat/long) simulator for all 8,281 parameter pairs and all eligible annual starts.

- Complexity of a direct implementation is O(91^2 T) per stock, with a modest multiplier for annual starts if each window is simulated independently. Reuse precomputed signals and execution prices; benchmark before introducing more elaborate optimizations.
- Represent every experiment as a separate state: shares held, cumulative realized P&L, current entry price, trade count, gross notional, and end date.
- Avoid accidental cross-run cash pooling, position sharing, or parameter leakage.
- Use double precision and reproducible test fixtures. Assert accounting identities and split invariance.
- Use multithreading with 12 parallel tasks. Each five-year run over a portfolio is a separate task which shares no variables with any other task. 
- Output columnar files (e.g., Parquet) or compact binary results for downstream statistical analysis, with metadata identifying data source, software version, parameter grid, and conventions.
- Verify the C++ results against a slow reference implementation on small synthetic price paths (steady increase, steady decline, whipsaws, split, dividend, missing session, terminal open position).

## 9. Suggested outputs

1. A 91×91 heatmap of training posterior predictive CE, with uncertainty intervals for selected parameter pairs.
2. Heatmaps of average five-year P&L, median P&L, downside quantiles, number of entries, and turnover.
3. A table of the top robust parameter regions and comparisons with cash and buy-and-hold.
4. Results by starting decade, sector, and stock liquidity category.
5. A final holdout report, with no further tuning.
6. Sensitivity checks for 5/10/20/25-bp one-way costs, next-open versus pessimistic execution, dividends, terminal liquidation, and alternative CARA calibration.

## 10. Decisions fixed versus open

**Fixed by research design:** annual five-year starts; stock-level independent runs; initial zero cumulative P&L; fixed $10,000 purchase on every entry; state-based buy/sell signals; 20-bp cost each side; pessimistic close/next-open convention; exhaustive M,N=10,...,100; CARA utility applied once to each run's total five-year dollar P&L; stock-level 1/3 - 1/3 - 1/3 for train/train/test separation.

**To finalize before coding:** exact fifth-anniversary boundary; whether terminal liquidation uses final close or last within-window next open; treatment of dividends, delistings, suspensions, and missing prices; minimum pre-start history; stock universe and eligibility; the empirical evidence needed before selecting any Bayesian likelihood or dependence structure; and the economic interpretation of repeated $10,000 entries without a shared capital account.

## 11. Recommended sequence

First build and verify the deterministic simulator and payoff accounting. Second examine the empirical 91×91 surfaces and dependence across annual starts. Third inspect the full empirical payoff distributions, including extremes and possible heavy tails, before selecting and checking any Bayesian predictive model; then select a robust parameter region using training stocks only. Finally evaluate the untouched stock holdout and conduct prespecified sensitivity tests. This sequence separates coding correctness, statistical modeling, and out-of-sample inference.

---------------

