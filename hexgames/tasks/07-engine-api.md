Copyright Ben Paul Wise. All Rights Reserved.

# Task 07: engine API after M6 (milestone M6b)

status: review
worker: W4 (opus)            started: 2026-09-14            finished: 2026-09-14 (review round 1 applied)
resume: ALL DONE, awaiting review of round 1 (see "Review round 1" below): full ctest 184/184 (no skip),
  -LE long 182/182, 0 build warnings; games/trc/golden/*.golden.xml byte-identical (sha256);
  game_records/xml/trc-test.golden.xml re-recorded as authorised (one line removed); validate-xml 11 valid;
  banner-check 0 failures. Before round 1: (3) stack, (2) game state, (1) steps all green: full ctest 180/180 (core 113,
  trc 43, records 18, golden 8, package 7, xsd 5, long 2, hygiene 1; one skip = PendingSaveTest by design), 0 build
  warnings, 7 goldens byte-identical (sha256), validate-xml 11 valid, banner-check 0 failures. Nothing
  outstanding but review: see "API changes", "XSD proposals", "Open questions for review".
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
- 2026-09-14 W4 started. Baseline: tools\build-dev.cmd win-msvc-debug builds, ctest 166/166. Golden sha256
  recorded before any change.
- 2026-09-14 STEP (3) green: resolution stack replaces CombatPlan (hexmodel Resolution.h, Polymorphic.h;
  hexengine AdjudicatorsStack.cpp replaces AdjudicatorsPlan.cpp; ObligationPolicy). ctest 169/169 (+2
  ResolutionStackTest, +1 PendingSaveTest skipped by design). Goldens byte-identical.
- 2026-09-14 STEP (2) green: game-owned typed state replaces Position's side flags (hexmodel GameState.h:
  GameState, GameStateCodec, UninterpretedFlags + VerbatimFlagCodec for engine defaults; Policies::state;
  PositionBuilder/readRecord take the codec; TRC TrcState + TrcStateCodec; TrcFlags split, markers now
  TrcMarkers). ctest 174/174 (+1 PositionTest, +4 TrcStateTest). Goldens byte-identical. Note: build-dev's
  last step (VS CMake 4.1.1 ctest) still fails to read CLion's test files; CLion ctest used as instructed.
- 2026-09-14 STEP (1) green: phase steps from the rules XML replace GameAdjudicator (RulesDoc StepDoc, RuleSet Step,
  StepRegistry, Sequence, PhaseCursor lineage; TRC's 66 steps in the-russian-campaign.xml; TrcGame split into
  TrcSteps*). First green run: ctest 180/180 with goldens byte-identical, but core 27.7 s -> 54.5 s and long
  22.7 s -> 48.7 s. Cause: the smoke/defaults sessions ran withheld no-op effects, each returning a full Position
  copy (nine after-command steps per command in rollouts). Fix: Sequence skips withheld steps
  (StepRegistry::withheldP). Final: ctest 180/180, core 25.3 s, long 20.2 s, total 34.6 s; goldens byte-identical;
  validate-xml all valid; banner-check 0 failures; uml/hexengine-session.puml and seq-combat-with-decision.puml redrawn
  ([PROPOSED, M6b]). Status review.
- 2026-09-14 REVIEW ROUND 1 (Ben's answers, PLAN.md decision log "M6b review"): (1) withheld steps accepted, no
  change; (5) CommandGrammar::verbs() and all unknown @commands verbs reported at Session build; (6) UninterpretedFlags
  and VerbatimFlagCodec removed, side flags refused without a game module; (4) hexsave.xsd <resolution> applied as
  proposed, with SaveDoc/SaveModel read and write, Policies::obligationCodec, and PendingSaveTest running for real.
  Build clean, full ctest 184/184, -LE long 182/182. Golden re-recorded with
  `hexgames_cli --record game_records/xml/trc-test-script.xml game_records/xml/trc-test.golden.xml --defaults`:
  game_records/xml/trc-test.golden.xml only. Reason to cite in the commit: "M6b review: no side flags without a game
  module". The six games/trc/golden/*.golden.xml are byte-identical (sha256). A mid-decision save recorded from a
  scratch script (the smoke script stopped after its attack) validates against the new hexsave.xsd with lxml.
  Status review.

## API changes (worker fills in)

### (3) One resolution stack (replaces CombatPlan)
  hexmodel/Resolution.h (new)   The decision types moved here from Position.h (unchanged). + Battle{router, involved};
                                OwedLoss, OwedRetreat, UnitRetreat now carry their Battle (CombatStep and CombatPlan
                                are gone). + GameObligation {clone, kind, appendDigest}, the base of a game's own kinds.
                                + Obligation = variant<OwedLoss, OwedRetreat, UnitRetreat, Polymorphic<GameObligation>>;
                                + Resolution{owed, asked}. Reason: one stack that Dai Senso and Tarawa extend.
  hexmodel/Polymorphic.h (new)  Polymorphic<Base>: a value that clones on copy, with one checked accessor as<T>(what)
                                (throws when empty or another type); Cloneable<Derived, Base>; makePolymorphic.
                                Reason: game-owned types inside a Position that still copies as a value for fork().
  hexmodel/Position.h           - setPending, pending_, combatPlan, setCombatPlan. + resolution() (bottom first),
                                top(), push(Obligation), pop() (both throw when nothing is owed), ask(PendingDecision).
                                pending() is now derived: the top entry's decision, NoDecision when nothing is owed.
                                digest() covers every entry and its decision.
  hexengine/Policies.h          + ObligationPolicy {settle(ctx, streams, sink), answer(ctx, answer, streams, sink)};
                                + Policies::obligations (nullptr: a game obligation reaching the top throws, naming it).
  hexengine/Adjudicators.h      settlePlan -> settleStack(ctx, policies, streams, sink); applyDecision gains
                                PrngStreams& (a game obligation may roll). applyAttack pushes the result's losses and
                                retreats with the first on top. A decision asked by a game obligation goes to
                                ObligationPolicy::answer, after the engine checks a GameChoice's verb and options.
                                AdjudicatorsPlan.cpp -> AdjudicatorsStack.cpp.

### (2) Game-owned typed state (replaces Position's side flags)
  hexmodel/GameState.h (new)    SideFlag{name, value}; SideFlags (one list per rules side); GameState {clone,
                                appendDigest}; GameStateCodec {decode(SideFlags) -> Polymorphic<GameState> (throws
                                naming the flag), encode(GameState) -> SideFlags}; UninterpretedFlags +
                                VerbatimFlagCodec, the engine default (see open question 6). hexsave is unchanged.
  hexmodel/Position.h           - flag, setFlag, flags. + gameState<T>() (throws when absent or another type),
                                heldGameState() (for a codec), setGameState(). digest() covers the state.
  hexmodel/PositionBuilder.h    build(save, board, roster, rules, const GameStateCodec&): the side flags go to the codec.
  hexengine/Policies.h          + Policies::state (const GameStateCodec*).
  hexrecord/Record.h            readRecord(path, definition, grammar, const GameStateCodec&); writeRecord encodes
                                through session.policies().state (throws when unset); compareWithGolden uses
                                policies.state.
  hexengine/Defaults.h          DefaultPolicySet owns a VerbatimFlagCodec.
  games/trc                     + TrcState.h/.cpp, TrcStateCodec.cpp: TrcState (Weather, weather DRM, surrendered
                                std::set<Nation>, helsinkiRussianP, SuddenDeathMet, WarsawGarrison) with one
                                TrcSideState per side (optional counts, std::set<HexCoord::HexId> railTouched,
                                std::set<SeaArea>, LeaderLost ...); stateOf(position), addCount, seaAreaName,
                                nationNamed. An absent count is not zero (a golden writes replacement-points="0").
                                - TrcFlags.h/.cpp; unit markers (out of scope, unchanged) moved to TrcMarkers.h/.cpp
                                as Trc::Markers.

### (1) Phase steps from the rules XML (replaces GameAdjudicator)
  hexxml/RulesDoc.h             + StepDoc; PhaseDoc::steps (document order); parsePhase reads <step> and checks @at.
  hexrules/RuleSet.h            + enum StepAt {Enter, BeforeCommand, AfterCommand, End}; + Step{id, does, at, commands,
                                rules (RuleId), turns (TurnSelector), text, line}; PhaseNode::steps. RuleSetBuilder
                                resolves each @rules token to a prose <rule> and throws naming the step and the token
                                otherwise (an IDREF to a terrain passes the XSD but not the loader).
  hexengine/Steps.h (new)       CheckCall{ctx, policies, command, step} (no streams: a check cannot roll), StepCall{ctx,
                                policies, streams, scratch, sink, command, step}, CommandCall; CheckFn, EffectFn,
                                CommandFn; StepKind and kindOf(step). StepRegistry: addCheck, addEffect, addCommand
                                (throw on a repeated name), withhold(does, kind, why), verify(rules) (throws naming the
                                step id, line and does), checkFor, effectFor, commandFor(verb), withheld(), withheldP,
                                claims(rules). registerEngineSteps: effects "stacking-repair", "supply-check", check
                                "refuse". withholdUnregistered(registry, rules, why).
  hexengine/Sequence.h (new)    Which steps run: commandSteps (the phase and its ancestors, outermost first, filtered by
                                @commands and @turns) and boundary (end steps of the phases left, innermost first; the
                                next stop; enter steps of the phases entered, outermost first); check, afterCommand,
                                endPhase run them.
  hexengine/PhaseCursor.h       + Frame{phase, side}; lineage(stop) (the frames a stop stands in); ancestry(phase);
                                node(phase).
  hexengine/Policies.h          - GameAdjudicator, - Policies::game. + Policies::steps (required).
  hexengine/Adjudicators.h      advancePhase(ctx, Stop, sink): the caller chooses the stop (was policies + cursor).
  hexengine/Session.h/.cpp      The constructor throws without Policies::steps and when verify() fails. apply(): the
                                pending gate, the before-command checks, the engine's adjudication, the after-command
                                effects (of the phase and turn the command was issued in). An EndPhase is
                                Sequence::endPhase. Place and a game verb go to the command registered for their
                                grammar verb. A CommandGrammar is now required by apply().
                                Behaviour change: an EndPhase no longer repairs stacking or checks supply by itself;
                                the rules place "stacking-repair" and "supply-check" where they apply.
  hexengine/Defaults.h          - NoGameAdjudicator. + enum GameSteps {Required, Withheld}; DefaultPolicySet(definition,
                                GameSteps) with no default argument; steps().
  games/trc                     - TrcGame.h/.cpp, TrcGamePhases.cpp. + TrcSteps.h (TrcParts, registration,
                                commandOf<T>), TrcStepsCommand.cpp (2 checks, 8 after-command effects),
                                TrcStepsPhase.cpp (19 enter/end effects), TrcStepsVerbs.cpp (place, sea-move,
                                paradrop, av-attack). TrcPolicySet owns the StepRegistry and adds its claims(). TrcGame's
                                check that refused a move in a mode other than rail or normal is not carried over:
                                TrcMovement::allowance already refuses such a move, so the command is still refused.
  game_rules/xml/the-russian-campaign.xml  66 steps (instance change; XSD untouched, validates): 9 on game-turn (the old
                                check and settle), the rest on the leaves (the old enterPhase/endPhase switch), a
                                "refuse rail-move" on axis-i2 and russian-i2 (second-impulse), @turns="15" on Italy's
                                surrender, and "stacking-repair" last at the end of every leaf (what M6's EndPhase did).
  tools/HexGamesCli.cpp         --defaults builds DefaultPolicySet(..., GameSteps::Withheld) and prints each withheld
                                behaviour on stderr.
  games/trc/engine/TrcLedger.cpp  symbol texts for leader-loss, retreat-leaders-workers, order-railheads-before-supply,
                                order-partisan-removal now name the steps.

### How the --defaults smoke run gets a registry (the task's required decision)
  TRC's rules now name 22 behaviours only TRC registers, so DefaultPolicySet takes an explicit GameSteps argument, with
  no default. Required: Session construction throws, naming the first unregistered step. Withheld: every behaviour the
  rules name and the registry lacks is registered as withheld, with a reason. Its steps are skipped, and each name is
  listed by StepRegistry::withheld(). hexgames_cli --defaults prints the list on stderr, and TrcSmokeGoldenTest asserts
  that roll-weather is withheld and stacking-repair is not. Every engine test that plays TRC's package on engine
  defaults says Withheld at the call site. So nothing is withheld silently: a caller has to ask for it, and the result
  names what was left out. The engine's own behaviours still run, so the smoke golden is byte-identical.

### Tests added (all hexgames_add_gtest, literal seeds)
  hexengine_sequencetest (SequenceTest, core): on the synthetic hexengine/test/rules-steps.xml, boundaries close the
    innermost phase first and open the outermost first; a turn wrap closes and reopens the root; @turns and @commands
    filter; command steps run outermost first; a step whose @rules names a terrain is refused at load
    (rules-step-unknown-rule.xml).
  SessionTest.UnregisteredStepBehaviourIsRefusedAtBuild and BeforeCommandStepRefusesACommand (the game-turn phase's
    check refuses an attack issued in its grandchild; position, digest and log untouched).
  TrcSequenceTest.TheRulesRefuseRailInTheSecondImpulse (the axis-i2 "refuse" step, named rule second-impulse).
  ResolutionStackTest (2): a game-defined obligation under an engine loss, answered through an ObligationPolicy and
    forked as a value; refused without a policy.
  PositionTest.PendingDecisionRoundTrip (rewritten for the stack) and GameStateIsCheckedCopiedAndDigested.
  TrcStateTest (4): codec round trip of every flag; codec failure naming the flag (bad value, unknown name, wrong side,
    repeated); fork and digest; hexsave write and read.
  PendingSaveTest.SaveMidDecisionReloadsTheDecision (records): GTEST_SKIP()s at run time while hexsave cannot hold the
    stack, and starts checking by itself once it can.

### Files touched outside the listed inputs
  hexmodel/CMakeLists.txt, Polymorphic.h, Resolution.h, GameState.h/.cpp (new), test/PositionTest.cpp;
  hexrecord/Record.h, test/CMakeLists.txt, test/ReplayTest.cpp, test/TrcSmokeGoldenTest.cpp, test/PendingSaveTest.cpp (new);
  hexengine/test/rules-steps.xml, rules-step-unknown-rule.xml (new fixtures). No .xsd touched. No golden re-recorded.

### Review round 1 (Ben's answers, 2026-09-14)
  Verbs (open question 5)
    hexengine/Command.h        + CommandGrammar::verbs() (pure virtual): every verb parse() reads.
    hexengine/Defaults.h       DefaultCommandGrammar::verbs(): move attack resolve answer place end-phase. The
                               open-ended "game:<verb>" is not listed, so a rules step cannot name one.
    games/trc                  TrcCommandGrammar::verbs(): the engine's verbs plus rail-move, sea-move, paradrop, av-attack.
    hexengine/Steps.h          verify(rules, grammar): first every @commands token of every step that is not withheld
                               is checked against grammar.verbs(); ALL unknown verbs go into one std::invalid_argument,
                               each as "step '<id>' (line N) names '<verb>'", and nothing further is checked. Then the
                               does check as before. + registeredP(step). withhold() now also accepts a name that is
                               already registered (it is then skipped, not replaced). withholdUnregistered(registry,
                               rules, grammar, why) also withholds the behaviour of any step whose verbs the grammar
                               lacks, and the reason names those verbs.
    hexengine/Session          The constructor requires a CommandGrammar and calls verify(rules, grammar).
                               Sequence::check skips withheld checks, as effects already were.
    Decision to review: the engine grammar has no rail-move, and TRC's rules name it on 5 steps, including two of
    the engine's own "refuse". On engine defaults those behaviours (refuse, count-rail-moves, note-rail-touched,
    note-warsaw) are therefore withheld and listed with the verb in the reason, so the accepted smoke run keeps
    working. In GameSteps::Required the check is strict: SessionTest.UnknownStepVerbsAreAllReportedAtBuild shows
    all five TRC steps reported at once.
  No side flags without a game module (open question 6)
    hexmodel/GameState.h       - UninterpretedFlags, - VerbatimFlagCodec (GameState.cpp removed). A GameStateCodec may
                               return an empty value (no game state).
    hexengine/Defaults.h       + NoGameStateCodec(rules): a document carrying any side flag is refused, naming every
                               flag and its side. + NoObligationCodec: a game obligation is refused by name. Both are
                               what DefaultPolicySet hands out (DefaultsCodecs.cpp).
    hexrecord/Record.cpp       writeRecord writes side flags only when the position holds a game state.
    Smoke inputs: I edited game_records/xml/trc-test.xml itself (weather-drm removed) rather than making a copy. It is
    the engine smoke scenario, read only by engine, record and document tests (hexengine TrcFixture, PositionTest,
    SaveDocTest, ReadRecordTest, RoundTripTest, and the smoke script through the package's "trc-test" scenario). No
    TRC module test reads it: those use TrcTest::blank() and trc-1941.xml, so they are untouched. SaveDocTest's flag
    coverage moved to trc-1941.xml (SaveDocTest.SideFlagsParse). PositionTest uses its own flag-free test codec.
    Re-recorded golden, game_records/xml/trc-test.golden.xml, full diff: one line removed,
      -      <flag name="weather-drm" value="0"/>
    under <sides><side id="axis">, after the turn-track register. Moves, events, units and control are unchanged.
  hexsave <resolution> (proposal approved)
    game_records/xml/hexsave.xsd  + complexTypes Ask and Owe exactly as proposed; + optional <resolution> (owe+) in
                               <save> after <regions>, before <piles>. Owe's children are arg* then ask?.
    hexxml/SaveDoc.h/.cpp      + SaveAskDoc, SaveOweDoc, SaveDoc::resolution (parse checks owe/@kind and ask/@what).
    hexrecord/SaveModel.h, ReadSaveModel.cpp, SaveModel.cpp  + SaveAsk, SaveOwe, SaveModel::resolution; read, and
                               written canonically in schema attribute order (omitted when empty).
    hexmodel/Resolution.h      + ObligationArg, ObligationCodec {decode(name, args), encode(obligation)}.
    hexengine/Policies.h       + Policies::obligationCodec, next to Policies::state (TrcPolicySet: NoObligationCodec,
                               because TRC pushes no game obligations).
    hexmodel/PositionBuilder   build(save, board, roster, rules, GameStateCodec, ObligationCodec): pushes each entry
                               bottom first, with its decision. An entry missing an attribute its kind needs is
                               refused, naming the entry and the attribute.
    hexrecord/Record.h         readRecord(path, definition, const Policies&): grammar, state codec and obligation
                               codec, each required (replaces the grammar + codec parameters). writeRecord writes
                               <resolution> via RecordResolution.cpp (new).
  Tests added or changed in round 1
    SequenceTest.EveryUnknownVerbIsReportedAtOnce (rules-bad-verbs.xml: two misspelt verbs, both reported with step
    id and line; the good verb is not reported). SessionTest.UnknownStepVerbsAreAllReportedAtBuild, and
    UnregisteredStepBehaviourIsRefusedAtBuild now with a grammar that knows TRC's verbs. PendingSaveTest runs for
    real: SaveMidDecisionReloadsTheDecision checks the same side, the same offered answers, and the same stack after
    one answer on both sessions; GameObligationsNeedTheGameCodec round-trips a game obligation through a test
    codec, and writing without one is refused by name. SaveDocTest.SideFlagsParse. TrcSmokeGoldenTest asserts
    "refuse" is withheld with rail-move in the reason. PositionTest.GameStateIsCheckedCopiedAndDigested uses a test
    codec.

## XSD proposals (worker fills in; nothing applied)
  APPLIED 2026-09-14 after Ben's approval (PLAN.md "M6b review"): the <resolution> proposal below, as written.

### hexsave.xsd: the resolution stack, so that a save taken while a decision is pending reloads
  Proposed: an optional <resolution> element in <save>, after <regions> and before <piles>, holding the entries bottom
  first, each an <owe> with an optional <ask>. Engine kinds carry their battle. A game kind carries name/value <arg>
  children, which a game's codec reads and writes the way GameStateCodec handles side flags.

    <resolution>
      <owe kind="retreat" side="russian" fewest="2" most="2" router="axis"
           involved="g-ge-41-armour g-ge-56-armour r-ru-11-infantry"/>
      <owe kind="unit-retreat" unit="r-ru-11-infantry" from="F25" path="F24" fewest="2" most="2" router="axis"
           involved="g-ge-41-armour g-ge-56-armour r-ru-11-infantry">
        <ask what="retreat" side="axis" unit="r-ru-11-infantry" candidates="E24 E25" may-stop="false"/>
      </owe>
    </resolution>

  owe/@kind = loss | retreat | unit-retreat | game. loss: @side @count; retreat: @side @fewest @most; unit-retreat:
  @unit @from @path? @fewest @most; all three: @router? (absent: each unit's owner) @involved (counter ids). game:
  @name, then <arg name value/>*.
  ask/@what = loss | retreat | card | choice. loss: @side @candidates @count; retreat: @side @unit @candidates
  @may-stop; card: @deck @candidates; choice: @verb @options.
  Engine side, once approved: SaveModel/SaveDoc gain the section; Policies gains an ObligationCodec for the game
  kinds, next to Policies::state; PendingSaveTest stops skipping.

## Open questions for review (worker fills in)
  1. ANSWERED (Ben): accepted as built. The --defaults smoke run withholds TRC's behaviours explicitly (above). Is an explicit, listed withhold acceptable,
     or should the engine smoke golden move to a rules document of its own?
  2. With steps in the rules, a document that declares none (PGG, Dai Senso, Tarawa today) gets no stacking repair or
     supply check at an EndPhase: the clock just moves. Their rules XML will need "stacking-repair"/"supply-check" steps
     (or game steps) as M7b and M8 start.
  3. TRC never ran the engine's supply check: TrcPhaseGate grants no Cap::Supply, so M6's EndPhase skipped it, and
     TRC's own "eliminate-unsupplied" is its supply rule. The TRC XML therefore does not place "supply-check". Played on
     engine defaults, TRC's document used to be supply-checked in its end phases (DefaultPhaseGate reads "End" in the
     name) and now is not. No golden or test reaches an end phase on defaults.
  4. At a phase boundary, which phase comes next is judged by the PhaseGate before the end steps run (Sequence.h). TRC
     and the defaults ignore the position in activeP. Dai Senso might want the gate to see the ended position (a faction
     knocked out in an end step); say if so.
  5. ANSWERED (Ben), implemented in round 1. @commands verbs are not checked against the grammar, so a misspelt verb never matches. Proposal: add
     CommandGrammar::verbs() and have StepRegistry::verify reject unknown verbs.
  6. ANSWERED (Ben): no bending. Both removed in round 1; flags are refused without a game module, and the smoke golden
     is re-recorded. Original question: UninterpretedFlags/VerbatimFlagCodec keep a module-less game's flags as strings inside the engine. Without them
     trc-test.golden.xml would lose its weather-drm flag under --defaults. Is that within "strings only in the hexsave
     codec", or should a module-less document refuse flags?
  7. "stacking-repair" is placed at the end of all 12 TRC leaf phases, reproducing M6 exactly. Rule 6.2 may want fewer
     places; moving any of them would change behaviour, not refactor it.
  8. After-command steps use the phase and turn in which the command was issued. For an EndPhase that is the phase
     that just ended, and the effects run after the enter steps of the new phase, as M6's settle() did.
  9. Anything owed on the stack, a future game obligation included, freezes TRC's control update and its stacking after
     combat. Intended?
  10. Before-command checks get no PrngStreams and cannot change the position, so a refused command leaves streams and
      position untouched. A game that needs a roll to refuse would need an effect plus a pushed obligation instead.
  11. Position::digest()'s text format changed (new sections B and G). Digests are compared only within a run and
      appear in no golden.
  12. Test time is back at or under M6's baseline (core 25.3 s, long 20.2 s against 27.7 s and 22.7 s) once withheld
      steps are skipped rather than run as no-ops. TRC sessions still copy the Position once per effect step, as
      M6's helpers did; if rollouts need speed later, an effect could take the Position by value and return it,
      which avoids the copies and keeps steps pure.

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
