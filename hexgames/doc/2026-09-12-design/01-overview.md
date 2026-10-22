Copyright Ben Paul Wise. All Rights Reserved.

# 01 — Executive overview (forward design, 2026-09-12)

hexgames is a library for hex-and-counter wargames written so that a new game is data first and code
second. A game arrives as four XML documents — its rules, its map sheet, its counters, and a package
manifest that binds the three — and the engine loads them into an immutable game definition. What a
game's prose rules say that the typed data cannot express is written in C++ in that game's own module,
keyed by the rule's identifier, and a test insists that every rule in the document is accounted for.

The engine is headless. A game in progress is a Session: the shared definition, a mutable Position
(where every counter is, who controls what, what decision is pending), a set of named random streams,
scratch memory for searches, and a log of events. Players — a person at a screen, a script, an
automaton — never move counters; they submit Commands, and the Session adjudicates each one with pure
functions that return a new Position and append events. Because nothing is shared mutably, thousands
of Sessions can run on as many threads for training an AI, and because every die roll and card draw
is an event, a game can be replayed from its command log to the byte.

Four games prove the design: The Russian Campaign (the conventional baseline), Panzergruppe Guderian
(the flat-topped map, untried units), Dai Senso (three mutually hostile factions, politics, two map
sheets) and D-Day at Tarawa (solitaire, cards, hidden units). Each has a golden record: a scripted game
whose replay must reproduce a stored save exactly.

The map is addressed in the author's ABC coordinate system, in which three unit vectors summing to
zero name hex centres and hex vertices with the same triples, so a river along a hexside or a fire
dot on one side of a hex is a first-class object, not a pair of hexes. Printed hex identifiers such as
KK19 remain the user-facing names; a sheet's grid maps them to the lattice.

A common Qt6 GUI renders any of these games from a Qt-free presentation model — a scene of primitives
in map coordinates — and turns clicks into Commands through a small state machine that only ever
offers what the engine says is legal. That same presentation model, command protocol and event stream
are what a future browser client would consume; nothing in the engine changes for it.

Copyright Ben Paul Wise. All Rights Reserved.
