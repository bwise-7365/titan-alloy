//
// M2 acceptance program, extended in M4 with the argument-parsing bootstrap.
//
// The shape of main() below is the one the library intends: declare the
// arguments, hand them to App, and bail out before building any UI if the
// command line already said what should happen. Because App parses BEFORE it
// constructs the terminal, `hello --help` prints to an ordinary stdout and
// works under a pipe - where curses would refuse to start at all.
//
#include <chrono>
#include <cstdio>
#include <string>

#include "modcurses/app.hpp"
#include "modcurses/args.hpp"
#include "modcurses/widgets.hpp"

using namespace modcurses;

namespace {

// A one-line footer showing the shortcut hints. StatusBar proper is M3; this
// stays hand-rolled to keep the M2 example honest about what it needed.
class Footer : public Widget {
public:
    Footer() { style = Style{}.with_fg(Color::Black).with_bg(Color::BrightBlack); }
    [[nodiscard]] SizeReq height_req() const override { return SizeReq::fixed(1); }

protected:
    void paint(Canvas& c) override {
        c.fill(Glyph{U' ', style});
        c.print({1, 0}, "Tab / Shift-Tab move focus  |  Enter or Space presses  |  click anything",
                style);
    }
};

}  // namespace

int main(int argc, char** argv) {
    try {
        ArgParser args{"hello", "0.1.0", "modcurses M2/M4 demo"};
        auto& who = args.positional<std::string>("name", "who to greet").default_value("world");
        auto& interval =
            args.option<int>('i', "interval", "clock tick in milliseconds").default_value(1000);
        auto& palette = args.flag('p', "palette", "apply the built-in DawnBringer-16 palette");
        interval.validate([](const int& ms, std::string& why) {
            if (ms >= 10 && ms <= 60000) return true;
            why = "must be between 10 and 60000 ms, got " + std::to_string(ms);
            return false;
        });

        App app{argc, argv, args, AppInfo{"hello", "0.1.0", "modcurses M2/M4 demo"}};
        if (app.should_exit()) return app.exit_code();

        // Only past this line does a terminal exist.
        if (palette.value() && !app.palette().apply_dawnbringer16())
            // Not fatal: plenty of terminals refuse to redefine their colours.
            app.palette().set(Color::BrightWhite, Rgb{0xde, 0xee, 0xd6});

        auto& root = app.make_root<VBox>();
        auto& title = root.emplace_child<Titlebar>("modcurses hello", "M4");

        auto& body = root.emplace_child<VBox>();
        auto& clicks =
            body.emplace_child<Label>("Nothing pressed yet.", Align::Center);
        auto& uptime = body.emplace_child<Label>("", Align::Center);
        body.emplace_child<Widget>();  // spacer; the default SizeReq expands

        auto& buttons = root.emplace_child<HBox>();
        // One line tall. The plain Widget spacers below expand on both axes,
        // so without this the row would claim a share of the leftover height
        // and float up off the bottom of the window.
        buttons.height_hint = SizeReq::fixed(1);
        buttons.emplace_child<Widget>();
        auto& greet = buttons.emplace_child<Button>("Greet");
        buttons.emplace_child<Widget>();
        auto& quit = buttons.emplace_child<Button>("Quit");
        buttons.emplace_child<Widget>();

        root.emplace_child<Footer>();

        int presses = 0;
        auto greeted = greet.pressed.connect([&] {
            ++presses;
            clicks.set_text("Greeted " + who.value() + " " + std::to_string(presses) +
                            (presses == 1 ? " time." : " times."));
            title.set_hint("M4 - " + std::to_string(presses));
        });
        auto quitted = quit.pressed.connect([&] { app.quit(0); });

        // The library's one animation primitive: a repeating timer that fires
        // inside the loop's wait. This is what Tetris gravity will use.
        int ticks = 0;
        auto tick = root.add_timer(std::chrono::milliseconds{interval.value()}, [&] {
            ++ticks;
            uptime.set_text("tick " + std::to_string(ticks));
        });

        greet.take_focus();
        return app.run();
    } catch (const TerminalError& e) {
        std::fprintf(stderr, "modcurses: %s\n", e.what());
        return 1;
    }
}
