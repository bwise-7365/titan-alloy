# modcurses

A compact C++20 library for building classic text-mode applications, on top of
**ncursesw** (POSIX) and **PDCurses** (Windows/MSVC), from one source tree with
no per-platform edits.

The capability target is set by example rather than by feature list: if
**nano**, **Tetris**, **lynx** and the CPPurses demos are expressible and
pleasant to write, the library is done. Three of the four ship in `examples/`.

---

## What it is for

Terminal UI toolkits tend to fail in one of two directions — either they are a
thin wrapper that leaves you managing curses' global state by hand, or they are
a framework large enough to have its own learning curve. modcurses aims at the
narrow middle: enough structure that a real application is short to write, and
few enough concepts that the whole thing fits in your head.

The runtime is deliberately boring. One thread, one event loop, no queues, no
lazy initialisation, no static state. A handler mutates some state and calls
`invalidate()`; the effect appears at the top of the next iteration. There is
no other path to the screen.

## Design philosophy

The design document (`TUI_DESIGN.md`) is a direct response to a real debugging
campaign porting CPPurses to Windows. Every rule below is traceable to a
specific defect found there, and they are treated as constraints rather than
preferences.

**The curses firewall.** No curses header is reachable from any public header.
Exactly one translation unit includes curses, through a shim that sets
`PDC_WIDE`, `PDC_FORCE_UTF8` and `NCURSES_MOUSE_VERSION` first. The public API
speaks only library types. This is what keeps backend surprises — and there
have been many — confined to one file.

**No static state, and no lazy initialisation.** The terminal is an RAII object
built explicitly by `App`, on the main thread, before any widget exists.
Nothing is configurable before it runs. CPPurses initialised curses from
whichever thread happened to touch a function-local static first, and applied
its colour palette before `initscr()` — so the palette silently never applied,
on any platform, for years.

**Signed geometry.** Every coordinate and dimension is `int`, and backend
returns are checked before use. A curses `ERR` (-1) stored into an unsigned
size is how CPPurses came to lay out a widget tree to 18446744073709551615
cells.

**Repaint everything; diff before writing.** Each frame paints the whole widget
tree into a back buffer, diffs it against the front buffer, and writes only the
runs that changed. At terminal scale this costs microseconds, and it deletes an
entire category of partial-repaint bugs. Correctness first — the diff already
minimises the only slow part, which is terminal I/O.

**Headless by construction.** `TerminalIO` is an interface with two
implementations: the real curses backend and a `MockTerminal` holding an
in-memory grid and a scripted event queue. Layout, rendering, focus and key
routing are all unit-testable with no TTY and no curses linked in.

**Single-threaded by contract.** There is no `post_event`, so there are no
queues to route wrongly. Timers are part of the loop's wait rather than events.

## Building

Requirements:

- **CMake ≥ 3.21** and a **C++20** compiler
- **Ninja** (the presets use Ninja Multi-Config)
- Windows: **MSVC** (VS 2022 or later) and **vcpkg** — PDCurses comes from the
  committed `vcpkg.json` manifest, pinned by `builtin-baseline`
- Debian/Ubuntu: `sudo apt install build-essential cmake ninja-build pkg-config libncurses-dev`
  (`libncurses-dev` ships the wide `ncursesw` library and its pkg-config file)

```sh
cmake --preset windows-msvc      # or linux-gcc / linux-clang
cmake --build --preset windows-msvc-all      # both Debug and Release
ctest --preset windows-msvc-debug
```

One configure directory produces **both Debug and Release**; binaries land in
`build/<preset>/<Config>/bin`. The `-all` build preset builds every target in
both configurations at once.

On Windows the vcpkg toolchain is found automatically from the `VCPKG_ROOT`
environment variable. To point at a specific vcpkg instead, override
`CMAKE_TOOLCHAIN_FILE` in `CMakeUserPresets.json` — that file is gitignored,
because machine-specific paths never belong in the committed build.

Run the examples from a real terminal. PDCurses rejects redirected stdin
outright, so an IDE's emulated console will not do; `BUILD_NOTES.md` §5 has the
details and the rest of this machine's hard-won environment knowledge.

## What is here

```
include/modcurses/   public headers - no curses reachable from any of them
src/                 implementation; src/backend/ is the only curses code
examples/            scratch, hello, kitchen, mtetris, mamon
tests/               vendored doctest + 275 cases, all headless
```

| Example | What it demonstrates |
|---|---|
| `scratch` | The M1 backend directly: styled output, resize, key and mouse decoding |
| `hello` | Widgets, layout, focus, timers, and the argument-parsing bootstrap |
| `kitchen` | The whole widget catalogue across five pages of a `Stack` |
| `mtetris` | A port of a real FLTK Tetris: `GridCanvas`, run-time colour schemes, a seeded PRNG |
| `mamon` | A nano-like editor: `TextBuffer`/`TextArea`, search, cut/paste, undo, file I/O |

`system-overview.md` explains how the pieces fit together and why they are
shaped the way they are. `TUI_DESIGN.md` is the original specification and
`BUILD_NOTES.md` the platform notes; both remain accurate and are worth reading
before changing anything in `src/backend/`.

## Status

Several small examples are included. The most complex is a
partial re-implementation of nano, called mamon. A partial 
re-implementation of the text-only browser lynx has not been 
started as it would be mainly web interactions irrelevant to
the core concerns of this library.

**Everything has been built and verified on Windows/MSVC/PDCurses only.** The
tree follows the portability rules and the Linux presets are committed, but no
build has yet been run on Debian. Two places are most likely to need attention
there: the `__cpp_lib_to_chars` fallback in `src/args.cpp` (floating-point
`from_chars` arrived in GCC 11), and anything touching colour, since the
`COLOR_*` constants are numbered differently by the two backends.
