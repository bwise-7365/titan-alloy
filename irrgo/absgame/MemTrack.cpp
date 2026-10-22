// Copyright Ben Paul Wise. All Rights Reserved.
#include "MemTrack.h"

#include <stdexcept>
#include <string>

#include "mb2.h"

namespace AbsGame {

namespace {

// MB2::Manager::start() asserts on anything above this.
constexpr unsigned kMaxLevel = 4;

}  // namespace

void MemTrack::start(unsigned level, std::uint64_t firstSuspectBlock) {
    if (level > kMaxLevel) {
        throw std::invalid_argument("memory tracking level must be 0.." +
                                    std::to_string(kMaxLevel) + ", got " +
                                    std::to_string(level));
    }
    // A suspect block only means something once MB2 keeps block IDs, which starts at
    // level 2. Asking for one below that is a mistake in the command line worth saying
    // out loud, not something to accept and quietly never act on.
    if ((firstSuspectBlock > 0) && (level < 2)) {
        throw std::invalid_argument(
            "a first suspect block needs tracking level 2 or higher; level " +
            std::to_string(level) + " keeps no block identities");
    }
    // Engage MB2 only when there is tracking to do, as demo.cpp does: at level 0 the
    // run should be the program without a tracker, not the program with a dormant one.
    if (level > 0) {
        MB2::Manager::start(level);
        MB2::Manager::setFSMB(firstSuspectBlock);
    }
}

void MemTrack::stop() {
    MB2::Manager::stop();
}

std::string MemTrack::levelHelp() {
    // Wording follows abzar's demo.cpp --mb help so the two agree.
    return
        "    --mb <n> <m>  set memory block tracking, level and FSMB\n"
        "                  n = level:\n"
        "                    0: no tracking, the default\n"
        "                    1: overall count of lost blocks\n"
        "                    2: level 1, plus identity of lost blocks and bytes leaked\n"
        "                    3: level 2, plus display each new allocation\n"
        "                    4: level 3, plus extremely verbose debugging\n"
        "                  m = FSMB, the first suspect block id, or 0 for none.\n"
        "                    MB2::Manager::pause() is called when that block is\n"
        "                    allocated, to carry a debugger breakpoint. Needs n >= 2;\n"
        "                    take the id from a level-2 run's \"First leaked\" line.\n";
}

}  // namespace AbsGame
// Copyright Ben Paul Wise. All Rights Reserved.
