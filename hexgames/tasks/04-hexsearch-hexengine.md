Copyright Ben Paul Wise. All Rights Reserved.

# Task 04: hexsearch + hexengine, plus the M4 glue in hexrecord and the side binding (milestone M4)

status: done   (reviewed and accepted by the coordinator 2026-09-13; TRC-module obligations logged in PLAN.md)
worker: W1 (opus)            started: 2026-09-13            finished: 2026-09-13
resume: ALL FOUR PARTS DONE. ctest 131/131 green (core 103, records 17, golden 2, trc 5, package 3,
  xsd 5, hygiene 1, long 1); banner-check 204 files, 0 failures. Nothing outstanding but review.
inputs:
  hexgames/hexsearch/Search.h, CMakeLists.txt                                     (contract)
  hexgames/hexengine/PrngStreams.h, Event.h, Command.h, Policies.h, Session.h, Player.h, CMakeLists.txt
  hexgames/hexrecord/Record.h, Record.cpp, SaveModel.h, GoldenCompare.h   (W3's; the `// M4:` markers are yours)
  hexgames/hexmodel/*.h, hexrules/*.h, hexxml/*.h   (done; read as needed; RosterBuilder.h/.cpp and
    hexxml/PackageDoc.{h,cpp}, hexrules/PackageLoader.cpp are yours to extend for the side binding)
  hexgames/packages/xml/hexpackage.xsd (new <side> element), trc.package.xml, game_records/xml/trc-test.xml
  hexgames/game_rules/xml/the-russian-campaign.xml (the pilot rules; phases, zoc, movement, combat table)
  hexgames/doc/2026-09-12-design/test-lists/hexmodel-hexsearch-tests.md (hexsearch part),
    hexrules-hexengine-tests.md (hexengine part), hexrecord-tests.md (the deferred replay tests)
  hexgames/uml/hexengine-session.puml, seq-move-command.puml, seq-combat-with-decision.puml,
    seq-replay-golden.puml
  hexgames/game_rules/common-abstractions.md sections 2-3 (the interfaces' rationale)
  hexgames/CLAUDE.md, .clang-format, cmake/HexGamesGtest.cmake
outputs:
  hexgames/hexsearch/*.cpp + test/ (STATIC)
  hexgames/hexengine/*.cpp (+ Defaults.h, PhaseCursor.h, Adjudicators.h as needed) + test/ (STATIC)
  hexgames/hexrecord/Record.cpp M4 glue completed; hexrecord/test replay + ScriptIsAlsoASave tests enabled
  side binding: hexxml/PackageDoc (parse <side>), hexmodel/RosterBuilder (style -> side), hexrules/PackageLoader
    (check: every unit/support/leader counter's style bound to exactly one side; > kMaxSides sides throws)
  hexgames/tools/hexgames_cli (CMake target: --replay <script> [--golden <file>], --record <script> <out>,
    --validate <package>) and tools/CMakeLists.txt
  hexgames/game_records/xml/trc-test-script.xml: a short script (a few moves and an EndPhase) over
    trc-test.xml, and its golden produced by the cli, as the first golden-record test (label golden;trc)
acceptance:
  tools\build-dev.cmd win-msvc-debug -> ctest 100%; labels core, records, golden
  determinism_test and parallel_rollout_test green (16 threads); hygiene_banner_check green
  no file touched outside hexsearch/, hexengine/, hexrecord/, tools/, the three side-binding files,
  game_records/xml/trc-test-script.xml + its golden, and this file; API changes listed here
log:
- 2026-09-13 created by the coordinator
- 2026-09-13 PART A green (91/91). hexxml/PackageDoc gained PackageSideBindingDoc + PackageDoc::side;
  RosterBuilder resolves UnitSpec::side from the counter's front style through that binding (the
  first-side placeholder is gone) and throws on an unbound/doubly bound style or a side the unit
  type's @side mask excludes; PackageLoader::loadRules throws past kMaxSides sides and
  PackageLoader::check collects every unbound/doubly bound counter style. TRC: 96 axis, 106 russian.
  Also touched (beyond the listed outputs, for the tests the change breaks/needs):
  hexrules/test/PackageTest.cpp, hexrules/test/package-broken.xml (a <side> so it still reports
  exactly four problems), new fixture hexrules/test/package-no-sides.xml.
- 2026-09-13 PART B green (109/109). hexsearch is a STATIC library: Search.cpp holds SearchScratch,
  monotoneWalk, regionFlood and the two Board adaptors; the algorithms are templates in Search.h.
  Search.h additions (contract change, all private): SearchScratch::begin(nodes) (size, bump the
  generation, clear the settled list) and SearchScratch::fieldOf<C>(generation);
  Field<C>::checkGeneration(); `template <CostLike C> class Field;` forward declaration; a
  HexSearch::Detail namespace with the priority-queue entry, its comparator and the arc-visitor
  alias. No public signature changed. bfsFlood's Field carries the step count in CostHalves::halves
  (it is a unit-cost flood, so the field is a depth, not a half-point cost).
  Toy boards: the tests build a real 5x5 Board through BoardBuilder from four tiny documents in
  hexsearch/test (toy-rules.xml, toy-sheet.xml, toy-counters.xml, toy-package.xml) rather than a
  hand-made Board, since BoardBuilder is Board's only friend. ToyBoard.h wraps that.
- 2026-09-13 PART C green (127/127; parallel_rollout_test 15 s, 16 threads x 200 commands).
  hexengine is a STATIC library. New files: GameNames.{h,cpp}, PhaseCursor.{h,cpp}, Defaults.h with
  DefaultsZoc/Movement/Supply/Combat/Misc/Grammar.cpp, Adjudicators.h + AdjudicatorsDetail.{h,cpp},
  AdjudicatorsMove.cpp, AdjudicatorsCombat.cpp, plus PrngStreams.cpp, Event.cpp, Session.cpp,
  Player.cpp. Contract-header additions are listed under "API changes" below.
- 2026-09-13 PART D green (131/131). hexrecord/Record.cpp's five `// M4:` markers are gone:
  readRecord builds the Position through PositionBuilder (a script with no <units> resolves its
  @scenario through the package manifest first), writeRecord writes a real canonical save from a
  Session, playRecord/replay record and compare outcomes and draws, and compareWithGolden runs the
  whole loop. tools/hexgames_cli (--validate, --replay [--golden], --record) is built and was used
  to record game_records/xml/trc-test.golden.xml from the new trc-test-script.xml; both pass
  tools/validate-xml.py. hexrecord/test gained ReplayTest.cpp, TrcSmokeGoldenTest.cpp (label
  "golden;trc") and TrcRecordFixture.h.
  One fix outside the listed files: cmake/HexGamesGtest.cmake's `string(REPLACE ";" ";" ...)` was a
  no-op, so gtest_discover_tests re-split the PROPERTIES list and every label after the first was
  read as a property name and silently dropped -- "golden;trc" registered only "golden", and the
  pre-existing "package;trc" only "package". The separators are now escaped, and `ctest -L trc`
  finds five tests where it found none.

API changes (every contract header touched, and why):
  hexsearch/Search.h      + forward declaration of Field; SearchScratch::begin(nodes) and
                            SearchScratch::fieldOf<C>(gen) (private); Field<C>::checkGeneration()
                            (private); namespace HexSearch::Detail (queue entry, comparator, arc
                            visitor alias); the template definitions themselves. No public
                            signature changed.
  hexengine/PrngStreams.h + the definition of the constexpr mixSeed (it was declared only, so no
                            constant expression could use it); PrngStreams::next(tag), the one
                            counted draw every consumer goes through.
  hexengine/Event.h       + struct EventFields and TextEventEncoder::fields(), so hexrecord can
                            write a golden's <event> children without re-parsing the text line.
  hexengine/Command.h     + CommandGrammar::arguments(const Command&), the inverse of parse();
                            without it writeRecord cannot put a command back into a document.
  hexengine/Policies.h    + struct CombatReport and CombatResolver::report(), which carries the odds
                            text, the result code and the die values the event log and a golden
                            need. resolve() stays, written as report(...).effects.
  hexengine/Session.h     + streams(), policies(), context() accessors and the private adjudicate();
                            a save has to write the seed and the draw counts, and a fork or a record
                            writer has to hand the policy set on.
  hexrules/RuleSet.h      + phaseIds(), the whole id->PhaseId map. PhaseNode keeps only the display
                            name, so this is the only route from a dense PhaseId back to the token a
                            document holds (a golden's cursor/@phase and every move/@phase).
  hexrecord/Record.h      + ScriptedMove::events (the hexsave <event> children a replay produced)
                            and playRecord(), which replay, writeRecord and the cli all share.
  hexxml/PackageDoc.h     + PackageSideBindingDoc and PackageDoc::side (the new <side> element).
  hexmodel/RosterBuilder.h  the open question in the header comment is answered and removed.
  cmake/HexGamesGtest.cmake the multi-label fix described above.

Files touched beyond the listed outputs, and why:
  hexrules/test/PackageTest.cpp     the TRC package test now counts 96 axis and 106 russian units
                                    and checks every counter has one of the two sides (asked for).
  hexrules/test/package-broken.xml  gained one <side> so BindingProblemsAreNamed still reports
                                    exactly the four problems it is about.
  hexrules/test/package-no-sides.xml  new fixture: a package with no <side> at all, for the new
                                    PackageTest.UnboundCounterStyleIsReported.
  cmake/HexGamesGtest.cmake         the label fix (acceptance asks for the label "golden;trc").

Open questions for review:
  1. Combat decisions are one round trip, not the chain seq-combat-with-decision.puml draws.
     Position has exactly one PendingDecision slot and no room for the remainder of a combat plan,
     and Position.h was outside this task, so applyAttack settles every choice the defender owes by
     a fixed rule (fewest steps left, then lowest counter id) and by taking the RetreatPolicy's
     first candidate, and raises a ChooseLoss only for a step the attacker owes with more than one
     candidate. That satisfies the gate the test list asks for and keeps a replay exact, but TRC's
     "attacker routes both sides, defender chooses his own loss" needs either a second slot on
     Position or a game-supplied adjudicator. Which?
  2. The default supply check never removes a unit, whatever the trace's @fatal says: it marks
     UnitFlags::isolatedP and emits SupplyChecked. Elimination is a game rule, and a wrong default
     would quietly empty the board. Confirm that is where the line belongs.
  3. BoundedTrace reads "a friendly-controlled city" as "a hex this side controls that carries a
     drawn feature", because the rules document holds the real source set only as prose. It is the
     weakest of the defaults and has no test of its own; the TRC module should replace it.
  4. DefaultPhaseGate reads capabilities off the printed phase name ("Movement", "Combat", "End",
     "Weather"). It is documented as a default a game overrides, but it is a string match on
     display text; a game_rules addition (phase/@grants) would be cleaner if you want one.
  5. defaultValueLine reads a two-number value line as one combat factor used in attack and defence
     plus a movement allowance ("8-7" is 8/8/7 hexes), which is TRC's and PGG's convention. A game
     whose counters print attack-defence supplies its own reader; say if the default should be the
     other way round.
  6. HexEngine::enemiesOf: no TRC side declares @hostile-to, so the HostilityMatrix is empty and
     nothing was an enemy of anything. The engine now reads an entirely empty matrix as "every
     other side is an enemy" and takes any declared matrix at its word. The alternative is to add
     hostile-to="russian"/"axis" to the-russian-campaign.xml, which is a rules-document change and
     so a review gate.
  7. parallel_rollout_test carries the labels "core;long" (15 s of the 20 s core run), so the fast
     set `ctest -LE long` skips it. Say if it should be core only.

Copyright Ben Paul Wise. All Rights Reserved.
