# aji: an approximate M-score evaluator for Java and C++

*aji* (味, "taste") is the Go term for the latent possibilities, good or bad, in a shape of stones. This program estimates the modularity of a code base with the **M-score** of Pisch, Cai, Kazman, Lefever and Fang (ESEM 2024), and reports where the score is lost. The background, the definition and its ambiguities are in `../mscore-session-notes.md`.

aji is written in Java 17 and uses only the JDK. It reads Java 17 or C++ source, finds the dependencies between files, applies the M-score formula, and writes a report.

## 1. Why the result is an approximation

The published M-score has three ingredients: a file dependency graph, a division of the files into **layers** and **modules**, and the formula. aji reproduces the formula exactly; the self-test confirms every worked example in the paper that the text allows to be rebuilt. The other two ingredients differ from the authors' setup.

1. **Dependencies.** The authors extracted dependencies with Understand, a commercial tool. aji finds type references and imports in Java, and `#include` lines in C++ (Section 5).
2. **Layers and modules.** The authors built layers with the Design Rule Hierarchy (DRH) clustering in their DV8 tool. That clustering method is the subject of a Drexel University patent application (US 2013/0219362), so aji does **not** implement it. aji takes the layers instead from one of two sources:
   - **Declared layers** (the main use): you state which folders form which layer, following your own architecture.
   - **Derived layers** (a fallback): a textbook longest-path layering of the dependency graph (Section 6).

   In both cases the modules of a layer are its *weakly connected components*: files of one layer that are linked, in either direction, by dependencies within that layer.

Scores from aji are therefore comparable with one another, for one project over time or between projects measured the same way. They are not directly comparable with the paper's published scores.

## 2. Building

**IntelliJ.** Open the `aji` folder as a project. The project expects a JDK named `corretto-17`. If yours has another name, choose it under *File > Project Structure > Project > SDK*. Six run configurations are included:

| Run configuration | What it does |
|---|---|
| `aji selftest` | checks aji against the paper's examples and small hand-made inputs |
| `aji on aji` | scores aji's own source |
| `aji on hexkrieg` | scores `..\..\..\..\HexKrieg` |
| `aji on expert-goggles` | scores `..\..\..\..\expert-goggles` |
| `aji test hexkrieg` | scores HexKrieg, then checks the outputs and the rules of HexKrieg's architecture (Section 7) |
| `aji test expert-goggles` | the same for expert-goggles |

Each runs with the `aji` folder as the working directory. Reports go to `aji-out\<name>\`.

**Command line (PowerShell, from the `aji` folder):**

```powershell
$src = Get-ChildItem .\src, .\test -Recurse -Filter *.java | % FullName
javac -encoding UTF-8 -d out\cli $src
java -cp out\cli aji.selftest.SelfTest fixtures
java -cp out\cli aji.app.Main projects\expert-goggles.aji.xml
java -cp out\cli aji.app.HexKriegTest
java -cp out\cli aji.app.ExpertGogglesTest
```

## 3. Running

```text
aji INPUT.xml [options]
  INPUT.xml              an <aji-project> file (aji extracts the dependencies)
                         or a <system> dependency model (files, layers, dependencies)
  --cross PAIRS|FILES    cross-layer count: dependency pairs (default) or files
  --lower ALL|ADJACENT   lower layers that count: all (default) or the next one only
  --upward EXCLUDE|FAIL  upward dependencies: list them and leave them out of the
                         score (default), or stop with exit status 2
  --layers declared|derived   for a project file with <layer> rules: use them
                         (default) or derive layers from the dependencies
  --out DIR              output folder (default: aji-out/NAME)
  --top K                modules listed in the report (default 25)
  --explain K            modules for which the report explains the loss
                         (default 3; 0 for none; Section 8)
  --quiet                do not print the report on the console
```

The main class is `aji.app.Main`. It writes three files in the output folder:

- `report.txt`: the report, also printed on the console (Section 8);
- `modules.csv`: one row per module with every term of the formula, at full precision;
- `model.xml`: the files, layers and dependencies that were scored, in the `<system>` format. You can edit it and score it again, or compare it between runs.

Exit status: 0 on success, 1 for a usage or input error, 2 when `--upward FAIL` finds a layering violation.

## 4. Input formats

### Project file

```xml
<aji-project name="expert-goggles" root="../../../../expert-goggles" language="cpp">
  <exclude glob="**/tests/**"/>
  <layer number="1" name="foundation">
    <path glob="foundation/**"/>
  </layer>
  <layer number="2" name="engines">
    <path glob="engines/**"/>
  </layer>
  ...
</aji-project>
```

| Element or attribute | Meaning |
|---|---|
| `root` | The project root, absolute or relative to the project file's folder. Every other path is relative to the root. |
| `language` | `java` or `cpp`. C++ covers `.h .hh .hpp .hxx .ipp .tpp .inl .ixx .c .cc .cpp .cxx .cppm`. |
| `name` | Used in the report and as the output folder's name. |
| `<source dir="..."/>` | Folders to search (optional, repeatable). By default aji searches the whole root. |
| `<include dir="..."/>` | C++ include folders to try first (optional, repeatable; Section 5). |
| `<exclude glob="..."/>` | Files to leave out (optional, repeatable). |
| `default-excludes="false"` | Turns off the built-in exclusions: `.git`, `.idea`, `.vs`, `.vscode`, `build`, `cmake-build-*`, `out`, `target` and `node_modules` folders. |
| `<layer number="N" name="...">` | A declared layer, with one or more `<path glob="..."/>`. Layer 1 is the **top**, the layer most depended upon; larger numbers are lower layers. Several `<layer>` elements may share a number. A file belongs to the first layer whose pattern it matches. |

When a project declares layers, files that match no layer are left out and listed at the end of the report. When it declares none, aji derives layers for all files.

**Patterns** match a file's path relative to the root, with forward slashes, ignoring case. `*` matches within one folder name, `?` matches one character, `**` matches any number of folders, a pattern ending in `/` matches everything below that folder, and a pattern without a slash matches a file name in any folder.

### Dependency model

This is the format of the session notes (Section 6.2), with optional layer names. The `layer` attribute may be left out; aji then derives layers for the whole model.

```xml
<system name="survey">
  <layer number="1" name="core"/>
  <file id="Survey.java"   layer="1"/>
  <file id="Question.java" layer="2"/>
  <file id="Logger.java"   layer="2"/>
  <dep from="Question.java" to="Survey.java"/>
</system>
```

`<dep from="A" to="B"/>` means that file A depends on file B. Each ordered pair counts once. The `fixtures` folder holds the paper's examples in this format.

## 5. How dependencies are found

**Java.** File A depends on file B when A imports a type declared in B, or when A's code names such a type. A simple name is resolved as the Java language resolves it: single-type imports first, then A's own package, then on-demand (`.*`) imports. Qualified names (`HexRules.Abc`, `java.util.List`) are matched against the project's types. Nested types count as part of the file that declares them. Comments, string literals and text blocks are ignored, so a type mentioned only in Javadoc is not a dependency. The package comes from the `package` statement, not the folder, so flat source roots such as HexKrieg's `src/` work.

*Limits.* aji does not compile the code. A local variable or method that has the same name as a type in scope is mistaken for that type (Java naming conventions make this rare). Member types reached only through inheritance are not seen, but the inheriting file already depends on the parent class. References to code outside the project (libraries, sibling repositories) are ignored.

**C++.** File A depends on file B when A has `#include` for B. An include is resolved in this order: relative to A's folder (for the quoted form), then relative to each declared `<include dir>`, then by searching for project files whose path ends with the included path. If that search finds several files, the one nearest to A in the folder tree is used, and the report notes any ties. Includes that match no project file (the standard library, Eigen, Qt, GoogleTest) are ignored; the report gives their count.

*Limits.* This is the physical include graph. A file that uses a declaration reached only through another header is linked to that header, not to the declaring one, so aji finds fewer dependencies than a symbol-level tool such as Understand. Includes inside `#if 0` blocks are counted; includes inside comments are not.

## 6. Layers, modules and the formula

**Declared layers.** Each file takes the first layer whose pattern it matches. Empty layers are dropped and the rest are ranked 1 (top) to n (bottom), so `--lower ADJACENT` always means the next non-empty layer.

**Derived layers.** aji collapses each dependency cycle to one node and computes each node's height: 0 for files that nothing depends on, otherwise one more than the largest height among the files that depend on it. The layer is `H - height + 1`, where H is the largest height. Layer 1 is then the most depended-upon code, and the bottom layer holds every file that nothing depends on, isolated files included. This is a standard graph-drawing layering, not the DRH. Because every dependency then crosses layers, the only modules larger than one file are dependency cycles. The derived score is therefore **lenient**: it measures cycles and the density of dependencies between layers, but not module size. For example, expert-goggles' include graph has no cycles, so it scores 99.1 % with derived layers. The report shows the derived score beside the declared one for comparison.

**Dependency classes.** Each dependency is *internal* (both files in one module), *downward* (a lower-layer file depends on a higher-layer file: the normal case), or *upward* (a layering violation, which the M-score model assumes does not occur). Upward dependencies are listed in the report and left out of the score, or they stop the run under `--upward FAIL`. With declared layers, two modules of one layer can never be linked, because linked files form one module.

**Formula.** For each module M in layer L, with N files in the system:

```text
SP    = 1 / log5 |M|        if |M| >= 5,  else 1
SF    = |M| / N * SP
CLDDP = lower-layer dependencies / (lower-layer files * |M|),  and 0 in the bottom layer
IMCP  = internal dependencies / (|M| (|M| - 1))   if |M| >= 5,  else 0
M-score = sum over all modules of  SF * (1 - CLDDP) * (1 - IMCP)
```

The published definition leaves two choices open. aji makes each one a switch and reports the score under all four combinations.

- **`--cross`**: the paper's text calls the CLDDP numerator a count of lower-layer *files* that depend on the module, but its denominator counts file *pairs*. `PAIRS` (the default) counts dependency pairs, which makes CLDDP a density between 0 and 1. The paper's Fig. 3 supports `PAIRS`: its published 75.00 % needs a penalty of 0.5, and with two modules of four files the `FILES` reading can never exceed 0.25. That figure is still not fully reproduced, because the 6 and 2 crossing dependencies named in the text give neither published value.
- **`--lower`**: the definition says *all* lower layers count, but the paper's Fig. 2B (71.43 %) is reproduced only if the adjacent lower layer alone counts (all layers give 73.81 %). The default is `ALL`, as defined.

**Propagation cost** is also reported. It is the share of ordered file pairs (a, b), counting a = b, such that b can be reached from a by following dependencies (MacCormack, Rusnak and Baldwin). It depends on the number of files, so it is useful for following one project over time but not for comparing projects of different sizes.

## 7. Self-test

`aji.selftest.SelfTest` needs no test library. It checks:

- the paper's Figs. 2A, 2B, 4A and 4B, from the XML fixtures and from code that rebuilds them at N = 10,000, against the published values (tolerance 0.005 percentage points, since the published values are rounded);
- that Fig. 2B with all lower layers gives the expected 73.81 % (the known discrepancy);
- Fig. 3, printed for information only;
- the lexer, Java and C++ dependency resolution, derived layering, propagation cost and upward-dependency handling, on small hand-made inputs;
- the search for joining files against a small graph with known answers and against taking each node out in turn, and the explanation of a five-file module whose answers can be worked out by hand (Section 8);
- the cycles, the finest layering and the dependencies from a lower layer of a second small module, also worked out by hand.

The self-test makes 29 checks.

### Tests on HexKrieg and expert-goggles

`aji.app.HexKriegTest` and `aji.app.ExpertGogglesTest` run the command line on `projects/hexkrieg.aji.xml` and `projects/expert-goggles.aji.xml` with the default options, and then read the three files it wrote to `aji-out\<name>\`. They are in the package `aji.app` because they call `Main` directly. Both projects change from day to day, so the tests fix no file count and no score. They print the figures, the points lost to each of the three penalties, and the explanation of the modules that lose the most (Section 8). Then they check the following.

For every project:

- `model.xml` scored again gives the M-score of `report.txt`, and the contributions in `modules.csv` add up to it;
- `report.txt` holds the same explanation that `model.xml` gives when scored again;
- in that explanation, the pieces left without a folder or a joining file account for every other file of the module, and every change offered raises the M-score and divides its module;
- the cycles of a module share no file, and the finest layering of a module gives one module for each cycle and one for every other file;
- the dependencies from lower layers, counted by file, add up to the number that the score uses;
- every dependency falls into exactly one class, and every file into exactly one module;
- the three losses add up to 100 % minus the M-score;
- the M-score lies between 0 and 1 under all four readings, and `--cross FILES` never scores below `--cross PAIRS`;
- the propagation cost lies between 1/N and 1;
- derived layers leave no upward dependency.

For HexKrieg: both declared layers hold files, the tests are left out, the engine in `src/` uses neither front end, and each front end uses the engine.

For expert-goggles: every declared layer holds files, the tests are left out, no `#include` points away from the foundation, the libraries of the facilities layer are independent of one another, no application includes another (and `apps/shared` includes none), and the include graph has no cycle. These checks do not depend on whether `apps/shared` is a layer of its own, so `ExpertGogglesTest` also accepts `projects/expert-goggles-shared-layer.aji.xml` as its argument.

A test ends with exit status 0 when every check passes and 1 otherwise. A failed architecture check names up to five of the dependencies that break the rule.

## 8. Reading the report

- **RESULT** gives the M-score at six decimals, the four readings of the open choices, and, for a project with declared layers, the score with derived layers.
- **WHERE THE SCORE IS LOST** splits 100 % minus the M-score into three parts that add up exactly: module size (SP), dependencies from lower layers (CLDDP), and dependencies inside modules (IMCP). Each module's loss is split in that order: size first, then the cross-layer penalty on what remains, then the inner penalty.
- **LAYERS** shows each layer's files, modules and largest module.
- **MODULES THAT LOSE THE MOST** ranks modules by the percentage points they cost. Under this metric, these are the places to look first.
- **WHY THE COSTLIEST MODULES LOSE POINTS** explains the modules that lose the most (three of them, or the number given with `--explain`). It is described below.
- **LAYERING VIOLATIONS** lists upward dependencies.

### Why a module loses points

For each module explained, the report first splits the module's loss among the three penalties. It then gives what bears on each: what joins the module, the cycles inside it, and the dependencies on it from lower layers.

**What joins it.** A large module loses most of its share of the score to the size penalty. The files of a module are one module only because dependencies link them, so the report shows which folders and files do the linking. A *piece* is a group of the module's files that are still linked to one another when a folder or a file is taken out. This part is given for modules of five or more files, since a smaller module pays nothing for its size. It has four items.

- **Folders.** The folders are those directly below the directory that all the module's files share. For each folder the table gives its files, the dependencies of its files on the module's other folders ("uses"), the dependencies of the other folders on it ("used by"), and the pieces that the rest of the module falls into without it. A folder that leaves several pieces is one that joins them. The files directly in the shared directory are counted as one folder, shown as `(its own files)`.
- **Dependencies between the folders.** Each line counts the dependencies from the files of one folder on the files of another.
- **Joining files.** A joining file is one without which the rest of the module falls into two or more pieces (in graph terms, an articulation point of the module's dependencies taken without direction). The files that cut off the most other files come first.
- **What if.** aji scores the system again with one folder, or one joining file, as a layer of its own directly above or below the module's layer, and reports the gain. The new layer goes above when the rest of the module uses the part, and below when the part uses the rest. A part that is linked to the rest in both directions cannot be a layer of its own without a layering violation, so no change is offered for it. Changes that lower the score are left out as well. aji tries every folder (the 24 largest when there are more) and the five joining files that cut off the most.

A what-if reports only what the metric would give. For a C++ library the largest gain often comes from declaring the headers a layer above the sources, which raises the score without changing any code. Whether a new layer describes the architecture is for a person to decide.

When a module has no folders, few joining files and no what-if, no one folder or file holds it together. Its cycles then show whether finer layers inside it could divide it.

**Cycles.** A cycle is a group of two or more files of the module, each of which depends on every other, directly or through the others (a strongly connected component of the dependencies inside the module). The files of a cycle cannot be put in different layers: whatever the order of the layers, one dependency of the cycle would point upward. The report lists each cycle's files and the number of dependencies among them. When some of the files depend on each other directly, it names those pairs. Otherwise it gives one of the shortest cycles as an example. A cycle that passes through several layers is not listed here; it contains an upward dependency, which appears under LAYERING VIOLATIONS.

For a module of five or more files, one more line gives the score with the module's files in layers derived from the dependencies among them (Section 6) and everything else unchanged. Each cycle is then one module and every other file is a module of its own, so layers inside the module cannot divide it further. The figure shows how much of the module's loss finer layers could remove. Like the derived score of the whole system, it is lenient.

**From lower layers.** The report counts the dependencies on the module from the files of lower layers (all of them, or the next layer only under `--lower ADJACENT`), and gives the cross-layer penalty they produce. It also gives what each one costs: the module's contribution falls by the same amount for every dependency added, or for every lower file added under `--cross FILES`. Two lists follow: the module's files that the most lower files depend on, and the lower files that depend on the most of the module's files. For a module of one file only the second list is given.

Following the session notes, the M-score is more useful for following trends and for choosing what a person should review first than as a pass/fail threshold.

## 9. Results on the test cases (5 November 2026)

| Project | Files | Deps | Layers | M-score | Derived | PC |
|---|---|---|---|---|---|---|
| aji | 26 | 84 | 6 declared (packages) | 56.067 % | 63.555 % | 22.189 % |
| HexKrieg (tests left out) | 83 | 393 | 2 declared (source roots) | 37.816 % | 87.074 % | 16.432 % |
| expert-goggles (tests left out) | 335 | 677 | 5 declared (README) | 45.719 % | 99.140 % | 4.348 % |
| expert-goggles, `apps/shared` as its own layer | 335 | 677 | 6 declared | 54.673 % | 99.140 % | 4.348 % |

What the reports show:

- **expert-goggles.** No layering violations: every include points down, as the architecture requires. Most of the loss (51.9 points) is module size. The largest single loss is the apps layer, which forms one module of 124 files because eight applications include `apps/shared`. Declaring `apps/shared` as its own layer (`projects/expert-goggles-shared-layer.aji.xml`) leaves each application a separate module and raises the score to 54.7 %.
- **HexKrieg.** `src/` is one declared layer of 63 files, and its dependencies connect all of them into one module. That module costs 49.9 points, almost all of it size. The two front ends form a second module because `HKSimFrame` uses `SwingControls` and `HKSimPanel` uses `HexPanel`. Declaring finer layers inside `src/` would give a more informative score.
- **aji.** Its own first run showed that `extract` depended on `graph` (the extractor derived layers itself), which joined two packages that should be independent. Moving that step to the command line raised the score from 52.9 % to 56.1 %.

  On 6 November, with the explanation section and the tests on real projects added, aji had 31 files and scored 59.139 %. Three changes that the explanation pointed to raised the score to 75.289 %. The 117 dependencies between files are the same before and after; only the packages and their layers changed.

  1. The `extract` package held two groups of four files with no dependency between them, joined only by `Extractor`. It was split into `aji.spec` and `aji.deps`, and `Extractor` went one layer down, beside `score` (+10.0 points).
  2. `Params` and `Module`, which depend on nothing, moved from `score` to `aji.model` (+4.5). Of that gain, 3.3 points are a threshold effect: `score` now has four files, and a module of fewer than five files pays no inner penalty.
  3. `Checks` moved to a package of its own, in a layer above the tests that share it (+1.6).

  The 24.7 points still lost all come from dependencies from lower layers. In a program of 31 files, a few dependencies fill a large share of the possible pairs.

## 10. Code layout

The packages are aji's own layers, declared in `projects/aji.aji.xml`:

| Layer | Package | Contents |
|---|---|---|
| 1 | `aji.model` | `FileNode`, `Dep`, `SystemModel`: the input to the formula; `Params`: the switches for its open choices; `Module`: files of one layer that dependencies link |
| 1 | `aji.lex` | a lexer for Java and C++ that drops comments and literals |
| 2 | `aji.graph` | connected components, Tarjan's strongly connected components, the pieces left when nodes are taken out, a shortest cycle, derived layering, propagation cost |
| 2 | `aji.spec` | the project specification: declared layers, path patterns, languages |
| 2 | `aji.deps` | the Java and C++ dependency finders |
| 3 | `aji.score` | the M-score: dependency classes, the per-module terms, the result |
| 3 | `aji.extract` | `Extractor`, which turns a project specification into a model |
| 4 | `aji.diagnose` | why the modules that lose the most lose points: folders, joining files, what-if scores, cycles, dependencies from lower layers |
| 5 | `aji.io` | XML input and output, the text report and the CSV table |
| 6 | `aji.app` | the command line |
| 7 | `aji.testing` (in `test/`) | `Checks`, which the tests share |
| 8 | `aji.selftest`, `aji.app` (in `test/`) | the self-test; the tests on HexKrieg and expert-goggles |

All functions are pure except reading source and XML files (`Extractor`, `XmlFiles`) and writing the outputs (`Main`, `XmlFiles.writeSystem`).
