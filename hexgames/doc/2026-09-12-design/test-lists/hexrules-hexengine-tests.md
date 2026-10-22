Copyright Ben Paul Wise. All Rights Reserved.

# hexrules and hexengine — test lists (contracts for W2 and W1)

## hexrules (`hexrules/test/*Test.cpp`, labels `core package`)
- `RuleSetTest.ThreeRuleFilesLoad` — TRC, DS, DDaT (and PGG once authored): sides, terrain counts,
  phase tree depth, resolver tables with checked cell arity, randomizers, prose rule count (TRC 54).
- `RuleSetTest.HostilityIsSymmetric` — DS three-way hostility from `@hostile-to`; an asymmetric
  synthetic document throws naming both sides.
- `RuleSetTest.IdrefsResolve` — every `blocked-by`, `units`, `phase`, `network`, `layer`,
  `randomizer`, `exempt`, `projected-by`, `stop-except`, `enter-only` resolves; a synthetic dangling
  id throws with `file:line` and the id.
- `RuleSetTest.TurnSelectorsAndTables` — `sudden-death` phase turns "5,11,17,23"; CRT 6×9.
- `PackageTest.TrcLoads` — `PackageLoader::load(trc.package.xml, TrcValueLines)` yields a Board of
  the sheet's hex count and a Roster of 214 counters minus markers; `check` returns no problems.
- `PackageTest.BindingProblemsAreNamed` — a manifest with an unknown sheet terrain, an unbound
  counter, a space bound to a missing panel, and a scenario with a non-existent hex: `check` lists
  exactly those four, each naming the id.
- `PackageTest.FourPackagesCheckClean` — once the four manifests exist (label `package;<g>`).
- `LedgerTest.Consistency` — synthetic ledger vs rule ids: missing, unknown, duplicate and
  unclaimed-implemented entries are each reported; a consistent ledger reports nothing.

## hexengine (`hexengine/test/*Test.cpp`, label `core`)
- `PrngStreamsTest.TagsAreDecorrelated` — first outputs of nine tags from one seed all differ;
  `mixSeed` is a pure function with known values (embed three constants).
- `PrngStreamsTest.RestoreReplays` — `restore(tag, n)` reproduces the (n+1)-th output; draws counted.
- `PhaseCursorTest.WalksTrcTree` — document order over the TRC phase tree: weather, axis i1 move,
  i1 combat, i2 move, i2 combat, axis end, russian ..., sudden death only on turns 5/11/17/23.
- `PhaseCursorTest.RepeatsPerSideForDs` — the Dai Senso faction turn repeats for axis, western,
  soviet in that order.
- `OddsTableResolverTest.TrcCrt` — 3:1 with die 4 → DR (from the loaded table); Stuka shift 3
  turns 3:1 into 6:1; shifts cap at 3; terrain doubling saturates at ×2.
- `SessionTest.IllegalCommandLeavesPositionUntouched` — a move for a unit not of the acting side
  throws; `position().digest()` unchanged; no events appended.
- `SessionTest.MoveThroughReachable` — on the TRC test scenario: `reachable(41st armour)` contains
  hexes within its allowance and excludes sea; applying a listed path emits `UnitMoved` and moves
  the unit; applying an unlisted path throws.
- `SessionTest.PendingDecisionGate` — after an EX result, only a matching `DecisionAnswer` is
  accepted; any other command throws with "decision pending".
- `DeterminismTest.SameSeedSameScript` — replaying the TRC test script twice yields identical
  digests after every command and identical event logs.
- `ParallelRolloutTest.ForksAgreeWithSerial` — 16 threads each fork the session and play 200
  random commands with `RandomPlayer` on their own stream; each thread's final digest equals a
  serial run with the same seed; runs under ASan/UBSan (Debian) and `/fsanitize=address` (MSVC).
- `TextEventEncoderTest.OneLinePerEvent` — every Event alternative renders one stable line.

Copyright Ben Paul Wise. All Rights Reserved.
