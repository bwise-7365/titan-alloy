Copyright Ben Paul Wise. All Rights Reserved.

# Task 07: engine API after M6 (milestone M6b)

status: assigned
worker: W4 (opus)            started: (not yet)
resume: (worker updates this line after every green build)
inputs:
  hexgames/PLAN.md decision log entries of 2026-09-13 "M4 review" and 2026-09-14 "Engine API after M6"
  hexgames/tasks/05-trc-engine.md                  (what M6 added and why: "API changes", open questions)
  hexgames/game_rules/xml/hexrules.xsd, README.md  (the new phase/step element: the contract, do not change it)
  hexgames/game_rules/xml/the-russian-campaign.xml (TRC's phase tree; you add its steps)
  hexgames/hexmodel/Position.{h,cpp}, PositionBuilder.{h,cpp}
  hexgames/hexxml/RulesDoc.{h,cpp}; hexgames/hexrules/RuleSet.h, RuleSetBuilder*.cpp, Ledger.{h,cpp}
  hexgames/hexengine/*.{h,cpp} and hexengine/test/ (Policies.h, Session, Adjudicators*, Defaults*)
  hexgames/hexrecord/Record.cpp, SaveModel*, ReadSaveModel.cpp; game_records/xml/hexsave.xsd (read only)
  hexgames/tools/HexGamesCli.cpp
  hexgames/games/trc/ (engine, test, golden, scenario)
  hexgames/uml/hexengine-session.puml, seq-combat-with-decision.puml
  hexgames/CLAUDE.md, .clang-format
outputs:
  1. Phase steps from the rules XML (Ben's option 2C), replacing HexEngine::GameAdjudicator.
     - hexxml RulesDoc and hexrules RuleSet read phase/step (@id @does @at @commands @rules @turns) into
       strong types (an enum for @at; resolved ids for @rules; a TurnSelector for @turns).
     - A step registry: the engine registers its own behaviours (what Session's EndPhase does today:
       supply check, stacking repair, advance after combat, and so on) and a game registers its own, each
       under a `does` name. Building a Session throws std::invalid_argument, naming the step id and the
       `does`, when the rules name a behaviour nobody registered. No silent fallback.
     - Session runs steps by the rules: enter steps as a phase begins (parent before child), before-command
       steps before the engine adjudicates a command (they may refuse it, naming the rule), after-command
       steps after it, end steps as a phase ends (child before parent); steps with the same `at` in
       document order; @commands and @turns filter. Step functions stay pure (Position in, Position out,
       events to the sink) and a mid-step choice becomes a resolution-stack entry, never a blocking call.
     - game_rules/xml/the-russian-campaign.xml gains the steps TRC's TrcGame hard-codes today (end phase:
       railheads before supply, 4.1.6; weather roll; surrenders; and the rest), each with @rules naming
       the prose rules it carries out. Instance change only; the XSD is fixed.
     - TRC's GameAdjudicator code becomes registered step functions; the GameAdjudicator interface,
       NoGameAdjudicator and Policies::game are removed. The ledger counts a rule as implemented when a
       registered step names it in @rules or a policy claims it; trc_ledger_test keeps both directions.
     - The --defaults smoke run of trc-test (engine defaults only) must keep working. If the TRC rules now
       name behaviours only TRC registers, decide how the smoke run gets a registry; stand-ins that silently
       do nothing are not allowed. Explain the choice in this file.
  2. Game-owned typed state (Ben's option 1C), replacing Position's string side flags.
     - A small base type in hexmodel for a game's state: copy (for fork), a stable hash into
       Position::digest(), and a codec to and from hexsave <sides><side><flag name value/> (the save
       format does not change). Position holds one; a game reads it with one checked accessor that throws
       std::invalid_argument when it is absent or of another type.
     - TRC defines its state as real types (Weather enum, ints, std::vector<HexId>, enum state machines
       for leader-lost and garrison-warsaw ...) and a codec; Trc::Flags string helpers for side flags go
       away. Parsing and validation happen once, in the codec, with the offending flag named on failure.
     - Position::flag/setFlag/flags are removed. Unit markers (unit/@status tokens) are out of scope.
  3. One resolution stack (Ben's option 3B), replacing Position's combat-only CombatPlan.
     - Position holds a stack of obligations; the engine's kinds are today's OwedLoss, OwedRetreat and
       UnitRetreat; a game can add its own kind with strong types (the same pattern as the game state),
       which Dai Senso card effects and Tarawa's reveal-and-consult-again will need.
     - Position::pending() is derived from the top of the stack; applyDecision answers it and works the
       stack down, as settlePlan does today.
     - hexsave has no element for a pending resolution, so a save written mid-decision cannot reload.
       Do NOT edit hexsave.xsd: write a proposal (element shape, one example) under "XSD proposals" below,
       and add a test that saves and reloads a position mid-decision, GTEST_SKIP()ped with the reason
       until the proposal is approved.
  4. Tests (hexgames_add_gtest, literal seeds): step order with nesting and filters; an unregistered
     `does` refused at session build; a before-command step refusing a command; game state fork, digest
     and hexsave round trip; codec failure naming the flag; a game-defined obligation on the resolution
     stack. Update uml/hexengine-session.puml and seq-combat-with-decision.puml to the new shapes
     (still [PROPOSED]).
acceptance:
  tools\build-dev.cmd win-msvc-debug builds with 0 warnings; ctest --preset win-msvc-debug 100% (run ctest
    with CLion's ctest.exe, see tasks/05 log); every label unchanged or grown
  every golden byte-identical: games/trc/golden/*.golden.xml and game_records/xml/trc-test.golden.xml. This is
    a refactor; if a golden must change, stop, set status: blocked and explain here. Nothing is re-blessed.
  python tools/validate-xml.py game_rules/xml game_records/xml packages/xml -> all valid;
    tools/banner-check.py . -> 0 failures
  no GameAdjudicator, CombatPlan, Position::flag or Trc::Flags side-flag helper remains
  no file touched outside inputs; every engine API change listed below with its reason
log:
- 2026-09-14 created by the coordinator (not yet launched; waits for Ben's review of the hexrules.xsd
  step element)

## API changes (worker fills in)

## XSD proposals (worker fills in; nothing applied)

## Open questions for review (worker fills in)

## Brief for the worker (paste as the agent prompt, prefixed by the standard W-role preamble)
You are worker W4. Read this task file first and stay within its inputs. Ben has chosen the engine API
that the next three games build on; M6 (TRC) is the code it replaces. Do the three changes in this order,
keeping the build green and every golden byte-identical after each: (3) the resolution stack, the
smallest; (2) game-owned typed state; (1) phase steps declared in the rules XML, the largest, which moves
TRC's sequence of play out of TrcGame and into the-russian-campaign.xml. The hexrules.xsd step element
is the contract: do not change it; if it cannot express what TRC needs, stop and say so here. Do not edit
hexsave.xsd either; propose. House style and build/test as in CLAUDE.md (2-space, Allman, banners top and
bottom, Yoda, trailing-P, throw not assert, no silent defaults, exhaustive switch, small files, strong
types over string keys). Update the resume line after every green build. Finish with status: review, the
API changes, any XSD proposal, and open questions.

Copyright Ben Paul Wise. All Rights Reserved.
