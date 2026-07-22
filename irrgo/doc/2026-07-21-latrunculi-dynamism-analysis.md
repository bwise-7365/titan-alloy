<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
# Latrunculi: why the games are boring — analysis and recommendations

*2026-07-21. Produced by a five-agent research workflow: two agents on Latrunculi rules
literature (scholarly/archaeological and modern/playable), one auditing this repo's search
and evaluation code, one on MCTS and game-decisiveness literature, and one verifying every
citation. 44 citations fetched and checked; 40 verified live and open access.*

*Recorded verbatim as delivered, for future reference.*

---

Workflow complete: 8 agents, 44 citations fetched and checked, 40 verified live and open access.

## First, a correction to the premise

`latrunculi_game/selfplay.cpp` does not use MCTS. It calls `AbsGame::Searcher::bestMove(game, 6, 1000)` (`selfplay.cpp:202`), which is **negamax alpha-beta** (`absgame/negamax.cpp:28`). And the placement phase is **explicitly random** — `mv = pool[rng() % pool.size()]` (`selfplay.cpp:198-199`), restricted to squares *not adjacent to an enemy*, so the two armies start deliberately separated. So the boring games you are seeing come from a random opening followed by a search that, as shown below, barely runs.

---

## Top five recommendations

- **1. Fix the self-play search first — it is currently a no-op (engine, small effort, do this before judging any rule set)**
  `bestMove` loops root moves with `if (Clock::now() >= deadline) break;` and no iterative deepening or move ordering. At depth 6 with a branching factor of ~27–80 on the 6×6/9-per-side config, alpha-beta needs ≳27,000 nodes for the *first* root child; per-node cost is 25–75 µs because every `applyMove` runs `checkImmobilizationTerminal` → a full `enumerateLegalMoves`, and every `moveIsLegalOn` copies the 64-cell board and runs a full FNV hash. That is ~1.35 s for one root move against a 1000 ms budget. `bestScore` starts at −∞ and the comparison is strict `>`, so **the engine returns the first move in `scanOrder_` nearly every ply**, with a deadline-truncated garbage score.
  - *Pro:* explains the symptom completely and on its own; iterative deepening + capture-first move ordering + a `hasAnyLegalMove()` fast path is a contained change; you cannot evaluate any rule change until the engine actually plays.
  - *Con:* fixes strength, not the game's design — a well-searched Seneca game may still grind, because recommendations 3 and 4 are genuine rule-level problems.
  - Standard remedy references: [Baier & Winands, MCTS-Minimax Hybrids (JAIR)](https://www.jair.org/index.php/jair/article/view/11208), [Lanctot et al., Implicit Minimax Backups](https://arxiv.org/pdf/1406.0486)

- **2. Rewrite `staticEval` — it currently pays both players to run away from each other (engine, medium effort)**
  `pressure(b) = 1 − M·D/(Mmax·Dmax)` with `score = pressure(opp) − pressure(me)` collapses algebraically to `(M_me·D_me − M_opp·D_opp)/(Mmax·Dmax)` — *maximise my own mobility × my own material*. Mobility is maximised by dispersing into empty space, i.e. **away from contact**, while the capture that contact enables sits beyond the horizon. Two supporting defects: `reachCount()` returns the number of legal move *triples*, not reachable squares, so it is multiplied by how many captives you happen to hold and routinely exceeds its own normaliser `Mmax = squares_` (`Game.cpp:700-707`); and it returns 0 during placement, so **all 40 placement plies evaluate to exactly 0.0** — the phase the Roman sources single out (*"callidiore modo tabula variatur aperta calculus"*, [Laus Pisonis 190ff.](https://penelope.uchicago.edu/Thayer/E/Roman/Texts/Laus_Pisonis/text*.html)) has no strategic content at all.
  - *Pro:* directly targets the anti-aggression bias; adding threat count, half-pin count, contact/tension and a real placement term (formation pairs, potential mobility, centre) needs no rule change and no rebuild of the search.
  - *Con:* hand-tuned eval terms need self-play calibration to avoid new pathologies; your own note in `modified-MCTS-search-rationale.txt` is right that this is the classic deep-search-vs-rich-eval tradeoff.

- **3. Adopt sliding (rook-like) movement with immediate removal — the best-evidenced dynamic rule set (rules, medium effort)**
  The Digital Ludeme Project searched 1006 traditional games for the four rules actually attested for latrunculi and ran 100 alpha-beta self-play playouts per ruleset across every intact Roman board size. The **Kharebga** ruleset — orthogonal *slide*, custodial capture with **immediate removal** — beat the alternatives on duration, completion and branching factor. Their own diagnosis is your symptom verbatim: under step movement "the AI may have difficulty detecting a move that brings it closer to an opposing piece in order to make a capture if they are distant from one another." Seega-rules playouts above 11×12 completed under 13% of the time; 17×17 never completed.
  - *Pro:* quantitatively playtested against the archaeological corpus; consistent with Ovid *Tristia* 2.477 and the *Laus Pisonis* "reserve piece… comes from its distant retreat"; long-range movement makes threats creatable in one ply, which is exactly what a shallow search can see; no Dux.
  - *Con:* Kharebga is documented only in the 20th century, a millennium after Rome, and the authors say so; it contradicts Schädler's reading of Isidore's *inciti* as a real piece state and discards your Freeing rule (Seneca *Ep.* 117.30); on large boards the slide also helps the *losing* side evade, so stay near 8×8.
  - [DLP paper, full PDF](https://cris.maastrichtuniversity.nl/ws/portalfiles/portal/221344302/Giannini-2024-Computational-Approaches-for-Recognising-and.pdf) · [Ludii game page](https://ludii.games/details.php?keyword=Ludus+Latrunculorum) · [Schädler 2001, *Abstract Games* 7](http://history.chess.free.fr/papers/Schadler%202001.pdf)

- **4. If you want the minimal rules diff: switch Seneca → Piso (rules, small effort)**
  Same Locus Ludi document you already have, differing in exactly one mechanism: the flanked counter is **removed immediately** rather than becoming Bound, and multi-captures "are removed altogether" instead of one per turn. Your current rules stack four independent brakes: a capture yields nothing on the turn it is made; removal consumes the next turn's action; only one removal per turn regardless of how many discs are trapped; and the defender gets a free tempo to unmake the capture by pinning a flanker. That is why the expected value of initiating a capture falls below its tempo cost once the board thins.
  - *Pro:* smallest possible change — delete Bound/removal/freeing, keep the placement phase, 8×8, 20 discs, no-suicide and corner rules; published by the same scholarly project as the variant you implemented, so it is drop-in comparable and equally defensible; no Dux.
  - *Con:* keeps the single-step move, so the DLP data predicts the capture-free tail shrinks but does not vanish — the step move is the other half of the problem; loses the *incitus* flavour that is arguably latrunculi's most distinctive attested feature.
  - [Locus Ludi rules PDF (Seneca + Piso)](https://locusludi.ch/wp-content/uploads/2022/08/LUDUS-LATRUNCULORUM_rules_GB.pdf) · [Ludii Piso variant](https://ludii.games/variantDetails.php?keyword=Ludus+Latrunculorum&variant=1589)

- **5. Make the capture payoff convex and stop rewarding the stall (rules, small effort, highest leverage per line changed)**
  Your terminal reward makes stalling *optimal*, provably: a quiet-game win at 10 free vs 9 scores `1000 + 1000·(30/49) = 1612`; annihilation 19 vs 1 scores 1950. A 1.21× ratio, for a win that is essentially certain once you are one disc ahead (`Game.cpp:632-638`). The leader's best policy is literally "make no captures for 40 plies." Three literature-backed fixes, all Dux-free: **(a)** Seega's *capture earns another move* rampage rule, making a breakthrough convex rather than linear; **(b)** a capture-*count* win threshold as in Callen's petteia (win at 7 of 12 captures) so you need not grind to one disc; **(c)** make a `QuietGame` termination worth far less than a Reduction/Immobilization win, or not a win for the material leader at all. Note that a designer independently diagnosed your exact complaint for classical rules — "a game played between two strong players will always end up in a draw… unless one of the players made several serious errors."
  - *Pro:* (c) is a few lines and removes the stalling equilibrium immediately; (a) and (b) are well attested in the same custodial-capture family; a non-material win condition gives the player who lost the opening exchange something to play for.
  - *Con:* Seega and petteia are cousins of latrunculi, not latrunculi itself, so this trades historical fidelity for playability; a capture threshold is an invented number needing tuning; the [Latrunculi XXI](https://www.chessvariants.com/rules/latrunculi-petteia-xxi) design that states the diagnosis is itself Dux-based, so it is cited for the reasoning only, not the rules.
  - [Petteia rulebook, nestorgames](https://nestorgames.com/rulebooks/petteia_en.pdf) · [Seega](https://en.wikipedia.org/wiki/Seega_(game)) · [Mak-yek (intervention capture)](https://en.wikipedia.org/wiki/Mak-yek) · [Forced Capture Hnefatafl](https://arxiv.org/pdf/2301.06127)

---

## If you go the MCTS route rather than negamax

Two defects would need fixing before MCTS is meaningfully better than random: **reward scale** — non-terminal leaf values are ~0.05–0.1 while the UCB1 exploration term at N=1000 is 0.37–3.72, so UCT selection is 10–70× dominated by exploration and is effectively uniform sampling; when a rollout does terminate the reward jumps to ~1600 and exploration vanishes entirely. Normalise to [−1, 1] before backup. And **branching** — `treePolicy` expands every child before any UCT comparison, so with B ≈ 60–250 and ~3,000 affordable iterations the tree is one ply deep; a capture in your rules needs two plies to cash and three to be safe. Progressive widening plus capture-first priors is the fix.
[Power-UCT (IJCAI 2020)](https://www.ijcai.org/proceedings/2020/0332.pdf) · [Scale-independent UCT exploration](https://arxiv.org/pdf/2510.21275) · [Rosin, PUCB](http://www.chrisrosin.com/isaim2010final.pdf)

## How to tell whether any of this worked

Rather than eyeballing self-play, instrument it with the Ludii/DLP game-quality metrics — completion rate, duration, decisiveness, lead change, drama — and compare rule sets numerically, which is exactly how the DLP chose Kharebga: [Browne's thesis (metric definitions)](https://eprints.qut.edu.au/17025/1/Cameron_Browne_Thesis.pdf) · [Foundations of Digital Archæoludology](https://arxiv.org/pdf/1905.13516) · [Ludii system paper](https://ludii.games/publications/ECAI2020.pdf)

**Two caveats on sourcing:** BoardGameGeek blocked automated fetching, so no BGG thread is cited. The Browne et al. MCTS survey is the standard reference for several engine techniques above, but the only mirror the agents found would not fetch, so I have cited primary papers instead. One incidental find worth knowing: *n×n* custodian capture is EXPTIME-hard ([Algorithms 14(3):70](https://www.mdpi.com/1999-4893/14/3/70)) — the game is deep enough; the shallowness is in the engine and the incentives.

Also flagged by the audit, unrelated to boredom but a live bug: a placement position with no legal placement is not terminal and returns an empty move list, and `selfplay.cpp:177-179` silently `break`s and prints "Draw" — a silent default substitution of the kind `CLAUDE.md` forbids.

---

## Appendix: full citation verification table

Status as reported by the verification agents (40 of 44 live and open access).

| Status | URL |
| --- | --- |
| live-and-relevant | https://cris.maastrichtuniversity.nl/ws/portalfiles/portal/221344302/Giannini-2024-Computational-Approaches-for-Recognising-and.pdf |
| live-and-relevant | https://cris.maastrichtuniversity.nl/en/publications/computational-approaches-for-recognising-and-reconstructing-ancient-games |
| live-and-relevant | https://research.dial.uclouvain.be/bitstreams/83b810aa-6e41-47cc-828e-7f6736fb54a8/download |
| live-and-relevant | https://locusludi.ch/wp-content/uploads/2022/08/LUDUS-LATRUNCULORUM_rules_GB.pdf |
| live-and-relevant | http://history.chess.free.fr/papers/Schadler%202001.pdf |
| live-and-relevant | https://nestorgames.com/rulebooks/petteia_en.pdf |
| live-and-relevant | https://www.chessvariants.com/rules/latrunculi-petteia-xxi |
| live-and-relevant | https://www.mastersofgames.com/rules/Ludus-Latrunculorum-Rules.pdf |
| live-and-relevant | https://penelope.uchicago.edu/Thayer/E/Roman/Texts/Laus_Pisonis/text*.html |
| live-and-relevant | http://www.perseus.tufts.edu/hopper/text?doc=Perseus:text:1999.04.0063:id=latrunculi-cn |
| live-and-relevant | https://ludii.games/details.php?keyword=Ludus+Latrunculorum |
| live-and-relevant | https://ludii.games/data.php?gameId=4 |
| live-and-relevant | https://ludii.games/variantDetails.php?keyword=Ludus+Latrunculorum&variant=1589 (Locus Ludi Piso) |
| live-and-relevant | https://ludii.games/variantDetails.php?keyword=Ludus+Latrunculorum&variant=1587 (Schadler 2001) |
| live-and-relevant | https://ludii.games/variantDetails.php?keyword=Ludus+Latrunculorum&variant=1644 (11x12 Seega rules) |
| live-and-relevant | https://ludii.games/variantDetails.php?keyword=Ludus+Latrunculorum&variant=539 (Kowalski 8x12) |
| live-and-relevant | https://ludii.games/variantDetails.php?keyword=Ludus+Latrunculorum&variant=541 (Bell 8x8 — Dux, excluded) |
| live-and-relevant | https://ludii.games/variantDetails.php?keyword=Ludus+Latrunculorum&variant=537 (Falkener) |
| live-and-relevant | https://en.wikipedia.org/wiki/Ludus_latrunculorum |
| live-and-relevant | https://en.wikipedia.org/wiki/Seega_(game) |
| live-and-relevant | https://en.wikipedia.org/wiki/Mak-yek |
| live-and-relevant | https://en.wikipedia.org/wiki/Hasami_shogi |
| live-and-relevant | https://en.wikipedia.org/wiki/Ming_mang_(game) |
| live-and-relevant | https://www.ancientgames.org/latrunculi/ |
| live-and-relevant | https://historicalgames.neocities.org/GreekRome/latrunculi |
| live-and-relevant | https://playcheckers.io/latrones |
| live-and-relevant | https://playculturalgames.com/how-to-play/latrunculi/ |
| live-and-relevant | https://www.mdpi.com/1999-4893/14/3/70 (EXPTIME hardness, custodian capture) |
| live-and-relevant | https://www.jair.org/index.php/jair/article/view/11208 (Baier & Winands) |
| live-and-relevant | https://arxiv.org/pdf/1406.0486 (implicit minimax backups) |
| live-and-relevant | http://www.chrisrosin.com/isaim2010final.pdf (PUCB) |
| live-and-relevant | https://www.ijcai.org/proceedings/2020/0332.pdf (Power-UCT) |
| live-and-relevant | https://arxiv.org/pdf/2510.21275 (scale-independent UCT exploration) |
| live-and-relevant | https://ludii.games/publications/ECAI2020.pdf |
| live-and-relevant | https://arxiv.org/pdf/1905.13516 (Digital Archaeoludology) |
| live-and-relevant | https://eprints.qut.edu.au/17025/1/Cameron_Browne_Thesis.pdf |
| live-but-irrelevant | https://ludii.games/details.php?keyword=Seega |
| live-but-irrelevant | https://arxiv.org/pdf/2301.06127 (Forced Capture Hnefatafl — cited anyway; topic matches) |
| live-but-irrelevant | https://arxiv.org/pdf/2101.11934 (Upper Bound on the Complexity of Tablut) |
| live-but-irrelevant | https://arxiv.org/pdf/2310.20008 (Evolutionary Tabletop Game Design: Risk) |
| live-but-irrelevant | https://arxiv.org/abs/2604.03683 (asymmetric draw rules in chess) |
| could-not-fetch | https://boardgamegeek.com/boardgame/209094/latrunculi-xxi (BGG blocks automated fetch) |
| could-not-fetch | http://www.incompleteideas.net/609%20dropbox/other%20readings%20and%20resources/MCTS-survey.pdf |

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
