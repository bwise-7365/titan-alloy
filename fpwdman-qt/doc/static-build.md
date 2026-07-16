Copyright Ben Paul Wise. All Rights Reserved.

# Building fpwdman_qt as a single static executable

The default build links against the Qt DLLs in `C:/Qt/6.8.3/msvc2022_64` and has
to ship with a directory of them beside the exe. This document covers the other
option: one `fpwdman_qt.exe` that runs on a bare Windows machine with no Qt DLLs,
no `platforms/` folder, and no VC++ redistributable.

The prebuilt Qt under `C:/Qt` cannot be linked statically -- it contains DLLs and
their import libraries, and nothing else. A static Qt has to be compiled from
source. That is the one expensive step here, and it only has to be done once; the
result is reused by every static build of the app afterwards.

## What gets built

Only `qtbase`. The app includes nothing outside QtCore, QtGui and QtWidgets --
no Network, no SVG, no QML -- so the rest of the Qt submodules are never
initialized and never compiled. That is what keeps this to a half-hour rather
than most of a day.

## Prerequisites

**A compiler.** Visual Studio Community 2026 (MSVC 14.51) is the only toolset
installed, and it is what compiles the app, so Qt is built with it too -- one
compiler for both, nothing to install, no mixing. Qt 6.8.3 dates from March 2025
and predates 14.51, so this was not a combination Qt ever tested, but it was run
on 2026-07-15 and compiled clean: 1884 targets, no errors. "If the Qt build
fails" below keeps the fallback on record in case a future Qt or compiler update
disturbs that.

**Ninja.** CLion bundles one at
`C:/Program Files/JetBrains/CLion 2026.1/bin/ninja/win/x64/ninja.exe`. It is not
on the system `PATH` by default, so the Qt build needs it added (below).

CMake 4.1.1 and the checkout at `C:/repos/ghub-ext/qt.6.8.3` are already in
place. Qt's own build needs neither Perl nor Python for qtbase alone.

Budget roughly 10 GB of disk for the Qt build tree and about 2.9 GB for the
installed result (the `-ltcg` build is much larger on disk than a plain one --
see "The cost of -ltcg").

## Step 1: fetch qtbase

The super-repo is checked out but its submodules are not populated -- `qtbase/`
is an empty directory. From a normal shell:

    cd C:/repos/ghub-ext/qt.6.8.3
    git submodule update --init --recursive qtbase

## Step 2: build and install the static Qt

This step must run in a **x64 Native Tools Command Prompt for VS 2026**, because
`configure` needs the MSVC environment. Start it from the Start menu -- or set
the environment up in a plain prompt -- and put Ninja on the path:

    "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
    set PATH=C:\Program Files\JetBrains\CLion 2026.1\bin\ninja\win\x64;%PATH%

Then configure and build:

    cd C:\repos\ghub-ext\qt.6.8.3
    mkdir build-static
    cd build-static
    ..\qtbase\configure.bat -static -static-runtime -release ^
        -ltcg -optimize-size ^
        -prefix C:\Qt\6.8.3-static ^
        -nomake examples -nomake tests ^
        -opengl desktop -no-icu ^
        -qt-zlib -qt-libpng -qt-libjpeg -qt-pcre -qt-freetype ^
        -opensource -confirm-license
    cmake --build . --parallel
    cmake --install .

Why these flags:

- `-static` builds Qt as `.lib` archives instead of DLLs.
- `-static-runtime` links the static MSVC runtime (`/MT`). This is what removes
  the VC++ redistributable from the requirements. It is also the flag that
  forces the app's own CRT setting, which `CMakeLists.txt` handles.
- `-ltcg` and `-optimize-size` are the two size levers, worth 22% together
  (17.3 MB down to 13.5 MB, measured). `-ltcg` compiles with `/GL` and defers
  code generation to link time, so the whole program is optimized at once;
  `-optimize-size` swaps `/O2` for `/O1`, which optimizes for size over speed.
  Neither is free: see "The cost of -ltcg" below.
- `-nomake examples -nomake tests` cuts out the large majority of the compile.
- `-opengl desktop` overrides the Windows default of dynamic OpenGL selection,
  which a static build cannot do -- there is no DLL to swap at load time.
- `-no-icu` keeps an external ICU dependency out of a build whose whole point is
  having no external dependencies. Qt falls back to its built-in collation.
- The `-qt-*` flags use Qt's bundled copies of zlib, libpng, libjpeg, PCRE and
  FreeType rather than hunting for system ones that would then need shipping.
- `-opensource -confirm-license` accepts the LGPL terms non-interactively.

If `configure` rejects `-opengl desktop`, drop the flag and let it choose.

### The cost of -ltcg

Two surprises, both harmless, both alarming if unexpected:

- **The installed Qt grows to about 2.9 GB**, up from ~1.5 GB, and individual
  archives balloon -- `Qt6Widgets.lib` goes from 35 MB to 765 MB. Under `/GL` an
  object file holds intermediate language rather than machine code, so the
  archives inflate. None of this reaches the exe.
- **Linking gets slow and memory-hungry**, both for Qt itself and for the app,
  because code generation for the entire program happens at link time. This is
  the price of the 22%.

Neither flag is required. Dropping both gives a 17.3 MB exe that builds and links
faster, from a 1.5 GB Qt.

To confirm the two flags actually took, check `config.summary` for
"Optimize release build for size .... yes" and "Using Link Time Optimization
(LTCG) .... yes". Do not judge by eyeballing a single `FLAGS` line in
`build.ninja` -- a couple of targets legitimately keep `-O2`, so a lone sample
misleads. Count instead: `-O1` and `/GL` should each appear on ~1660 of the 1732
`FLAGS` lines.

Note that this installs to `C:/Qt/6.8.3-static`, beside the stock
`C:/Qt/6.8.3` rather than inside it. The two Qt installations are independent and
neither disturbs the other.

## Step 3: build the app

`FPWDMAN_STATIC` is the switch, and it is off by default, so nothing about the
existing DLL build changes. The static build wants its own build directory --
its CRT and Qt prefix differ from the default build's, so the two cannot share a
CMake cache.

    cmake -B cmake-build-static -G Ninja -DCMAKE_BUILD_TYPE=Release -DFPWDMAN_STATIC=ON
    cmake --build cmake-build-static

Release is required. The static Qt above is built `-release` only and has no
debug libraries, so a Debug configure is refused with an explanatory error rather
than allowed to fail later in the link. Debug against the default DLL build.

### From CLion

CLion selects configurations through CMake profiles, so the static build gets its
own alongside the existing Debug and Release ones. Settings > Build, Execution,
Deployment > CMake > `+`, then:

    Name             Static
    Build type       Release
    CMake options    -DFPWDMAN_STATIC=ON
    Build directory  cmake-build-static

Apply, then pick **Static** in the toolbar's configuration dropdown and build the
`fpwdman_qt` target as usual. If `cmake-build-static` was already configured from
the command line, CLion adopts it and does not re-run CMake.

Build type has to be Release, for the reason above. The build directory has to be
its own rather than shared with `cmake-build-debug`: the CRT setting and the Qt
prefix both differ, so the two cannot share a CMake cache.

For a one-click tarball, add a run configuration the same way the existing
`package (Release)` one was set up -- a `CMakeRunConfiguration` with
`TARGET_NAME=package`, pinned to the Static profile. Note that CLion keeps run
configurations in `.idea/workspace.xml`, which is git-ignored, so anything added
there is local to this machine.

To ship:

    cmake --build cmake-build-static --target package

This produces `cmake-build-static/fpwdman_qt.tgz`, unpacking to a single
`fpwdman_qt/` folder as before -- now holding one file. The `package` target
works the same way in both link flavors; under `FPWDMAN_STATIC` there is simply
no Qt runtime to stage alongside the exe, and windeployqt is not run at all.

## Verifying it

The claim to check is that the exe imports nothing but Windows system DLLs. From
the VS command prompt:

    dumpbin /dependents cmake-build-static\fpwdman_qt.exe

The list should hold only Windows system libraries. Any `Qt6*.dll` means the
static Qt was not picked up; any `VCRUNTIME140.dll` or `MSVCP140.dll` means
`-static-runtime` did not take.

As actually measured: 30 dependencies, all of them Windows system DLLs
(`KERNEL32`, `USER32`, `GDI32`, `d3d11`, `DWrite` and the like), with no Qt and no
C runtime among them -- identical before and after the `-ltcg -optimize-size`
rebuild.

The exe comes out at **13,486,592 bytes** (13.5 MB) with those two flags, down
from 17,265,664 without them. Compare roughly 1 MB plus a folder of DLLs for the
dynamic build. That is the trade: the static link pulls in only the Qt code
actually reached, so the shipped total shrinks, but the single file is much
larger than the dynamic exe alone.

Do not read anything into the app's own object code being ~0.2 MB of that. The
rest is the transitive closure of what a Widgets GUI genuinely reaches, and the
linker has already discarded the overwhelming majority of Qt: `Qt6Network`,
`Qt6Sql`, `Qt6DBus`, `Qt6PrintSupport` and `Qt6Test` are all built and installed,
and none of them are linked. Module-level subsetting is automatic and is not a
lever.

One caveat on `-static-runtime`, worth knowing if this is ever rebuilt: Qt does
not record `CMAKE_MSVC_RUNTIME_LIBRARY` in its CMake cache, so grepping the cache
proves nothing about whether the flag took. Check the generated build rules
instead -- `build-static/build.ninja` should show `-MT` on essentially every
`FLAGS` line. (`-MDd` occurrences under `config.tests/` are throwaway
feature-detection probes and do not matter.)

The real test is running it on a machine that has neither Qt nor the VC++
redistributable installed.

## If the Qt build fails

Neither of the two failure modes below occurred on the 2026-07-15 run. They are
kept on record because both would surface well into a long compile rather than at
configure time, and a future compiler or Qt update could revive either.

**Compiler conformance errors inside Qt's own sources** -- errors in files under
`qtbase/src/`, in code that was never touched. That would be a compiler
incompatibility rather than a mistake in the setup. The fix is to build Qt with
the compiler Qt 6.8.3 was actually tested against:

1. Visual Studio Installer > Modify on Visual Studio Community 2026 >
   Individual components > check **MSVC v143 - VS 2022 C++ x64/x86 build tools
   (Latest)**. This adds a second `cl.exe` under the existing VS 2026
   installation; it does not install a second Visual Studio.
2. Start a fresh command prompt and select that toolset when setting up the
   environment, then confirm `cl` reports 14.4x rather than 14.51:

        "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" -vcvars_ver=14.4
        cl

3. Delete `build-static` and redo step 2 from the top. A half-built tree carries
   the old compiler in its cache and will not switch cleanly.

The app itself can stay on 14.51 in that case. Linking v143-built static
libraries into a v145-built exe is the supported direction of MSVC's binary
compatibility guarantee -- the newer toolset's CRT carries the older symbols, not
the reverse. If it does cause trouble, point CLion's toolchain at v143 as well
(Settings > Build, Execution, Deployment > Toolchains > Toolset: 14.4x) so both
halves match.

**CMake 4 rejecting an old `cmake_minimum_required`.** CMake 4.1 dropped
compatibility with projects declaring less than 3.5, which can surface in bundled
third-party code. This one cannot bite qtbase as it stands: the lowest minimum
anywhere in the tree is 3.16. Should it ever appear, the escape hatch is to add
`-DCMAKE_POLICY_VERSION_MINIMUM=3.5` to the configure line, passed through as
`-- -DCMAKE_POLICY_VERSION_MINIMUM=3.5` after the other `configure` arguments.

Copyright Ben Paul Wise. All Rights Reserved.
