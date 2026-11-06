# Open question: should `apps/shared` be a layer of its own in expert-goggles?

5 November 2026. Undecided; Ben's design decision.

## The question

The official architecture of expert-goggles (`restructure/README.md`) has five layers: foundation, engines, orchestration, facilities and applications. It does not mention `apps/shared`, which sits in the applications folder. Should `apps/shared` be recognized as a separate layer between the facilities and the applications?

## Background

`apps/shared` holds ten files of support code that several applications use: solver bindings (`fleet_solver`, `nonlinear_solver`), the planner window layout (`plannergui`, `flow_view`), runtime handling (`runtime.hpp`) and a Qt text helper (`qt_text.hpp`). Eight applications include it: minppd, pform, glra, fleet, glra5, network_demo, ppdi and glrademo.

The expert-goggles documentation already treats it as something below the applications, though not as a named layer:

- `apps/README.md`: an application uses the lower libraries and the shared support, but never another application.
- `apps/shared/README.md`: the directory is application support, not another application, and its common code lives below the applications' executable targets.
- `apps/shared/tests/check_application_boundaries.cmake` enforces that applications do not include one another; it does not forbid including `apps/shared`.

## Why it matters for the M-score

The M-score treats the files of one layer that are linked by dependencies as one module, and it penalizes large modules. The dependencies are the same either way; only the declaration changes. aji gives these results (tests left out, default settings):

| Layering | Applications layer | M-score |
|---|---|---|
| Five layers, `apps/shared` inside the applications layer | one module of 124 files, which alone costs 24.8 points | 45.719 % |
| Six layers, `apps/shared` as its own layer 5 | twelve separate modules, the largest 36 files (minppd) | 54.673 % |

The project files are `aji/projects/expert-goggles.aji.xml` and `aji/projects/expert-goggles-shared-layer.aji.xml`.

## Points to weigh

- **For a separate layer.** It matches what the code and the application READMEs already say: shared support is used by applications and uses no application. The score then measures what the architecture intends, namely that the applications are independent of one another.
- **For keeping five layers.** The official plan does not name it, and the extra score comes from a declaration rather than from any change in the code. If `apps/shared` is meant to stay a small convenience folder, not a library with its own contract, keeping it inside the applications layer records honestly that the applications are linked through it.
- **Either way,** the choice should be made once on design grounds and then kept fixed. Scores taken under different layer declarations cannot be compared for trends.

## If the answer is yes

1. Add `apps/shared` to the layer table in `restructure/README.md`, with its permitted dependencies (from its CMake file: `vimcp::solve`, `vimcp::logistics`, `vimcp::gui`, `vimcp::mb2`).
2. Make `aji/projects/expert-goggles-shared-layer.aji.xml` the standard project file for expert-goggles, or merge it into `expert-goggles.aji.xml`.
