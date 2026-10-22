# Copyright Ben Paul Wise. All Rights Reserved.
add_library(abs_game STATIC
        AbsGame.cpp
        mcts.cpp
        negamax.cpp
        Prng.cpp
        utils.cpp
)

target_include_directories(abs_game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(abs_game PUBLIC cxx_std_20)

# ── MB2 memory tracking ──────────────────────────────────────────────────────────
#
# MB2 (panj/abzar/libsrc/mb2) works by REPLACING the global operator new / operator
# delete, so anything that links it has its allocation behaviour changed program-wide.
# That is why it is confined to its own `mem_track` target instead of going into
# abs_game: only the headless bench drivers link mem_track, never the Qt GUIs. The
# reasons the GUIs must not are in absgame/MemTrack.h.
#
# Tracking is always BUILT IN and chosen at run time with the drivers' --mb flag, so
# checking for a leak never means reconfiguring and rebuilding. Level 0 leaves MB2
# dormant on its untracked malloc/free fast path.
#
# MB2 is built FROM SOURCE rather than linked as abzar's prebuilt mb2.lib, so the
# tracker's compile-time settings are whatever the source in MB2_ROOT currently says
# (notably CheckSizeP, whose per-allocation tree walk is O(live blocks) and makes a
# search unrunnable when true), and so a Debian build has a route at all.
#
# NOTE: this makes an abzar checkout a hard build dependency of the two bench drivers.
# MB2_ROOT defaults to abzar's location alongside this repo; point it elsewhere with
#   cmake -DMB2_ROOT=/path/to/abzar ...
#
# The default is applied AFTER the cache declaration rather than as its initial value,
# because `set(... CACHE ...)` does not overwrite an entry that already exists: a cache
# carrying an empty MB2_ROOT from an earlier configure would otherwise keep it, and the
# source paths below would silently resolve to "/libsrc/mb2/mb2.cpp".
set(MB2_ROOT "" CACHE PATH "Path to the abzar checkout providing libsrc/mb2")
if(NOT MB2_ROOT)
    set(MB2_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../panj/abzar")
endif()
get_filename_component(MB2_ROOT "${MB2_ROOT}" ABSOLUTE)

# mb2.cpp needs Splay::STree (libsrc/splay.*), and splay.h includes abzar.h, whose
# implementation is abzar.cpp. Those three are the minimal set. If the link ever
# reports undefined abzar symbols, name the extra libsrc/*.cpp files in
# MB2_EXTRA_SOURCES rather than pulling in abzar's own CMake project, which also
# builds demos and tests this project has no use for.
set(MB2_EXTRA_SOURCES "" CACHE STRING
    "Additional abzar libsrc/*.cpp files needed to link MB2")

set(_mb2Sources
    ${MB2_ROOT}/libsrc/mb2/mb2.cpp
    ${MB2_ROOT}/libsrc/splay.cpp
    ${MB2_ROOT}/libsrc/abzar.cpp
    ${MB2_EXTRA_SOURCES}
)
foreach(_src IN LISTS _mb2Sources)
    if(NOT EXISTS ${_src})
        message(FATAL_ERROR
            "MB2 source not found:\n"
            "    ${_src}\n"
            "MB2_ROOT is currently \"${MB2_ROOT}\".\n"
            "The bench drivers link the MB2 memory tracker; point CMake at an abzar\n"
            "checkout with -DMB2_ROOT=<path> (the directory containing libsrc/mb2).")
    endif()
endforeach()

add_library(mb2_src STATIC ${_mb2Sources})
target_include_directories(mb2_src PUBLIC
    ${MB2_ROOT}/libsrc
    ${MB2_ROOT}/libsrc/mb2
)
target_compile_features(mb2_src PUBLIC cxx_std_20)

# MB2 reports a suspected double delete through assert(). A Release build defines
# NDEBUG, which would compile every one of those checks away and leave the tracker
# quietly doing half its job. Assertions are forced back on for MB2's own sources so a
# double delete aborts loudly instead of passing unnoticed. This is why the bench
# drivers abort rather than return non-zero on that particular fault.
target_compile_options(mb2_src PRIVATE
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/UNDEBUG>
    $<$<AND:$<CONFIG:Release>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-UNDEBUG>
)

# MB2 guards its tracker state with a std::mutex and a thread_local, and the bench
# drivers run games on std::threads. Both need -pthread on Debian; on Windows this
# resolves to nothing. PUBLIC so the drivers inherit it through mem_track.
find_package(Threads REQUIRED)
target_link_libraries(mb2_src PUBLIC Threads::Threads)

add_library(mem_track STATIC MemTrack.cpp)
target_include_directories(mem_track PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(mem_track PUBLIC cxx_std_20)
target_link_libraries(mem_track PUBLIC mb2_src Threads::Threads)

message(STATUS "MB2 memory tracking: built in, selected at run time (--mb); "
               "source ${MB2_ROOT}")

# Copyright Ben Paul Wise. All Rights Reserved.
