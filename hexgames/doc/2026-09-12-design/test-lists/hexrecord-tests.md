Copyright Ben Paul Wise. All Rights Reserved.

# hexrecord — test list (contract for W3; label `records`)

Until M4 lands, W3 stubs `HexEngine::Session` through the contract in `hexengine/Session.h` with a
test double that applies commands by appending events and bumping the digest.

- `ReadRecordTest.TrcTestScenario` — `game_records/xml/trc-test.xml` reads: kind Scenario, 11
  units, two placed in Moscow (P12), one in the OMB space, control of P12 and E31, two streams.
- `ReadRecordTest.ReferencesAreChecked` — synthetic records with an unknown hex, an unknown counter,
  a unit with both hex and space, a unit with neither, and a `#k` beyond the counter count: each
  throws naming `file:line`, the attribute and the id.
- `WriteRecordTest.CanonicalAndStable` — writing the same session twice gives identical bytes;
  attribute order, unit sort, LF, indent as specified; `created` copied from the source script.
- `RoundTripTest.SaveReloadSave` — read → write → read → write yields identical bytes for the TRC
  test scenario and for a synthetic DDaT-style record with a draw pile order and `@next`.
- `ReplayTest.ScriptAppliesInOrder` — a five-move script produces five `Applied` results and the
  expected final digest (from the double).
- `ReplayTest.StrictModeFindsFirstDivergence` — a golden whose 3rd move's recorded draw differs
  from the double's output: `Divergence{3, "draw", ...}`.
- `GoldenTest.ReportOnMismatch` — `compareWithGolden` writes `*.actual.xml`, `matchP == false`,
  names move 3, and the unified diff contains the differing line; on match, nothing is written but
  the actual file and `matchP == true`.
- `GoldenTest.ScriptIsAlsoASave` — a save with a log replays from its scenario to the same digest as
  its own `position`.

Copyright Ben Paul Wise. All Rights Reserved.
