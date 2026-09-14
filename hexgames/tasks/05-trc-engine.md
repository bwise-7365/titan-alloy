Copyright Ben Paul Wise. All Rights Reserved.

# Task 05: TRC game module (milestone M6)

status: review
worker: W4 (opus)            started: 2026-09-14            finished: 2026-09-14
resume: ALL DONE, awaiting review. Full ctest 165/165 (core 104, trc 38, golden 8, long 2, package 7,
  records 17, xsd 5, hygiene 1). Nothing outstanding but review: see "API changes", "Files touched outside
  games/trc", "Open questions for review" and "Rules not implemented" below.
  Build: vcvars64 + CLion cmake/ninja; ctest via CLion's ctest.exe (see log)
inputs:
  hexgames/games/trc/                                  (empty; you create engine/ test/ golden/ scenario/)
  hexgames/hexengine/*.h, Defaults.h, Adjudicators.h   (the policies and defaults you override)
  hexgames/hexrules/RuleSet.h, Ledger.h, Package.h; hexmodel/*.h; hexsearch/Search.h; hexrecord/Record.h
  hexgames/game_rules/xml/the-russian-campaign.xml     (the 54 prose rules and the typed tables)
  hexgames/game_rules/the-russian-campaign.md          (the digest: rule references and intent)
  hexgames/packages/xml/trc.package.xml, game_records/xml/trc-test.xml, trc-test-script.xml, trc-test.golden.xml
  hexgames/map_graphics/xml/the-russian-campaign.xml   (only to look up hex ids for scenarios)
  hexgames/unit_graphics/xml/the-russian-campaign.xml  (counter ids)
  hexgames/tasks/04-hexsearch-hexengine.md             (the engine defaults and their limits, "API changes")
  hexgames/PLAN.md decision log entry "M4 review" (obligations a-g), CLAUDE.md, .clang-format
outputs:
  hexgames/games/trc/engine/CMakeLists.txt (STATIC trc_game, links hexengine hexrecord), *.h/*.cpp:
    TrcBinding (value-line reader "8-7" -> combat/combat/allowance; sides), TrcPolicies (ZOC with
    Kerch/blocked/all-water hexsides and partisan own-hex ZOC; multiset stacking 2 armies/3 corps;
    movement per unit type x weather x impulse with woods/mountain/swamp stops; rail movement mode on
    friendly rail hexes; supply trace 8 (4 in snow) to friendly cities or rail chains to a city or the
    owner's edge, fatal at end phase; combat: mandatory attacks, odds 1-6..9-1 defender-rounded,
    Stuka/Sturmovik shifts capped at 3, terrain doubling capped x2, woods convert AR/DR to C,
    Kerch doubling; retreat: attacker routes both sides, 1-2 / 2 hexes, exclusions; weather roll with
    the cumulative DRM; victory checks (Berlin, Moscow+Stalin, sudden death sets); phase gate with
    real caps for the two impulses), TrcCommandGrammar (game verbs: rail-move, sea-move, paradrop,
    replace, convert-rail, av-attack), TrcLedger.cpp covering all 54 rule ids
  the CombatPlan fix in hexengine (obligation b): a queue of pending decisions so the defender chooses
    its own loss and the attacker routes retreats, chained as uml/seq-combat-with-decision.puml shows;
    keep the engine change minimal and note it in the task file
  hexgames/games/trc/scenario/trc-1941.xml: the June 1941 set-up transcribed from the 2020 remake
    backs (unit_graphics/xml/the-russian-campaign.xml back faces name AGN/AGC/AGS, MDs, cities, dates)
    -- units whose back is a date are reinforcements, placed in the Off-Map box with @delay
  hexgames/games/trc/golden/*.script.xml + *.golden.xml: rail-move, combat-crt, weather, supply,
    retreat, plus a `long` full-turn script; recorded with hexgames_cli --record; label "golden;trc"
  hexgames/games/trc/test/: policy unit tests, trc_ledger_test, trc_package_test, trc_golden_test
acceptance:
  tools\build-dev.cmd win-msvc-debug -> ctest 100%; labels trc, golden, core unchanged
  trc_ledger_test: every <rule @id> in the-russian-campaign.xml accounted for; OutOfScope <= 8, each with why
  hygiene_banner_check green; no file touched outside games/trc, the CombatPlan change in hexengine
    (+ its tests), and this file; every engine API change listed here
log:
- 2026-09-13 created by the coordinator (not yet launched)
- 2026-09-14 W4 started. First full build with the coordinator's staged BoardBuilder change: builds, and
  `ctest -LE long` 131/131. Environment note: tools\build-dev.cmd's final `ctest` is Visual Studio's
  CMake 4.1.1, which cannot read the CTest files CLion's CMake 4.3 generated ("CMake 4.2 or higher is
  required"), so the script reports errors after a good build; running
  "C:\Program Files\JetBrains\CLion 2026.1\bin\cmake\win\x64\bin\ctest.exe" --preset win-msvc-debug works.
- 2026-09-14 STEP 1 green (131/131): the CombatPlan fix and the GameAdjudicator hook (see "API changes").
  game_records/xml/trc-test.golden.xml re-recorded with hexgames_cli --record: the attacker's EX loss now
  precedes the defender's (13.3), and the scenario's weather-drm side flag now round-trips.
- 2026-09-14 BUILD-DIR REPAIR (no source change): one of my configure runs could not find `cl` (a one-line
  cmd expanded %PATH% before vcvars ran) and wrote empty CMAKE_CXX_FLAGS* into
  cmake-build-debug/CMakeCache.txt; the next build compiled without /EHsc. I deleted
  cmake-build-debug/CMakeCache.txt and cmake-build-debug/CMakeFiles/4.3.1 and reconfigured with vcvars64
  loaded: flags restored, full rebuild, 0 warnings. If CLion shows a stale cache, a "Reload CMake Project"
  is all it needs.
- 2026-09-14 games/trc/engine (27 .cpp) compiles clean.
- 2026-09-14 DONE, status review. Full `ctest --preset win-msvc-debug` 165/165 (was 131).
  games/trc/engine: STATIC trc_game (links hexengine hexrecord), 27 .cpp/26 .h, 5,147 lines (tests 969): TrcFacts (ids and
  derived map facts, data gaps), TrcCalendar, TrcFlags/TrcUnits (state names and helpers), TrcBinding (strict
  value-line reader, loadPackage), TrcZoc, TrcMovement (+ TrcMovementChart, provisional), TrcStacking, TrcSupply,
  TrcCombat (TRC table, ratio ladder, doubling, woods), TrcAir, TrcRetreat, TrcControl, TrcRail (railheads, city
  conversion), TrcVictory (Berlin, Moscow+Stalin, turn 25, sudden death), TrcMandatory (12.1/12.5/16.3),
  TrcArrivals (reinforcement, Off-Map box, replacements, partisans), TrcSpecialMoves (sea-move, paradrop,
  av-attack), TrcPolitics (surrenders, partisan cycle, Warsaw garrison), TrcPhaseGate, TrcGame +
  TrcGamePhases (the GameAdjudicator), TrcCommandGrammar, TrcPolicySet, TrcLedger.
  games/trc/test: trc_package_test (package;trc), trc_ledger_test, trc_rules_test, trc_combat_test,
  trc_sequence_test (trc), trc_golden_test (golden;trc), trc_golden_long_test (golden;trc;long).
  Ledger: 43 Implemented (each claimed by a policy), 1 Common (eliminate-vs-surrender: the engine's pool and
  surrendered boxes), 3 OutOfScope, 7 OptionalNotImplemented; trc_ledger_test checks both directions.
  games/trc/scenario/trc-1941.xml: 98 counters (94 on the map, 3 Stukas boxed, the Special Rail army in the
  Off-Map box), 79 control points, 254 owned rail links, weather clear, DRM 0. Generated once by a scratch script
  from the sheet, counters and package (method in the file's notes); it validates against hexsave.xsd.
  games/trc/golden (all recorded with hexgames_cli --record, none hand-edited): rail-move, combat-crt (Stuka
  3-1 -> 6-1, EX: attacker's then defender's loss choice, routed two-hex retreat), retreat (DR, routed walk),
  weather (two rolled turns, DRM carried), supply (end-phase elimination), full-turn (long: Sep/Oct 1941 from
  Weather Phase to Weather Phase, both players). Scripts validate against hexsave.xsd.
  Rules I could not implement: see "Rules not implemented" (3 out of scope, 7 optional; 4 implemented but inert
  until the sheet has country membership; the movement chart and weather DRM run on provisional data).

## API changes (every contract header touched, and why)

  hexmodel/Position.h     + UnitFlags::defendedP (12.4: a unit is attacked once per phase).
                          ~ ChooseLoss{side, candidates, count}, ChooseRetreat{side, unit, candidates, mayStopP}:
                            a combat decision names the side that answers it (defender's own loss, router).
                          + OwedLoss, OwedRetreat, UnitRetreat, CombatStep, CombatPlan; combatPlan()/setCombatPlan()
                            -- obligation (b): the rest of a battle, worked down one answer at a time.
                          + flag()/setFlag()/flags(): per-side flags (hexsave sides/side/flag, already in the schema
                            and in trc-test.xml but dropped by PositionBuilder). TRC keeps weather, DRM, rail
                            capacity, air used, replacement points and surrenders there. digest() covers all of it.
  hexmodel/PositionBuilder.{h,cpp}  reads <side><flag>; sizes the flag store per rules side.
  hexengine/Policies.h    ~ RetreatEffect{side, fewest, most} (TRC attackers retreat 1-2, defenders 2).
                          + enum RetreatFate {Walk, Stay, Surrender}; RetreatPolicy::fate() (woods, leaders, workers).
                          + MovementPolicy::stopsInP(ctx, unit, hex, mode): Session::reachable read the terrain
                            table itself, so no game could say "swamp is clear in snow".
                          + GameAdjudicator {check, apply, settle, endPhase, enterPhase} and Policies::game: the
                            game's own sequence of play around the engine's adjudicators (the only way to run
                            TRC's end-phase supply elimination, weather roll, rail conversion, surrenders, AV...).
  hexengine/Defaults.h/.cpp  + TerrainMovement::stopsInP (Session's old terminal test, moved), AdjacentRetreat::fate
                            (Walk), NoGameAdjudicator (the no-op default; Place/GameCommand still throw as before),
                            DefaultPolicySet wires it. effectsOf: EX now attacker loss, defender loss, defender
                            retreat 2 (13.3 order); A1/AR retreat 1-2; DR/D1 2.
  hexengine/Adjudicators.h  - applyRetreat (replaced by the plan); + settlePlan; comments rewritten.
  hexengine/Session.h     + private adjudicateCommand (check -> engine adjudication -> settle).
  Implementation only: hexengine/AdjudicatorsCombat.cpp (rewritten: immediate eliminations, plan built from the
    effects, answerLoss/answerRetreat), AdjudicatorsPlan.cpp (new: loss with no real choice taken, retreat walk with
    one-step look-ahead so a route into elimination is refused while another exists, "stop" once the fewest hexes
    are walked, unsatisfiable -> eliminate), AdjudicatorsMove.cpp (applyRetreat removed, defendedP reset),
    Session.cpp (prompt().side is the decision's side; legalCommands offers retreat "stop"; EndPhase runs
    game.endPhase before the engine's supply/stacking/advance and game.enterPhase after; Place and GameCommand go
    to game.apply; a policy set without a GameAdjudicator throws), hexengine/CMakeLists.txt.
  hexrecord/Record.cpp    writes side flags; package path now relative to absolute(out).parent_path() -- a golden
                          written to a bare file name got package="" (found while recording).
  tools/HexGamesCli.cpp, tools/CMakeLists.txt  the cli plays a "trc" record with Trc::TrcPolicySet (links trc_game);
                          --defaults plays it with the engine defaults (the engine smoke golden); --replay of a
                          script that stops on a decision prints one "pending answer what=.. answer=.." per option.

## Files touched outside games/trc, and why
  hexmodel/Position.{h,cpp}, PositionBuilder.{h,cpp}, hexengine/* (above), hexrecord/Record.cpp, tools/* (above).
  hexmodel/test/PositionTest.cpp    decision constructors gained the side.
  hexengine/test/SessionTest.cpp    PendingDecisionGate answers until the plan runs out (a battle can owe several).
  game_records/xml/trc-test.golden.xml  re-recorded (engine defaults): attacker's EX loss before the defender's,
                                    and the scenario's weather-drm flag now round-trips. Cite "rules 13.3".
  packages/xml/trc.package.xml      + <scenario id="trc-1941" path="../../games/trc/scenario/trc-1941.xml"/>
                                    (instance change; no XSD change).
  hexxml/test/PackageDocTest.cpp    TrcParses now expects the manifest's two scenarios.
  cmake-build-debug/CMakeCache.txt  deleted and regenerated (see log); no source effect.

## Open questions for review
  1. PROVISIONAL DATA (TODO(decide) in code): the Movement Allowance Chart (TrcMovementChart.cpp) is on a
     player-aid card no input carries; the Weather Chart panel prints Mar/Apr and Sep/Oct but no Nov/Dec row;
     the rules give no DRM change per weather result (TrcWeather.cpp). Placeholders are clearly marked; the
     weather and full-turn goldens will need re-blessing when they are transcribed.
  2. SHEET DATA GAPS (TrcFacts::dataGaps, asserted by trc_package_test): (a) no <region> membership for any layer,
     so no hex is "in Russia/Hungary/Finland/Germany"; (b) Riga F17, Helsinki C14 and Sevastopol KK23 are cities on
     sea-terrain hexes that no unit can enter. Also the Kerch Strait is an unbound `strait` symbol on KK20:e, so
     TrcFacts takes KK19/KK20 from the rules text; there are no coastline hexsides at all.
  3. "German cities" for the 1945 objectives are hard-coded (Berlin, Posen, Breslau, Königsberg) until (2a).
  4. Kerch stop (8.5): the engine's reachable() asks stopsInP per hex, not per crossing, so entering KK19 or KK20
     always stops. Both are peninsula tips; say if a per-arc stop is wanted (an engine change).
  5. River doubling (14.1.1) is read literally: doubled when every attacker stands on a river hex and the defender
     is not on a hex of the same river; a river is the river hexes joined through river hexsides.
  6. The 1941 scenario's start line is DERIVED (games/trc/scenario notes): the backs name areas and cities, no
     input has their hexes. Units with blank backs (no arrival turn printed) are left out, so the reinforcement
     schedule has no data; the mechanics (Off-Map box + @delay + place) are implemented.
  7. The package binds no counter to mountain, paratroop, ss, guards, luftwaffe or panzer-grenadier types (the
     SS/Guards/Luftwaffe distinction is read from the counter style instead). Paradrop and mountain stop-except
     are implemented but have no counters.
  8. The Archangel die (22.7) has no stream tag of its own and rolls on StreamTag::Setup.
  9. The rules document contradicts itself on leaders: unit-type note "eliminated if forced to retreat" vs rule
     retreat-leaders-workers "Leaders who retreat surrender". Implemented: surrender (the rule id).
  10. Simplifications: after combat the engine's stacking repair removes the newest units, not the owner's
      choice (6.2); the mandatory-attack test asks whether a unit could attack alone; air power does not count
      toward an automatic victory (15.1); railhead advance and city-line conversion convert the shortest traced
      path; the rail-only segment GG19-HH21 (8.6) is not drawn on the sheet.
  11. Goldens under games/trc/golden are written with xsi:noNamespaceSchemaLocation="hexsave.xsd" (writeRecord's
      default), so tools/validate-xml.py cannot validate them in place; the scripts and the scenario validate.
  12. The task's verbs "replace" and "convert-rail" were not added: replacements, reinforcements, the Off-Map box
      and partisans all use the engine's `place`, and rail conversion happens by rule in the end phases.
  XSD proposals: none needed.

## Rules not implemented (ledger)
  OutOfScope (3): withdrawals-first (no withdrawal schedule in any input), garrison-bucharest (the summoned
    mountain corps cannot be identified), encirclement-debt (no exit off the map exists).
  OptionalNotImplemented (7): zoc-battlegroup, national-rows, strategic-movement, fortress-cities,
    industrial-evacuation, battlegroups, bidding.
  Implemented but inert until the sheet carries country membership: surrender-hungary, partisan-placement ("in
    Russia"), combat-supply-halving ("in Russia"), the Finland half of surrender-finland-1944.
  Implemented on provisional data: second-impulse/weather-effects (movement chart), weather-drm (DRM amounts).

## Brief for the worker (paste as the agent prompt, prefixed by the standard W-role preamble)
You are worker W4. Read this task file first and stay within its inputs. Implement the TRC game module
so that the engine plays The Russian Campaign by its rules: every prose <rule @id> in
game_rules/xml/the-russian-campaign.xml is either implemented in a policy (which lists it in claims()),
covered by an engine default, or declared OutOfScope with a reason; trc_ledger_test enforces this.
Start with the CombatPlan fix in hexengine (a queue of pending decisions; defender chooses losses,
attacker routes retreats), because every combat golden depends on it; keep the engine diff small and
list it. Then bindings and policies, then the 1941 scenario, then scripts and goldens recorded with
hexgames_cli. House style and build/test as in CLAUDE.md (2-space, Allman, banners top and bottom,
Yoda, trailing-P, throw not assert, no silent defaults, small files). Update the resume line after every
green build. Finish with status: review and a summary naming any rule you could not implement.

Copyright Ben Paul Wise. All Rights Reserved.
