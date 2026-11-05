# Muddled AI-Generated Code: Research Notes and an Approximate M-score Evaluator

Session notes, 5 November 2026. Sources are listed at the end; every link was retrieved during the session.

---

## 1. The problem

AI coding tools produce plausible code faster than people can understand it. The proposed working standard is that a person must be able to **detect, describe, diagnose, and fix** a defect **with no AI help at all**. That standard is a property of the team as well as of the code: the same file can pass for one reader and fail for another.

## 2. What the research says

### 2.1 Comprehension (or cognitive) debt

- Storey and colleagues describe a "Triple Debt" model. Technical debt lives in the code, cognitive debt lives in the people, and intent debt is the missing rationale that both humans and agents need.
- A mixed-methods study of teams building with coding agents found that teams knowingly deferred understanding because the code worked. When bugs appeared, some developers handed diagnosis back to the AI and relied on regression tests. That bypasses the debt instead of repaying it.
- A study of student projects found that an explicit **comprehension gate** before committing generated code was a key protective practice.

### 2.2 Effects on skill and speed

- **Anthropic's randomized controlled trial (RCT):** the largest score gap between the AI group and the no-AI group was on debugging questions. Participants who relied on AI to debug were among the lowest scorers.
- **METR's RCT:** experienced developers working in their own mature repositories were 19% slower with AI tools, although they believed they were faster. The study is small (16 developers) and used early-2025 tools.

### 2.3 Measured code quality

- LLM-generated code showed about 63% more code smells than professional reference solutions. The gap was larger on complex and object-oriented tasks.
- In a study of more than 6,000 repositories, 22.7% of the tracked AI-introduced issues still existed at the latest revision. The code often looked locally correct but was inconsistent with its surroundings.
- GitClear's longitudinal data, which comes from a vendor, shows more churn, more duplication, and much less refactoring since AI assistants spread.

### 2.4 Approaches with the best support so far

1. **Process discipline.** Write specifications and tests first, keep diffs small, require a written rationale, and use a comprehension gate. This approach has the best evidence. Its weaknesses are that specifications drift from the code, and tests written by the same model share its blind spots.
2. **Mechanical gates.** Smell detectors, metrics, and drift checks in continuous integration (CI). These scale well with volume, but they can be gamed.
3. **Change what is generated.** Generate less code, or generate formally verified code ("vericoding" in Dafny, Verus, or Lean). Verification gives correctness but says nothing about readability.

Most of the sources from 2025–2026 are preprints that have not been peer-reviewed.

## 3. Code-smell tools

**What the evidence shows**

- **Tools disagree.** Accuracy varies widely between tools and between projects. Their agreement looks high, but mostly because they agree on code that is clean.
- **Coverage is narrow.** No tool covers all 22 of Fowler's smells; a typical tool covers three or four.
- **Results depend on thresholds.** The cutoffs a tool uses strongly affect its accuracy.
- **Complexity metrics are one-sided.** Low cognitive or cyclomatic complexity reliably indicates code that is easy to understand. High values do not reliably indicate code that is hard to understand.
- **Developers often ignore structural smell reports.**

**Recommendation:** use the tools to track trends over time, and to rank which files a human should review first. A hard pass/fail gate is the easiest setup, but generated code can satisfy its numbers without becoming clearer.

**Open-source tools**

| Language | Tool | What it does |
|---|---|---|
| Java | DesigniteJava | Design and implementation smells, plus object-oriented metrics |
| Java | PMD | Common code-level flaws |
| C++ | clang-tidy | Static analysis (mainly bugs and style) |
| C++ | Cppcheck | Static analysis (mainly bugs) |
| C++ | PMD CPD | Duplicated code |
| C++ | Lizard | Complexity measures |

I did not find a well-validated open-source tool that detects *design* smells in C++.

## 4. Methods and metrics for "muddle"

### 4.1 Methods

- **Information hiding (Parnas) and design rules (Baldwin and Clark).** Each module hides one decision that is likely to change, and modules interact only through stable interfaces.
- **Ousterhout's "deep modules."** A good module hides a lot of functionality behind a simple interface. He names three symptoms of complexity:
  - change amplification: a small change requires edits in many places;
  - cognitive load: how much a developer must know to make a change;
  - unknown unknowns: it is not obvious what a developer needs to know.

  Ousterhout argues *against* reflexively splitting code into short methods, because each split adds an interface. Splitting helps only when each piece hides something.
- **Functional core with explicit data flow.** Pure functions cannot become entangled through hidden state, so the visible dependency graph is the real one.

### 4.2 Metrics, from strongest evidence to weakest

1. **Architecture level**
   - *Propagation cost (PC):* the density of the transitive closure of the file dependency graph. It depends on the number of files, so it cannot compare projects of different sizes.
   - *Decoupling Level (DL):* measures how well the system splits into small, independently replaceable modules. It becomes unstable when isolated files are present.
   - *M-score (2024):* combines the strengths of PC and DL. It is the subject of the rest of these notes.
2. **Change-history level.** Files that repeatedly change together without a declared dependency reveal hidden coupling.
3. **Code level.** Readability models exist, but the human ratings they are trained on are noisy.

## 5. M-score definition (Pisch, Cai, Kazman et al., ESEM 2024)

### 5.1 Notation

Files are grouped into $n$ layers. Layer 1 is the top layer, the one most depended upon. Layer $n$ is the bottom layer: its modules are fully independent, and no file depends on them.

Within each layer, files are grouped into modules that do not depend on each other. For a module $M_j$:

- $|M_j|$ is the number of files in the module;
- $N$ is the total number of files in the system.

### 5.2 Equations (as published)

$$
\text{M-score} = \sum_{L_i=1}^{n} \sum_{M_j} SF(M_j)\cdot CLDDF(M_j)\cdot IMCF(M_j)
$$

**Size factor**

$$
SF(M_j) = \frac{|M_j|}{N}\, SP(M_j),
\qquad
SP(M_j) = \begin{cases} 1/\log_5 |M_j| & |M_j| \ge 5 \\ 1 & |M_j| < 5 \end{cases}
$$

**Cross-layer dependency density factor**

$$
CLDDF = 1 - CLDDP,
\qquad
CLDDP(M_j) = \begin{cases}
\dfrac{\#LowerLayerDeps(M_j)}{\#LowerLayerFiles \cdot |M_j|} & L_i \ne n \\[1ex]
0 & L_i = n
\end{cases}
$$

**Inner-module complexity factor**

$$
IMCF = 1 - IMCP,
\qquad
IMCP(M_j) = \begin{cases}
\dfrac{InternalDeps(M_j)}{(|M_j|-1)\,|M_j|} & |M_j| \ge 5 \\[1ex]
0 & |M_j| < 5
\end{cases}
$$

### 5.3 Behavior

- The score rewards many small, independent modules.
- It penalizes dependencies from lower layers into a module, and dense dependencies inside a module.
- It applies the size penalty in every layer, so adding an isolated file barely changes the score.
- It correlated significantly with revision-history maintenance measures across 1,220 releases of 37 projects.

### 5.4 Ambiguities found during this session

1. **Clustering.** The paper delegates the construction of layers and modules to the Design Rule Hierarchy algorithm (Wong et al., 2009), which is computed by the DV8 tool. **This mostly does not matter for our use case, because the layers already exist** (Section 6).

2. **What $\#LowerLayerDeps$ counts.** The text says it is a count of *files* in lower layers that depend on the module. The denominator, however, is a count of possible file *pairs*. I could not reproduce the paper's cross-layer example (Fig. 3: published values of 75.00% and 87.50%) from its text description under either reading. The figure itself is needed.

3. **"All lower layers" versus "the adjacent lower layer only".** The definition explicitly says *all* layers below. However, my reconstruction of the isolated-file example (Fig. 2B) reproduces the published 71.43% only if the denominator counts the **adjacent** lower layer. Counting all lower layers gives 73.81%.
   - Either the example was computed with the adjacent reading, or my reconstruction of Fig. 2B is wrong. I cannot tell which without seeing the figure.
   - **Correction:** earlier in the session I said that this example reproduced exactly. That was true only under an assumption I had not stated.

4. **Dependency extraction.** The authors used Understand, a commercial tool, and they note that a different extractor could change the results. Their named open-source alternative is Depends.

### 5.5 Worked-example checks

The figures were reconstructed from the paper's text descriptions.

| Example | Published | Reconstruction | Result |
|---|---|---|---|
| Fig. 2A, N = 6 | 66.67% | 66.67% | match |
| Fig. 2B, N = 7 | 71.43% | 71.43% with the adjacent-layer reading; 73.81% with all lower layers | match only with the adjacent-layer reading |
| Fig. 2A / 2B, N = 10,000 | 17.47% / 17.48% | 17.471% / 17.479% | match (2B uses the adjacent-layer reading) |
| Fig. 4A / 4B, N = 10 | 36.62% / 51.27% | 36.62% / 51.27% | match |
| Fig. 4A / 4B, N = 10,000 | 17.466% / 17.469% | 17.466% / 17.469% | match |
| Fig. 3 | 75.00% / 87.50% | not reproduced | see ambiguity 2 |

These numbers were checked in Python with floating-point arithmetic. Maxima was not available in the session environment, so the exact-arithmetic script in Section 7 has **not yet been run**.

## 6. Design for our approximate evaluator (layers already known)

### 6.1 What known layers simplify

The paper's hardest unspecified step is building the hierarchy. Because our system already has distinct layers, the evaluator takes the layers as input and only has to:

1. assign each file to its given layer;
2. split each layer into modules;
3. compute the penalties.

### 6.2 Inputs

- **Files**, each with an identifier and a layer number. Use the paper's convention: layer 1 is the top, and higher numbers are lower layers. If our own numbering runs the other way, convert it on input.
- **Directed dependencies.** An edge `A -> B` means file A depends on file B.
- **XML input**, which is easy to read and write from Java:

```xml
<system>
  <file id="Survey.java"   layer="1"/>
  <file id="Question.java" layer="2"/>
  <file id="Logger.java"   layer="2"/>
  <dep from="Question.java" to="Survey.java"/>
</system>
```

### 6.3 Rules

1. **Modules.** The modules of a layer are the *weakly connected components* of the graph formed by that layer's files and the dependencies among them. In other words, files connected in either direction belong to one module.
   - This is consistent with the paper's example, in which a 5-file chain with no cycle is one module.
   - It also guarantees that modules in the same layer do not depend on each other, which the formulas assume.
2. **Edge classification.** Each dependency falls into exactly one class:
   - **internal:** both files are in the same module. These count toward $InternalDeps$.
   - **downward:** a file in a lower layer depends on a file in a higher layer. These count toward $\#LowerLayerDeps$ of the target file's module.
   - **upward (a layering violation):** the paper's model assumes these do not exist. **Report them separately; do not hide them.**
     - *Option A (default):* exclude them from the score and print a violation count with the list of edges. The score will then be somewhat optimistic.
     - *Option B:* fail the evaluation whenever any violation exists.
3. **Counting dependencies.** Count each distinct ordered file pair once, ignoring how many calls or references it represents. This matches the paper, which treats every dependency type equally and ignores dependency strength.
4. **Parameters, so that each ambiguity becomes a visible switch rather than a hidden choice:**
   - `crossCount = PAIRS | FILES` (ambiguity 2). Default to `PAIRS`, which keeps the penalty within [0, 1] as a true density.
   - `lowerScope = ALL | ADJACENT` (ambiguity 3). Report **both** values until the question is settled.
5. **Edge cases**
   - $N = 0$: the score is undefined; report an error.
   - Modules with fewer than 5 files have no size penalty and no inner-complexity penalty (as published).
   - If $\#LowerLayerFiles = 0$ in a layer other than the bottom (possible with `ADJACENT` when the next layer is empty), set the penalty to 0.

### 6.4 Outputs

- The total M-score as a percentage.
- A **per-module breakdown**: layer, file count, SF, CLDDP, IMCP, and the module's contribution. The breakdown matters more than the total, because it tells a human *where* the muddle is.
- The layering-violation list.
- Propagation cost as a companion number. It is cheap and fully specified, and it is still valid for tracking one project over time.
- *Recommended use:* run the evaluator per commit and compare commits written by AI with commits written by humans.

### 6.5 Code structure (Java 17, pure functions)

- `record FileNode(String id, int layer)`
- `record Dep(String from, String to)`
- `record Module(int layer, Set<String> files)`
- `record ModuleScore(Module m, double sf, double clddp, double imcp, double contribution)`
- `SystemModel parse(Path xml)` (the only input step)
- `List<Module> modulesOfLayer(...)` (union-find over edges inside the layer)
- `EdgeClasses classify(...)` (returns internal, downward, and upward edges)
- `ModuleScore score(Module, EdgeClasses, Params, int n, int N)`
- `double mScore(List<ModuleScore>)`
- `void writeReport(Path, ...)` (the only output step)

Each function should stay short. All side effects are confined to parsing the input and writing the report.

### 6.6 Tests

- Encode Figs. 2A, 2B, 4A, and 4B as XML fixtures, using the reconstructions in Section 5.5, and assert the published values within 0.005%.
- Keep Fig. 3 as a known-unresolved test until the figure or the authors' replication package settles it.

### 6.7 Next steps

1. Download the authors' replication package (Zenodo, doi:10.5281/zenodo.12747463). Look for their scoring script and input data, which could settle ambiguities 2 and 3.
2. Decide how dependencies will be extracted for our code. Options include Depends, compiler-based tools, or dependencies we already know from our own layer design.
3. Implement the evaluator (Section 6.5) and run the tests in 6.6.

## 7. Maxima verification script (not yet run)

**Definitional claims**, not checked: Equations 1–7 as stated in Section 5.2.

**Derivational claims**, checked below: the published worked-example values, computed from the equations and my reconstructions of the figures.

```maxima
/* M-score worked-example checks. Exact arithmetic; float only at display. */
sp(k) := if k >= 5 then log(5)/log(k) else 1;
check(name, val, pct) := block([got, d],
  got : float(100*val),
  d   : abs(got - pct),
  print(name, ": computed", got, "% vs published", pct, "% ->",
        if d < 0.006 then "PASS" else concat("FAIL, discrepancy ", d)))$

/* Fig 2A, N=6: top file t; bottom module of 5 files, 4 internal deps, all depend on t */
m2a : 5/6*sp(5)*(1 - 4/20) + 1/6*(1 - 5/(5*1))$
check("Fig2A N=6", m2a, 66.67)$

/* Fig 2B, N=7: add an isolated file as its own bottom layer */
m2b_adj : 5/7*sp(5)*(1 - 4/20) + 1/7 + 1/7*(1 - 5/(5*1))$  /* adjacent lower layer only */
m2b_all : 5/7*sp(5)*(1 - 4/20) + 1/7 + 1/7*(1 - 5/(6*1))$  /* all lower layers */
check("Fig2B adjacent reading", m2b_adj, 71.43)$
check("Fig2B all-lower reading", m2b_all, 71.43)$  /* expected FAIL: 73.81 */

/* Fig 2A/2B general N; big module of N-1 files, N-2 internal deps */
f2a(N) := (N-2)/N * sp(N-1)$
f2b(N) := (N-2)/(N+1) * sp(N-1) + 1/(N+1)$
check("Fig2A N=10000", f2a(10000), 17.47)$
check("Fig2B N=10000", f2b(10000), 17.48)$

/* Fig 4, N=10: bottom module of 9 files; 4(N-2)=32 vs 2(N-2)=16 internal deps */
check("Fig4A N=10", 9/10*sp(9)*(1 - 32/72), 36.62)$
check("Fig4B N=10", 9/10*sp(9)*(1 - 16/72), 51.27)$

/* Fig 4 general N */
f4a(N) := (N-5)/N * sp(N-1)$
f4b(N) := (N-3)/N * sp(N-1)$
check("Fig4A N=10000", f4a(10000), 17.466)$
check("Fig4B N=10000", f4b(10000), 17.469)$

/* Algebraic identities behind the general-N forms (should print 0) */
print(ratsimp((N-1)/N*(1 - (N-2)/((N-1)*(N-2))) - (N-2)/N))$
print(ratsimp((N-1)/N*(1 - 4*(N-2)/((N-1)*(N-2))) - (N-5)/N))$
print(ratsimp((N-1)/N*(1 - 2*(N-2)/((N-1)*(N-2))) - (N-3)/N))$
```

The expected result is that every check passes except `Fig2B all-lower reading`, which should report a discrepancy of about 2.38 percentage points.

## Sources

**AI-generated code and comprehension debt**

- [Storey et al., From Technical Debt to Cognitive and Intent Debt (arXiv 2603.22106)](https://arxiv.org/pdf/2603.22106)
- [How Do Practitioners Build SE Agents? (arXiv 2607.10856)](https://arxiv.org/pdf/2607.10856)
- [Comprehension Debt in GenAI-Assisted SE Projects (arXiv 2604.13277)](https://arxiv.org/html/2604.13277v1)
- [Anthropic, How AI assistance impacts the formation of coding skills](https://www.anthropic.com/research/AI-assistance-coding-skills)
- [Summary of METR's RCT, with a link to the paper](https://newsletter.getdx.com/p/metr-study-on-how-ai-affects-developer-productivity)
- [Investigating the Smells of LLM Generated Code (arXiv 2510.03029)](https://arxiv.org/abs/2510.03029)
- [Debt Behind the AI Boom (arXiv 2603.28592)](https://arxiv.org/html/2603.28592v2)
- [GitClear 2025 AI Code Quality report](https://www.gitclear.com/ai_assistant_code_quality_2025_research)
- [A benchmark for vericoding (arXiv 2509.22908)](https://arxiv.org/abs/2509.22908v1)

**Code smells and complexity metrics**

- [Paiva et al., On the evaluation of code smells and detection tools](https://jserd.springeropen.com/articles/10.1186/s40411-017-0041-1)
- [Lacerda et al., Code Smells and Refactoring: Tertiary Review (arXiv 2004.10777)](https://arxiv.org/pdf/2004.10777)
- [Esposito et al., Early Career Developers' Perceptions of Code Understandability](https://oulurepo.oulu.fi/handle/10024/57829)
- [Palomba et al., The Scent of a Smell (author preprint)](https://dibt.unimol.it/staff/fpalomba/documents/J8.pdf)
- [DesigniteJava (GitHub)](https://github.com/sgl-umons/DesigniteJava)

**Modularity metrics and design**

- [MacCormack, Rusnak, Baldwin, Exploring the Structure of Complex Software Designs](https://typeset.io/pdf/exploring-the-structure-of-complex-software-designs-an-4dxdxbabyj.pdf)
- [Mo, Cai, Kazman et al., Decoupling Level (ICSE 2016)](https://www.cs.drexel.edu/~yc349/papers/2016/icse2016-DL.pdf)
- [Pisch et al., M-score: An Empirically Derived Software Modularity Metric (ESEM 2024)](https://par.nsf.gov/servlets/purl/10590179)
- [M-score replication package (Zenodo)](https://doi.org/10.5281/zenodo.12747463)
- [Scalabrino et al., A Comprehensive Model for Code Readability](https://www.cs.wm.edu/~denys/pubs/JSEP'18-ReadabilityPaper.pdf)
- [Lebrero, Notes on A Philosophy of Software Design](https://danlebrero.com/2021/02/24/philosophy-of-software-design-summary/)
