Copyright Ben Paul Wise. All Rights Reserved.

# 08 — Subsystems (forward design, 2026-09-12)

Overview: grouped by responsibility, not directory. Core domain logic is in hexcoord, hexmodel,
hexsearch and hexengine; orchestration in Session and the players; persistence in hexxml and
hexrecord; presentation in hexview and hexqt; simulation of play in hexgames_bench.

## Coordinates (`hexcoord/`)
Purpose: the ABC algebra and everything geometric. Key symbols: `Abc`, `Qrs`, `HexCentre`,
`HexVertex`, `HexEdge`, `Direction`, `Grid`, `HexIdFormat`. Inputs: a sheet's `<grid>`. Outputs:
addresses, neighbours, pixels. Owns no game data.

## Documents (`hexxml/`)
Purpose: one TinyXML2 façade and value models mirroring the five schemas. Inputs: XML files. Outputs:
`RulesDoc`, `SheetDoc`, `CounterSetDoc`, `SaveDoc`, `PackageDoc`. Owns nothing after loading.

## Domain model (`hexmodel/`)
Purpose: strong ids and quantities, the immutable `Board` and `Roster`, the mutable `Position`.
Inputs: built by hexrules' builders. Outputs: what every other subsystem reads.

## Rules and packages (`hexrules/`)
Purpose: `RuleSet` (typed rules and prose rules), `PackageLoader` (binding three id spaces into a
`GameDefinition`), `Ledger`. Boundary note: this is where XML stops and C++ starts; the ledger test
guards it.

## Search (`hexsearch/`)
Purpose: bounded Dijkstra, A*, floods, components, reach counting, monotone walks, region floods, over
graph adaptors of the Board. Data: only the caller's `SearchScratch`.

## Engine (`hexengine/`)
Purpose: `Session`, `Command`/`CommandGrammar`, `Player`s, `PhaseCursor`, the policy interfaces and
their defaults, pure adjudicators, `Event`/`EventLog`/`EventSink`, `PrngStreams`. Owns all mutable
game state through Session.

## Records (`hexrecord/`)
Purpose: read and canonically write hexsave documents; replay scripts; compare with goldens.
Interaction: reads the definition through `Session`; writes only files.

## Presentation model (`hexview/`, no Qt)
Purpose: `Scene` of primitives with hit tags built from Board, Position and the sheet/counter
documents; `InteractionMachine` from intents to legal commands; `ReplayController`; SVG and JSON
writers. This is the browser seam.

## Qt shell (`hexqt/`)
Purpose: `MapView`, painters, panels, `GameSession`, `AiTurnRunner`, `GameMainWindowBase`. The only
subsystem, with the game GUIs, that links Qt.

## Game modules (`games/<g>/engine`, `games/<g>/gui`)
Purpose: bindings, value-line reader, policy overrides, command verbs, ledger, scenarios, scripts,
goldens, and the game's panels.

## Tools (`tools/`, `tests/`)
`hexgames_cli` (replay, record, validate), `hexgames_bench`, the Python validators and banner check,
the smoke test that compiles every contract header.

## Dependency and interaction summary
hexcoord ← hexxml ← hexmodel ← hexrules ← hexsearch ← hexengine ← hexrecord ← games/engine ←
hexview ← hexqt ← games/gui. Major call paths: load (PackageLoader → builders → GameDefinition →
readRecord → Session), play (Player → Session::apply → adjudicator → Position → events → sinks),
test (script → replay → canonical writer → byte compare). Boundaries worth watching: hexrules ↔ game
modules (the ledger), hexview ↔ hexqt (no Qt leaks left), hexengine ↔ hexrecord (the command grammar
is the contract).

Copyright Ben Paul Wise. All Rights Reserved.
