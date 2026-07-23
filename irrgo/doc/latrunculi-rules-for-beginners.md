<!-- Copyright Ben Paul Wise. All Rights Reserved. -->

# Ludus Latrunculorum — a gentle introduction

*Ludus Latrunculorum* ("the game of little soldiers") is an ancient Roman board
game of surrounding and trapping the enemy. Two players each command an army of
identical round discs — call them Black and White — and try to grind the other
army down until it can no longer fight. There are no kings, no special pieces,
and no dice: every disc is equal, and everything is decided by position.
Though it was widely played throughout the Roman Empire, no complete set of rules
has yet been found. Several variant rule sets have been reconstructed;
the ones below were selected to produce lively and interesting play.


This page describes the game exactly as the *Latrunculi* program plays it. If you have never
seen the game before, read straight through; each idea builds on the one before.

## The board and the armies

- The board is a plain rectangular grid of squares. The standard size here is
  **8 rows by 10 columns**, and each player starts with **20 discs**. (The board
  can be set anywhere from 6 to 12 squares on a side, with a matching number of
  discs, but the defaults are a fine place to learn.)
- The board starts **empty**. Unlike Chess, you do not begin with your army already lined up.
  You place it yourself, however you want, which is the first half of the game.

A game has two phases, played in order: the **Placement Phase**, then the
**Movement Phase**. Players always alternate turns, and Black moves first.

## Phase 1 — Placement

Taking turns, each player drops one of their discs onto any **empty** square,
until both armies (all 40 discs, by default) are on the board.

There is only one restriction, but it matters:

> **You may not place a disc in a way that captures anything.**

Because capturing is done by sandwiching (explained below), this means you can't
set a disc down in the gap between two enemy discs — that would be walking into a
capture — and you can't drop a disc so that it finishes surrounding an enemy
either. Placement is a careful, strategic jockeying for position; no disc is ever
taken during it. When the last disc has been placed, the fighting begins.

## Phase 2 — Movement

Now players take turns **moving** one of their discs. In the standard rules the
program uses, a disc moves like a **rook in chess**:

- It slides any number of empty squares in a straight line — up, down, left, or
  right (never diagonally).
- It must stop **before** the first disc in its path, whether friend or foe. You
  cannot jump over pieces, and you cannot land on an occupied square.

Moving onto a square never captures by landing on or bumping an enemy. Capturing
works only one way, described next.

> **A note on variants.** The program also offers an older "step-and-leap" style,
> where a disc instead takes a single step to an adjacent empty square, or hops in
> a chain over its **own** discs (like checkers, but jumping friends, not foes,
> and taking nothing). The rook-slide is the default and the livelier game; the
> capturing rules below are identical either way.

## Capturing — the heart of the game

You capture an enemy disc by **flanking** it: getting your own discs on **both
opposite sides** of it, in a straight orthogonal line.

- Two of your discs directly **left-and-right** of an enemy disc, or directly
  **above-and-below** it, trap that disc.
- A disc in a **corner** of the board is trapped when your discs hold the two
  squares next to it: one horizontally adjacent and one vertically adjacent.

The trap only springs as the **result of a move you just made** — you move a disc
into place so that it completes the sandwich. (You do not get captured merely for
sitting between two enemies who were already there; that is why placement forbids
walking into such a gap in the first place.)

Here is the twist that makes this game its own thing:

> **A captured disc is not removed from the board. It is turned over — "bound"
> (immobilized), and marked with an X.**

A bound disc is a prisoner. It **cannot move**, and it **cannot help make a
capture** — an already-bound disc is dead weight and does not count as one side of
a future sandwich. It just sits there, marked, belonging still to its owner but
useless to them.

### Taking prisoners off the board

A bound enemy disc is removed on a **later** turn — and removing it is
**mandatory**:

> **If you have any enemy prisoners on the board, then at the start of your turn
> you must first remove exactly one of them, and only then make your move.**

So capturing is really a two-beat rhythm. First you flank an enemy disc and it
flips to a prisoner. Then, on one of your following turns, you must take it
prisoner — pluck one bound enemy disc off the board — before you move. This is
how an army actually shrinks. If you capture more than one enemy disc with your move, you have to take them all off, one by one over the next few moves. Be careful, because sometimes the order matters.

Two more rules keep things sane:

- **No suicide.** You may not make a move that leaves one of your **own** discs
  flanked by the enemy. You can't hand the opponent a capture by walking into it.
- **No repeating a position.** A move that would recreate a board arrangement that
  has already occurred earlier in the game is forbidden. (This stops the game from
  looping forever over the same shuffle. Whose turn it is doesn't matter — it's
  the arrangement of discs that may not repeat.)

## How a game ends

A game can end in three ways. The program checks for them in this order:

1. **Reduction** — you win the moment your opponent is ground down to a **single
   disc**. An army of one can accomplish nothing, so the game is over.
2. **Immobilization** — if the player whose turn it is has **no legal move at
   all** (every disc blocked, nothing to place, no prisoner they're allowed to
   take), that player **loses**. Being completely stuck is a defeat, not a pass.
3. **The pacific (quiet game) rule** — if a long stretch of play (40 turns in a row) goes by
   with **no capture and no prisoner removed**, the game is declared over to
   prevent an endless stalemate. Whoever has more discs standing wins.

There are **no draws**. To guarantee that even a dead-even quiet game produces a
winner, the second player (who moved second and is at a slight disadvantage) is
credited with a small fractional bonus — one and a half discs — when the
final discs are counted. Because that bonus is a fraction, the two counts can
never come out exactly equal.

## The shape of a game, in one breath

You spend the opening carefully **placing** your army without giving anything
away. Then you **maneuver**, sliding discs to line up flanking traps. Each
successful flank turns an enemy disc into an immobile **prisoner**; on your
following turns you **take prisoners in**, one per turn, whittling the enemy army
down. You win by curring the enemy to a lone disc, by trapping them so they cannot
move, or — if the fighting fizzles out — by simply having more soldiers left
standing.

## The step-and-leap variant

Everything above describes the default game, where a disc moves like a rook. The
program also offers an older movement style, called **step-and-leap**. It changes
**only how a disc travels across the board** — placement, flanking, prisoners,
the mandatory removal, the no-suicide and no-repeat rules, and all three ways to
win are exactly the same. If you have understood the game so far, you already know
almost all of this variant too.

The one difference is that a disc no longer slides across open ranks and files.
On your turn you pick one disc and do **one** of these two things:

- **Step** — move it one square to an orthogonally adjacent **empty** square (up,
  down, left, or right). Just one square; no gliding across the board.
- **Leap** — jump it over one of your **own** discs sitting in the next square,
  landing on the empty square immediately beyond, in a straight line. You may only
  hop over a *friendly* disc, never an enemy, and the landing square must be empty.

A leap can **chain**: after landing, if another of your own discs sits just beyond
in some orthogonal direction with an empty square past it, you may leap again, and
again, all as a single move. You're allowed to change direction between hops, but
you may not leap over the same disc twice in one move. This is a little like the
multi-jumps of checkers — except you hop over your **own** discs, not the enemy's,
and, crucially:

> **A leap captures nothing.** It is pure travel. No disc is ever taken by being
> jumped, whether friend or foe.

Capturing in this variant works the same as always — by flanking, resolved after
your disc finishes its step or its leap-chain and comes to rest.

**Why the default is the rook-slide.** Notice how short-range step-and-leap
movement is: a lone disc with no friends beside it can only shuffle one square at a
time, so it takes many turns to bring a distant disc to bear on the enemy. The
armies tend to sit apart and the game drifts. The rook-slide lets a disc cross the
whole board in one move to spring a trap, which makes for a sharper, livelier
game — so it is the default here, with step-and-leap kept as the traditional
alternative.

---

## Appendix: can you free a prisoner by capturing its captors?

A natural question, and one worth answering precisely, because several historical
reconstructions of this game *do* allow it: if your disc has been bound because
two enemy discs flank it, and you then capture (or remove) one of those two
flankers, is your disc set free again?

**In this implementation, no. A capture is permanent. There is no way to free a
bound disc.**

This was checked directly against the code. When a disc is flanked it is flipped
to the "bound" state and **stored** that way. Nothing in the program ever looks at
a bound disc again to ask whether it is *still* surrounded — the only two things
that can ever happen to a bound disc are:

- it stays bound (forever, if left alone), or
- it is **removed** from the board by the player who trapped it.

Once bound, a disc can never turn back into a free, moving disc — not by the
flankers moving away, and not by the flankers themselves being captured. Removing
or scattering the captors changes nothing for the prisoner; it is already counted
as good as lost. (The classical "freeing chains" rule, where springing the captors
would release the captive, is a known variant, but it is deliberately **not**
built here.)

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
