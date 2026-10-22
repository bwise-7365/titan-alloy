Copyright Ben Paul Wise. All Rights Reserved.

# 05 — Emergent behaviour (forward design, 2026-09-12)

## Deterministic replay
Participants: Session, PrngStreams, EventLog, CommandGrammar, hexrecord's writer and replayer.
Coordination: every random consumption goes through a named stream and is logged as an event; every
choice a rule requires becomes a command in the log rather than a call inside an adjudicator; the
canonical writer sorts and formats so two equal sessions write identical bytes. Emergent property: a
save is a script, a script is a test, and a golden diff names the first move where behaviour changed.
Hidden coupling: any container iterated on the way to a stream must be insertion-ordered; an
`unordered_map` there would silently break replay. Failure mode: a game module that rolls a die
without emitting the event — the ledger cannot catch it, the golden test will.

## Legality by construction
Participants: PhaseCursor, PhaseGate, `Session::legalCommands`, `reachable`, InteractionMachine.
Coordination: a phase carries a capability mask; the engine computes what is legal; the GUI only ever
offers what that list contains and the machine only emits from it. Emergent property: the GUI cannot
re-implement a rule, even by accident, and an AI enumerates the same options a human sees.

## Shared definition, private state
Participants: GameDefinition, Session, SearchScratch, fork(). Coordination: everything immutable is
behind `shared_ptr<const>` and has no caches; everything mutable is owned by one Session. Emergent
property: N sessions on N threads with no locks; the parallel test asserts each fork's digest equals a
serial twin's. Hidden coupling: a "harmless" memo table in a policy would break this — policies are
stateless by contract.

## Rules coverage
Participants: RuleSet's prose rules, each game's Ledger, each policy's `claims()`, the ledger test.
Coordination: the XML is the list of obligations; the ledger is the list of answers; the test refuses
both an unanswered rule and an answer no policy claims. Emergent property: adding a rule to the XML
breaks the build until someone decides what it means in code, and out-of-scope decisions stay visible.

## One lattice, many identities
Participants: Grid, HexIdFormat, Board, the sheet's several grids. Coordination: printed ids are
generated from grid indices; grids on one sheet are placed on one ABC lattice; adjacency across a sheet
seam is ordinary. Emergent property: a rule that names "KK19" and a search that walks the lattice
agree without a lookup table, and Dai Senso's west/east prefix is just a name.

## Package binding
Participants: the three documents, the manifest, PackageLoader, the package tests. Coordination:
identity by default, exceptions listed. Emergent property: a renamed terrain in the sheet or a new
counter without a unit type is a named failure at load, not a silent default at play.

Copyright Ben Paul Wise. All Rights Reserved.
