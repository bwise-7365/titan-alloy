# aji: notes for Claude sessions

aji estimates the modularity of a Java 17 or C++ code base with the M-score and reports where the score is lost. This note tells a session how to run aji from any directory and how to use what it writes. `README.md` in this folder is the full reference, and the section numbers cited below are its sections.

aji lives in `C:\repos\ghub-per\titan-alloy\m-score-etc\aji`. It is written in Java 17 and needs only a JDK. Every command in this note was run on 6 November 2026 from outside this folder.

## 1. Build

Compile the sources into a scratch folder of the session. The `out/` folder here belongs to IntelliJ, so leave it alone. Ben approved compiling and running aji in a scratch folder on 6 November 2026. Whether a session may build or test the repository being analysed is decided by that repository's own instructions.

Git Bash. Write the paths with a drive letter and forward slashes, so that the argument file holds paths `javac` can read:

```bash
AJI=C:/repos/ghub-per/titan-alloy/m-score-etc/aji
WORK=C:/path/to/the/session/scratch/folder
find "$AJI/src" "$AJI/test" -name '*.java' > "$WORK/aji-sources.txt"
javac -encoding UTF-8 -d "$WORK/aji-classes" @"$WORK/aji-sources.txt"
java -cp "$WORK/aji-classes" aji.selftest.SelfTest "$AJI/fixtures"
```

PowerShell:

```powershell
$aji = 'C:\repos\ghub-per\titan-alloy\m-score-etc\aji'
$work = 'C:\path\to\the\session\scratch\folder'
$src = Get-ChildItem "$aji\src", "$aji\test" -Recurse -Filter *.java | % FullName
javac -encoding UTF-8 -d "$work\aji-classes" $src
java -cp "$work\aji-classes" aji.selftest.SelfTest "$aji\fixtures"
```

The self-test ends with `29 passed, 0 failed`. The build takes a few seconds, so rebuild at the start of a session and whenever aji's source has changed.

## 2. Run

```bash
java -cp "$WORK/aji-classes" aji.app.Main PROJECT.aji.xml --out "$WORK/aji-out/NAME" --quiet
```

aji reads the source tree and changes nothing in it. It writes `report.txt`, `modules.csv` and `model.xml` to the output folder, and its last line on the console names that folder. Without `--out` the folder is `aji-out/NAME` under the current directory, which is the repository the session is working in. Pass `--out` to keep that repository clean.

With `--quiet` the report is written and not printed. Read `report.txt` afterwards; for the four projects here it is 100 to 180 lines.

| Option | Use |
|---|---|
| `--explain K` | The number of modules whose loss the report explains (default 3). Raise it when the first three are settled. |
| `--top K` | The number of modules in the ranked list (default 25). |
| `--upward FAIL` | Stop with exit status 2 when a dependency breaks the declared order of the layers. |
| `--layers derived` | Ignore the declared layers and derive them from the dependencies. |
| `--cross`, `--lower` | The two open choices of the published definition (Section 6). The report gives the score under all four combinations in any case. |

Exit status is 0 on success and 1 for a usage or input error.

## 3. The project file

A project file names the root of a source tree, the language, and the layers. The four in `projects/` are `aji`, `hexkrieg`, `expert-goggles` and `expert-goggles-shared-layer`. A project file may be kept anywhere: `root` is absolute, or relative to the folder of the project file.

```xml
<aji-project name="myproject" root="C:/repos/ghub-per/MyProject" language="java">
  <exclude glob="test/**"/>
  <layer number="1" name="foundation">
    <path glob="src/core/**"/>
  </layer>
  <layer number="2" name="application">
    <path glob="src/**"/>
  </layer>
</aji-project>
```

- Layer 1 is the top: the code that the most other code depends on. A larger number is a lower layer, and a lower layer depends on the layers above it. In the report, "from lower layers" therefore means "from the code that uses this module".
- A file belongs to the first layer whose pattern matches it, so a specific pattern must come before a general one. A pattern may name one file.
- With declared layers, a file that matches no layer is left out of the score. The report counts such files under INPUT and lists them under FILES LEFT OUT at its end. Check that list after every change to the patterns.
- `language` is `java` or `cpp`. For Java the package comes from the `package` statement, so source roots need no declaration. For C++, `<include dir="..."/>` names include folders.
- References to code outside the root are ignored. For HexKrieg that is the sibling repository HexRules.
- Folders named `.git`, `.idea`, `.vs`, `.vscode`, `build`, `cmake-build-*`, `out`, `target` and `node_modules` are excluded unless `default-excludes="false"` is given.

Take the layers from the architecture the repository documents (its `CLAUDE.md`, its README, its build files). Where it documents none, run once with no `<layer>` element. aji then derives the layers, and `model.xml` gives the derived layer of every file, which is a starting point for an order. Section 4 of the README has the whole format.

## 4. Reading the report

The sections come in this order.

- INPUT gives the number of files and of dependencies. A restructuring that only moves files between packages or folders leaves the number of dependencies unchanged. HexKrieg had 394 before and after each of its three steps. A change in the number means that the code changed, or that a name now resolves differently.
- RESULT gives the M-score, the score under the four readings, and the score with derived layers. The derived score is lenient, since only dependency cycles form modules under it.
- WHERE THE SCORE IS LOST divides the loss among module size (SP), dependencies from lower layers (CLDDP) and dependencies inside modules (IMCP). The largest of the three decides what kind of change can help.
- LAYERS gives the files, the modules and the largest module of each layer. A layer whose files all form one large module is where a size loss comes from.
- MODULES THAT LOSE THE MOST ranks the modules by the points they cost. Work from the top of this list.
- WHY THE COSTLIEST MODULES LOSE POINTS explains the first modules of that list (Section 8).
- LAYERING VIOLATIONS lists the dependencies that point from a higher layer to a lower one.

A module is computed, never declared: it is a set of files of one layer that dependencies inside the layer link together. Its label is the folder its files share and its size, so several modules can carry the same folder. The column `first_file` of `modules.csv` tells them apart.

The explanation of a module has these parts.

| Part | What it tells | What to do with it |
|---|---|---|
| Folders, joining files | Which folders or single files link the rest of the module together. | A folder or file that leaves several pieces is a candidate for a layer of its own. |
| What if | The score with one folder or one joining file as a layer of its own, with the gain in points. | These are trial results that cost nothing. Each is one change taken alone. |
| Cycles | Groups of files that depend on each other, directly or through others. | The files of a cycle cannot be put in different layers. Dividing them needs a change to the code that removes one direction of a mutual dependency. |
| Layered as finely as its cycles allow | The score if the module were divided as far as layers can divide it. | This is the most that new layers inside the module can gain. When it is small, leave the module alone. |
| From lower layers | The number of dependencies on the module from below, what each one costs, the files most used from below, and the lower files that use the most of it. | The loss falls when the code below uses fewer of the module's files. |

`model.xml` holds the files with their layers and every dependency as `<dep from="A" to="B"/>`, meaning that A depends on B. Search it to see what uses a file. Compare the sorted `<dep` lines of two runs to see which dependencies a change added or removed. It is also a valid input to aji.

`modules.csv` has one row for each module with every term of the formula. Its values are fractions of 1, where the report gives percentage points.

## 5. From a report to a change

This method produced the three steps on HexKrieg (37.8 % to 84.6 %) and the three on aji itself (59.1 % to 75.3 %).

1. Run aji with the layers of the documented architecture. Confirm that no file is left out by mistake and that the violations are the ones expected.
2. Take the modules that lose the most. Use the explanation to find what holds each one together.
3. Try a candidate layering before moving any file. Copy the project file to the scratch folder, give it an absolute `root`, and assign single files to new layers with patterns placed before the general ones. This was the trial for HexKrieg's front ends, when `HexPanel` and `SwingControls` were still directly in `src-gui/`:

   ```xml
   <layer number="10" name="trial: base of the front ends">
     <path glob="src-gui/HexPanel.java"/>
     <path glob="src-gui/SwingControls.java"/>
   </layer>
   <layer number="11" name="front ends">
     <path glob="src-gui/**"/>
     <path glob="src-hksim/**"/>
   </layer>
   ```

   The score of the trial is the score the real change will give, provided the change moves files and leaves the dependencies as they are. On HexKrieg the trials gave 81.182 % and 84.615 %, and the runs after the real changes gave the same figures. Quote the trial score when recommending a change.
4. Make the change in the code so that packages or folders follow the new layers, and shorten the patterns to match. Run aji again and compare three figures with the trial: the score, the number of dependencies, and the number of violations.
5. Record the rules of the architecture as checks (Section 7 below).

When asked for the best improvements, rank the candidates by the points their trials gain, and say for each one whether it only declares or moves layers or also changes code.

## 6. Points that are easy to misread

- Upward dependencies are left out of the score. A wrong order of layers therefore does not lower the score by itself. Read the score as a statement about the declared architecture only when LAYERING VIOLATIONS is 0.
- A module of fewer than five files pays no size penalty and no penalty for the dependencies inside it. Part of the gain from taking a five-file module down to four files is this threshold. Say so when reporting such a gain.
- In a small system a few dependencies fill a large share of the possible pairs, so the loss to dependencies from lower layers is high. aji itself, with 31 files, loses all its points that way.
- A what-if gives what the metric would give. Whether a new layer describes the architecture is for a person to decide. For a C++ library, declaring the headers a layer above the sources raises the score with no change to the code.
- Scores from aji can be compared with each other when the layers were declared in the same way. They cannot be compared with the scores published in the paper (Section 1).
- A type named only in a comment or in Javadoc is no dependency. A local variable or method with the name of a type in scope is taken for that type, which is rare under Java naming conventions (Section 5).
- The M-score serves better for following one project over time and for choosing what to review than as a pass or fail threshold.

## 7. Tests on the projects

`aji.app.HexKriegTest` and `aji.app.ExpertGogglesTest` run aji on a project and check the outputs, the formula and the rules of that project's architecture. Each takes the project file as its argument and writes to `aji-out/NAME` under the current directory, so run them from the scratch folder:

```bash
cd "$WORK" && java -cp aji-classes aji.app.HexKriegTest "$AJI/projects/hexkrieg.aji.xml"
```

Each check prints PASS or FAIL, and a failed rule names up to five of the dependencies that break it. On 6 November 2026 HexKriegTest passed 25 checks and ExpertGogglesTest 22. To give another project such a test, write a class like `test/aji/app/HexKriegTest.java`. `ProjectRun.test` runs aji and the general checks; `run.deps(...)` and `run.files(...)` select dependencies and files for the project's own rules.

## 8. Reference figures (6 November 2026, default options)

| Project file | Root | Files | Dependencies | Layers | M-score |
|---|---|---|---|---|---|
| `projects/aji.aji.xml` | this folder | 31 | 118 | 8 | 75.155 % |
| `projects/hexkrieg.aji.xml` | `C:\repos\ghub-per\HexKrieg` | 83 | 394 | 13 | 84.615 % |
| `projects/expert-goggles.aji.xml` | `C:\repos\ghub-per\expert-goggles` | 335 | 677 | 5 | 45.719 % |
| `projects/expert-goggles-shared-layer.aji.xml` | the same | 335 | 677 | 6 | 54.673 % |

None of the four has a layering violation. aji on aji needs only this folder, so it is the quickest check that a build works. Its figures move whenever aji's own source changes: Section 9 of the README records 75.289 % with 117 dependencies, from before `HexKriegTest` came to use `SystemModel`.

## 9. Changing aji

aji uses only the JDK; keep it so. Its packages are its layers, declared in `projects/aji.aji.xml` and listed in Section 10 of the README. After a change, run the self-test, the two project tests, and aji on aji, and bring the README up to date where behaviour or recorded figures changed.
