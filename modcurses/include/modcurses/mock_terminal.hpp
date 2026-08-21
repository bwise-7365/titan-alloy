#pragma once
//
// modcurses/mock_terminal.hpp - headless TerminalIO for tests and examples.
//
// An in-memory grid plus a scripted event feed. This is the single biggest
// testability win over CPPurses: layout, rendering, focus and key routing are
// all testable with no TTY and no curses linked in at all.
//
#include <deque>
#include <optional>
#include <string>
#include <vector>

#include "modcurses/terminal.hpp"

namespace modcurses {

class MockTerminal final : public TerminalIO {
public:
    explicit MockTerminal(Size s = {80, 24});

    // ---- TerminalIO ----
    Size size() override { return size_; }
    std::optional<Event> poll_event(std::optional<std::chrono::milliseconds> timeout) override;
    void draw_run(Point origin, std::u32string_view run, Style style) override;
    void set_cursor(std::optional<Point> pos) override;
    void flush() override;
    bool define_color(Color slot, Rgb value) override;
    [[nodiscard]] bool can_define_colors() override { return colors_definable_; }
    void beep() override;

    // ---- test controls ----
    void feed(Event e);                    // queue one event
    void feed(const KeyEvent& e) { feed(Event{e}); }
    void feed_text(std::u32string_view s);  // one Key::Char event per codepoint
    void set_size(Size s);                  // resizes the grid AND feeds a ResizeEvent
    void set_colors_definable(bool v) { colors_definable_ = v; }

    // ---- inspection ----
    [[nodiscard]] const Cell& cell_at(Point p) const;
    [[nodiscard]] std::string row_text(int y) const;   // UTF-8 of one row
    [[nodiscard]] std::string screen_text() const;     // rows joined with '\n'
    [[nodiscard]] std::optional<Point> cursor() const { return cursor_; }
    [[nodiscard]] int flush_count() const { return flush_count_; }
    [[nodiscard]] int draw_run_count() const { return draw_run_count_; }
    [[nodiscard]] int beep_count() const { return beep_count_; }
    [[nodiscard]] std::size_t pending_events() const { return queue_.size(); }
    void reset_counters();

    // Number of poll_event calls that ran off the end of the script.
    [[nodiscard]] int starved_polls() const { return starved_polls_; }

private:
    void resize_grid(Size s);

    [[nodiscard]] std::size_t index(Point p) const {
        return static_cast<std::size_t>(p.y) * static_cast<std::size_t>(size_.width) +
               static_cast<std::size_t>(p.x);
    }

    Size size_;
    std::vector<Cell> grid_;
    Cell outside_{};  // returned for out-of-bounds cell_at()
    std::deque<Event> queue_;
    std::optional<Point> cursor_;
    std::vector<Rgb> defined_colors_{static_cast<std::size_t>(kColorCount), Rgb{}};
    bool colors_definable_ = true;
    int flush_count_ = 0;
    int draw_run_count_ = 0;
    int beep_count_ = 0;
    int starved_polls_ = 0;
};

}  // namespace modcurses
