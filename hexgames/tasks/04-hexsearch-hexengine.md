Copyright Ben Paul Wise. All Rights Reserved.

# Task 04: hexsearch + hexengine, plus the M4 glue in hexrecord and the side binding (milestone M4)

status: in-progress
worker: W1 (opus)            started: 2026-09-13
resume: (worker updates this line after every green build)
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

Copyright Ben Paul Wise. All Rights Reserved.
