Copyright Ben Paul Wise. All Rights Reserved.

# 04 — Class insight (forward design, 2026-09-12; from the contract headers and `uml/*.puml`)

Vocabulary: a *hex* is a cell; a *hexside* is the edge between two hexes; a *side* is a player's
faction; a *counter* is a printed piece; a *unit* is a counter in play; a *position* is where every
unit is and every other mutable fact; a *policy* is a game's implementation of one rules interface.

## Library classes

**`HexCoord::Abc`, `Qrs`** — why: the author's coordinate system, in which one triple names a centre
or a vertex and equivalence under +(d,d,d) makes A+B+C=0 real. Owns nothing but three ints, stored
reduced so equality is structural. Invariant: always the canonical representative. Remove it and every
edge- and vertex-addressed rule collapses back into pairs of hexes. Mistake to avoid: treating
`hvCode` as something to test in game code — only the address constructors do.

**`HexCentre`, `HexVertex`, `HexEdge`** — why: three kinds of place with three kinds of arithmetic;
`HexCentre + Qrs` cannot fail, `HexEdge::hexes()` cannot hit a non-centre. Who calls: Board, search,
the pixel mapping. What breaks without them: a hexside rule (river, seawall, Kerch) written as a hex
pair and tested defensively everywhere.

**`Grid`, `HexIdFormat`** — why: the sheet's printed identifiers are the user's language; the grid
generates them (never parses them by regex), interns them, and is the only place the offset
formulas and orientation live. Assumes the sheet's `<grid>` element is complete. Several grids on one
sheet share one lattice by construction.

**`HexModel::Id<Tag>`, quantities** — why: a UnitId handed to a hex parameter must not compile;
`Steps` cannot be zero; `Odds` always has a one; movement points carry halves. Common mistake: adding
a raw int where a quantity belongs.

**`Board`** — why: everything that never changes, built once, shared by pointer. Owns hex tables,
edge terrains, neighbours, networks, region layers, spaces, grids. Invariant: immutable after
`BoardBuilder::build`. Related: `Roster` (the counters), `RuleSet` (the rules).

**`Position`** — why: the one thing that changes, as a value; its mutators keep stacks, control and
units in step so nothing can disagree. `digest()` is what determinism tests compare. Assumes it is
indexed by the same Board and Roster it was built for.

**`RuleSet`, `ProseRule`, `Ledger`** — why: typed rules feed the engine; prose rules are named and
accounted for. `checkLedger` is the join between the XML and the game module; its test is the only
thing that keeps "every rule handled" honest.

**`PackageLoader`, `GameDefinition`** — why: three id spaces become one, and every dangling
reference is reported with its name before any game runs.

**`SearchScratch`, `Field`, `Algorithms`** — why: one bounded multi-source Dijkstra serves movement,
supply, retreat and planning; generation stamps make clearing free and stale reads throw.

**`Session`** — why: the unit of parallelism. Owns Position, streams, scratch, log; shares the
definition. Invariant: an illegal command leaves everything untouched. `fork()` is the only copy.

**`Command`, `CommandGrammar`, `Player`** — why: a move is a parameterised value, not an enumerable
index; a Player is a chooser, so a human, a script, a random walker and an AI are interchangeable.

**Policies (`ZocPolicy` … `PhaseGate`)** — why: the three games disagree exactly here; the engine ships
a default for each and a game overrides only what its prose changes. Each declares `claims()` so the
ledger can check it.

**`Event`, `EventLog`, `EventSink`** — why: views follow play without touching state; the log is the
golden record's body; every roll and draw is an event, so randomness is pinned.

**`PrngStreams`** — why: adding a die roll to weather must not change combat; per-tag seeding does
that. `restore(tag, draws)` is how a save resumes.

**`HexRecord::Record`, `replay`, `compareWithGolden`** — why: a save is also a script; a script is a
test; a golden is what the canonical writer produced last time someone looked.

## Application classes (planned)
`GameSession` (hexqt) owns a Session and the player assignment; `MapView` renders a Scene and emits
intents; `InteractionMachine` (hexview) turns intents into legal commands; `AiTurnRunner` keeps the UI
thread free. Per game, `<G>MainWindow` adds only that game's panels.

## Test classes (planned)
One suite per contract (see `test-lists/`); `FakeFacade` for the interaction machine; a Session test
double for hexrecord until the engine lands; goldens per game.

Copyright Ben Paul Wise. All Rights Reserved.
