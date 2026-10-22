Copyright Ben Paul Wise. All Rights Reserved.

# Task 11: Panzergruppe Guderian game module (milestone M7b)

status: review
worker: W4 (opus)            started: 2026-09-14            finished: 2026-09-14
resume: ALL DONE, awaiting review. tools\build-dev.cmd win-msvc-debug 0 warnings; CLion ctest full 224/224
  (pgg 37 incl. 6 goldens, golden 14, long 3, package 13, core 116 + 2 new ResolutionStackTest cases), -LE long
  221/221; games/trc/golden/*.golden.xml and trc-test.golden.xml byte-identical (sha256); validate-xml 11 valid
  (PGG scenario, scripts and goldens also validate with lxml); banner-check 479 files, 0 failures.
  See "API changes" (E1 OweEffect, E2 GameChoice::side, rules XML instance changes, what was built),
  "XSD proposals" (none), "Open questions for review" (14), "Rules not implemented" (3 optional, 0 out of scope).
inputs:
  hexgames/PLAN.md "Terms", RESUME HERE and the decision log (especially 2026-09-14 "Engine API after M6", "M6b
    review", "Hidden units", "PGG"); CLAUDE.md, .clang-format
  hexgames/tasks/05-trc-engine.md and tasks/07-engine-api.md   (how TRC was built, then moved onto the M6b API)
  hexgames/games/trc/                                          (the reference game module on the M6b API: layout,
                                                                policy set, steps, typed state and codec, ledger,
                                                                goldens, tests; copy its shape, not its rules)
  hexgames/hexengine/*.h, hexmodel/GameState.h, Resolution.h, Polymorphic.h, Position.h; hexrules/RuleSet.h, Ledger.h,
    Package.h; hexsearch/Search.h; hexrecord/Record.h; tools/HexGamesCli.cpp, tools/CMakeLists.txt
  hexgames/game_rules/panzergruppe-guderian.md                 (the digest: rule references and intent)
  hexgames/game_rules/xml/panzergruppe-guderian.xml            (47 prose rules, typed tables, the phase tree)
  hexgames/packages/xml/pgg.package.xml, game_records/xml/pgg-test.xml
  hexgames/map_graphics/xml/panzergruppe-guderian.xml (READ ONLY), unit_graphics/xml/panzergruppe-guderian.xml
  the PGG rulebook as text, for set-up and reinforcement schedules and anything the digest compresses:
    C:\Users\bwise\AppData\Local\Temp\claude\C--repos-ghub-per-titan-alloy-hexgames\abed42e6-fc69-4a99-aee8-2f630e666300\scratchpad\pgg\
      text\page-01.txt .. page-08.txt, crt.txt, amendments.txt, errata.txt   (OCR of a 1976 scan; the digest and rules
      XML already fixed its misreads; amendments are base rules, decision log "PGG")
outputs:
  hexgames/games/pgg/engine/CMakeLists.txt (STATIC pgg_game, links hexengine hexrecord) and *.h/*.cpp, in TRC's shape:
    PggBinding (value lines "9-7", "4-6-8", "U-6", "(3)*10"; sides), PggState + codec (typed game state), policies
    (ZOC; stacking by nationality 7.1/7.2 and divisional integration 7.3; movement with road rate, rail movement for
    the Soviets and rail-cut markers, air interdiction costs; supply: German trace to the west edge by road, Soviet
    trace through a Leader's printed radius to the east edge, out-of-supply halving; combat: the CRT with defender-
    rounded odds 1-3..10-1, split results, Engaged, terrain and river doubling, divisional integration doubling,
    leader attack bonus; retreat routed by the opponent, 1-2 hexes, advance after combat; untried units revealed
    at the instant of combat or overrun using RuleSet concealment, zero-strength units removed, the two 0-1-6
    exceptions; overrun during movement (3 MP, halved strength, die, disruption on "*" results, halts on a split
    result); disruption markers and their removal phases; German air interdiction and Soviet interdiction placement;
    reinforcements and the provisional/South-Western Front schedules; victory points and levels), PggCommandGrammar
    (game verbs such as overrun, rail-move, interdict, regroup/breakdown if the rules have them), step functions
    registered under the `does` names the rules XML declares, PggPolicySet, PggLedger covering all 47 rule ids
  hexgames/game_rules/xml/panzergruppe-guderian.xml   phase <step> elements for PGG's sequence of play (instance change,
    each step naming its @rules; the XSD is fixed), including "stacking-repair"/"supply-check" or PGG's own steps
    wherever PGG needs them (an end phase does nothing the rules do not name)
  hexgames/games/pgg/scenario/pgg-1941.xml   the July 1941 set-up from rulebook section 5 (Soviet initial deployment
    and first-turn rules) with reinforcements placed off-map with @delay per section 16; untried units face down
  hexgames/games/pgg/golden/*.script.xml + *.golden.xml recorded with hexgames_cli --record: untried-reveal, overrun
    (with a disruption), combat with a split result and a routed retreat, supply (German road trace and Soviet leader
    radius), interdiction, plus a `long` full-turn script; labels "golden;pgg" (and "long")
  hexgames/games/pgg/test/: policy unit tests, pgg_package_test, pgg_ledger_test, pgg_golden_test
  hexgames/tools/HexGamesCli.cpp, tools/CMakeLists.txt   the CLI plays a "pgg" record with the PGG policy set
acceptance:
  tools\build-dev.cmd win-msvc-debug builds with 0 warnings; ctest --preset win-msvc-debug 100% with CLion's ctest.exe;
    labels trc, golden, core, records unchanged in count and green; TRC goldens byte-identical
  pgg_ledger_test: every <rule @id> accounted for, both directions; OutOfScope at most 6, each with why
  python tools/validate-xml.py game_rules/xml game_records/xml packages/xml -> all valid; banner-check 0 failures
  engine changes only where the M6b API cannot express a PGG rule: each listed below with its reason, kept small,
    with tests; no .xsd edited (write proposals below)
  no file touched outside games/pgg, the PGG rules XML, tools/HexGamesCli.cpp and tools/CMakeLists.txt, and any
    listed engine change. In particular do NOT edit the root CMakeLists.txt (it already adds games/pgg/engine when it
    exists), anything under map_graphics/ (another worker is repairing the Dai Senso sheet and its tools), or the
    TRC module
log:
- 2026-09-14 created by the coordinator; Ben approved starting it while M6d (Dai Senso map) is in flight

## API changes (worker fills in)

Two engine changes, both small, both where the M6b API could not express a PGG rule.

### E1  hexengine/Policies.h: CombatEffect gains OweEffect {Polymorphic<GameObligation> owed}
  Why: a PGG result is not a loss or a retreat the engine can settle. D1/D2/A1/A2 give the affected player a
  choice between a step loss and a retreat (9.65); a German step is taken by turning a counter over or replacing
  a German infantry division's first counter with its second (9.61-9.63), not by Steps; "*" Disrupts an overrun
  defender (6.61); Engaged forbids retreat (9.67); advance after combat follows the path of retreat (9.81). The
  resolver could only return GameEffect, which the engine turns into an event and nothing else, so no game
  obligation could come out of an engine attack. With OweEffect the resolver returns the battle as a game
  obligation, and applyAttack pushes it in the result's order like any other entry, then works the stack down.
  Files: hexengine/Policies.h (the struct and the variant), hexengine/AdjudicatorsCombat.cpp (one branch).
  Tests: ResolutionStackTest.AnAttackCanOweAGameObligation; every PGG battle goes through it
  (PggCombatTest.DefenderRetreatsAttackerLosesAndAdvances, the golden records).

### E2  hexmodel/Resolution.h: GameChoice gains std::optional<SideId> side
  Why: the side that answers a PGG choice is not always the acting side: in the German Combat Phase the Soviet
  player chooses between loss and retreat (9.65). GameChoice named no side, so Session::prompt() reported the
  acting side. ChooseLoss and ChooseRetreat already name theirs; GameChoice now may too (nullopt: the acting side).
  Files: hexmodel/Resolution.h; hexengine/Session.cpp (prompt reads it); hexmodel/Position.cpp (digest);
  hexmodel/PositionBuilder.cpp and hexrecord/RecordResolution.cpp (read and write hexsave ask/@side, which the
  approved schema already has for every ask). Existing two-member GameChoice{verb, options} initialisers compile
  unchanged. Tests: ResolutionStackTest.AGameChoiceNamesTheSideThatAnswers,
  PendingSaveTest.GameObligationsNeedTheGameCodec (the side survives a save), PggCombatTest (prompt side).

Everything else PGG needs is the M6b API used as designed: 34 steps in the rules document, 21 registered
effects/checks and 6 verbs, PggState + PggStateCodec, PggBattle + PggBattleCodec on the resolution stack.

### Rules document (instance changes, XSD untouched, validates)
  - Three spaces: soviet-pool (the counter pool of 12.1), soviet-arrivals and german-arrivals (units waiting
    to enter, with @delay). hexsave requires every unit on a hex or in a space, so reinforcements need one.
    Declared after dead-pile, so the engine's boxFor(soviet, returns) still finds the dead pile first.
  - A phase "set-up" (side german, turns 1) before the Soviet Player-Turn: 5.0 has the German Player place his
    air markers before the first Soviet turn, and a session never runs the enter steps of the phase it starts
    in, so a scenario starting at soviet-move would skip turn 1's supply check, army dice, schedule and
    Provisional Reinforcement. See open question 3.
  - 34 <step> elements on game-turn, the two player-turns and eight of the ten leaves.

### Files touched outside games/pgg
  hexengine/Policies.h, AdjudicatorsCombat.cpp, Session.cpp; hexmodel/Resolution.h, Position.cpp,
  PositionBuilder.cpp; hexrecord/RecordResolution.cpp (E1, E2); hexengine/test/ResolutionStackTest.cpp,
  hexrecord/test/PendingSaveTest.cpp (their tests); game_rules/xml/panzergruppe-guderian.xml (above);
  tools/HexGamesCli.cpp (a "pgg" record is loaded with Pgg::valueLine and played with PggPolicySet),
  tools/CMakeLists.txt (links pgg_game). Nothing under map_graphics/, packages/, the root CMakeLists.txt or games/trc.

### What was built (games/pgg)
  engine/ (STATIC pgg_game, 34 .cpp): PggBinding (value lines 9-7, 2-4-6, U-6, (3)*10 with the printed star),
  PggFacts (+Map: arms, move classes, divisions from counter ids, the 9-7 -> 2-7 successor, Leader ratings, map
  facts, VP hexes and levels read from the rules' lists, provisional entrance areas, data gaps), PggSchedule
  (16.1, 16.2 with errata, 5.1), PggState + PggStateCodec, PggUnits (face, step loss, elimination to the dead
  pile face down, reveal), PggZoc, PggTerrain (the TEC), PggMovement (normal and rail), PggSupply (German road
  and 20-point traces, Soviet LOC through Leaders), PggStacking, PggCrt, PggCombat (assessment order, CRT),
  PggRetreat, PggBattle (+Codec, +Flow, +Walk, +Advance, +Answer), PggObligations, PggOverrun, PggInterdiction
  (air, Soviet, rail cuts), PggArrivals (place, reinforce, schedule, provisional), PggFirstTurn (5.2), PggVictory,
  PggPhaseGate, PggCommandGrammar (rail-move, overrun, interdict, cut-rail, lift-cut, reinforce), PggSteps*,
  PggPolicySet, PggLedger.
  test/: pgg_package_test (package;pgg), pgg_ledger_test, pgg_rules_test, pgg_combat_test, pgg_sequence_test,
  pgg_state_test (pgg), pgg_golden_test (golden;pgg), pgg_golden_long_test (golden;pgg;long).
  scenario/pgg-1941.xml: 174 units: the 5.1 deployment (39 on the map, Soviet divisions drawn at random by type,
  seed 1941, face down), 94 further Soviet divisions in the pool, 10 Leaders and 45 German counters waiting
  with their delays; the army-13/16/19/20 flags. Generated once by a scratch script; validates.
  golden/: untried-reveal, overrun (with a Disruption and the move on), combat-split (D1*/A1, two routed
  retreats, a step loss, an advance), supply (Soviet Leader radius; German 20-point trace, no road to 0120),
  interdiction (air markers on a march and a rail move, the Soviet marker on a German move), full-turn (long:
  Game-Turn One of the 1941 scenario, 55 commands). All recorded with hexgames_cli --record; scripts and
  goldens validate against hexsave.xsd.

## XSD proposals (worker fills in; nothing applied)
  None needed. (The entrance areas, the road into 0120 and the two off-sheet Victory Point hexes are sheet data,
  not schema; see open question 5.)

## Open questions for review (worker fills in)
  1. E1 and E2 above: acceptable as the shape of "a combat result the game settles" and "a choice another side
     answers"?
  2. Markers are not counters on the map. The engine reads every enemy counter on a hex as a unit: Session::
     reachable refuses the hex, attackTargets offers it, applyAttack defends it. A German Air Interdiction Marker
     on a hex (which 13.34 allows on Soviet units) would stop Soviet movement and could be attacked. So the three
     air markers, the Soviet marker, Disruption and the six Rail Cut markers live in PggState, and the marker
     counters (s2-lw-air*, s2-sair-sair) never leave play. game_records/xml/pgg-test.xml (the engine test position,
     not mine to edit) still places s2-lw-air on 0513. A later engine change could skip UnitKind::Marker counters
     in those three places; not made.
  3. A session runs no enter steps for the phase it starts in. I added a "set-up" phase (turn 1) so that the 1941
     scenario starts before the Soviet Movement Phase and its enter steps run. Alternative: an engine option to
     run the starting stop's enter steps. Which does Ben prefer?
  4. Reveal order. On an engine attack the resolver is pure, so the die and combat-resolved events come before
     the revealed and no-strength events; the odds already use the true values and leave the 0-0-6s out, so the
     outcome is the one 12.2 gives. The overrun, which PGG adjudicates itself, keeps the same order for
     consistency.
  5. Data gaps (PggFacts::dataGaps, asserted by pgg_package_test): (a) the entrance areas A, C-H, V, W, X, Z and
     1-6 are provisional hexes (PggFactsMap.cpp, TODO(decide)); only B is in the text; (b) no road reaches 0120,
     so the road trace (11.11) never succeeds and German supply is the 20-point trace alone; (c) Victory Point
     hexes 5907 and 5915 are outside the sheet's 56 columns and score nothing; (d) no rail reaches the south
     edge, so South-Western Front divisions enter on any south-edge hex at or east of Z.
  6. Owner's choices simplified: overstacked units go newest first (7.1); in a retreat each unit walks in turn
     and vacant hexes are preferred, without the amendments' full priority order (9.73); a Leader stacked with
     attackers adds his points and joins the battle's attackers automatically (10.36), and 10.26's assignment of
     a Leader to one of several attacks is not asked.
  7. One target a battle: DeclareAttack names one hex, so 9.23's multi-hex attack is not possible.
  8. One move a unit a phase (the hand leaves the counter, 6.1); only a successful overrun lets it move on. An
     overrun may follow the move that brought the units up (6.51). Reinforcements pay their first hex's terrain
     entry cost (14.0) and may then move.
  9. 10.23: a committed 0-1-6 revealed in its own attack withdraws to the hex it entered the enemy zone from,
     else is eliminated. A Leader that entered with such a unit is not withdrawn.
  10. 5.22: a frozen army's unit may move one hex, off an overstacked hex only. 5.23 "full allowance" is judged as
      no further legal step east, north or south within the points left, on each move and at end of phase.
  11. Divisional integration doubles the division's participating units when the whole division, and nothing
      else (GD or Lehr excepted with a two-regiment division), stands in the hex.
  12. The amendments' Smolensk decay (15.11) is counted from the turn the German Player took Smolensk; losing it
      resets the count.
  13. Streams: the first-turn army dice, the Provisional Reinforcement die and the random draw of a Soviet
      division use StreamTag::Setup; there is no stream named for them (TODO(decide) in PggArrivals.cpp).
  14. The movement supply check writes a supply event only for units found out of supply, to keep the goldens
      readable; supplied units are silent.

## Rules not implemented (ledger)
  OutOfScope (0). OptionalNotImplemented (3): historical-result (errata 15.2), german-side-bidding (amendments
  15.3), step-loss-vp-variant (amendments 15.12).
  Implemented with simplifications (open questions 5-13): soviet-provisional-reinforcement and the schedules'
  entry (provisional areas), german-vp-occupation (two VP hexes off the sheet), soviet-swf-reinforcement (no
  south-edge rail), zoc-stop-and-leave/advance-after-combat (one target a battle), no-strength-units (Leader not
  withdrawn), soviet-first-turn-die (5.22 shuffle), stacking-soviet-leader-bonus (newest removed).

## Brief for the worker (paste as the agent prompt, prefixed by the standard W-role preamble)
You are worker W4. Read this task file first and stay within its inputs. Build the Panzergruppe Guderian game module
on the engine API Ben chose in M6b, the way the TRC module now uses it: the sequence of play as <step> elements in the
PGG rules XML with PGG's step functions registered under their `does` names, PGG's game state as a typed struct with a
hexsave codec, and mid-resolution choices on the resolution stack. PGG is the first game written for that API from the
start, so where the API makes a PGG rule awkward, say so in "Open questions" before bending it; change the engine only
where it truly cannot express the rule, keep each change small, and list it. Order: bindings and typed state, then the
rules XML steps and a first turn that runs, then movement, ZOC, stacking and supply, then combat with untried reveal,
retreat and advance, then overrun and disruption, interdiction, reinforcements and victory, then the 1941 scenario,
the goldens and the ledger. House style and build/test as in CLAUDE.md (2-space, Allman, banners top and bottom, Yoda,
trailing-P, throw not assert, no silent defaults, exhaustive switch, small files, strong types over string keys).
Update the resume line after every green build. Finish with status: review, the engine changes, any XSD proposal, the
open questions and the rules you could not implement.

Copyright Ben Paul Wise. All Rights Reserved.
