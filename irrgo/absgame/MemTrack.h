// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include <cstdint>
#include <string>

// A thin front end to the MB2 memory tracker (panj/abzar/libsrc/mb2), matching the
// --mb flag of abzar's own demo.cpp.
//
// MB2 works by REPLACING the global operator new / operator delete, so linking it
// changes allocation behaviour for the whole program. That is why this shim lives in
// its own tiny `mem_track` library rather than in abs_game: only the headless bench
// drivers link it. The Qt GUIs must not, for two reasons --
//
//   * Qt allocates heavily during QApplication construction, before main() could call
//     start(). At tracking level >= 2 every one of those blocks is unknown to the
//     tracker when it is later freed, so MB2 counts a double delete, declines to free
//     it (a genuine leak), and -- because MB2::Manager::ThrowDD defaults to true --
//     asserts. A Qt program would abort during normal teardown.
//   * MB2 takes one global mutex per allocation on the tracked path, which would
//     serialise a search that allocates millions of times.
//
// Tracking is always compiled in and chosen at run time: level 0 leaves MB2 dormant on
// its untracked malloc/free fast path, so a normal run costs nothing and no rebuild is
// needed to go looking for a leak.
namespace AbsGame {

class MemTrack {
public:
    // Begin tracking at `level` (the levels abzar's demo.cpp documents for --mb):
    //   0  no tracking, the default
    //   1  overall count of lost blocks
    //   2  level 1, plus the identity of lost blocks and total bytes leaked. This is
    //      the level worth using.
    //   3  level 2, plus a line per allocation. Enormous output.
    //   4  level 3, plus extremely verbose debugging. For debugging MB2 itself.
    //
    // `firstSuspectBlock` is MB2's FSMB: the block ID at whose ALLOCATION the tracker
    // calls MB2::Manager::pause(), which exists solely to carry a debugger breakpoint.
    // The workflow it serves is two runs -- the first at level 2 reports "First leaked
    // number and size: <id> <bytes>", and the second passes that <id> here and breaks in
    // pause() to catch the leaked block being allocated, with the call stack intact.
    // 0 means no suspect block.
    //
    // As in demo.cpp, MB2 is only engaged when level > 0, so a level-0 run is exactly
    // the program without a tracker rather than the program with a dormant one.
    //
    // Throws std::invalid_argument if level > 4, or if a suspect block is named at a
    // level below 2 (where MB2 keeps no block identities to match it against).
    static void start(unsigned level, std::uint64_t firstSuspectBlock = 0);

    // Print MB2's report and reset its state. Safe to call when start() was never
    // called or was called with level 0.
    static void stop();

    // The --mb usage text, so the drivers cannot drift from each other or from what
    // start() accepts.
    static std::string levelHelp();
};

}  // namespace AbsGame
// Copyright Ben Paul Wise. All Rights Reserved.
