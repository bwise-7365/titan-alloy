# Sash — Design for a Modern, Compact C++ TUI Library

*Architecture document, v1.0 — written to be handed to an implementing agent as the
authoritative specification. The working name "Sash" (a window-frame part) and the
`sash` namespace are placeholders; rename freely.*

---

## 1. Goals & Non-Goals

### Goals
- A **compact** (~5–7k LOC core) C++20 library for building classic text-mode
  applications on top of **ncursesw** (POSIX) and **PDCurses** (Windows/MSVC).
- Capability ceiling, by example: the CPPurses demos (notepad, chess, glyph
  paint), **nano**, **lynx**, and the ncurses **Tetris**. If those four are
  expressible and pleasant to write, the library is done.
- **Deterministic, boring runtime behavior**: single-threaded, RAII-initialized,
  no static state, headless-testable.
- A **uniform command-line argument story** built into the application bootstrap.
- Zero runtime dependencies beyond the curses library itself.

### Non-Goals
- No animation framework, easing curves, or FTXUI-style graphics. Animation
  support is exactly one primitive: a repeating timer that fires in the event
  loop (sufficient for Tetris gravity and a blinking cursor).
- No modal dialog toolkit, theming engine, or styling DSL in v1.
- No CJK double-width glyph handling in v1 (see §13 Risks).
- No support for curses implementations other than ncursesw and PDCurses.

---

## 2. Lessons from CPPurses (why this design)

This design is a direct response to a real debugging campaign porting CPPurses
(2018, C++14) to Windows/MSVC over PDCurses. Every principle below is traceable
to a specific defect found there. The implementing agent should treat these as
**hard constraints**, not suggestions.

| # | CPPurses defect (observed, not hypothetical) | Design response |
|---|---|---|
| 1 | Curses was initialized lazily by whichever thread first touched a function-local static paint engine; with background loops running, `initscr()` ran on a nondeterministic background thread. | `Terminal` is an RAII object constructed explicitly by `App`, on the main thread, before any widget exists. No lazy init anywhere. |
| 2 | A static `Terminal_properties` object queried `getmaxx(nullptr)` (→ −1) at static-init time and assigned it into `std::size_t`, wrapping to `SIZE_MAX`; the widget tree was then laid out to that size, crashing in a mask allocator. | Zero static state. All geometry uses **signed `int`**. Backend return values are checked before use. |
| 3 | The palette (`init_color`) and `cbreak()` were called before `initscr()` at static-init time; curses silently returned `ERR`, so **the color palette was never applied on any platform** and nobody noticed for years. | Initialization is one explicit, ordered sequence inside `Terminal`'s constructor. Nothing configurable exists before it runs. |
| 4 | Widget constructors spawned background `Event_loop` threads (animation timers, a chess game loop). One of them blocked forever holding posted corrective events; events posted from the "wrong" thread landed in queues that never drained. | **Strictly single-threaded.** One loop. Timers are part of the loop's wait. There is no `post_event`; there are no queues to route wrongly. |
| 5 | Public headers included `<ncurses.h>`, leaking macros (`border`, `getch`, …) that required `#undef` hacks; a `#if defined(add_wchstr)` macro-probe silently selected a broken fallback on PDCurses (where `add_wchstr` is a function, not a macro), and that fallback OR'd a raw color-pair number into the chtype's *character bits* — rendering the entire UI black-on-black. | **Curses firewall**: no curses header is reachable from any public header. Exactly one implementation file includes curses, via a shim that sets `PDC_WIDE`, `PDC_FORCE_UTF8`, and `NCURSES_MOUSE_VERSION 2` first. The public API speaks only library types. |
| 6 | A per-widget partial-repaint engine (`staged_changes` / `screen_state` / `paint_middleman`) with just-enabled/moved/resized/child-event special cases was the largest source of complexity and bugs. | **Always repaint the full tree into a back buffer; diff against the front buffer; write only changed cells.** At TUI scale (≤ ~300×100 cells) this is microseconds. All partial-repaint cleverness is deleted by design. |
| 7 | MSVC parsed UTF-8 source literals in the system codepage, corrupting every box-drawing `L'─'` literal (warning C4066). | `/utf-8` is mandatory on MSVC and set by the build. Glyphs are `char32_t`. |
| 8 | Terminal-size env vars (`LINES=1` from an IDE console) were trusted by curses over the real console size, aborting `initscr()`. | The backend clears `LINES`/`COLS` env vars with values < 2 before `initscr()`. |
| 9 | Resize handling assumed SIGWINCH; PDCurses needed an explicit `resize_term(0, 0)` on `KEY_RESIZE` that was missing. | Resize is driven purely by `KEY_RESIZE` (both backends deliver it); the PDCurses branch calls `resize_term(0, 0)` inside the backend. No signal handlers. |

---

## 3. Architecture Overview

```
┌──────────────────────────────────────────────────────────┐
│ App                                                      │
│   ArgParser (runs BEFORE curses init) → Terminal (RAII)  │
│   → root Widget → EventLoop::run()                       │
├──────────────────────────────────────────────────────────┤
│ Widget catalog                                           │
│   Label Button Checkbox TextInput TextArea ListView      │
│   Menu StatusBar Titlebar ScrollBar GridCanvas …         │
├──────────────────────────────────────────────────────────┤
│ Widget core                                              │
│   Widget base · tree ownership · layout (SizeReq,        │
│   VBox/HBox/Stack) · focus · Signal/Connection           │
├───────────────────────────┬──────────────────────────────┤
│ EventLoop                 │ Renderer                     │
│   poll → dispatch →       │   paint tree → back buffer   │
│   timers → composite      │   diff vs front → flush      │
├───────────────────────────┴──────────────────────────────┤
│ TerminalIO (abstract interface)                          │
│   CursesTerminal (the ONLY curses-including TU)          │
│   MockTerminal (headless grid, for tests)                │
├──────────────────────────────────────────────────────────┤
│ ncursesw (POSIX)              PDCurses via vcpkg (Win)   │
└──────────────────────────────────────────────────────────┘
```

Data flows one way per frame: input event → widget handlers mutate state and
call `invalidate()` → loop repaints tree → diff → terminal writes. There is no
other path to the screen.

### Directory layout

```
sash/
├── CMakeLists.txt          CMakePresets.json   vcpkg.json
├── include/sash/           # public headers — NEVER include curses here
│   ├── core.hpp            # Geometry, Signal, Event, Key, Timer handle
│   ├── render.hpp          # Color, Style, Glyph, Canvas, Palette
│   ├── widget.hpp          # Widget, layouts, Focus
│   ├── widgets.hpp         # the catalog (or one header per widget)
│   ├── app.hpp             # App, EventLoop
│   └── args.hpp            # ArgParser
├── src/
│   ├── backend/curses_shim.hpp    # PRIVATE: the only file naming <curses.h>
│   ├── backend/curses_terminal.cpp
│   ├── backend/mock_terminal.cpp  # (or under tests/)
│   └── … one .cpp per module
├── examples/   hello/  tetris/  notepad/  browser/
└── tests/      vendored doctest.h + unit tests
```

---

## 4. Core Types (`core.hpp`)

### Geometry — signed, aggregate, constexpr

```cpp
namespace sash {

struct Point { int x = 0; int y = 0;  auto operator<=>(const Point&) const = default; };
struct Size  { int width = 0; int height = 0;  auto operator<=>(const Size&) const = default; };

struct Rect {
    Point origin;
    Size  size;
    [[nodiscard]] constexpr bool contains(Point p) const;
    [[nodiscard]] constexpr Rect intersect(const Rect&) const;   // may be empty
    [[nodiscard]] constexpr bool empty() const { return size.width <= 0 || size.height <= 0; }
};

}  // namespace sash
```

**Rule:** all coordinates and dimensions are `int`. Unsigned geometry caused a
`SIZE_MAX` explosion in CPPurses; never repeat it. Convert at the backend edge
only, with checks.

### Signal — ~150 lines, single-threaded, reentrancy-safe

```cpp
template <typename... Args>
class Signal {
public:
    using Slot = std::function<void(Args...)>;

    [[nodiscard]] Connection connect(Slot s);          // returns handle
    void emit(Args... args);                           // a.k.a. operator()
    std::size_t slot_count() const;

private:
    struct Entry { std::uint64_t id; Slot slot; };     // id 0 == tombstone
    std::vector<Entry> entries_;
    std::uint64_t next_id_ = 1;
    int emit_depth_ = 0;                               // for safe disconnect-during-emit
};

class Connection {                                     // copyable handle
public:
    void disconnect();
    [[nodiscard]] bool connected() const;
};

class ScopedConnection {                               // RAII: disconnects in dtor
public:
    ScopedConnection(Connection);
    ~ScopedConnection();
    // movable, non-copyable
};
```

Semantics the implementation must guarantee:
- `emit` iterates by index; slots connected *during* an emit are not called in
  that emit; slots disconnected during an emit are tombstoned (id = 0) and
  compacted after the outermost emit returns.
- No thread-safety machinery — the library is single-threaded by contract.

### Keys, modifiers, events

```cpp
enum class Key : int {
    None = 0,
    Char,                       // printable input; codepoint in KeyEvent::text
    Enter, Escape, Tab, BackTab, Backspace, Delete, Insert,
    Up, Down, Left, Right, Home, End, PageUp, PageDown,
    F1, F2, /* … */ F12,
};

struct Mods { bool ctrl = false; bool alt = false; bool shift = false; };

struct KeyEvent    { Key key; char32_t text = 0; Mods mods; };
struct MouseEvent  {
    enum class Button { None, Left, Middle, Right, WheelUp, WheelDown };
    enum class Action { Press, Release, DoubleClick };
    Button button; Action action; Point pos; Mods mods;
};
struct ResizeEvent { Size size; };

using Event = std::variant<KeyEvent, MouseEvent, ResizeEvent>;
```

Notes:
- Printable input always arrives as `{Key::Char, codepoint}`. Ctrl-letter
  combos arrive as `{Key::Char, letter, mods.ctrl = true}` (backend translates
  codes 1–26). This is what nano-style keymaps need.
- Modifier fidelity is best-effort and documented per backend (§10). Do not
  design app behavior that requires Ctrl+Shift+Arrow to be distinguishable.
- Timers are **not** events — they are callbacks owned by widgets (below).
  This avoids routing questions entirely.

### Timers

```cpp
class TimerHandle {                 // RAII: cancels on destruction; movable
public:
    void cancel();
    [[nodiscard]] bool active() const;
};
// Created only via Widget::add_timer / App::add_timer — see below.
```

The loop owns a min-heap of `{deadline, period, callback, id}`. Repeating
timers re-arm from their scheduled deadline (not from "now") so Tetris gravity
doesn't drift.

---

## 5. Rendering (`render.hpp`)

### Color & Style

```cpp
enum class Color : std::uint8_t {
    Default = 0,                       // the terminal's own default fg/bg
    Black, Red, Green, Yellow, Blue, Magenta, Cyan, White,
    BrightBlack, BrightRed, BrightGreen, BrightYellow,
    BrightBlue, BrightMagenta, BrightCyan, BrightWhite,
};

enum class Trait : std::uint8_t { Bold, Underline, Reverse, Dim, Blink, Italic };

struct Style {
    Color fg = Color::Default;
    Color bg = Color::Default;
    std::bitset<8> traits;             // indexed by Trait
    // fluent helpers: with_fg(), with_bg(), with(Trait), constexpr where possible
    auto operator<=>(const Style&) const = default;
};

struct Glyph {
    char32_t ch = U' ';
    Style style;
    auto operator<=>(const Glyph&) const = default;
};
```

**Color-pair management is entirely internal to the backend** and lazy: a map
from `(fg, bg)` → pair id, allocated on first use. Rationale: 17 colors
including Default → 289 combinations, but `COLOR_PAIRS` is only 256 on PDCurses
and some ncurses builds. Real apps use a dozen combos; lazy allocation never
hits the ceiling in practice, and the backend logs-and-reuses pair 0 if it ever
does. (CPPurses eagerly ground out exactly 256 pairs and had no headroom.)

`Color::Default` maps to `-1` via `use_default_colors()` (ncurses) /
`use_default_colors()` (PDCurses supports it too); if unavailable, fall back to
White-on-Black.

### Palette

```cpp
struct Rgb { std::uint8_t r, g, b; };

class Palette {                        // owned by Terminal, accessed via App
public:
    void set(Color slot, Rgb value);   // calls init_color post-init; no-op if
                                       // the terminal can't change colors
    [[nodiscard]] bool can_redefine() const;
};
```

Redefinition happens strictly **after** `initscr()` (lesson #3). Ship one
built-in optional palette (e.g. DawnBringer-16) as data, applied only on
request.

### Canvas — the only way widgets draw

```cpp
class Canvas {
public:
    // All coordinates are LOCAL to the widget (0,0 = widget top-left).
    // All writes are clipped to the widget's rect automatically.
    void put(Point p, Glyph g);
    void put(Point p, char32_t ch, Style s = {});
    void print(Point p, std::u32string_view text, Style s = {});
    void print(Point p, std::string_view utf8, Style s = {});   // decodes UTF-8
    void fill(Rect local, Glyph g);
    void fill(Glyph g);                                          // whole widget
    void draw_box(Rect local, Style s, BoxStyle = BoxStyle::Light); // ─│┌┐└┘ etc.
    [[nodiscard]] Size size() const;                             // widget size
};
```

`Canvas` is a thin view over the `ScreenBuffer` back buffer, carrying the
widget's absolute rect for translation + clipping. It is constructed by the
renderer and passed to `Widget::paint`; widgets cannot obtain one any other way.

### ScreenBuffer and the frame algorithm

```cpp
struct Cell { char32_t ch = U' '; Style style; auto operator<=>(const Cell&) const = default; };

class ScreenBuffer {
    std::vector<Cell> front_, back_;   // both W×H
    Size size_;
public:
    void resize(Size);                 // clears both, forces full redraw
    Cell& back_at(Point);
    void flush_to(TerminalIO&);        // the diff
};
```

**Frame algorithm (normative):**

1. If nothing is dirty and the layout is valid → skip frame entirely.
2. Clear the back buffer to the root widget's background glyph.
3. Walk the widget tree depth-first, parents before children, skipping
   `!visible` subtrees and empty rects. For each widget, construct a `Canvas`
   and call `paint(canvas)`. The base implementation fills the widget's rect
   with its background style, so children composited after always sit on a
   defined surface.
4. `flush_to(terminal)`: for each row, find runs of cells where
   `back != front`; for each run, one `move` + batched writes grouped by equal
   `Style`; copy back→front as you go. One `terminal.flush()` (→ `wrefresh`) at
   the end.
5. Position or hide the hardware cursor according to the focused widget's
   cursor state (`Widget::cursor()` → `optional<Point>` in local coords).

There are **no partial repaints**. `invalidate()` on any widget sets one global
`frame_dirty` flag. This is lesson #6 and it is not negotiable: correctness
first, and the diff already minimizes terminal I/O, which is the only slow part.

---

## 6. Widget Core (`widget.hpp`)

### The base class

```cpp
enum class FocusPolicy { None, Click, Tab, Strong /* = Click|Tab */ };

class Widget {
public:
    Widget();
    virtual ~Widget();
    Widget(const Widget&) = delete;            // identity type
    Widget& operator=(const Widget&) = delete;

    // ---- tree (parent owns children) ----
    template <typename W, typename... Args>
    W& emplace_child(Args&&... args);          // constructs in place, returns ref
    std::unique_ptr<Widget> remove_child(Widget&);
    void destroy_later();                      // deferred to end of loop iteration
    [[nodiscard]] Widget* parent() const;
    [[nodiscard]] std::span<const std::unique_ptr<Widget>> children() const;

    // ---- geometry (assigned by layout; never self-assigned) ----
    [[nodiscard]] Rect  rect() const;          // absolute screen coords
    [[nodiscard]] Size  size() const;
    virtual SizeReq width_req()  const { return {}; }
    virtual SizeReq height_req() const { return {}; }

    // ---- appearance & state ----
    Style style;                               // background/default text style
    bool  visible = true;
    void  invalidate();                        // request a repaint this frame

    // ---- focus ----
    FocusPolicy focus_policy = FocusPolicy::None;
    [[nodiscard]] bool has_focus() const;
    void take_focus();

    // ---- simple-case hooks (checked BEFORE the virtuals) ----
    std::function<bool(const KeyEvent&)>   on_key_hook;
    std::function<bool(const MouseEvent&)> on_mouse_hook;

    // ---- timers (RAII; cancelled automatically when the widget dies) ----
    [[nodiscard]] TimerHandle add_timer(std::chrono::milliseconds period,
                                        std::function<void()> fn);

protected:
    // ---- overridables; defaults do the sane minimal thing ----
    virtual void paint(Canvas& c) { c.fill({U' ', style}); }
    virtual bool on_key(const KeyEvent&)     { return false; }   // true = handled
    virtual bool on_mouse(const MouseEvent&) { return false; }
    virtual void on_focus(bool /*gained*/)   {}
    virtual void on_geometry(Rect /*old_r*/, Rect /*new_r*/) {}
    [[nodiscard]] virtual std::optional<Point> cursor() const { return {}; }
};
```

Design notes:
- `emplace_child<W>(…)` is the only construction path for non-root widgets —
  it mirrors CPPurses' pleasant `make_child` and keeps ownership unambiguous.
- `destroy_later()` (not immediate delete) is the *only* deferred mechanism in
  the whole library; the loop reaps at a safe point each iteration. Focus is
  repaired automatically if the focused widget dies.
- Geometry is set synchronously by the layout pass (`on_geometry` is a
  notification, not a request). Widgets never post resize events to themselves.

### Layout

```cpp
struct SizeReq {
    int min = 0;
    int preferred = 1;
    int max = std::numeric_limits<int>::max();
    int weight = 1;                            // share of leftover space
    static constexpr SizeReq fixed(int n) { return {n, n, n, 0}; }
    static constexpr SizeReq expand(int w = 1) { return {0, 1, INT_MAX, w}; }
};
```

Containers (which are themselves widgets):

- **`VBox`** — stacks children top-to-bottom. Height distribution
  (single pass + relaxation):
  1. Give every visible child its `preferred` height, clamped to `[min, max]`.
  2. If space remains, distribute it proportionally to `weight` among children
     not yet at `max` (repeat until space or candidates run out).
  3. If over budget, shrink proportionally to `weight` among children not yet
     at `min` (repeat likewise).
  4. Children that end below their `min` are hidden for this pass (they keep
     their state; they reappear when space returns).
  Width: every child gets the container's width clamped to its own `[min, max]`.
- **`HBox`** — the mirror image.
- **`Stack`** — shows exactly one child at a time (`set_active(index | Widget&)`,
  signal `page_changed`); the active child fills the container. This is the
  page/menu-navigation primitive (CPPurses' `Widget_stack`).

That is the whole layout system — three containers and one struct — replacing
CPPurses' seven `Size_policy` types. `Layout` invalidation: any tree or
`SizeReq`-affecting change marks layout invalid; the loop re-runs layout from
the root before compositing.

### Focus

- A single `FocusChain` owned by the loop: `Tab`/`BackTab` cycle through
  visible widgets with `Tab`-capable policy, in tree (pre-order) order.
- Mouse press on a `Click`-capable widget focuses it before dispatch.
- `take_focus()` for programmatic moves.
- Focused widget receives keys first (see dispatch rules, §7).

---

## 7. Event Loop & Dispatch (`app.hpp`)

```cpp
class EventLoop {
public:
    int  run();                                // returns exit code
    void quit(int code = 0);
    // timers are created via Widget/App::add_timer, not here directly
};
```

**Main loop (normative pseudocode):**

```
run():
  layout_invalid = true; frame_dirty = true
  while not quitting:
      # 1. paint if needed (paint first so the initial frame appears
      #    before we block on input)
      if layout_invalid: run_layout(root, terminal.size()); layout_invalid = false
      if frame_dirty:    render_tree(); buffer.flush_to(terminal); frame_dirty = false
      update_hardware_cursor()

      # 2. wait for input OR the next timer deadline, whichever is sooner
      timeout = timer_heap.time_until_next()        # nullopt = wait forever
      ev = terminal.poll_event(timeout)             # nullopt on timeout

      # 3. dispatch
      if ev: dispatch(*ev)
      timer_heap.fire_due(now())                    # callbacks run here

      # 4. housekeeping
      reap_destroy_later_list()                     # may repair focus
  return exit_code
```

**Dispatch rules:**

- `ResizeEvent` → backend already resynced curses (`resize_term(0,0)` on
  PDCurses); the loop resizes the `ScreenBuffer`, marks layout invalid. Nothing
  reaches widgets except through `on_geometry` during the next layout pass.
- `KeyEvent` →
  1. App-level shortcut table (`App::add_shortcut`) — for global quit keys etc.
  2. The focused widget: `on_key_hook` first, then virtual `on_key`.
  3. If unhandled (`false`), bubble to parent, up to the root.
- `MouseEvent` → hit-test from the root downward to the deepest visible widget
  whose rect contains the point; focus it if `Click`-capable and it's a press;
  deliver (hook, then virtual, then bubble). Coordinates are translated to
  local coordinates at each delivery.

Everything is synchronous. A handler that mutates the tree or state calls
`invalidate()`; the effect appears at the top of the next iteration.

### App

```cpp
struct AppInfo { std::string name, version, description; };

class App {
public:
    // Order is contractual: ArgParser::parse runs BEFORE Terminal is
    // constructed, so --help/--version/errors print to a normal stdout
    // and never enter curses mode.
    App(int argc, char** argv, ArgParser& args, AppInfo info = {});
    App(AppInfo info = {});                    // no-args variant

    template <typename W, typename... Args>
    W& make_root(Args&&...);

    void add_shortcut(KeyEvent match, std::function<void()> fn);
    Palette&   palette();
    EventLoop& loop();
    int  run();                                // returns immediately with the
                                               // right code if --help ran
    void quit(int code = 0);
};
```

`App` composes; it has no static members. Two `App`s in one process is
unsupported and asserts.

---

## 8. Widget Catalog (`widgets.hpp`) — mapped to the target apps

| Widget | Essentials | Needed by |
|---|---|---|
| `Label` | static text, alignment, wrap on/off | everything |
| `Button` | `Signal<> pressed`, Enter/Space/click | demos, dialogs |
| `Checkbox` | `Signal<bool> toggled` | settings panes |
| `TextInput` | single line, cursor, horizontal scroll, `Signal<std::u32string> submitted`, `changed` | nano's prompt line, lynx's URL/search |
| `TextBuffer` *(model, not a widget)* | `std::vector<std::u32string>` lines; insert/erase/split/join; cursor (line, col); load/save UTF-8 file; `Signal<> changed` | nano core |
| `TextArea` | view over a `TextBuffer&`: viewport scrolling, cursor rendering, default emacs/nano-ish keymap, read-only mode | nano, lynx page body, log panes |
| `ListView` | homogeneous rows, single selection, `Signal<int> activated`, keyboard + wheel scrolling | lynx link list, file pickers, menus |
| `Menu` | `ListView` specialization: labeled entries → callbacks | CPPurses main-menu demo |
| `Stack` *(from §6)* | one-of-N pages | app navigation |
| `Titlebar` | one-line header, optional right-aligned hint | most apps |
| `StatusBar` | one-line footer; `set_text`, transient `flash(text, ms)` (uses a timer) | nano, lynx |
| `ScrollBar` | vertical indicator, optionally interactive; pairs with TextArea/ListView via signals | nicety |
| `GridCanvas` | fixed logical W×H cell matrix, `set_cell(x, y, Glyph)`, cell→screen mapping (1×1 or 2×1 cells for square-ish look) | Tetris board, chessboard, glyph-paint |
| `Divider` | 1-cell line | layout polish |

**Proof-by-composition against the target apps:**
- **nano-lite** = `VBox{ Titlebar, TextArea(TextBuffer), StatusBar, Label(shortcut hints) }` + `ArgParser` positional file + Ctrl-key shortcuts. Undo is a `TextBuffer` extension (§13).
- **lynx-lite** = `VBox{ Titlebar, HBox{ TextArea(read-only), ScrollBar }, StatusBar }` with a `ListView` link panel in a `Stack`, loading local files in v1.
- **Tetris** = `HBox{ GridCanvas(10×20), VBox{ Label(score), GridCanvas(next piece) } }` + one 500ms gravity timer + `on_key` rotation/movement. This is the intended ceiling of "graphics."
- **CPPurses demos** — notepad (as nano-lite), chess (GridCanvas + mouse hit-testing + Labels), glyph-paint (GridCanvas + mouse), main menu (`Menu` + `Stack`).

---

## 9. Argument Parsing (`args.hpp`) — built-in, ~300 LOC

Declarative, typed, GNU-style. No exceptions in the happy path.

```cpp
sash::ArgParser args{"notepad", "0.1.0", "A tiny terminal editor"};

auto& file = args.positional<std::string>("file", "file to open").required(false);
auto& ro   = args.flag('r', "readonly", "open read-only");
auto& tabw = args.option<int>('t', "tabwidth", "tab width").default_value(4);

sash::App app{argc, argv, args, {"notepad", "0.1.0"}};
if (int rc = app.run(); rc != 0) return rc;      // covers --help/--version too

// after parse: typed access
int tw   = tabw.value();
bool r   = ro.value();
auto f   = file.value_or("");
```

Specification:
- Grammar: `--long`, `--long=value`, `--long value`, `-s`, `-svalue`,
  `-s value`, bundled boolean shorts (`-abc`), `--` terminator, then
  positionals in declaration order.
- Types: `bool` (flags), `int`, `double`, `std::string`; conversion failures
  produce `error: --tabwidth: expected an integer, got 'x'` and a nonzero
  parse result.
- Auto-generated `--help` (usage line, aligned option table from the
  descriptions) and `--version`, both printed to stdout **before curses starts**
  (guaranteed by `App`'s construction order) with a "please exit 0" result.
- `parse()` returns a small result object `{ok, exit_requested, exit_code,
  message}`; `App` interprets it so user code usually never touches it.
- Repeatable options (`std::vector<T>`) and value validation callbacks are
  v1.1 extensions; leave hooks in the `Arg<T>` type.

---

## 10. Backend Abstraction & Platform Notes

### The interface

```cpp
class TerminalIO {
public:
    virtual ~TerminalIO() = default;
    virtual Size size() = 0;
    virtual std::optional<Event> poll_event(
        std::optional<std::chrono::milliseconds> timeout) = 0;
    virtual void draw_run(Point origin, std::u32string_view run, Style) = 0;
    virtual void set_cursor(std::optional<Point>) = 0;   // nullopt = hidden
    virtual void flush() = 0;
    virtual bool define_color(Color, Rgb) = 0;
    virtual void beep() = 0;
};
```

### `CursesTerminal` — the only curses-touching code

`src/backend/curses_shim.hpp` (private, included by exactly one `.cpp`):

```cpp
#if defined(_WIN32)
  #define NCURSES_MOUSE_VERSION 2   // makes PDCurses expose getmouse(MEVENT*)
  #ifndef PDC_WIDE
  #define PDC_WIDE                  // vcpkg builds WIDE=Y; consumers must ask
  #endif
  #ifndef PDC_FORCE_UTF8
  #define PDC_FORCE_UTF8
  #endif
  #include <curses.h>
#else
  #include <ncurses.h>              // ncursesw variant; NCURSES_WIDECHAR
#endif
```

Constructor sequence (contractual order — every line is a past bug):
1. Clear `LINES` / `COLS` env vars whose integer value is < 2
   (`_putenv_s(name, "")` on Windows, `unsetenv` elsewhere).
2. `setlocale(LC_ALL, "")` (POSIX; needed for wide output).
3. `initscr()`; **check the result / verify `stdscr != nullptr`** and throw
   `TerminalError` with a human-readable message on failure.
4. `raw()` (the library wants Ctrl-C as a key; App installs a default quit
   shortcut so users aren't trapped), `noecho()`, `keypad(stdscr, true)`,
   `curs_set(0)`.
5. `mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, …)`,
   `mouseinterval(0)` (the library does its own click synthesis).
6. `start_color()`, `use_default_colors()`; pair cache initialized empty.
7. Read the real size (checking for negative returns) — never before this line.

Destructor: `endwin()`. Nothing else; the object is only ever destroyed on the
main thread by `App`.

Event translation details:
- Input via `wget_wch` (ncursesw) / `wgetch`+PDCurses wide (`get_wch` exists in
  wide PDCurses); timeout via `wtimeout`.
- `KEY_RESIZE`: on PDCurses call `resize_term(0, 0)` **before** reporting the
  `ResizeEvent`; on ncurses report directly. Never install SIGWINCH handlers.
- ESC disambiguation: after ESC, poll once with ~25ms timeout; a following key
  becomes Alt+key, otherwise it's a bare `Key::Escape`
  (`set_escdelay(25)` on ncurses does this natively; do it manually for
  PDCurses, which has no `ESCDELAY`).
- Mouse: translate `MEVENT.bstate` bit-by-bit; guard `BUTTON5_*` with
  `#ifdef` (older ABIs lack it).
- Output: `draw_run` = `wmove` + `setcchar`/`wadd_wchnstr` per glyph run
  (**always** `COLOR_PAIR(pair_id)` via the pair cache; never a raw pair
  number — CPPurses' invisible-UI bug).

### Platform facts to preserve (hard-won)

- **PDCurses via vcpkg** builds `WIDE=Y UTF8=Y` but its header hides the wide
  API unless the *consumer* defines `PDC_WIDE`. The shim handles it; nothing
  else may include curses.
- PDCurses `chtype` has a 16-bit character field → **BMP-only on Windows**.
  `Glyph::ch` is `char32_t`; the backend narrows with a check and substitutes
  `U'�'` for non-BMP on Windows.
- MSVC requires `/utf-8` (set `PUBLIC` on the library target) or every
  `U'─'`-style literal is silently corrupted (C4066).
- ncurses ABI: prefer `wget_wch`/`cchar_t`; require the `w` (wide) library via
  pkg-config `ncursesw`.
- IDE consoles (CLion "emulate terminal") may report a 1-row console or pipe
  stdin; PDCurses hard-exits on piped stdin ("Redirection is not supported").
  Document: run examples in a real terminal; the env-var sanitization covers
  the common bogus-`LINES` case.

### `MockTerminal` — headless testing

An in-memory `std::vector<Cell>` grid + a scripted `std::deque<Event>` feed.
`poll_event` pops the script (or returns timeout). Test helpers:

```cpp
MockTerminal term{{80, 24}};
term.feed(KeyEvent{Key::Char, U'x'});
// … run one loop iteration via a test-only App::pump_once() …
CHECK(term.row_text(0) == "  my title  ");
CHECK(term.cell_at({2, 5}).style.fg == Color::Yellow);
```

This makes layout, rendering, focus, and key routing unit-testable with no TTY
and no curses — the single biggest testability win over CPPurses.

---

## 11. Build, Packaging, Testing

- **CMake ≥ 3.21** with `CMakePresets.json`: `linux-gcc`, `linux-clang`,
  `windows-msvc` (sets the vcpkg toolchain file).
- One static library target `sash` (`add_library(sash STATIC …)`,
  `sash::sash` alias). C++20 (`target_compile_features cxx_std_20`).
  MSVC: `/utf-8 /W4 /permissive-` (PUBLIC `/utf-8`); GCC/Clang:
  `-Wall -Wextra`.
- **Windows deps**: `vcpkg.json` manifest with
  `{"name": "pdcurses", "platform": "windows"}`; link
  `unofficial::pdcurses::pdcurses` (config package `unofficial-pdcurses`).
- **POSIX deps**: `pkg_check_modules(NCURSESW ncursesw)` with a
  `find_library(ncursesw)` fallback.
- **Tests**: vendored single-header **doctest** (the only dev-only third-party
  file, consistent with the zero-dependency policy). Coverage targets:
  Signal semantics (incl. disconnect-during-emit), ArgParser grammar + errors +
  help text, VBox/HBox distribution incl. shrink and below-min hiding,
  TextBuffer editing ops + UTF-8 round-trip, renderer diff correctness
  (MockTerminal write-count assertions), key dispatch/bubbling/focus order.
- `examples/` are real CMake targets, built by default in dev presets.

---

## 12. Implementation Roadmap (phased; each milestone independently verifiable)

**M1 — Foundations (no widgets yet).**
Geometry, Signal, Event/Key types, `TerminalIO`, `CursesTerminal`,
`MockTerminal`, `ScreenBuffer` + diff, `Canvas`, Color/Style/pair-cache,
Palette. *Accept:* a scratch program paints styled text, survives resize,
reads keys incl. Alt/Ctrl detection; doctest suite green on MockTerminal;
builds clean on MSVC and Linux.

**M2 — Widget core.**
Widget base + tree + `destroy_later`, SizeReq + VBox/HBox/Stack, focus chain,
EventLoop + dispatch + timers, App bootstrap (without args).
*Accept:* `examples/hello` — Titlebar, Label, two Buttons (one quits), Tab
focus traversal, mouse clicking; routing tests green.

**M3 — Widget catalog.**
Label/Button/Checkbox, TextInput, TextBuffer+TextArea, ListView/Menu,
StatusBar/Titlebar/ScrollBar/Divider, GridCanvas.
*Accept:* per-widget unit tests via MockTerminal; a kitchen-sink example page.

**M4 — ArgParser + App integration + Palette API.**
*Accept:* parser test suite (grammar table, error messages, help output
golden-file); `--help` prints without entering curses.

**M5 — The proving-ground examples.**
`tetris` (GridCanvas + gravity timer), `notepad` (nano-lite with file
load/save, dirty-flag in StatusBar, Ctrl-S/Ctrl-Q), `browser` (lynx-lite over
local files with a link ListView). *Accept:* all three run on Windows Terminal
and a Linux terminal from the same source.

**M6 — Stretch (post-v1).**
TextBuffer undo/redo, mouse drag + selection in TextArea, repeatable ArgParser
options, CJK double-width, 256-color `Color::Extended(uint8_t)`.

---

## 13. Risks & Open Questions

- **Key fidelity varies by terminal.** Ctrl/Alt/Shift detection is best-effort
  on both backends; keymaps in shipped widgets must use only robust chords
  (plain keys, Ctrl+letter, Alt+letter, F-keys). Documented, not solved.
- **PDCurses mouse under Windows Terminal** is functional but historically
  quirkier than wincon; keep every shipped widget fully keyboard-operable
  (also the right accessibility posture).
- **BMP-only on Windows** (16-bit PDCurses char field). Accepted for v1;
  substitution glyph on narrowing failure.
- **CJK/double-width glyphs** would need width-aware `Canvas::print` and diff
  cells; deferred to M6 — affects `TextArea` column math, so keep column
  arithmetic behind small helpers (`col_width(char32_t)` returning 1 for now).
- **Color-pair ceiling** (256 on PDCurses): mitigated by the lazy pair cache;
  on exhaustion, reuse the closest existing pair and log once. Not expected at
  this library's scale.
- **Open question:** should `TextArea` keybindings be a swappable `Keymap`
  table from day one (nano vs emacs style)? Recommended: yes, a simple
  `std::vector<{KeyEvent match, Action}>` — cheap now, painful to retrofit.

---

*End of design. Implementation should follow the roadmap in §12 and treat §2's
table and the constructor sequence in §10 as acceptance-relevant constraints.*
