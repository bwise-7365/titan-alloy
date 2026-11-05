Copyright Ben Paul Wise. All Rights Reserved.

# Run summaries of Panzergruppe Guderian with scripted agents

Each `.txt` file is one batch from `pgg_play`, one line per game, then totals. Its `run` line names the
set-up and the tactics, under the names `pgg_play` takes today. All games use the base seed 19410703; a line
gives the seed of its own game.

A line gives the points, the level of victory, the turn, the commands, the battles (in the later files), the
refused commands, the game's fingerprint and the time taken. Files made before the seize tactic have no
battle or recapture columns.

`pgg_trc_viewer` opens these files. It plays the chosen game again and compares the fingerprint.

Any game can be played again, because the set-up, the tactics and the seed fix it:

    pgg_play --first 7 --games 1 --setup drawn --german seize-first --soviet seize-first --events

**A game number gives the same game only with the same code.** A later change to a tactic can change what a
number gives, unless the old version stays as a named option, as `seize-first` does. A different fingerprint
says this has happened.

**The target-change figures in the `seizefirst` files with `consolidate` or `fixed` in their names are
overstated.** Those batches came before a correction: the allocation also ran in combat phases, when no one
could move, and overwrote what a unit last aimed for. The games themselves are unaffected, and their
fingerprints still match. `2026-11-04-drawn-seizefirst-seizefirst.txt` was run after the correction.

The tactics and what these results show are in `../2026-11-04-seize-locations-tactic.md`.

Copyright Ben Paul Wise. All Rights Reserved.
