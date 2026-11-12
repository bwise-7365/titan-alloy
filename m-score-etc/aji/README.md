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
  --measures all|mscore  report the four measures beside the M-score and the moves
                         tried (default), or the M-score alone (Section 11)
  --group folder|component|top|layer   how files are grouped for the direction and
                         partition-fit measures (default: folder for Java, component
                         for C++; Section 11)
  --quiet                do not print the report on the console
```

The main class is `aji.app.Main`. It writes five files in the output folder:

- `report.txt`: the report, also printed on the console (Sections 8 and 11);
- `modules.csv`: one row per module with every term of the formula, at full precision;
- `measures.csv`: one row per grade (the M-score and the four measures), for trend tracking;
- `moves.csv`: every move tried, in rank order, with the grades after it (Section 11);
- `model.xml`: the files, layers and dependencies that were scored, and the tests with their dependencies, in the `<system>` format. You can edit it and score it again, or compare it between runs.

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
| `<tests glob="..."/>` | The test files (optional, repeatable). They stay out of the score, but aji reads their dependencies for the test-reach measure (Section 11). The patterns are matched before the exclusions, so a project may exclude its test folder and still name it here; the three project files do. |
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

`<dep from="A" to="B"/>` means that file A depends on file B. Each ordered pair counts once. The `fixtures` folder holds the paper's examples in this format. A model may also list tests, `<test id="..."/>`, and their dependencies on files, `<dep from="test" to="file" test="true"/>`; `model.xml` carries them, so a saved model gives the same test reach when measured again.

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
- the cycles, the finest layering and the dependencies from a lower layer of a second small module, also worked out by hand;
- the four measures of Section 11 on graphs worked by hand: CCD of a chain, a star, a balanced tree and a cycle, with the cuts that lower it; Martin's Ca, Ce and I of three groups in a line with one back-edge; Q of two triangles, of one group, of two separate cliques, and of a file that belongs with the other group; the dependency profiles; test reach on a chain and on a C++ pair; the grouping rules; and the combined trials against direct recomputation.

The self-test makes 55 checks.

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
- derived layers leave no upward dependency;
- for the measures of Section 11: CCD lies between N and N squared and agrees with the propagation cost; every cut offered lowers CCD by the amount stated; the dependencies that break the Stable Dependencies Principle add up by group and by pair; every instability lies in [0, 1]; the profile counts of every group add up to its files; every move offered raises Q by the amount stated; the reached and unreached files add up; the tests in `model.xml` depend on files only; every triable move is tried once and ranked by the measures it improves; and `report.txt` holds the sections that `model.xml` gives when measured again.

For HexKrieg: every declared layer but the tests holds files, the tests are left out, there is no layering violation (no package of the engine uses a later one, the engine uses no game and no front end, and no layer of the front ends uses a later one), no game uses another game's files except the game whose rules and counters it plays (Panzergruppe Kleist plays Panzergruppe Guderian's, so `src-pgk/` may use `src-pgg/`; the map `RULES_PLAYED` in the test names such pairs), each game uses the engine directly or through that game, both games of each such pair are in the tree, each front end uses the engine, of the Swing front end the simulation uses the widgets alone, and its vehicle and its tank use none of it. A game is a source root `src-NAME` other than `src-gui` and `src-hksim`.

For expert-goggles: every declared layer holds files, the tests are left out, no `#include` points away from the foundation, the libraries of the facilities layer are independent of one another, no application includes another, `appsupport` includes no application, the headers of each application are in its folder `include/NAME`, and the include graph has no cycle. These checks do not depend on how finely the layers are declared, so `ExpertGogglesTest` also accepts `projects/expert-goggles-shared-layer.aji.xml`, which declares one layer for each of the six folders.

A test ends with exit status 0 when every check passes and 1 otherwise. A failed architecture check names up to five of the dependencies that break the rule.

On 8 November 2026 `ExpertGogglesTest` passed its 35 checks on both project files, and `HexKriegTest` its 37. Two of HexKrieg's rules were revised that day, after HexKrieg's own changes broke them: a game may now use the game whose rules and counters it plays (PGK uses PGG and reaches the engine through it), where before no game used another.

## 8. Reading the report

- **RESULT** gives the M-score at six decimals, the four readings of the open choices, and, for a project with declared layers, the score with derived layers.
- **WHERE THE SCORE IS LOST** splits 100 % minus the M-score into three parts that add up exactly: module size (SP), dependencies from lower layers (CLDDP), and dependencies inside modules (IMCP). Each module's loss is split in that order: size first, then the cross-layer penalty on what remains, then the inner penalty.
- **LAYERS** shows each layer's files, modules and largest module.
- **MODULES THAT LOSE THE MOST** ranks modules by the percentage points they cost. Under this metric, these are the places to look first.
- **WHY THE COSTLIEST MODULES LOSE POINTS** explains the modules that lose the most (three of them, or the number given with `--explain`). It is described below.
- **REACH**, **DIRECTION**, **PARTITION FIT**, **TEST REACH** and **MOVES THAT HELP MORE THAN ONE MEASURE** are the four measures beside the M-score and the combined list of their moves (Section 11). `--measures mscore` leaves them out.
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

  On 6 November expert-goggles was restructured on that finding and on two more. `apps/shared` became `appsupport/`, a folder of its own between the facilities and the applications, and the headers of each application moved to `apps/NAME/include/NAME/`. The project file (`projects/expert-goggles.aji.xml`) now declares 15 layers: in each library and in the applications the headers are a layer above the sources, and `appsupport/` is a layer of its own. The score rose from 45.719 % to 85.691 %, with the same 677 dependencies and no layering violation. Taken alone from 45.719 %, the headers of the applications give +20.9 points, the headers of the libraries +16.8, and `appsupport/` +9.0; the gains overlap, because the first and the third both divide the module of 124 files.

  Two cautions apply to that figure. The header layers change no code. They also rest on aji reading only `#include` lines, so that no source file appears to use another (Section 5). Of the 14.3 points still lost, 12.7 are in six groups of headers that include one another, the largest being the 20 public headers of logistics (3.0 points). `projects/expert-goggles-shared-layer.aji.xml` now declares the six folders as six layers; it gives the 54.673 % of 5 November, for comparison with the earlier figures.
- **HexKrieg.** `src/` is one declared layer of 63 files, and its dependencies connect all of them into one module. That module costs 49.9 points, almost all of it size. The two front ends form a second module because `HKSimFrame` uses `SwingControls` and `HKSimPanel` uses `HexPanel`. Declaring finer layers inside `src/` would give a more informative score.

  On 6 November HexKrieg was restructured on that finding. The engine in `src/` became six packages, one for each layer cut by depth of dependency, and Panzergruppe Guderian moved to a source root of its own, `src-pgg/`, with a package for what the game is and one for how it is played. With those layers declared (`projects/hexkrieg.aji.xml`), the score rose from 37.809 % to 73.976 %. The 394 dependencies between files are the same before and after. The two front ends were then still one module of 20 files, which cost 12.2 points.

  The same day the front ends were given a base layer. The map canvas and the shared controls became the package `HexKrieg.gui.widgets`, and the ground vehicle of the simulation, with its event and the entity data, became `HKSim.vehicle`. With the base declared as a layer below the rest of the front ends, the score is 81.182 %, again with the same 394 dependencies. The 20-file module was then nine modules. The largest was the rest of the simulation, 8 files, which cost 3.9 points.

  Last, the simulation was divided into three packages that are layers. The two route followers joined the ground vehicle in `HKSim.vehicle`, the tank with its events and the duel became `HKSim.tank`, and the viewer stayed in `HKSim`. With the widgets, the vehicle, the tank and the windows declared as four layers, the score is 84.615 %, with the same 394 dependencies. The largest module of HexKrieg now has five files, and no points are lost to size.
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
| 3 | `aji.extract` | `Extractor`, which turns a project specification into a model, and reads the tests |
| 3 | `aji.measure` | the four measures beside the M-score: `Reach`, `Direction`, `Fit`, `TestReach`; `Move`, one suggested change; `Grouping` |
| 4 | `aji.diagnose` | why the modules that lose the most lose points: folders, joining files, what-if scores, cycles, dependencies from lower layers |
| 4 | `aji.moves` | `Trials`: every triable move tried on the model, with the change to every measure |
| 5 | `aji.io` | XML input and output, the text report (`ReportWriter`, `MeasureWriter`) and the CSV tables |
| 6 | `aji.app` | the command line |
| 7 | `aji.testing` (in `test/`) | `Checks`, which the tests share |
| 8 | `aji.selftest`, `aji.app` (in `test/`) | the self-test; the tests on HexKrieg and expert-goggles |

All functions are pure except reading source and XML files (`Extractor`, `XmlFiles`) and writing the outputs (`Main`, `XmlFiles.writeSystem`).

## 11. Four more measures, each with its moves (8 November 2026)

The M-score asks whether the modules are small and layered. Four measures added on 8 November 2026 ask different questions of the same dependency graph. Each is reported in the form of the M-score's own sections: the grade (six decimals where it is a fraction of 1), the source, the parts (three decimals), and MOVES, a ranked list of single changes with their gains. The formulas are taken from the papers named, never from another tool's code; the clustering of Decoupling Level is still not used (Section 1). All four read the include or import graph alone, so they share the M-score's blind spot (Section 5). `PLAN.md` records the design.

**Groups.** Direction and partition fit work on groups of files. `--group folder` is the parent directory, which for Java is the package; `--group component` is the path above the first folder named `include`, `src`, `source`, `test` or `tests`, or failing those `gui`, which is the unit a C++ tree is laid out in (`apps/fleet/include/fleet/gui/w.hpp` and `apps/fleet/gui/w.cpp` are both `apps/fleet`; `facilities/gui/src/v.cpp` is `facilities/gui`); `top` is the first segment of the path; `layer` the declared layer. The default is the folder for Java and the component for C++, where the folder would make every source a stray from its own headers.

### Reach (Lakos)

*Source.* J. Lakos, *Large-Scale C++ Software Design*, Addison-Wesley 1996, chapter 4; propagation cost from MacCormack, Rusnak and Baldwin, *Management Science* 52 (2006).

A file's reach is every file it depends on, directly or through others, counting itself. CCD, the cumulative component dependency, is the sum of the reach sizes over the files; ACD is CCD / N; NCCD divides CCD by that of a balanced binary tree of N files, (N + 1) log2 (N + 1) − N, so a tree-shaped system scores 1.0 and a chain of N about N / (2 log2 N). Files in one cycle reach one another, which inflates CCD as Lakos intends. Propagation cost is CCD / N², the same quantity as a density; the RESULT section keeps it.

The parts are the ten files with the largest reach (the heaviest users) and the ten reached by the most others (the heaviest dependencies). The moves are cuts: for every dependency on the edge of a file's reach aji recomputes CCD without it and lists the ten largest falls. A cut is not an instruction to delete an include; it marks a dependency worth taking out of the users' reach by one of Lakos's three remedies, named in the section: escalate the shared part to a level above both files, demote it below both, or insulate it by including it in the source instead of the header.

### Direction (Martin)

*Source.* R. C. Martin, "OO Design Quality Metrics: An Analysis of Dependencies", 1994, and *Agile Software Development*, 2002, chapter 20. The hub rule follows the idea of Arcan's hub-like dependency (Fontana and others, 2016); the threshold is aji's own.

For a group g, Ca is the number of files outside g that depend on a file of g, Ce the number of files of g that depend on a file outside g, and the instability I = Ce / (Ca + Ce), from 0 (wholly stable) to 1 (wholly unstable). The Stable Dependencies Principle asks that a group depend only on groups at least as stable as itself. The grade is the share of dependencies between groups that obey it. Abstractness, and with it the distance from the main sequence, needs a parser and is not computed.

The parts are a table of the groups with Ca, Ce, I and the dependencies out of each that break the principle. The moves are the violating group pairs, ranked by the dependencies they carry times the difference in instability, each with up to three of its dependencies, which enter the combined list as cuts; and the hubs, files whose fan-in and fan-out are both at or above the 90th percentile of the files and both at least 3, offered as splits.

Under this definition every layer between the top and the bottom has Ce > 0, since its files use the layers above, so a well-layered system can still show a middle layer as less stable than the layer that uses it. Read the violations as questions about those pairs, not as faults by themselves.

### Partition fit (Newman and Girvan; Bouwers, van Deursen and Visser)

*Sources.* M. E. J. Newman and M. Girvan, "Finding and evaluating community structure in networks", *Physical Review E* 69 (2004); E. Bouwers, A. van Deursen and J. Visser, "Dependency profiles for software architecture evaluations", ICSM 2011. Q is past any patent; the profiles are from an academic paper with, as far as is known, no patent.

The modularity Q takes the dependency graph as undirected and unweighted: with m edges, k_i the degree of file i and A_ij 1 when i and j are joined, Q = (1 / 2m) Σ over pairs in one group of (A_ij − k_i k_j / 2m). Dependencies inside groups raise Q; large groups are discounted by what they would hold by chance. One group gives 0, two separate equal cliques 0.5. The share of edges that stay inside groups is printed beside it as the plain reading.

The dependency profiles sort the files of a group by how their dependencies cross its boundary: hidden (none cross), inbound (used from outside only), outbound (uses outside only), transit (both). The share of hidden files is the second grade of the section.

The moves are single-file moves: for each file and each group that holds a neighbour of it, aji recomputes Q with the file moved and lists the ten largest gains, with the file's neighbours there and at home; and the transit files with the most crossing traffic, offered as splits into an interface part and an internal part.

### Test reach

A folk measure; the SIG test-quality model has a version. A project file names its tests with `<tests glob="..."/>` (Section 4). They stay out of the score, and aji reads their dependencies, resolving them together with the production files so that a reference resolves as it would in the build; the production dependencies themselves are the ones found without the tests, so a test file never changes them.

A production file is reached when a test depends on it directly or on a file that reaches it through production dependencies. In C++ a test includes headers and sources are linked, so the share is taken over the headers alone (by extension: `.h .hpp .hh .hxx .inl .ipp`) and the section says so; in Java every file counts. The parts are the share per layer, the unreached files by folder, and the unreached files that the most other files depend on, where a test would reach the most. The moves are `TEST folder` for every folder none of whose countable files is reached.

### The combined list of moves

MOVES THAT HELP MORE THAN ONE MEASURE takes every cut and move suggested above, tries each one alone on the model, and recomputes all six grades: the M-score, NCCD, the stable-direction share, Q, the hidden share and test reach. A cut drops the dependency. A move gives the file the other group and, for the M-score, the layer that most files of that group have (or the layer itself under `--group layer`); a move that adds an upward dependency is marked. Test reach is recomputed for cuts only, since a move changes no dependency, and a cut can take a file out of a test's reach.

A grade counts as improved or worsened when it changes by at least 0.001 (0.1 points for a share); smaller changes are what one dependency more or less does by itself and are shown as unchanged. The list is ranked by the number of measures improved, then by the number worsened (fewer first), then by the gain in M-score points. The table shows `+`, `-` or `.` under each measure, the measures that asked for the move, and the changes themselves; the first twenty are printed and all of them go to `moves.csv`. The splits and the tests, which the model cannot be put through, follow as advice with the measure that asked for each.

### Reference figures (8 November 2026, default options)

| Project | Files | Deps | M-score | NCCD | CCD | Direction | Q | Hidden | Test reach |
|---|---|---|---|---|---|---|---|---|---|
| expert-goggles (component groups) | 335 | 677 | 85.691 % | 1.963926 | 4880 | 91.429 % | 0.556049 | 43.284 % | 94.512 % of 164 headers, 115 tests |
| HexKrieg (package groups) | 102 | 557 | 84.136 % | 2.750935 | 1614 | 99.163 % | 0.031110 | 1.961 % | 96.078 % of 102 files, 68 tests |
| aji (package groups) | 39 | 179 | 71.526 % | 2.110686 | 367 | 89.677 % | 0.039949 | 0.000 % | not declared |

What the reports show, in brief. On expert-goggles the heaviest dependencies are the foundation headers (`types.hpp` is reached by 304 of 335 files), and the one cut that lowers CCD the most is `plan.hpp -> request.hpp` in the orchestration headers (CCD 4880 to 4198), which the M-score also rewards (+0.500 points); Martin's reading finds the orchestration headers depending on the less stable engines headers; Q is 0.556 over the components, with `solver_contract.hpp` the one foundation header whose neighbours are mostly in engines; 94.5 % of the headers are in some test's reach, and the headers kept in `src/` folders are the ones that are not. HexKrieg's packages are many and small, so Q is near 0 and almost nothing is hidden; its one weak point by these measures is NCCD 2.75, from a deep engine. aji's own figures moved with the new packages: 39 files, 179 dependencies, M-score 71.526 % (75.155 % before them, Section 9), with no layering violation.
