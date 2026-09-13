Copyright Ben Paul Wise. All Rights Reserved.

# 02 — Architectural overview (forward design, 2026-09-12)

## Purpose
A reusable engine, GUI and record format for hex wargames specified in the hexrules, hexsheet and
hexcounters XML languages, with four games as use cases and room for more.

## Runtime architecture
Nine static libraries in a strict dependency chain: hexcoord (ABC algebra) → hexxml (document models)
→ hexmodel (Board, Roster, Position) → hexrules (RuleSet, package bindings, ledger) → hexsearch
(searches over a const Board) → hexengine (Session, commands, players, policies, adjudicators, events)
→ hexrecord (saves, scripts, goldens) → hexview (presentation model, no Qt) → hexqt (widgets). Per game,
an engine module and a GUI executable sit on top. Only hexqt and the game GUIs link Qt.

## Major subsystems
Coordinates; document loading and binding; the domain model; search; the engine proper; records; the
presentation model; the Qt shell; the game modules (TRC, PGG, DS, DDaT).

## Control flow
A Session exposes a prompt (turn, phase, acting side, pending decision). A Player reads it and submits
one Command. `Session::apply` validates against the phase's capability mask and the legal set, runs the
adjudicator for that command as a pure function over (definition, position, policies, streams), stores
the returned Position, appends events and notifies sinks. A PhaseCursor walks the rules' phase tree,
repeating a node once per acting side, skipping by turn selector and by the game's PhaseGate.

## Data flow
XML → typed document models → RuleSet/Board/Roster (immutable, shared by pointer) → Session state
(Position, streams, scratch, log) → events → views and records. Records flow back: a hexsave document
becomes a Position plus a command list; a script replays into a Session; a golden is what the canonical
writer produces afterwards.

## Lifecycle
Load package → build definition → read scenario → construct Session → loop (prompt, choose, apply)
→ victory or end of script → write record. Forking a Session copies Position, streams and scratch.

## External interfaces
The four XML languages (validated by lxml under ctest); the command grammar as text; the event log as
text; SVG and JSON writers for the presentation model; the Qt widgets; `hexgames_cli` for replay and
recording; `hexgames_bench` for parallel throughput.

## Concurrency model
Share nothing mutable. The definition is `shared_ptr<const>` with no caches; each Session owns its
mutable state; algorithms take a `SearchScratch&`. No globals, no static mutable state, no
`thread_local`. Determinism comes from named PRNG streams and insertion-ordered containers wherever a
stream is consumed. The GUI runs AI searches on a worker thread with value capture and a staleness
token, and the engine never learns the GUI exists.

## Error handling philosophy
Validate once at the boundary — XML loading and command submission — and throw
`std::invalid_argument` naming the file, line and identifier. Inside the engine, invariants live in
types (HexCentre vs HexVertex, Steps, Odds, strong ids, MovementPoints in halves), so there are no
defensive null checks and no silent default substitution. An illegal command leaves the Position
untouched.

## Known risks
The boundary between typed rules and game code drifting (mitigated by the ledger test and loader tests
on the real documents); Position copy cost for search at Dai Senso scale (measured before optimised);
the sheet grid formulas versus the renderer (pinned by a four-sheet pixel test); Debian font
differences (image goldens are informal; text goldens are the contract).

Copyright Ben Paul Wise. All Rights Reserved.
