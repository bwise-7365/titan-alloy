# Review of "Five-Year EMA Trading Experiments with CARA Utility" (plan v2)

Date: November 5, 2026

## Summary

Several problems in the plan should be fixed before any code is written. Two of them change the results: the CARA scaling (Section 1.1) and the share rounding (Section 1.2). The C++ part is straightforward, and a plain implementation is probably fast enough.

---

## 1. Problems in the plan

### 1.1 CARA scale (most serious)

The parameter a = ln 2 / 10,000 is calibrated to a single $10,000 trade. But X is a total over every stock in a portfolio and every entry over five years.

- With a few hundred stocks, X will plausibly vary by hundreds of thousands of dollars. That is many multiples of 1/a ≈ $14,427.
- For any n equally weighted outcomes, the empirical CE always lies between min(X) and min(X) + ln(n)/a. This holds because (1/n)·e^(−a·min X) ≤ (1/n)·Σ e^(−a·X_i) ≤ e^(−a·min X).
- With n = 82 or 90, that band is about $64,000 wide. If the spread of X is much larger than that, CE is essentially the single worst start-year. The optimizer then becomes a maximin over about 82 numbers.
- The ranking of (M, N) pairs also depends on how many stocks are in a portfolio, which is an accident of the stock split. Doubling the portfolio roughly doubles X.
- Portfolio membership also changes by start year as stocks list and delist. X therefore has a different scale in different years, and CE mixes those scales.

The size of the spread of X is an estimate, because no data have been run. The bound itself is exact.

Three ways to handle this:

| Option | Advantage | Disadvantage |
|---|---|---|
| Keep a fixed | Matches the stated calibration literally | Results depend on portfolio size and reduce to worst-case selection |
| Apply CARA to P&L per stock (X divided by the number of stocks in that portfolio and year) | Independent of portfolio size; the $10,000 calibration stays meaningful | The utility no longer refers to a real dollar total |
| Report CE over a grid of a values | Shows how sensitive the selection is to risk aversion | Gives no single answer |

### 1.2 Share rounding breaks split invariance

Computing q = floor(10000/B) on split-adjusted prices is not invariant to splits.

- Suppose a stock later had a 1:100 reverse split. Its earlier adjusted prices are 100 times the actual prices. An actual $150 price becomes an adjusted $15,000, so q = 0 and the trade never happens.
- Large forward splits make the rounding negligible, so the error is not even consistent across stocks.
- The plan's requirement to "assert split invariance" (Section 8) would fail under this rule.

Fix: use fractional shares (q = 10000/B), or compute q from the unadjusted price on the trade date.

Related point: prices must be adjusted for splits only, not for dividends, when dividends are added as cash. Otherwise dividends are counted twice. Dividend amounts must be in the same split-adjusted per-share units.

### 1.3 The observation count is wrong

- Data from 1980 through 2025 gives start years 1980–2020 for complete five-year windows. That is 41 starts and 82 values, not 90.
- A 500-day warm-up needs data from about 1978. If the data begin in 1980, the first usable start is about 1982.
- Stocks that list after the first start also need their own warm-up before they enter any window.

### 1.4 The effective sample is small

The 82 values come from about 8 non-overlapping five-year periods. They are also two portfolios exposed to the same market. Selecting from 8,281 correlated rules on that sample is a textbook case of backtest overfitting.

Bailey, Borwein, López de Prado and Zhu propose estimating the probability of backtest overfitting (PBO) with combinatorially symmetric cross-validation (CSCV). I suggest adding it to Section 7. See References.

The plan already notes that a Bayesian posterior does not remove selection bias. PBO gives a direct measure of it.

### 1.5 Rapid re-entry when M ≠ N

With a large M and a small N, a dip below the N-day average triggers a sale. If the price is still above the M-day average, the rule buys again on the next signal. Each round trip pays 40 bp plus the pessimistic fill.

This is a real property of the rule, not a bug, but the turnover heatmaps will show it, and you should expect it.

### 1.6 The benchmark does not match the strategy's exposure

The benchmark is one $10,000 purchase of an index, while the strategy can hold $10,000 in every stock at once. With several hundred stocks, the exposures differ by a factor of several hundred.

A fairer simple benchmark is $10,000 in each stock at the start, held to the end (or to delisting). This matches the strategy's limit of one position per stock.

### 1.7 The left tail is bounded

Each entry can lose at most $10,000, and the number of entries in five years is finite (each round trip takes at least two trading days). So the true X is bounded below, and E[e^(−aX)] always exists.

- The "CE = −∞" case in Section 7.5 cannot occur for the real payoff.
- A fitted model with an unbounded left tail is misspecified in that respect.
- In practice the bound is far too loose to matter, so tail modeling still has value for judging how far beyond the observed losses a model places probability.

### 1.8 Partition randomness is not the uncertainty of interest

Section 7 states that the only random choice is the partition into portfolios. The uncertainty that matters for a predictive distribution is over future market periods, not over which stocks fall in which portfolio. The partition can still be useful: if per-stock results are stored (Section 2), many random re-partitions of the training stocks cost almost nothing and show whether the optimum is stable.

### 1.9 Smaller ambiguities

- Section 4: "summed over stocks, and start year y" can be read as summing over years too. X_(y,M,N) is indexed by y, so presumably the sum is over stocks only.
- Section 1 describes a two-thirds / one-third split; Section 6 describes three thirds. These agree only if "training" means two portfolios of one third each. State that explicitly.
- Section 6 calls the training CE the "final evaluation," which conflicts with the holdout.
- Dividend entitlement is unclear when the day after a signal is an ex-date. The pessimistic fill could be either the close of day t or the open of day t+1. Suggested rule: treat every trade as occurring at the open of t+1 for entitlement. Shares are then entitled to an ex-date dividend if held at the close of the day before the ex-date.
- Section 3 says signals are evaluated at each close, but a position signaled at close t is not executed until t+1. State that the position changes at the open of t+1, and that a new signal can occur at the close of t+1.
- The EMA initialization (first value equal to the first price, or a simple average) should be stated. After 500 days at K = 100, its weight is about 4.5 × 10⁻⁵, so the choice barely matters.
- Delisting and trading halts in the middle of a window need a liquidation rule, including delisting returns when available.
- Free price feeds are generally survivorship-biased. Survivorship-free data for 1980–2025 normally require a commercial source.

---

## 2. Proposed C++ approach

### 2.1 Parallelize over stocks, not over portfolio runs

Stocks share no capital, so X for any portfolio is just the sum of per-stock P&L.

- Each task takes one stock and computes P&L for all 41 starts × 8,281 pairs.
- Store these per-stock results. Any portfolio's X is then a sum.
- This lets you draw many random partitions of the training stocks at almost no cost.
- It keeps each stock's data in cache while it is used.

This is equivalent to the plan's "each five-year run over a portfolio is a separate task," because the runs over different stocks do not interact.

### 2.2 Three simulator options

| Option | Description | Advantage | Disadvantage |
|---|---|---|---|
| A. Direct daily loop | Loop over days for each (stock, start, M, N) | Easiest to verify; serves as both reference and production | Slowest of the three, though probably fast enough |
| B. Bitset jump scan | Store each buy and sell condition as a 64-bit bitset per K; find the next signal with `std::countr_zero` (C++20) | Work is proportional to trades rather than days; likely several times faster | Moderately more complex indexing |
| C. Continuous path with spliced window boundaries | Simulate one path per (stock, M, N) over all history; patch each window's start | Fastest | Most error-prone |

Option C rests on this observation: runs that start in different years converge to the same flat/long state after the first few trades, and then follow the same trades.

**Recommendation:** implement A first. Use B only if benchmarks require it, with A kept as the test oracle. C is not worth the risk unless A and B are too slow.

**Rough cost of option A.** For 1,000 stocks: 1,000 × 41 starts × 8,281 pairs × about 1,260 days ≈ 4 × 10¹¹ day-steps. At an assumed 2–5 × 10⁸ simple steps per second per core on 12 cores, that is about 1–3 minutes. This estimate is uncertain; benchmark it.

### 2.3 Module layout

Pure functions and no global state:

- `series.hpp`: struct holding open, close, dividend per split-adjusted share, and dates; CSV loader.
- `ema.hpp`: `std::vector<double> ema(const std::vector<double>& price, int K)`, plus signal bitsets for P > A and P < A. These are stored separately, because equality triggers neither signal.
- `simulate.hpp`: `RunResult simulate(const Series&, const Signals&, Window, int M, int N, const Conventions&)`. It returns P&L, entry count and gross notional, and has no side effects.
- `conventions.hpp`: cost rate, notional, execution mode (pessimistic or next-open), terminal rule, fractional or whole shares. All sensitivity runs change only this struct.
- `driver.cpp`: a thread pool of 12 workers taking one stock each. Results are summed in a fixed stock order so the floating-point totals are reproducible.
- `output`: raw little-endian binary arrays indexed [stock][start][M][N], plus an XML metadata sidecar (data source, software version, parameter grid, conventions, random seed, stock IDs per partition).
- `tests`: the synthetic price paths listed in plan Section 8 (steady increase, steady decline, whipsaws, split, dividend, missing session, terminal open position), with accounting identities checked.

### 2.4 Output format: simpler alternative to Parquet

Parquet requires the Apache Arrow C++ library, which is a large dependency. Raw binary with an XML header is trivial to write, and NumPy or R can memory-map it directly. The trade-off is that you lose Parquet's self-description and compression.

Size estimate for 1,000 stocks: 41 × 8,281 doubles ≈ 2.7 MB per stock for P&L, about 2.7 GB in all; entry counts as 16-bit integers add about 0.7 GB.

### 2.5 Maximum capital committed

This does not decompose by stock, because it depends on how many positions are open on the same day. Compute it in a second pass for the shortlisted (M, N) pairs only.

---

## 3. Maxima checks

The definitions (U, CE, the trading rule) are not checked. The derivations are:

1. The calibration U(0) − U(−10000) = 2[U(10000) − U(0)] gives a = ln 2 / 10000.
2. The plan's EMA recursion equals the standard form with α = 2/(K+1).
3. The width of the empirical CE band, ln(n)/a, for n = 82.
4. The weight of the EMA initial value after 500 days at K = 100.

```maxima
/* Exact arithmetic throughout; float only at display. */
L : 10000$
U(x) := -exp(-a*x)$
d : (U(0)-U(-L)) - 2*(U(L)-U(0))$
r1 : ratsimp(subst(a = log(2)/L, d))$
print("Claim 1, a = ln2/10000:", if r1 = 0 then "PASS" else r1)$
print("  a =", float(log(2)/L))$

alpha : 2/(K+1)$
r2 : ratsimp(((K-1)*A0 + 2*P)/(K+1) - ((1-alpha)*A0 + alpha*P))$
print("Claim 2, EMA equals alpha form:", if r2 = 0 then "PASS" else r2)$

print("Claim 3, CE band width ln(82)/a =", float(L*log(82)/log(2)))$
print("Claim 4, K=100 init weight after 500 days =", float((99/101)^500))$
```

Expected results: PASS on claims 1 and 2, a ≈ 6.93147 × 10⁻⁵, a band width of about $63,600, and an initialization weight of about 4.5 × 10⁻⁵. I have not run this script.

The bound in Section 1.1 (CE between min X and min X + ln(n)/a) follows directly from the inequality stated there; it is not checked symbolically.

---

## 4. Questions to settle before coding

1. How many stocks are in the universe, and what is the data source? Does it include delisted stocks?
2. Fractional shares, or a share count from unadjusted prices?
3. Which CARA scaling: fixed a, per-stock P&L, or a grid of a values?
4. Terminal rule: liquidate at the final close, or stop signals one day early?
5. What is the first year of price data? This determines the first usable start after the warm-up.
6. Raw binary with an XML header, or Parquet?

---

## References

- D. H. Bailey, J. M. Borwein, M. López de Prado, Q. J. Zhu, "The Probability of Backtest Overfitting," *Journal of Computational Finance*, 2017 (DOI 10.21314/JCF.2016.322). Preprint: https://papers.ssrn.com/abstract=2326253
- D. H. Bailey, J. M. Borwein, M. López de Prado, Q. J. Zhu, "Pseudo-Mathematics and Financial Charlatanism: The Effects of Backtest Overfitting on Out-of-Sample Performance," *Notices of the American Mathematical Society* 61(5), May 2014, pp. 458–471. The Notices is freely available at http://www.ams.org/notices (I did not verify a direct article link).
- D. H. Bailey, "Backtest overfitting in stock fund design and market prediction," slides, Berkeley CDAR, October 2017: https://cdar.berkeley.edu/sites/default/files/dhb-risk-2017.pdf
- R package `pbo`, an implementation of PBO/CSCV, on CRAN (readme: https://packages.oit.ncsu.edu/cran/web/packages/pbo/readme/README.html)
