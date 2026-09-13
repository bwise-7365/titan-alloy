Copyright Ben Paul Wise. All Rights Reserved.

# Task 05: TRC game module (milestone M6)

status: assigned
worker: W4 (opus)            started: (not yet)
resume: (worker updates this line after every green build)
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
