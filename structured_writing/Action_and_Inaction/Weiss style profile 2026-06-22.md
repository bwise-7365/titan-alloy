# Style Profile — "Doktor Weiss" voice
*Measured from two semi-formal papers, 2026-06-22. For confirmation/correction before v3.*

## A. Corpus and what was stripped

| | Paper 1 — Pricing (policymakers, KAPSARC) | Paper 2 — Coalitions (management, MITRE) |
|---|---|---|
| Prose words after stripping | 8,074 | 6,100 |
| Math removed | 51 environments + 174 inline | 11 environments + 83 inline |
| Floats/figures removed | 5 | 4 |
| `\includegraphics` removed | 2 | 2 |
| `\cite` calls removed | 0 | 61 |
| Kept | abstract, footnote prose, list-item prose | footnote prose, list-item prose |
| Opening/closing available? | yes (abstract + conclusion) | no — body sub-file, a "middle" |

Combined prose: **14,174 words**. Trivial artifacts: the word "abstract" leaked twice; one footnote fused to the word "analyst" (~3 tokens total). Your own spellings/typos ("through out", "dillema", "equilbrium") were preserved as data, not corrected. Because Paper 2 has no opening or closing, the read on how you *start* and *end* leans on Paper 1.

## B. Measured profile

| Metric | Paper 1 | Paper 2 | Combined | My v2 draft |
|---|---|---|---|---|
| Mean sentence length (words) | 23.3 | 21.3 | 22.4 | 23.5 |
| Median sentence length | 22 | 20 | 21 | — |
| SD of sentence length | 11.4 | 10.3 | 11.0 | — |
| Sentence length p10 / p90 | 11 / 37 | 9 / 35 | 10 / 36 | — |
| % short (<12 words) | 13.0 | 21.3 | 16.7 | 24.5 |
| % long (>30 words) | 21.4 | 18.5 | 20.1 | 26.4 |
| Mean word length (chars) | 5.06 | 5.15 | 5.10 | — |
| % long words (≥7 chars) | 27.9 | 30.5 | 29.0 | — |
| Mean syllables/word | 1.75 | 1.76 | 1.75 | — |
| Flesch Reading Ease | 35.3 | 36.1 | 35.7 | — |
| Flesch–Kincaid grade | 14.1 | 13.5 | 13.8 | — |
| Sentences per paragraph | 3.17 | 3.99 | 3.50 | — |
| Subordinators per sentence | 1.17 | 0.82 | 1.01 | — |
| **Commas / 1,000 words** | 61.1 | 52.6 | 57.4 | 60.2 |
| **Semicolons / 1,000** | 0.74 | 1.64 | 1.13 | 6.42 |
| **Colons / 1,000** | 6.19 | 3.28 | 4.94 | 7.62 |
| **Em-dashes / 1,000** | 0.0 | 0.0 | 0.0 | 11.24 |
| **Parentheses / 1,000** | 5.82 | 6.39 | 6.07 | 0.0 |
| Quotes / 1,000 | 3.96 | 6.72 | 5.15 | — |
| Passive (approx) / 1,000 | 9.2 | 12.3 | 10.5 | — |

Register difference between the two papers: the policymaker paper (P1) runs longer and more subordinated with more colons; the management paper (P2) is punchier (more short sentences) with a touch more passive voice. Both use zero em-dashes and comparable parenthetical rates.

## C. Style sheet — what the numbers miss, and the v3 rules

**Punctuation (highest impact, falsifiable):**
1. **No em-dashes.** Zero in 14k words. Recast every aside as a comma clause or a parenthesis. This is the single biggest change.
2. **Semicolons rare** — target ~2–3 in the whole essay, not ~16.
3. **Parentheses for asides**, including inline "(i.e. …)" and "(e.g. …)" — a signature you use and v2 lacks entirely.
4. **Commas generous** (already matched) — long sentences are carried on commas, not dashes.

**Sentence architecture:**
5. Fewer one-line punches; let some short dramatic sentences fold into comma-linked complex ones. Keep the long tail (~20% over 30 words) but trim the very-short fraction from ~25% toward ~17%.
6. Keep mean length ~22–23 and high variance; that part already fits.

**Diction and rhetoric (the part statistics can't set):**
7. **Reductive method.** State the received view, expose its buried assumption, then show it fails "even in very simple cases," then generalize. (Your abstract is the template: common expectation → hidden premise → "does not always hold even in very simple cases.")
8. **"simple" / "very simple" / "the simplest case"** as a recurring move toward clarity (47 uses across the corpus).
9. **Careful negation / litotes:** "by no means inevitable," "does not always hold," "not inevitable for any."
10. **Causal chaining:** "hence … hence … so … then" to spell the logic out step by step.
11. **Analytical "we":** "we will consider," "we show," "we suggest." Use it freely; v2 used it once.
12. **Paragraphs ~3–4 sentences**, slightly tighter than v2.

**What to preserve from v2 (do not over-correct toward pastiche):**
- The piece must still read as a Phalanx feature, not a forgery; apply these as a nudge, not a mold.
- Keep the bottom-up structure (Action / Will / Inaction / synthesis) and the argument intact.
- A *few* rhetorical closings are fine; just not 28 em-dashes' worth.

## D. Open decisions before v3

1. **Target register.** Match Paper 1 (policymaker: longer, more subordinated), Paper 2 (management: punchier), or the blend in the "Combined" column? Recommendation: lean to **Paper 1**, since the essay argues to decision-makers, which is its closest analog — but this is your call.
2. **Carried defaults** still open from earlier: the Athenian "hired out its forces for pay" wording, and the surveillance-*dependency* framing of the China link.
3. Confirm or edit this style sheet; I will not write v3 until you have.