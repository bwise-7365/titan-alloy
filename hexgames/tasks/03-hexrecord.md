Copyright Ben Paul Wise. All Rights Reserved.

# Task 03: hexrecord — hexsave reading, canonical writing, replay, golden compare (milestone M5)

status: review
worker: W3 (sonnet)          started: 2026-09-13
resume: Done. Full build/test green (72/72, 15 under label `records`), banner-check 0 failures,
  canonical output validated against hexsave.xsd. Document-level model, writer, golden compare and
  reading are all implemented and tested; the Session-dependent glue (readRecord/writeRecord/replay/
  sessionFor/compareWithGolden) compiles against the contract headers with `// M4:` comments marking
  what needs Board/Roster/a working Session. See log for detail and API additions.
inputs:
  hexgames/hexrecord/Record.h, CMakeLists.txt                                  (contract)
  hexgames/game_records/xml/hexsave.xsd, trc-test.xml, packages/xml/trc.package.xml
  hexgames/doc/2026-09-12-design/test-lists/hexrecord-tests.md
  hexgames/uml/seq-replay-golden.puml
  hexgames/hexengine/Session.h, Command.h, Event.h, PrngStreams.h   (contract only; not implemented yet)
  hexgames/hexmodel/Position.h, Ids.h                                 (contract only; W2 implementing)
  hexgames/hexxml/XmlDocument.h                                       (W2 delivers first; API fixed in the brief)
  hexgames/tools/validate-xml.py, CLAUDE.md, .clang-format, cmake/HexGamesGtest.cmake
outputs:
  hexgames/hexrecord/*.cpp, CMakeLists.txt (STATIC), test/CMakeLists.txt, test/*Test.cpp
  hexgames/hexrecord/SaveModel.h (if you need a document-level record model beside Record.h; see brief)
acceptance:
  tools\build-dev.cmd win-msvc-debug -> ctest 100%; new suites carry label records
  hygiene_banner_check green; no file outside hexrecord/ (and this file) touched
  canonical writer output validates with tools/validate-xml.py against hexsave.xsd
  Session-dependent glue (replay, sessionFor, compareWithGolden) compiles against the contract;
  its tests are marked and deferred to M4 unless a stub makes them runnable
log:
- 2026-09-13 created by the coordinator
- 2026-09-13 W3: read all inputs. hexgames/hexxml/XmlDocument.h existed but hexxml had no
  CMakeLists.txt/.cpp yet (not a real target), so per the brief did the document-level model, writer,
  diff and golden-compare work first: added hexgames/hexrecord/SaveModel.h (plain value model of the
  hexsave document: kind/game/package/scenario/seed/engine/created/title, package files, cursor,
  sides/registers/flags, units, control hexes/links, regions, piles, streams, log of moves with
  args/result/draws/events, notes -- all strings and numbers, no HexModel/HexEngine dependency) and
  SaveModel.cpp (writeCanonical/canonicalText: fixed attribute order per hexsave.xsd, units and control
  hexes sorted by @id, moves sorted by @n, two-space indent, LF, no trailing whitespace, schema
  location copied through). Added GoldenCompare.h/.cpp (compareLogs -> HexRecord::Divergence reused
  from Record.h; unifiedDiff, an LCS line diff capped at 80 lines; actualPathFor; buildGoldenReport
  assembling a HexRecord::GoldenReport; formatGoldenReport for the printable move/diff/re-bless
  message). Wrote 4 suites (WriteRecordTest, CanonicalOrderTest, DiffTest, GoldenTest) against this,
  all green with no hexxml dependency.
- 2026-09-13 W3: hexxml grew a real CMakeLists.txt/.cpp mid-session (it also ships its own
  HexXml::SaveDoc, a generic hexsave parse with no invariant checks -- deliberately not reused here so
  that the three document-level checks below keep file:line precision from XmlNode). Wrote
  ReadSaveModel.cpp (SaveModel::read over HexXml::XmlDocument/XmlNode) checking: exactly one of
  unit/@hex and unit/@space; a "#k" copy suffix with k >= 1; move numbers contiguous from 1 -- each
  throwing std::invalid_argument naming file:line, the unit id or "log", and what's wrong.
  hexrecord/CMakeLists.txt gates ReadSaveModel.cpp and the hexxml link behind `if(TARGET hexxml)` so
  the build stays green either way. Added ReadRecordTest (TrcTestScenario against the real
  game_records/xml/trc-test.xml, plus the 3 checks above against synthetic fixtures) and RoundTripTest
  (trc-test.xml and a synthetic DDaT-style pile-with-@next record, each read/write/read/write byte
  identical), gated the same way in test/CMakeLists.txt; both suites built and passed once hexxml
  landed. Wrote Record.cpp: readRecord/writeRecord/sessionFor/replay/compareWithGolden compile against
  the hexengine/hexrules/hexmodel contract headers; sessionFor is a real one-line Session
  construction, readRecord/writeRecord convert every log-move field (including HexEngine::Command via
  CommandGrammar::parse/verb) but leave HexModel::Position empty, and replay/compareWithGolden call
  session.apply() but cannot yet compare outcomes -- each gap marked `// M4:` naming what Board/Roster/
  a working Session would supply. Fixed one MSVC warning (C4457, shadowed `path` parameter). Full
  build green: 72/72 tests (15 under label `records`), hygiene_banner_check and all 5 xsd_validate_*
  suites passing, 0 build warnings. Ran tools/validate-xml.py by hand against a canonical write of the
  TRC scenario and a synthetic DDaT-style pile record (both OK) and tools/banner-check.py directly (0
  failures). Deferred to M4, per the brief: ReplayTest.ScriptAppliesInOrder,
  ReplayTest.StrictModeFindsFirstDivergence, GoldenTest.ScriptIsAlsoASave (all need a working Session);
  the cross-document parts of ReadRecordTest.ReferencesAreChecked (unknown hex, unknown counter, "#k"
  beyond the counter count -- these need Board/Roster; the document-level parts of that same named test
  are implemented now). status: review.

Copyright Ben Paul Wise. All Rights Reserved.
