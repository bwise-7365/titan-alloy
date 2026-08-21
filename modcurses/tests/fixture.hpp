#pragma once
//
// Shared test fixture: an App wired to a MockTerminal, plus the pumping
// helpers every widget test needs.
//
#include <memory>

#include "modcurses/app.hpp"
#include "modcurses/mock_terminal.hpp"

namespace modcurses::testing {

struct AppFixture {
    App app;
    MockTerminal* term;

    explicit AppFixture(Size s = {20, 8})
        : app(std::make_unique<MockTerminal>(s)),
          term(static_cast<MockTerminal*>(&app.terminal())) {}

    // Exactly n loop iterations. Use this when the count is the point.
    void pump(int n = 1) {
        for (int i = 0; i < n; ++i) app.pump_once();
    }

    // Runs the loop until the scripted events are drained AND the frame they
    // produced has been painted.
    //
    // Worth understanding rather than working around: the loop paints BEFORE
    // it waits for input, so the screen reflecting a key press only reaches
    // the terminal on the NEXT iteration. That is right for a real terminal -
    // the frame is always on screen before the loop blocks - but it means a
    // test feeding N keys needs N+1 pumps to observe the result. Prefer this
    // over counting pumps whenever the assertion is about what is on screen.
    void sync(int limit = 64) {
        for (int i = 0; i < limit; ++i) {
            const bool pending = term->pending_events() > 0;
            const bool dirty = app.loop().frame_dirty() || app.loop().layout_invalid();
            if (!pending && !dirty) return;
            if (!app.pump_once()) return;  // the loop quit
        }
    }
};

}  // namespace modcurses::testing
