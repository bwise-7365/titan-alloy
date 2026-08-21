#include <algorithm>
#include <limits>

#include "modcurses/utf8.hpp"
#include "modcurses/widgets.hpp"

namespace modcurses {
namespace {

int text_width(std::u32string_view s) {
    int w = 0;
    for (char32_t c : s) w += col_width(c);
    return w;
}

Style merge(Style base, const Style& over) {
    base.traits = static_cast<std::uint8_t>(base.traits | over.traits);
    if (over.fg != Color::Default) base.fg = over.fg;
    if (over.bg != Color::Default) base.bg = over.bg;
    return base;
}

}  // namespace

// ---------------------------------------------------------------- ListView

ListView::ListView() { focus_policy = FocusPolicy::Strong; }

void ListView::set_items(std::vector<std::string> items) {
    items_.clear();
    items_.reserve(items.size());
    for (auto& s : items) items_.push_back(utf8_decode(s));
    selected_ = items_.empty() ? 0 : std::min(selected_, item_count() - 1);
    scroll_ = 0;
    invalidate_layout();
    emit_scrolled();
}

void ListView::add_item(std::string item) {
    items_.push_back(utf8_decode(item));
    invalidate_layout();
    emit_scrolled();
}

void ListView::clear_items() {
    items_.clear();
    selected_ = 0;
    scroll_ = 0;
    invalidate_layout();
    emit_scrolled();
}

std::string ListView::item(int index) const {
    if (index < 0 || index >= item_count()) return {};
    return utf8_encode(items_[static_cast<std::size_t>(index)]);
}

std::u32string ListView::row_text(int index) const {
    if (index < 0 || index >= item_count()) return {};
    return items_[static_cast<std::size_t>(index)];
}

void ListView::set_selected(int index) {
    if (items_.empty()) return;
    const int clamped = std::clamp(index, 0, item_count() - 1);
    if (clamped == selected_) return;
    selected_ = clamped;
    ensure_selection_visible();
    invalidate();
    selection_changed.emit(selected_);
}

void ListView::activate_selected() {
    if (items_.empty()) return;
    activated.emit(selected_);
}

void ListView::scroll_to(int first_visible) {
    const int max_scroll = std::max(0, item_count() - visible_rows());
    const int clamped = std::clamp(first_visible, 0, max_scroll);
    if (clamped == scroll_) return;
    scroll_ = clamped;
    invalidate();
    emit_scrolled();
}

void ListView::ensure_selection_visible() {
    const int rows = visible_rows();
    if (rows <= 0) return;
    int wanted = scroll_;
    if (selected_ < wanted) wanted = selected_;
    if (selected_ > wanted + rows - 1) wanted = selected_ - rows + 1;
    scroll_to(wanted);
}

void ListView::emit_scrolled() { scrolled.emit(scroll_, item_count()); }

void ListView::on_geometry(Rect /*old_rect*/, Rect /*new_rect*/) {
    ensure_selection_visible();
    emit_scrolled();
}

SizeReq ListView::width_req() const {
    int widest = 0;
    for (const auto& s : items_) widest = std::max(widest, text_width(s));
    return SizeReq{0, widest, std::numeric_limits<int>::max(), 1};
}

SizeReq ListView::height_req() const {
    return SizeReq{1, std::max(1, item_count()), std::numeric_limits<int>::max(), 1};
}

void ListView::paint(Canvas& c) {
    c.fill(Glyph{U' ', style});
    const int rows = c.size().height;
    if (rows <= 0 || c.size().width <= 0) return;

    for (int y = 0; y < rows; ++y) {
        const int index = scroll_ + y;
        if (index >= item_count()) break;

        // The selection is only highlighted while the list has focus, so a
        // page with several lists does not look like it has several cursors.
        const bool is_selected = index == selected_;
        const Style row = is_selected && has_focus() ? merge(style, selected_style) : style;
        if (is_selected && has_focus()) c.fill(Rect{{0, y}, {c.size().width, 1}}, Glyph{U' ', row});
        c.print({0, y}, row_text(index), row);
        if (is_selected && !has_focus()) c.put({0, y}, U'>', style);
    }
}

bool ListView::on_key(const KeyEvent& ev) {
    if (ev.mods.ctrl || ev.mods.alt) return false;
    const int rows = std::max(1, visible_rows());

    switch (ev.key) {
        case Key::Up: set_selected(selected_ - 1); return true;
        case Key::Down: set_selected(selected_ + 1); return true;
        case Key::PageUp: set_selected(selected_ - rows); return true;
        case Key::PageDown: set_selected(selected_ + rows); return true;
        case Key::Home: set_selected(0); return true;
        case Key::End: set_selected(item_count() - 1); return true;
        case Key::Enter: activate_selected(); return true;
        default: return false;
    }
}

bool ListView::on_mouse(const MouseEvent& ev) {
    if (ev.button == MouseEvent::Button::WheelUp) {
        scroll_to(scroll_ - 1);
        return true;
    }
    if (ev.button == MouseEvent::Button::WheelDown) {
        scroll_to(scroll_ + 1);
        return true;
    }
    if (ev.button != MouseEvent::Button::Left) return false;

    const int index = scroll_ + ev.pos.y;
    if (index < 0 || index >= item_count()) return false;

    if (ev.action == MouseEvent::Action::DoubleClick) {
        set_selected(index);
        activate_selected();
        return true;
    }
    if (ev.action != MouseEvent::Action::Press) return false;
    // A click on the already-selected row activates it: double-click reporting
    // is unreliable across backends, so this is the dependable path.
    const bool reselect = index == selected_;
    set_selected(index);
    if (reselect) activate_selected();
    return true;
}

// -------------------------------------------------------------------- Menu

Menu::Menu() {
    activated_conn_ = activated.connect([this](int index) {
        if (index < 0 || index >= static_cast<int>(callbacks_.size())) return;
        // Copy before calling: the callback may rebuild the menu.
        auto fn = callbacks_[static_cast<std::size_t>(index)];
        if (fn) fn();
    });
}

void Menu::add_entry(std::string label, std::function<void()> on_select) {
    add_item(std::move(label));
    callbacks_.push_back(std::move(on_select));
}

void Menu::clear_entries() {
    clear_items();
    callbacks_.clear();
}

}  // namespace modcurses
