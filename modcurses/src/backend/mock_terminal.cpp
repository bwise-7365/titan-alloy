#include "modcurses/mock_terminal.hpp"

#include "modcurses/utf8.hpp"

namespace modcurses {

MockTerminal::MockTerminal(Size s) { resize_grid(s); }

void MockTerminal::resize_grid(Size s) {
    if (s.width < 0) s.width = 0;
    if (s.height < 0) s.height = 0;
    size_ = s;
    grid_.assign(static_cast<std::size_t>(s.width) * static_cast<std::size_t>(s.height), Cell{});
}

void MockTerminal::set_size(Size s) {
    resize_grid(s);
    // A real terminal announces a resize through the event stream; so does
    // this one. Construction does not, so a fresh mock starts with an empty
    // script.
    queue_.push_back(Event{ResizeEvent{size_}});
}

std::optional<Event> MockTerminal::poll_event(std::optional<std::chrono::milliseconds>) {
    // The mock never actually waits: an empty script is a timeout. Tests that
    // care about starvation can assert on starved_polls().
    if (queue_.empty()) {
        ++starved_polls_;
        return std::nullopt;
    }
    Event e = queue_.front();
    queue_.pop_front();
    return e;
}

void MockTerminal::draw_run(Point origin, std::u32string_view run, Style style) {
    ++draw_run_count_;
    int x = origin.x;
    for (char32_t c : run) {
        const Point p{x, origin.y};
        if (p.x >= 0 && p.y >= 0 && p.x < size_.width && p.y < size_.height)
            grid_[index(p)] = Cell{c, style};
        ++x;
    }
}

void MockTerminal::set_cursor(std::optional<Point> pos) { cursor_ = pos; }

void MockTerminal::flush() { ++flush_count_; }

bool MockTerminal::define_color(Color slot, Rgb value) {
    if (!colors_definable_) return false;
    const auto i = static_cast<std::size_t>(slot);
    if (i >= defined_colors_.size()) return false;
    defined_colors_[i] = value;
    return true;
}

void MockTerminal::beep() { ++beep_count_; }

void MockTerminal::feed(Event e) { queue_.push_back(std::move(e)); }

void MockTerminal::feed_text(std::u32string_view s) {
    for (char32_t c : s) feed(Event{KeyEvent{Key::Char, c, {}}});
}

const Cell& MockTerminal::cell_at(Point p) const {
    // Out-of-bounds reads answer with a blank cell rather than UB, so tests
    // can probe freely. (No function-local static: the library keeps zero
    // static state, test doubles included.)
    if (p.x < 0 || p.y < 0 || p.x >= size_.width || p.y >= size_.height) return outside_;
    return grid_[index(p)];
}

std::string MockTerminal::row_text(int y) const {
    if (y < 0 || y >= size_.height) return {};
    std::u32string row;
    row.reserve(static_cast<std::size_t>(size_.width));
    for (int x = 0; x < size_.width; ++x) row.push_back(grid_[index({x, y})].ch);
    return utf8_encode(row);
}

std::string MockTerminal::screen_text() const {
    std::string out;
    for (int y = 0; y < size_.height; ++y) {
        if (y > 0) out.push_back('\n');
        out += row_text(y);
    }
    return out;
}

void MockTerminal::reset_counters() {
    flush_count_ = 0;
    draw_run_count_ = 0;
    beep_count_ = 0;
    starved_polls_ = 0;
}

}  // namespace modcurses
