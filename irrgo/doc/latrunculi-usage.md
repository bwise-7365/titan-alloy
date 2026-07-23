<!-- Copyright Ben Paul Wise. All Rights Reserved. -->

# Using the Latrunculi program

This page describes the on-screen conventions of the *Latrunculi* 
program itself: what the
mouse does, what the colored rings and dots on the board mean, and what each
menu is for. It assumes you already know the rules of the game (see the
**Rules** page under the About menu); if you don't, read that first.

## The window

- The **board** sits on the left. Discs are drawn in the two side colors you
  chose (or the defaults); a bound (captured but not yet removed) disc is
  marked with an **X** in its captor's color.
- The **right panel** shows, top to bottom: the status line ("A to place",
  "B to move", "Thinking...", or the game-over announcement) with a **Stop**
  button that appears while the computer is searching; the two side tallies,
  each led by a color swatch and showing total / free / bound disc counts; a
  **Suggested** box with a **Clear** button; a playback transport bar; and the
  **move log**.

## Making a move with the mouse

**Placement phase.** Click any empty square. If the square is legal (it won't
complete a capture), the disc is placed immediately — one click per disc, no
second step.

**Movement phase.** If you have no enemy prisoners on the board, this is a
plain two-click move:

1. Click one of your own free discs to select it as the origin.
2. Click one of its highlighted legal destinations to complete the move.

If you **do** have enemy prisoners waiting (mandatory removal), it becomes a
three-click sequence instead:

1. Click the enemy prisoner you want to take off the board first (if there is
   more than one, pick whichever you like).
2. Click one of your own free discs as the origin.
3. Click a highlighted destination to complete the move.

At any point, clicking somewhere that isn't a valid next step (an empty
square, an opponent's free disc, etc.) clears the in-progress selection so you
can start over — except a chosen prisoner, which stays chosen until the move
completes, since that choice is mandatory and already locked in.

## Reading the board highlights

The board uses color consistently to mean the same thing everywhere:

- **Green ring** — the engine's suggested move (see Suggest, below): one ring
  on the origin and one on the destination, or a single ring on the target
  square during placement.
- **Red ring (thick)** — capture/removal cues. Every enemy prisoner is ringed
  in red whenever a removal is owed but not yet chosen; once you pick one, only
  that one stays ringed.
- **Blue ring (thick)** — your selected origin disc, once chosen.
- **Blue dot (translucent)** — a legal destination for the currently selected
  origin.
- **Small dot on a disc** — marks the disc that moved or was placed last (the
  dot color contrasts with the disc so it stays visible on light or dark
  pieces).
- **Translucent ghost disc under the cursor** — a hover preview, shown only
  over a square your next click would actually act on (a legal placement
  square, or a highlighted destination once an origin is selected).

None of these are clickable in themselves — they're read-outs. You always act
by clicking the underlying square.

## Suggested moves

The **Suggest** menu runs a search (NegaMax or MCTS, with a time budget you
choose) on the current position **without playing anything**. When it
finishes, the suggested move appears both as text in the "Suggested:" box and
as a green ring on the board. It stays there — even across further clicks on
your own board — until you make a move, ask for a new suggestion, or press
**Clear**, at which point it is dropped.

## Playing against the computer, or letting it play itself

The **Play** menu switches out of manual (both-sides-by-mouse) mode:

- **NegaMax** / **MCTS** — the engine plays *both* sides for a chosen number
  of turns at a chosen time budget per move ("auto-play"); useful for watching
  the engine play itself or for fast-forwarding an opening.
- **Computer** — you take one side (A or B) and the engine answers every move
  you make on the other side, at its own time budget. If the computer holds
  the opening move, it moves as soon as you press Go!.
- **Manual** — hands both sides back to the mouse and stops anything running.

While the engine is thinking, the board and menu bar are disabled and the
status line reads "Thinking..."; press **Stop** to cancel the search and fall
back to manual play at the current position.

## Move log and playback

Every completed move appears as a row in the move log, numbered by ply, with
placements marked `[random]` if the opening policy chose that one instead of
searching it. Click any row, or use the transport bar underneath it (First /
Prev / Play / Next / Last, the slider, or the Left/Right/Home/End keys) to
rewind the board to the position right after that move — this is read-only
review and does not affect the game record.

If you then make a new move from a rewound position, the moves after that
point are discarded and play continues from there — the same way editing a
branch works in a text editor's undo history. This is the only way to "take
back" a move: rewind to just before it, then move again.

## Board setup, generating new boards, and seeding an opening

- **Board** menu: set rows, columns, discs per side, the movement rule
  (Kharebga slide or Seneca step-and-leap; see the Rules page), and the komi
  credited to side B, plus the three color pickers. **Generate** starts a
  fresh game with these settings.
- **Setup** menu: after generating a board, seed the placement phase with a
  chosen percentage (0-100%) of each side's discs, dropped automatically at
  random legal squares, one at a time, as a short animation. Any percentage
  less than 100% leaves the rest of the placement phase for you (or the
  engine) to finish normally.

## Saving and loading

**File > Save As...** writes the current game (board settings, full move
history, and colors) to a file; **File > Load...** reads one back and replays
it to its final position, ready to be reviewed with the playback bar or
continued with more moves.

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
