# Ludus latrunculorum, or Latrunculi

The ancient Roman game of "little soldiers" was played throughout the Empire. 

Players take turns placing discs on a board then moving them.

## Objective of the game

The objective of the game is to remove all but one opposing disc or to immobilize the opponent. 

## Game Board

The board is a rectangular grid of squares. The most common size is 8x8, similar to a chessboard except that colors are not used and do not matter. Typical modern boards are beige squares outlined in black. Board sizes vary: 6x6, 6x7, 7x7 , 8x5, 7x10, 9x10 and others have been found. 

For a GUI, the user should be able to select 6-12 rows and 6-12 columns, with 8x8 being the initial, default values. The board color should be selectable from predefined choices, such as beige, burgundy, grey or forest green. 

## Pieces

Each player is assigned a color. For example, one player might get White and the other Black, though the colors do not seem to have been standardized.

Each player starts with the same number of discs (fairly flat counters, pieces, pot shards, stones, etc). One side is blank and the other is marked (e. g. with an "X"). 

For a GUI, the user should be able to select from predefined complementary color-pairs such as black & white, red (RGB 255,0,0) & blue (RGB 0,0,255), or orange (RGB 255,128,0) & (RGB 0, 127, 255), teal (RGB 0, 128, 128) and maroon (RGB 128, 0, 0). Classic color theory says that the complement of (r, g, b) is (255-r,255-g,255-b)

For 8x8, each side should have at least 16 and no more than 24 discs, with 20 being most common. With different board sizes, the number for each side varies proportionally to be between 2/8 and 3/8 of the board's area (rounded to the nearest integer). For a 7x7 board, each side would have between 12 and 18 discs, with 15 being the default.  For an 8x10 board, each side would have between 20 and 30 discs, with 25 being the default. 

For a GUI, the user should be able to select the number for each side as an integer between 2/8 and 3/8 of the board's area (after board dimensions were selected). The initial, default value should be the integer closest to 5/16 of the area. 

## Game play

The game consists of two phases: placement and movement. Pieces can be removed from the board if they stay captured for two or more turns. 

### Placement Phase

To start the game the players take turns in placing one of their discs on a vacant 
square. In this phase, the discs are called “vagi”: vagues. During this phase, no captures are made: “vagi” are never captured. Discs are placed with the blank side up. 

Because no captures can be made during Placement, a player cannot place a disc directly between two enemies (see the Capture Mechanism section below) 

### Movement Phase

Once all the discs have been placed, the discs can be moved one space orthogonally (thus, to any adjacent vacant square), one disc per turn. The discs are placed with the blank side up and are called “ordinarii”: regulars. A disc can leap over a single disc of its own color, provided the square behind is unoccupied. As in checkers, multiple leaps in one turn are possible. As in checkers, multiple leaps in one turn need not all be in the same direction. Multiple leaps over the same disc in one turn are not allowed.

#### Capture Mechanism
The mechanism of capture is enclosure from two opposite sides, perhaps recalling the double envelopment at Cannae. The captured disc is not immediately removed from the board. If a player can trap an opponent disc between two of their own, the disc is captured and cannot be moved. The disc is flipped to show the mark on the back (e. g. an "X" to signify bound hands). Such a stone is called “incitus”: immobile. It cannot move and cannot help capture an opposing disc. A disc in the corner can be trapped by placing two discs on either side of it, 90 degrees from each other. 

A player can move a disc into the space between two oponnents only if it captures at least one of the two opponents by doing so, thus avoiding self-capture.  It is possible to have three opponents arranged around a square, say left, right and top. If moving into the center captures the top opponent, but neither the left nor right opponents, then it would still be self-capture (by the left and right pieces) even though the top was captured.

#### Freeing Captives
If a player moves a disc to trap one or more discs surrounding their own captured discs (flipped to show "X"), those friendly discs are immediately set free (flipped to show the blank side) and can also help to catch an adversary disc. Though unlikely, this could theoretically initiate a chain reaction: B captures one or more W discs, immediately freeing B discs, which immediately capture more W, immediately freeing more B, and so on. 

#### Removing Captives
If there are any opposing discs (“incitus”) still trapped at the start of their turn, the player must select one to remove before moving a friendly disc. The player must, if possible, remove one enemy captive and must always move one friendly disc. If such a move is not possible, they lose immediately. 

Because it is possible to trap more than one opposing disc in one move, but only one can be removed per turn, a player may have to carefully decide in what order to remove captives. This also means that a captive might be freed after several turns of captivity (while the opposing player removed other captives). 

## Super Ko Rule

A board position cannot be repeated in a game.

The comparison to earlier positions is done at the end of a player's turn; it does not apply to the leaps over other pieces. 

It does not matter whose turn it is.

The moves in a games are not constrained in by moves or positions in other games.

## Score

There are two victory conditions: reduce your opponent to just one disc, or create a situation where they are immobilized, I. E.  have no legal moves. The same formula is used to score both cases. 

If the winner has M pieces left, and the loser has N pieces, then the winner's  score is s=(3M) /(3M+2N). 

The loser's score is -s.

### Victory by Removal

This occurs at the instant a disc is removed and one side is reduced to a single disc. 

Because M must be at least 2, and N=1 by definition, the lowest winning score is 6/8 or 0. 75. If the winner had 4 pieces left, his score would be 12/14 or about 0.857. Thus, a wider margin of victory is preferred. 

### Victory by Immobilization 

This occurs at the start of a player's turn, when no legal moves can be found.

If the winner has M pieces left, and the loser (with no legal moves) has N pieces, then the winner's  score is the same formula: s=(3M) /(3M+2N). Thus, if the winner has 8 pieces and the loser has 3 immobilized pieces, the winning score is 24/(24+6) which is  exactly 0.800

### Draws

If neither victory condition is achieved by either side after zMN moves on an MxN board, the game is a draw. We suggest z be equal to 2 or 3. Note that placements in the Placement Phase are not counted; only moves in the Movement Phase count toward a draw. 

In a draw, both sides score zero. 



