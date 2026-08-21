#pragma once
//
// modcurses/terminal.hpp - the backend interface.
//
// PUBLIC HEADER: no curses. This is the firewall (lesson 5). Exactly one
// translation unit in the whole library includes a curses header, and it
// implements this interface behind make_curses_terminal().
//
#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "modcurses/core.hpp"
#include "modcurses/render.hpp"

namespace modcurses {

class TerminalError : public std::runtime_error {
public:
    explicit TerminalError(const std::string& what) : std::runtime_error(what) {}
};

class TerminalIO {
public:
    virtual ~TerminalIO() = default;

    [[nodiscard]] virtual Size size() = 0;

    // Blocks until input arrives, the timeout expires, or (with a zero
    // timeout) returns immediately. nullopt means "nothing happened".
    // A nullopt timeout waits forever.
    virtual std::optional<Event> poll_event(
        std::optional<std::chrono::milliseconds> timeout) = 0;

    // Writes a run of glyphs sharing one style, starting at an absolute
    // screen position. The run never wraps and never exceeds the screen.
    virtual void draw_run(Point origin, std::u32string_view run, Style style) = 0;

    virtual void set_cursor(std::optional<Point> pos) = 0;  // nullopt = hidden
    virtual void flush() = 0;
    virtual bool define_color(Color slot, Rgb value) = 0;
    [[nodiscard]] virtual bool can_define_colors() = 0;
    virtual void beep() = 0;
};

// Constructs the real curses terminal, running the contractual init sequence
// (env sanitisation, setlocale, initscr, raw/noecho/keypad, mouse, colours,
// size). Throws TerminalError if the terminal cannot be initialised.
// Destroying the returned object calls endwin(). At most one may exist.
[[nodiscard]] std::unique_ptr<TerminalIO> make_curses_terminal();

}  // namespace modcurses
