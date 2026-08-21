# modcurses — system overview

*How the pieces fit together, and why they are shaped the way they are. For the
original specification see `TUI_DESIGN.md`; for platform detail, `BUILD_NOTES.md`.*

---

## Executive overview

modcurses is a compact C++20 library for building classic text-mode
applications over two curses implementations — ncursesw on POSIX and PDCurses
on Windows — from a single source tree. It is roughly six thousand lines,
small enough that the whole architecture fits in one reading, and it is
deliberately unambitious about what a terminal UI should be: no animation
framework, no styling DSL, no theming engine. What it provides is a widget
tree, a layout system of three containers, one event loop, and a rendering
path with exactly one way to reach the screen.

The architecture is best understood as a response to a specific failure. The
design was written after porting an older toolkit, CPPurses, to Windows, and
each of its rules is traceable to a defect found during that port: curses
initialised lazily from a background thread; a colour palette applied before
`initscr()` and therefore silently never applied at all; a curses error code of
`-1` stored into an unsigned size and laid out as eighteen quintillion cells; a
macro probe that selected a broken drawing path and rendered an entire UI
black-on-black. The response was not to fix those bugs but to make each class
of them unrepresentable.

Four decisions carry most of that weight. **The curses firewall**: no curses
header is reachable from any public header, and exactly one translation unit
includes curses, so every backend surprise is confined to one file. **No static
state and no lazy initialisation**: the terminal is an RAII object constructed
explicitly, on the main thread, before any widget exists. **Signed geometry
everywhere**, with backend returns range-checked at the boundary. And **full
repaint with a diff**: every frame paints the entire widget tree into a back
buffer, compares it against the front buffer, and writes only the cells that
actually changed — which makes an entire category of partial-repaint bugs
impossible to write.

The library is single-threaded by contract. There is no way to post an event,
so there are no queues that can route a message wrongly; timers are part of the
loop's wait rather than events with a delivery question attached. A handler
mutates state and calls `invalidate()`, and the effect appears at the top of
the next loop iteration.

The last structural decision is testability. `TerminalIO` is an interface with
two implementations: the real curses backend, and a mock holding an in-memory
grid and a scripted event queue. Layout, focus, key routing, rendering and the
frame diff are therefore all exercisable with no terminal attached — 275 test
cases run headless in well under a second. The parts that genuinely cannot be
tested that way, such as how PDCurses encodes an Alt-chord, were instead
verified by attaching to a live console and reading its screen buffer back.

*(≈470 words)*

---

## TerminalIO, and the two backends behind it

`TerminalIO` is the narrowest interface the rest of the library could be built
on: report the screen size, wait for an event with an optional timeout, draw a
run of same-styled glyphs at a position, move or hide the cursor, flush,
redefine a colour, beep. Nine functions. Everything above it is written against
those and nothing else.

That narrowness is the firewall. `CursesTerminal` is the only translation unit
in the project that includes a curses header, and it does so through a private
shim that first defines `PDC_WIDE`, `PDC_FORCE_UTF8` and
`NCURSES_MOUSE_VERSION` — each of which was diagnosed the hard way. The
consequence is that when the backends disagree, and they disagree constantly,
the disagreement is absorbed in one place: PDCurses delivering Alt-chords as
its own `ALT_A` key codes rather than as an ESC prefix; PDCurses reporting
`x = y = -1` for wheel events where ncurses gives real coordinates; the four
control codes above Ctrl-Z that both send but neither documents; and the fact
that `COLOR_RED` is 1 in ncurses and 4 in PDCurses. Each of those is a few
lines inside the backend, and invisible everywhere else.

The constructor's ordering is contractual rather than incidental, and every
line of it is a past bug: sanitise the `LINES`/`COLS` environment variables
before `initscr()`, because both backends trust them over the real console size
and an IDE that exports `LINES=1` will abort startup; set the locale; call
`initscr()` and *check the result*; configure input mode; enable the mouse;
start colour; and only then ask how big the screen is.

`MockTerminal` implements the same interface with a `std::vector<Cell>` and a
`std::deque<Event>`. It is not a testing afterthought bolted on later — it is
the reason the widget layer can be tested at all, and it was written in the
first milestone alongside the real backend.

## ScreenBuffer, and the frame algorithm

`ScreenBuffer` holds two grids of cells, front and back, and implements the
single most consequential decision in the library: there are no partial
repaints.

Each frame clears the back buffer, walks the entire widget tree painting into
it, then diffs. The diff walks each row looking for runs of cells where back
differs from front, splits those runs at style boundaries, and issues one
`draw_run` per run — copying back to front as it goes. Everything else,
including the count of runs written, falls out of that.

The reasoning is worth stating plainly, because "repaint everything" sounds
wasteful. At terminal scale a full repaint is a few tens of thousands of cell
comparisons, which is microseconds; the genuinely slow part is writing bytes to
the terminal, and the diff already minimises that. In exchange, the entire
category of bugs where a widget forgets to invalidate the right region — the
largest single source of complexity in the toolkit this replaced — cannot be
written. The unit tests assert on the *number of runs* the diff emits, which is
what turns "the diff is minimal" from a claim into a measurement.

## Canvas

`Canvas` is the only way a widget draws. It is a thin view over the back
buffer, constructed by the renderer and handed to `Widget::paint`; a widget
cannot obtain one any other way.

It carries the widget's absolute rectangle and a clip rectangle, and every
write is translated from widget-local coordinates and clipped to that
intersection. A widget therefore cannot paint outside itself even by accident,
and cannot paint over a sibling — not by convention, but because the coordinate
it passed simply does not exist from the canvas's point of view. Out-of-bounds
writes are dropped rather than clamped, since clamping would silently move a
glyph somewhere the caller did not ask for.

## Style, colour, and the pair cache

Curses does not have colours so much as *colour pairs*: a foreground and
background combination allocated by number, of which PDCurses offers 256. A
naive mapping of seventeen colours to pairs needs 289 and does not fit.

`Style` is therefore just data — a foreground, a background, and a bitmask of
traits — with no notion of a pair. The backend keeps a lazy `(fg, bg) → pair
id` cache and allocates on first use. Real applications use a dozen
combinations, so the ceiling is never reached in practice; the toolkit this
replaced eagerly ground out all 256 and had no headroom left.

Two hard-won details live here. Colours are always applied through
`COLOR_PAIR(n)` or through `setcchar`'s dedicated pair argument — never by
OR-ing a raw pair number, which lands in the character bits of the `chtype` and
renders the whole UI black-on-black. And the curses colour *number* is obtained
from the `COLOR_*` macros rather than computed from the enum's own ordering,
because ncurses and PDCurses number them differently; deriving the index
arithmetically produced a build where every blue rendered red on Windows while
looking perfect on Linux.

`Palette` can redefine what RGB a colour slot holds, strictly after
`initscr()`. It is genuinely useful and genuinely unreliable — many terminals
accept the call and ignore the result, and there is no way to ask, since
PDCurses' `can_change_color()` answers "yes" on any Windows system regardless.
The lesson, learned by shipping it the wrong way round first, is that a visible
distinction must come from *which slot* a cell uses, never from what RGB sits
behind it.

## Signal

A ~150-line single-threaded signal, used for every "this happened" notification
in the library: a button pressed, a buffer changed, a list scrolled.

Two properties matter. Connections may outlive the signal — they go dead rather
than dangle, via a shared control block whose owner pointer is nulled by the
signal's destructor. And connecting or disconnecting from *inside* an emit is
safe: the emit snapshots the slot count so newly-connected slots are not called
by the emit that created them, disconnected slots are tombstoned rather than
erased, and compaction happens only when the outermost emit returns.

The slots are held indirectly, by `unique_ptr`, and that indirection is load
bearing. With a flat vector, a slot calling `connect()` during an emit would
reallocate the vector and destroy the very `std::function` whose `operator()`
was still running further up the stack. That was a real crash, caught by a test
written specifically to provoke it.

## Widget

The base class, and the tree. A parent owns its children by `unique_ptr`, and
`emplace_child` is the only way to construct a non-root widget, so ownership is
never ambiguous.

Geometry is assigned *to* a widget by the layout pass and never chosen by the
widget itself; `on_geometry` is a notification, not a request. Painting and
event handling are protected virtuals with sane defaults, each shadowed by an
optional `std::function` hook that is consulted first — the hook exists so that
simple cases need no subclass, and so that an application can observe a key
before the widget acts on it, which is how the editor example snapshots undo
state before a keystroke mutates the buffer.

The only deferred mechanism in the entire library is `destroy_later()`, which
exists so a handler can safely delete the widget it is currently running
inside. The loop reaps at a point where nothing is mid-dispatch, and repairs
focus afterwards.

Two additions came from building real applications on it. Per-instance
`width_hint`/`height_hint` override a widget's own size request without
subclassing — needed constantly, because a plain `Widget` used as a spacer
expands on *both* axes, so a row of one-line controls otherwise claims
unbounded height. And the default `layout_children` gives every child the
parent's rectangle, because a child of a non-container widget that receives no
rectangle at all is invisible to painting, hit-testing and focus alike, which
is never what adding it meant.

## SizeReq and the three containers

The whole layout system is one struct and three containers, replacing the seven
size policies of its predecessor.

`SizeReq` is a minimum, a preferred size, a maximum, and a weight. `distribute`
hands out space along one axis: everyone gets their preferred size clamped to
their range; leftover space goes out proportionally to weight, skipping anyone
at their maximum; an overdraft is taken back the same way, skipping anyone at
their minimum; and if the minimums still do not fit, trailing children are
dropped to zero rather than squeezed below what they said they needed. A child
given zero width or height keeps all of its state and simply has no rectangle
this pass.

`VBox` and `HBox` are that function applied along one axis, with each child
clamped to its own range on the other. `Stack` shows one child at a time and is
the page-navigation primitive.

The subtle part is how a container combines its children's requirements across
the axis it does *not* stack on. The maximum must be the **largest** child
maximum, not the smallest: layout already clamps each child individually, so
taking the smallest just starves every other child — a single fixed-width
checkbox once held an entire page to fourteen columns that way.

## FocusChain

Focus is a single pointer owned by the loop, with tab order defined as tree
pre-order over widgets that are visible, actually laid out, and willing to take
keyboard focus.

Two behaviours are worth knowing. Focus traversal is consulted *after* the
focused widget and its ancestors have declined a key, so a text area can claim
Tab for indentation — the ordering exists precisely to make that possible.
And focus is released automatically when the widget holding it stops being
reachable, which is checked after every layout pass rather than only when
widgets are destroyed. Hiding a `Stack` page leaves its widgets with empty
rectangles, and focus stranded on one of them means an invisible widget goes on
consuming the keyboard — which is exactly how a help dialog once froze an
application with no way to dismiss it or even quit.

## EventLoop

One loop, and its body is short enough to state completely: run layout to a
fixed point, paint if anything is dirty, position the cursor, wait for input or
the next timer deadline, dispatch, fire due timers, reap anything scheduled for
destruction.

Painting happens *before* the wait, so the opening frame is on screen before
the loop ever blocks. Layout runs to a fixed point rather than once, because a
pass may legitimately request another — a wrapping label only learns its width
when layout assigns it, and only then can it report an honest height. Painting
between those two passes would leave stale geometry on screen until the next
keypress, because the loop blocks on input immediately afterwards. The
iteration count is bounded, so a misbehaving widget degrades to a wrong size
rather than hanging.

Dispatch is layered: application-level shortcuts first, so a global quit key
works regardless of focus; then the focused widget's hook, then its virtual,
then the same pair on each ancestor up to the root; then focus traversal. Mouse
events are hit-tested from the root down to the deepest visible widget under
the point and delivered with coordinates translated to each widget's local
space on the way back up.

Timers are a min-heap keyed by deadline, and a repeating timer re-arms from its
*scheduled* deadline rather than from now, skipping missed ticks. That is what
keeps a game's gravity from drifting.

## App

`App` composes the whole thing, and its only real job is to enforce an order.

The argument-parsing constructor parses *first* and constructs the terminal
only if the command line has not already decided the program should stop. This
is the difference between `--help` printing to an ordinary stdout and `--help`
painting into a curses screen that is immediately torn down — the second is
what happens if the terminal is created first, and it is verifiable: run the
editor under a pipe and it dies where curses cannot start, while `--help`
prints cleanly and exits zero.

`App` has no static members. Accessors throw with an explanatory message rather
than returning something unusable when there is no terminal, so a caller who
forgets to check `should_exit()` gets a diagnostic instead of a crash.

## ArgParser

A declarative, typed, GNU-style parser, deliberately part of the library rather
than left to the application, because the ordering guarantee above only works
if `App` owns the parse.

Arguments are declared as typed handles; the parser holds them type-erased and
hands back a reference whose `value()` has the right type. The full grammar —
`--long`, `--long=value`, `--long value`, `-s`, `-svalue`, `-s value`, bundled
boolean shorts, `--` — is supported, `--help` and `--version` are generated
from the declarations, and errors name the canonical option regardless of which
spelling the user typed. Parsing is locale-independent, which matters more than
it sounds: it runs before the terminal calls `setlocale`, so a decimal point is
still a decimal point.

## TextBuffer, Keymap and TextArea

The clearest model/view split in the library, and the template for how larger
widgets should be built.

`TextBuffer` is a model and nothing else: lines, a cursor, editing and movement
operations, a dirty flag, and file I/O. It knows nothing about screens, so an
editor's behaviour can be tested without one. Its file handling opens streams
in binary and handles line endings explicitly, preserving whether a file was
LF or CRLF and whether it ended with a newline — because silently rewriting
every line of a colleague's file is not an acceptable side effect of opening
it.

`TextArea` is a scrolling view onto a buffer, and several views onto one buffer
are legal. It owns the viewport, tab expansion and the cursor's screen
position. Column arithmetic is funnelled through two functions so that
tab expansion — and, later, double-width glyphs — stay in one place.

`Keymap` answers a question the design left open, in the affirmative: editing
keys are a swappable table of `(KeyEvent, EditAction)` pairs from day one,
because retrofitting that later is the painful direction. Three maps ship, and
the nano-like editor selects one at startup.

One rule here was got wrong first and is worth stating: a read-only view
*declines* the keys it cannot act on rather than consuming them. Consuming them
seems defensive and is not — a read-only text area used as a help pager
swallowed every key, so once it had focus there was no way to dismiss it or
quit. A widget that cannot act on a key has not handled it.

## The widget catalogue

The rest — `Label`, `Button`, `Checkbox`, `TextInput`, `Titlebar`,
`StatusBar`, `Divider`, `ScrollBar`, `ListView`, `Menu`, `GridCanvas` — are
small classes, most under a hundred lines, and they are best read as evidence
that the core is adequate rather than as an interesting layer in their own
right.

Two are worth singling out. `ScrollBar` deliberately holds no opinion about
what it scrolls: it is paired with a `TextArea` or `ListView` through their
`scrolled` signal in one direction and its own `position_changed` in the other,
which keeps both sides ignorant of each other. And `GridCanvas` maps a fixed
logical grid onto the screen at a configurable number of columns per cell,
which is what lets a Tetris board or a chessboard read as square rather than as
tall thin slots — the intended ceiling of "graphics" in a library that
deliberately has no graphics.
