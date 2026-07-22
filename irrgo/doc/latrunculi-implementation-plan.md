<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
# Latrunculi ("Ludus Latrunculorum") — staged implementation plan

Source of rules: `design_resources/Latrunculi rules - BPW.md`. This is a multi-stage build of
a third game for the AbsGame / MCTS+negamax framework, with a Qt6 GUI that draws the board and
pieces by reusing the `irregular_grids` SVG library (rendered via `QSvgRenderer`).

Directories: `latrunculi_game` (engine static lib `latrunculi_lib`) and `latrunculi_gui`
(Qt app `latrunculi_gui`). The board is configurable (default 8×8), pieces ≈ 5/16 of squares
per side. Reuse `absgame` (AbsGame::Game, Searcher), the `irrgo_gui`/`mancala_gui` GUI patterns,
and `irregular_grids` (build_grid, generate_position_svg, BoardSpec/draw_params/board_params).

## Reuse map
- Engine interface: `absgame/AbsGame.h` (currentPlayer/getLegalMoves/isLegalMove/applyMove/
  isTerminal/staticEval/negamaxEval/clone; MoveId=int, kPass=-1), plus the two per-game policy
  hooks `chooseRolloutMove` (MCTS playouts) and `moveOrderScore` (alpha-beta ordering), both of
  which default to the old behaviour so other games are unaffected. Search: `absgame/Searcher.h`.
- Game templates: `mancala_game/Game.{h,cpp}` (minimal), `irrgo_game/Game.{h,cpp}` (richer).
- GUI skeleton: `irrgo_gui/MainWindow.{h,cpp}`+`BoardWidget.{h,cpp}`+`main_gui.cpp` (threaded
  search + searchGen_ + progress bars; menu helpers buildNegaMaxMenu (depth-budgeted, still used
  by irrgo/mancala) / buildNegaMaxTimeMenu (time-budgeted, used by latrunculi) / buildMctsMenu /
  makeStatusBar / retainSizeWhenHidden).
- SVG: `irregular_grids/irregular_grid.h` (generate_position_svg — see Stage 0), `draw_params.h`,
  `board_params.h` (BoardParams, stones_per_side, to_grid_spec, kMin/MaxRowsCols=6..12).
- Custodial/immobilization logic mirrors `irregular_grid.cpp` is_immobilized/active_enemy.

## Move encoding (MoveId : int)
- Placement phase: `moveId = square` (0..S-1).
- Movement phase: `moveId = removeSq*S*S + from*S + to`, `S = rows*columns`, `removeSq==S` ⇒
  no removal. Packs the mandatory remove-captive+move compound. Max ≈ 144³ < INT_MAX.
- (Stage 2) leap destinations reuse the movement encoding (`to` = final landing square; only the
  final position matters for state/captures/super-ko).

## Cell model
`enum Cell { Empty, P0Free, P0Bound, P1Free, P1Bound }` in a flat `rows*columns` vector. "Bound"
= immobilized (flipped to show X): cannot move, cannot help capture. Player owns Free+Bound discs;
"total discs" for win checks counts both.

## Stages

### Stage 0 — irregular_grids: generate_position_svg  [DONE]
Done: `generate_position_svg` + `PlacedPiece` added; shared `emit_background_layer`/
`emit_markers_layer`; per-square disc seeding; `square_to_notation`/`notation_to_square`
chess-like accessors (column letter + row, 1 at bottom) in irregular_grid.{h,cpp}; main.cpp
writes position.svg and prints notation.

Add explicit-position rendering (the existing generate_board_svg only scatters pieces randomly).
```cpp
struct PlacedPiece { int square; std::string fill; bool immobilized; };
std::string generate_position_svg(const BoardSpec& board,
                                  const std::vector<PlacedPiece>& pieces,
                                  const RenderConfig& config = {}, const SvgStyle& style = {});
```
- board.grid/disc/outline/outer_margin/mark_* supply the look; board.pieces and board.seed unused.
- Disc noise seeded per square (qTrans(config.seed + square)) so a square's wobble is stable.
- Factor the shared Background and Markers layers (emit_background_layer / emit_markers_layer);
  keep generate_board_svg output byte-identical.
Files: `irregular_grids/irregular_grid.{h,cpp}`. (User tests after this stage.)

### Stage 1 — Milestone 1 engine + minimal GUI  [ENGINE DONE; GUI DONE]
GUI built in `latrunculi_gui` (derives `guicommon::GameMainWindow`): `BoardWidget` renders the
position via `generate_position_svg` → `QSvgRenderer` → cached `QPixmap`, with pixel→square
inversion (`col=floor((px-offX-margin)/scale)`); two-click movement UX with mandatory
captive-removal-first; `MainWindow` has the banner (bundled OFL **Constantine** font under
`latrunculi_gui/fonts/` (`Constantine.ttf` + `OFL-Constantine.txt`)), per-side tallies, move log, File(New/Save/Load/Quit),
Board(rows/cols 6-12 + discs/side + side colors + Generate), Play & Suggest (NegaMax/MCTS).
Save/Load is XML (`MainWindow_save.cpp`) restored through the engine's new state-injection ctor;
ko history is not serialised (documented). The shared GUI scaffolding was extracted into a new
`gui_common` static lib (`SearchController` + `GameMainWindow` + menu builders) and irrgo_gui +
mancala_gui were migrated onto it (behaviour identical).
latrunculi_game `latrunculi_lib` is created (Game.h/Game.cpp/CMakeLists; wired into top-level
CMake; links abs_game). Cell enum {Empty,P0Free,P0Bound,P1Free,P1Bound}; MoveId = square
(placement) or removeSq*S*S+from*S+to (movement, removeSq==S means none). Implements placement,
single-step movement, custodial capture->Bound (pinnedOn mirrors is_immobilized), mandatory
remove-then-move, self-capture rejection, win by reduction/immobilisation, s=3M/(3M+2N).
Remaining for Stage 1: the latrunculi_gui (SVG board via generate_position_svg + QSvgRenderer,
two-click moves, menus, threaded search) and the bundled Roman-font banner. Rules detail:
- Placement phase: alternate placing one disc on any empty square until each side reaches quota.
- Movement: single orthogonal step of an own Free disc to an adjacent empty square.
- Custodial capture on the moved disc's neighbours → flip flanked enemy to Bound; corner capture
  via the two in-board orthogonal enemies; an already-Bound flanker does not pin.
- Mandatory: enemy captives at turn start ⇒ remove exactly one + move one; else just move. If
  neither possible ⇒ immediate loss.
- Terminal: by-reduction (opponent total discs == 1 at a removal) or by-immobilization (no legal
  move at turn start). Score s = 3M/(3M+2N), current-player perspective; staticEval gradient.
- clone() via default copy ctor; applyMove returns false on illegal.
latrunculi_gui: MainWindow (banner, right panel, Board/Play/Suggest menus, threaded search) +
BoardWidget (generate_position_svg → QSvgRenderer → QPixmap; two-click move UX; squareAt inverse
transform). Bundle OFL Roman font (Stage 1 or 5).

### Stage 2 — Leaping  [ENGINE DONE; validated via self-play PNGs]
Own-color leaps: hop a single adjacent own disc to the empty square beyond; multi-leaps in
multiple directions (base variant); cannot leap the same disc twice per turn; only the final
square matters. Implemented in latrunculi_game: reachableMask/collectLeaps (DFS over leaps +
single step) drive both isLegalMovement and enumerateLegalMoves; Move.path + movePath/findLeapPath
reconstruct the landing sequence; the self-play driver prints moves as "C7 -> E7 -> E5".
Remaining: the reachability allocates per-candidate vectors (fine for self-play, optimise before
heavy MCTS). (The MD spec has no variant rules; ignore the TXT's A1/A2/B/C variants.)

### Stage 3 — Freeing chains  [NOT IMPLEMENTED — and see the caveat before starting]
When a move binds enemies that were flanking your Bound discs, free those discs (flip to Free)
and let them participate; cascade until no further change (B captures→frees B→captures W→…).
Resolve as a fixpoint within applyMove.

Status as of 2026-07-21: nothing of this exists. `moveAndCapture` only ever flips
`freeCell(opp)` → `boundCell(opp)`; no code path anywhere flips Bound back to Free. Both halves
of the rule are missing, not just the cascade:
- Seneca/Locus Ludi rule 6, "if the opponent traps one of the counters trapping their Alligatus,
  their Alligatus is made free" — absent.
- The removal proviso, "as long as both counters that are trapping it are still free" —
  `isLegalMovement` only checks that the removal target is a Bound enemy disc; it never
  re-validates the flankers.
So the engine is currently HARSHER than the variant it claims to implement: a capture, once made,
is permanent until removed.

CAVEAT (decide before building this). Freeing is one of the four brakes identified in
`doc/2026-07-21-latrunculi-dynamism-analysis.md` as the reason captures stop paying for
themselves: a capture yields nothing on the turn it is made, removal consumes the next turn's
action, only one removal per turn is allowed, and the defender gets a free tempo to unmake the
capture by pinning a flanker. Implementing Stage 3 faithfully adds the fourth brake and will
likely make games driftier, not livelier. The two coherent choices are (a) build Stage 3 for
fidelity to the Seneca variant, or (b) adopt the Locus Ludi **Piso** variant instead
(recommendation 4 in the analysis), which deletes the Bound state altogether and makes Stage 3
moot. Do not do half of each.

### Stage 4 — Super-ko [DONE] + draw counter [DONE]
Super-ko DONE: hashBoard (FNV-1a over the Cell bytes) of every end-of-turn board is recorded in
seenPositions_; moveIsLegalOn rejects any move whose resulting board repeats a seen position
(occupancy + bound flags; whose-turn irrelevant; leap intermediates aren't hashed since only the
final board is). clone() copies the set so each search line respects it. Cyclic games now
terminate -- a player whose only moves all repeat a position is immobilised -> loss.
Quiet-game ("Pacific") termination DONE -- this SUPERSEDES the earlier draw counter, which no
longer exists (`movementPlies_`, `kDrawPliesFactor` and `winner_ == -1` draws are all gone).
applyMove ends the game once `pacificPlies_` reaches `pacificMoveLimit` (= 40) consecutive
movement plies with neither a capture nor a captive removal, AFTER the reduction/immobilisation
checks so a decisive result takes precedence (`if (!gameOver_ && ...)`). A turn that captures
(the opponent's Free count drops) or removes a captive resets the counter to 0. The result is
decided on Free-disc material plus komi and recorded as `WinReason::QuietGame`.
DRAWS NO LONGER EXIST: player 1 (second player) receives `komi` = 0.5 in every disc-count
evaluation, so an even Free count resolves in player 1's favour and an integer-count tie is
impossible. `winnerScore()` therefore throws rather than papering over a winner-less terminal.
`immobilizationDiscount` = 0.375 (a Bound disc's weight against a Free one). Not serialised:
`pacificPlies_` resets to 0 on Load (like the ko set), so the quiet-game counter restarts for a
loaded game.
NOTE: the analysis in `doc/2026-07-21-latrunculi-dynamism-analysis.md` shows this reward makes
"get one disc ahead, then stall for 40 plies" the leader's optimal policy (a bare quiet-game win
scores 1612 against 1950 for annihilation -- only 1.21x for a far safer outcome). Recommendation 5
there is to make a QuietGame win worth substantially less than a Reduction/Immobilization one.

### Stage 5 — Polish & persistence  [MOSTLY DONE]
Save/Load game (XML like irrgo) DONE, Theme/color menus DONE, bundled Roman font banner DONE,
move log DONE. Remaining: board-color presets, hover/last-move overlays. (No variant rules --
the MD spec is the single source; the TXT's A1/A2/B/C variants are out of scope.)

### Stage 6 — Search & evaluation overhaul  [2026-07-21; negamax side DONE, MCTS side DEFERRED]
Driven by the complaint that self-play front-loads a dozen captures then drifts for hundreds of
capture-free plies and is decided on material. Full research, code audit and citations:
`doc/2026-07-21-latrunculi-dynamism-analysis.md` (five-agent workflow; 44 sources, 40 verified
open access). READ THAT FIRST before touching search or evaluation. Headline findings: the
self-play driver used **negamax, not MCTS**; and the implemented rules are the Locus Ludi
**Seneca** variant.

DONE:
- `absgame/negamax.cpp` rewritten. Iterative deepening (depths 1..N, only a COMPLETED iteration
  may change the answer) + best-move-first re-ordering between iterations. `negaMax` now takes
  `bool& aborted`; a deadline-truncated iteration is discarded rather than trusted. Previously
  `bestScore = -inf` + strict `>` + a root-loop `break` meant the engine returned the first move
  in `scanOrder_` nearly every ply with a garbage score.
- `AbsGame::Game::moveOrderScore(MoveId)` virtual added, default 0 + a stable sort, so irrgo and
  mancala are unaffected. `Latrunculi::Game::moveOrderScore` ranks removals (10) above new binds
  (5) above quiet moves (0), with a reduction win at 10000. Scores 0 during placement -- placement
  search is therefore still unordered, which is the obvious next optimisation.
- `staticEval` replaced. The old `pressure()` was mobility x material, which collapses to
  "maximise my own move count times my own material" and is maximised by DISPERSING -- it paid
  both sides to avoid the contact a custodial capture needs. Now material + antisymmetric
  positional terms (threats, pairs, openNeighbours, centrality) in "disc units", in a new pure
  -function module `latrunculi_game/Eval.{h,cpp}`. Antisymmetry is required: negamax negates the
  child score, so a term both sides valued positively would make the search incoherent.
  Weights are FIRST GUESSES and untuned. Placement is no longer evaluated as exactly 0.0.
- `reachCount()` deleted: it counted legal move TRIPLES (multiplied by how many captives the side
  held), not reachable squares, routinely exceeded its own normaliser, and cloned the whole game
  twice per eval.
- `playerOf` / `freeCell` / `boundCell` moved from Game.cpp's anonymous namespace into Eval.h so
  both translation units share one copy.
- Placement openings. The self-play driver's old `adjacentToOpponent` filter guaranteed the two
  armies started separated; removed. Opposing discs may now be placed adjacent -- the placement
  rule already forbids the only thing that must not happen, a placement that completes a capture.
  New `latrunculi_game/PlacementPolicy.{h,cpp}`, shared by the self-play driver and the GUI: each
  side plays its first placement at random, then has `kRunMin`(=2) or 3 placements searched, then
  another random one, and so on, each side drawing its own run lengths from one seeded RNG.
  Random placements prefer squares off the border and orthogonally clear of every existing disc,
  falling back to off-the-border, then to any legal placement.
- GUI. `guicommon::GameMainWindow` gained `autoPlayMoveOverride(MoveId&)` (default false) so a
  derived window can supply an auto-play ply instead of searching it; the shared turn-run tail is
  factored into `continuePlay`. Latrunculi uses it for the random placements and tags them
  `[random]` in the move log (display-only, not saved). The NegaMax menu is now budgeted by TIME,
  not depth, since iterative deepening is bounded by the clock: `buildNegaMaxTimeMenu` shares its
  body with `buildMctsMenu` via an anonymous-namespace `buildTimeMenu(title, ...)`, and
  `MctsOption`/`MctsMenuConfig` are now aliases of `TimeOption`/`TimeMenuConfig` so irrgo and
  mancala compile untouched. `kMaxNegamaxDepth` = 64 is only the ID ceiling.

DEFERRED (MCTS). Do these IN THIS ORDER; the first is a prerequisite for the rest:
1. Normalise rewards to [-1, 1] before backup. Alpha-beta is scale-invariant, UCT is not:
   non-terminal leaf values are ~0.05-0.1 against a UCB1 exploration term of 0.37-3.72, so UCT
   selection is currently uniform random; when a rollout does terminate the reward jumps to ~1600
   and exploration vanishes entirely.
2. Ordered expansion + progressive widening. `treePolicy` expands EVERY child before any UCT
   comparison, so with B ~ 60-250 and ~3,000 affordable iterations the tree is one ply deep --
   and a capture needs two plies to cash, three to be safe.
3. `robustChild`'s strict `>` returns `children[0]` on an all-tied root, i.e. a fixed `scanOrder_`
   bias for the whole game (the MCTS twin of the negamax bug fixed above).
4. Tree reuse between plies; then RAVE/AMAF and a transposition table.

ALSO OPEN:
- Tune the `Eval` weights against self-play.
- `moveOrderScore` returns 0 in placement, so placement search gets no ordering and iterative
  deepening reaches only ~depth 2-3 in a second. Rank placements by positional delta.
- Rules-side recommendations 3-5 of the analysis (slide movement + immediate removal; Seneca ->
  Piso; convex capture payoff and not rewarding the stall). Recommendation 4 interacts with
  Stage 3 above -- read that caveat.
- BUG: a placement position with no legal placement is not terminal and returns an empty move
  list; `checkImmobilizationTerminal` returns early unless `phase_ == Movement`. The self-play
  driver then silently `break`s and prints "Draw". Violates the no-silent-default rule in
  CLAUDE.md.

## CMake
- Top `CMakeLists.txt`: add_subdirectory(latrunculi_game); add_subdirectory(latrunculi_gui) AFTER
  irregular_grids.
- latrunculi_game: add_library(latrunculi_lib STATIC Game.cpp Eval.cpp PlacementPolicy.cpp);
  link abs_game; cxx_std_20.
- latrunculi_gui: find_package(Qt6 REQUIRED COMPONENTS Widgets Svg); AUTOMOC; qt_add_executable;
  qt_add_resources(latrunculi.qrc); link Qt6::Widgets Qt6::Svg latrunculi_lib irregular_grid.
  Qt6::Svg is REQUIRED for QSvgRenderer (not in Widgets/Gui).
- Copyright line first AND last on every new file.

## Verification per stage
- Engine: known-position asserts for legality/capture/immobilize/win; clone determinism;
  Searcher::bestMove / mcts return legal moves; self-play terminates with sensible score.
- GUI: SVG board renders, banner font, click-to-move, captures show X, mandatory removal enforced,
  Play/Suggest run with progress bar.
- Regression: irregular_grid_demo still emits byte-identical grid/disc/board SVGs.
<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
