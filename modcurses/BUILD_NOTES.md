# Sash — Build & Configuration Notes (companion to TUI_DESIGN.md)

*Environment- and platform-specific knowledge from the CPPurses→PDCurses porting
session on this machine (Windows 11, August 2026). Read this before writing any
CMake or backend code. Target platforms: **Windows 11 / MSVC** and **Debian /
GCC or Clang** — the same source tree must configure and build on both with no
edits.*

---

## 1. This machine — what exists and where

### vcpkg (already installed and bootstrapped)
| What | Path |
|---|---|
| vcpkg root | `C:\Users\bwise\vcpkg` |
| vcpkg executable | `C:\Users\bwise\vcpkg\vcpkg.exe` |
| **CMake toolchain file** | `C:\Users\bwise\vcpkg\scripts\buildsystems\vcpkg.cmake` |
| Binary cache (prebuilt pdcurses restores from here in ~µs) | `%LOCALAPPDATA%\vcpkg\archives` |

### PDCurses on this machine (installed via the vcpkg `pdcurses` port, 3.9#7)
| What | Path |
|---|---|
| Extracted, patched **source tree** (readable reference!) | `C:\Users\bwise\vcpkg\buildtrees\pdcurses\src\3.9-2f00932d3e.clean\` |
| …the Windows console backend | `…\3.9-2f00932d3e.clean\wincon\` (`pdcscrn.c`, `pdcdisp.c`, …) |
| …portable core (docs in comments are excellent) | `…\3.9-2f00932d3e.clean\pdcurses\` (`initscr.c`, `color.c`, `getch.c`, …) |
| Original tarball | `C:\Users\bwise\vcpkg\downloads\wmcbrine-PDCurses-3.9.tar.gz` |
| Installed package (headers/libs, per vcpkg) | `C:\Users\bwise\vcpkg\packages\pdcurses_x64-windows\` |
| Per-project install (manifest mode) | `<build-dir>\vcpkg_installed\x64-windows\include\curses.h`, `…\lib\pdcurses.lib`, `…\bin\pdcurses.dll` (+ `debug\` variants) |
| Upstream | `https://github.com/wmcbrine/PDCurses` (semi-active; 3.9 is the vcpkg version) |

The vcpkg build compiles PDCurses with **`WIDE=Y UTF8=Y`** using nmake and the
`wincon` backend, and it is a **DLL** under the default `x64-windows` triplet —
vcpkg's applocal step copies `pdcurses.dll` next to your `.exe` automatically at
build time.

**When in doubt about PDCurses behavior, read the source above** — that is how
the wide-API gating (`PDC_WIDE`), the `getmouse` macro condition, the
`resize_term(0,0)` requirement, and the `LINES` env-var precedence were all
diagnosed. Key files: `wincon/pdcscrn.c` (`PDC_scr_open` — env vars + size
checks), `pdcurses/initscr.c` (the `resize_term` doc-comment), `curses.h`
(what `PDC_WIDE`/`NCURSES_MOUSE_VERSION` actually gate).

### Toolchain on this machine
| What | Path / note |
|---|---|
| Visual Studio 2026 Community (v18) | `C:\Program Files\Microsoft Visual Studio\18\Community` |
| Dev shell (PowerShell — **use this**) | `& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64` |
| vcvars batch (worked only from real cmd, hung when driven through Git-Bash → cmd.exe) | `…\VC\Auxiliary\Build\vcvars64.bat` |
| Ninja (bundled with VS) | `…\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe` |
| System CMake | 4.1.1 at `C:\Program Files\CMake\bin` |
| CLion | 2026.1 — see §5 |
| **Not available / not allowed** | MinGW, WSL, Cygwin-style builds. MSVC only on Windows. |

**Generator gotcha:** CMake 4.1.1's `"Visual Studio 17 2022"` generator does
**not** detect VS 2026, and there is no VS-2026 generator name in it. The
default bare `cmake` picks NMake and fails to find `nmake` outside a dev shell.
**The combination that works: VS dev shell (PowerShell script above) + `-G Ninja`**
(pass `-DCMAKE_MAKE_PROGRAM=<bundled ninja path>` if ninja isn't on PATH).

**CMake 4.x note:** compatibility with `cmake_minimum_required(VERSION < 3.5)`
is gone and `< 3.10` warns. Use `cmake_minimum_required(VERSION 3.21)` (needed
for presets anyway).

### The exact command sequence that worked here

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64
cmake -S . -B build -G Ninja `
  -DCMAKE_MAKE_PROGRAM="C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" `
  -DCMAKE_TOOLCHAIN_FILE="C:\Users\bwise\vcpkg\scripts\buildsystems\vcpkg.cmake"
cmake --build build
```

On first configure, vcpkg reads `vcpkg.json`, restores/builds `pdcurses`
(~6 s cold, instant from cache), and `find_package` just works.

---

## 2. Dual-backend requirement: ncursesw (Debian) + PDCurses (Windows)

The library **must interface with both**, selected at configure time. Neither
backend's header may appear in public headers or in more than one `.cpp`.

### The private shim (one header, included by exactly one translation unit)

```cpp
// src/backend/curses_shim.hpp  — PRIVATE. Nothing else includes curses.
#if defined(_WIN32)
  // PDCurses exposes ncurses-compatible getmouse(MEVENT*) only under this:
  #define NCURSES_MOUSE_VERSION 2
  // vcpkg builds the DLL WIDE=Y/UTF8=Y, but the header hides the wide API
  // (cchar_t, setcchar, wadd_wchnstr, get_wch...) unless the CONSUMER defines:
  #ifndef PDC_WIDE
  #define PDC_WIDE
  #endif
  #ifndef PDC_FORCE_UTF8
  #define PDC_FORCE_UTF8
  #endif
  #include <curses.h>
#else
  #include <ncurses.h>   // the ncursesw variant (see CMake below)
#endif
```

### API differences table (every row was hit in practice)

| Topic | ncursesw (Debian) | PDCurses (Windows) | Rule for Sash |
|---|---|---|---|
| Header | `<ncurses.h>` | `<curses.h>` | shim decides |
| Wide API visibility | always (widechar build) | only if consumer defines `PDC_WIDE` | shim defines it |
| Wide-support detection | `add_wchstr` is a **macro** | `add_wchstr` is a **function** | never probe with `#ifdef add_wchstr`; use `PDC_WIDE`/platform |
| Color into chtype | `COLOR_PAIR(n)` | `COLOR_PAIR(n)` (shift 24) | **always `COLOR_PAIR`, never a raw pair number** (raw pair lands in the *character bits* → black-on-black UI) |
| `getmouse` | `getmouse(MEVENT*)` | native is `getmouse(void)`; `NCURSES_MOUSE_VERSION` maps it to `nc_getmouse(MEVENT*)` | shim defines the macro |
| Resize delivery | `KEY_RESIZE` (internally SIGWINCH-driven) | `KEY_RESIZE` | on PDCurses **must** call `resize_term(0, 0)` on `KEY_RESIZE` to resync internal buffers; harmless pattern: guard with `#ifdef _WIN32` |
| SIGWINCH / `sigaction` | available | **does not exist** | never use; rely on `KEY_RESIZE` only |
| `ESCDELAY` | global variable / `set_escdelay()` | **does not exist** | ESC disambiguation done manually in the backend (poll ~25 ms after ESC) |
| `sys/ioctl.h`, `TIOCGWINSZ` | available | **does not exist** | never use; `resize_term(0,0)` + `getmaxx/getmaxy` cover it |
| `setenv`/`unsetenv` | available | **does not exist**; use `_putenv_s(name, "")` | wrap in one helper in the backend TU |
| `LINES`/`COLS` env vars | respected by ncurses | respected by PDCurses **over the real console size** | clear them pre-`initscr()` when value < 2 (IDE consoles export `LINES=1`) |
| `getmaxx(nullptr)` | returns `ERR` (−1) | returns `ERR` (−1) | check before assigning; **never** into an unsigned |
| `wchar_t` | 32-bit | 16-bit (Windows); PDCurses char field is 16-bit | store `char32_t`; narrow with check on Windows (BMP only, substitute `U'�'`) |
| `COLOR_PAIRS` | often 256–65536 | 256 | lazy pair cache, never a fixed 16×16 grid |
| `use_default_colors` | yes | yes | OK to rely on |
| `BUTTON5_*` mouse bits | usually present | version-dependent | guard with `#if defined(BUTTON5_PRESSED)` |
| Piped stdin | curses may cope | hard `exit(1)` ("Redirection is not supported") | document; nothing to fix in code |

---

## 3. Portability rules for CMake (Windows + Debian from one tree)

1. **Never hardcode machine paths** (`C:\Users\bwise\...`) in `CMakeLists.txt`.
   Machine specifics go in `CMakePresets.json` / `CMakeUserPresets.json` or the
   command line. The vcpkg toolchain path belongs in a **user** preset or the
   `VCPKG_ROOT` env var — commit a preset that references
   `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`.
2. **Dependency lookup is the only place allowed to branch on platform:**

   ```cmake
   if(WIN32)
       find_package(unofficial-pdcurses CONFIG REQUIRED)   # from vcpkg
       set(SASH_CURSES_LIB unofficial::pdcurses::pdcurses)
   else()
       find_package(PkgConfig REQUIRED)
       pkg_check_modules(NCURSESW REQUIRED IMPORTED_TARGET ncursesw)
       set(SASH_CURSES_LIB PkgConfig::NCURSESW)
   endif()
   target_link_libraries(sash PRIVATE ${SASH_CURSES_LIB})   # PRIVATE: firewall
   ```

3. **Compiler flags branch on compiler, not OS:**

   ```cmake
   if(MSVC)
       target_compile_options(sash PUBLIC /utf-8)           # PUBLIC + mandatory:
       # sources contain UTF-8 wide literals (─ │ ┌ …); without /utf-8 MSVC
       # parses them in the system codepage and silently corrupts them (C4066).
       target_compile_options(sash PRIVATE /W4 /permissive-)
   else()
       target_compile_options(sash PRIVATE -Wall -Wextra)
   endif()
   ```

   Do **not** pass `-Wall` to MSVC — it interprets it as `/Wall` (every warning
   in existence, thousands of lines of noise).
4. **vcpkg manifest** (`vcpkg.json` at repo root) so Windows deps are automatic
   and Debian ignores it entirely:

   ```json
   {
     "name": "sash",
     "version-string": "0.1.0",
     "dependencies": [ { "name": "pdcurses", "platform": "windows" } ]
   }
   ```

   (If the repo has a `.gitignore` with `*.json`, add `!vcpkg.json` — this bit
   the CPPurses port.)
5. **No shell-isms in CMake.** CPPurses had
   `add_custom_target(... COMMAND [ -f x ] && mv ... || :)` — POSIX-shell
   syntax that can never run on Windows. Use `cmake -E` commands
   (`copy_if_different`, `rm`, …) or generator expressions only.
6. Save sources as **UTF-8 without BOM**; `/utf-8` (MSVC) and GCC/Clang
   defaults then agree.
7. Use `cxx_std_20` via `target_compile_features`; avoid
   `CMAKE_CXX_STANDARD` globals.
8. Ninja works as the generator on both platforms; presets should default to it.

---

## 4. Portability rules for C++ code

- **`#ifdef _WIN32` appears only inside `src/backend/`** (ideally only in
  `curses_terminal.cpp` and the shim). If any other file needs a platform
  branch, the abstraction is wrong.
- No POSIX headers anywhere: `<unistd.h>`, `<sys/ioctl.h>`, `<termios.h>`,
  `<signal.h>`-as-in-`sigaction`. The backend interface makes them unnecessary.
- No `setenv`/`unsetenv` outside the backend helper; no `fileno(stdin)` tricks.
- `std::setlocale(LC_ALL, "")` on POSIX before `initscr()` (required for wide
  output with ncursesw); harmless on Windows.
- Locale-independent parsing everywhere else (`std::from_chars` for ArgParser —
  also avoids locale surprises on Debian).
- File I/O (TextBuffer load/save): open streams in **binary** mode and handle
  `\r\n` vs `\n` explicitly; preserve the file's original line-ending style on
  save. Use `std::filesystem::path` (handles wide Windows paths); construct
  from UTF-8 via `std::filesystem::u8path`-equivalent (C++20:
  `path(u8string)`).
- `char32_t` for all glyph data; conversion to the backend's form
  (`wchar_t[2]`/`cchar_t`) happens only in the backend TU.
- Threads: none. (Also sidesteps every Windows-vs-pthread difference.)

---

## 5. IDE / running notes (this machine)

- **CLion**: CLion discovers `CMakePresets.json` / `CMakeUserPresets.json` and
  lists every preset as a profile, but leaves them all **disabled**, with its
  own bare `Debug` profile (no CMake options, therefore no toolchain file)
  enabled instead. That profile fails at
  `find_package(unofficial-pdcurses)`. Either enable a preset profile in
  *Settings → Build, Execution, Deployment → CMake* and disable the default
  one, or rely on `VCPKG_ROOT` (below). Toolchain = Visual Studio.
  *(Observed 2026-08-20, CLion 2026.1.)*

### `VCPKG_ROOT` on this machine — read this before debugging a vcpkg failure

- `VCPKG_ROOT` is set at **user scope** to `C:\Users\bwise\vcpkg`, and
  `CMakeLists.txt` picks it up before `project()` when no toolchain file was
  supplied. That is what makes a bare configure (CLion's default profile, a
  plain `cmake -S . -B build`) work at all.
- **`Launch-VsDevShell.ps1` overwrites it** with Visual Studio's own bundled
  vcpkg, `C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg`.
  `vcvars64.bat` does **not** — so a dev shell and CLion disagree about which
  vcpkg they mean. Both work, but they build into different `vcpkg_installed`
  trees, so a first configure under the dev shell rebuilds pdcurses from
  source instead of restoring it in microseconds. The `here` preset pins the
  absolute path and sidesteps this entirely; prefer it locally.
- **`vcpkg.json` must keep its `builtin-baseline`.** VS's bundled vcpkg is a
  git-registry instance and refuses to resolve any port without one:
  `error: this vcpkg instance requires a manifest with a specified baseline`.
  Without the baseline the project builds under one vcpkg on this machine and
  fails under the other, decided purely by which one `VCPKG_ROOT` happens to
  name at the time.
- Environment variables are read at process start: after changing
  `VCPKG_ROOT`, **restart CLion** (and any shell) before expecting it to take.
- vcpkg build failures that end in
  `fatal error C1083: Cannot open compiler generated file: '': Invalid argument`
  are MAX_PATH, not a broken port. vcpkg's per-port scratch paths
  (`vcpkg_installed/vcpkg/blds/<port>/x64-windows-rel/CMakeFiles/CMakeScratch/TryCompile-xxxxxx/`)
  are long, so keep the build directory shallow.
- **Running TUI apps from CLion is unreliable**: its emulated output console
  has exported `LINES=1` (the backend's env sanitization handles that case) and
  its debugger runs the process with **piped stdin**, which PDCurses rejects
  outright ("Redirection is not supported"). Use *Run → Edit Configurations →
  Run in external console*, or just run the exe from **Windows Terminal** —
  that is the environment everything was verified in.
- Ctrl-C: the library uses `raw()` mode, so Ctrl-C arrives as a key, not a
  signal. `App` must install a default quit shortcut (design §7) or testers
  will have to kill the window — this was observed, not theoretical.

## 6. Debian side (to verify early — not yet tested this session)

- Packages: `sudo apt install build-essential cmake ninja-build pkg-config libncurses-dev`
  (`libncurses-dev` ships the wide `ncursesw` library and its pkg-config file
  on current Debian; the old name `libncursesw5-dev` is a transitional alias).
- Verify `pkg-config --libs ncursesw` prints `-lncursesw` before wiring CMake.
- Terminal must be UTF-8 (`locale` shows `*.UTF-8`) or wide glyphs degrade.
- First cross-platform smoke test should be **milestone M1** (design §12):
  same source, `cmake --preset linux && cmake --build`, run the paint/keys
  scratch program over SSH and in a local terminal.

---

## 7. Quick-reference: past failure modes to re-check after any backend change

1. UI paints but is invisible → a raw pair number was OR'd instead of
   `COLOR_PAIR(n)`, or pair 0's colors are fg==bg.
2. `initscr()` fails with `LINES value must be >= 2 ... got 1` → env-var
   sanitization not running before `initscr()`.
3. Resize garbles or freezes on Windows → missing `resize_term(0, 0)` on
   `KEY_RESIZE`.
4. Box-drawing renders as garbage on Windows only → `/utf-8` missing, or a
   literal outside the BMP.
5. `getmouse` fails to compile / takes 0 args on Windows →
   `NCURSES_MOUSE_VERSION` not defined before the include.
6. Wide functions undeclared on Windows → `PDC_WIDE` not defined before the
   include.
7. Huge bogus sizes (`18446744073709551615`) → a curses `ERR` (−1) assigned
   into unsigned geometry; geometry must stay `int` and returns must be checked.
