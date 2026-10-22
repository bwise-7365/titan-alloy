#include <algorithm>

#include "modcurses/utf8.hpp"
#include "modcurses/widgets.hpp"

namespace modcurses {

TextInput::TextInput(std::string text) : text_(utf8_decode(text)) {
    focus_policy = FocusPolicy::Strong;
    cursor_ = static_cast<int>(text_.size());
}

void TextInput::set_text(std::string text) {
    auto decoded = utf8_decode(text);
    if (decoded == text_) return;
    text_ = std::move(decoded);
    // The cursor lands at the END, exactly as it does when the field is
    // constructed with text. Clamping it instead left it at column 0 for a
    // field seeded from empty, so a caller pre-filling a prompt with a
    // suggested value got typing PREPENDED to it and backspace doing nothing
    // - which is never what replacing the whole content is meant to mean.
    cursor_ = static_cast<int>(text_.size());
    ensure_cursor_visible();
    invalidate();
    changed.emit(text_);
}

std::string TextInput::text() const { return utf8_encode(text_); }

void TextInput::set_placeholder(std::string text) {
    auto decoded = utf8_decode(text);
    if (decoded == placeholder_) return;
    placeholder_ = std::move(decoded);
    invalidate();
}

std::string TextInput::placeholder() const { return utf8_encode(placeholder_); }

void TextInput::clear() { set_text(""); }

void TextInput::set_cursor_col(int col) {
    const int clamped = std::clamp(col, 0, static_cast<int>(text_.size()));
    if (clamped == cursor_) return;
    cursor_ = clamped;
    ensure_cursor_visible();
    invalidate();
}

SizeReq TextInput::height_req() const { return SizeReq::fixed(1); }

void TextInput::on_geometry(Rect /*old_rect*/, Rect /*new_rect*/) { ensure_cursor_visible(); }

void TextInput::ensure_cursor_visible() {
    const int width = size().width;
    if (width <= 0) return;
    if (cursor_ < scroll_) scroll_ = cursor_;
    // The cursor is allowed to sit one past the last character, so the usable
    // span is width - 1 when it is at the end of a full line.
    if (cursor_ > scroll_ + width - 1) scroll_ = cursor_ - width + 1;
    scroll_ = std::max(0, scroll_);
}

void TextInput::emit_changed() {
    ensure_cursor_visible();
    invalidate();
    changed.emit(text_);
}

void TextInput::paint(Canvas& c) {
    c.fill(Glyph{U' ', style});
    if (c.size().height <= 0 || c.size().width <= 0) return;

    if (text_.empty() && !placeholder_.empty() && !has_focus()) {
        c.print({0, 0}, placeholder_, placeholder_style);
        return;
    }
    const auto start = static_cast<std::size_t>(std::min<int>(scroll_, static_cast<int>(text_.size())));
    c.print({0, 0}, std::u32string_view{text_}.substr(start), style);
}

std::optional<Point> TextInput::cursor() const {
    if (size().width <= 0) return std::nullopt;
    const int x = cursor_ - scroll_;
    if (x < 0 || x >= size().width) return std::nullopt;
    return Point{x, 0};
}

bool TextInput::on_key(const KeyEvent& ev) {
    if (ev.mods.alt) return false;

    switch (ev.key) {
        case Key::Left:
            if (cursor_ > 0) set_cursor_col(cursor_ - 1);
            return true;
        case Key::Right:
            if (cursor_ < static_cast<int>(text_.size())) set_cursor_col(cursor_ + 1);
            return true;
        case Key::Home:
            set_cursor_col(0);
            return true;
        case Key::End:
            set_cursor_col(static_cast<int>(text_.size()));
            return true;
        case Key::Enter:
            submitted.emit(text_);
            return true;
        case Key::Backspace:
            if (read_only) return false;  // declined, not swallowed - see TextArea::apply
            if (cursor_ == 0) return true;
            text_.erase(static_cast<std::size_t>(cursor_ - 1), 1);
            --cursor_;
            emit_changed();
            return true;
        case Key::Delete:
            if (read_only) return false;
            if (cursor_ >= static_cast<int>(text_.size())) return true;
            text_.erase(static_cast<std::size_t>(cursor_), 1);
            emit_changed();
            return true;
        default:
            break;
    }

    if (ev.key != Key::Char) return false;
    // Ctrl-letter chords belong to the application, not to a text field.
    if (ev.mods.ctrl) {
        if (ev.text == U'a') {
            set_cursor_col(0);
            return true;
        }
        if (ev.text == U'e') {
            set_cursor_col(static_cast<int>(text_.size()));
            return true;
        }
        return false;
    }
    if (read_only) return false;       // text it will not insert, it has not handled
    if (ev.text < U' ') return false;  // an unmapped control code is not text
    if (max_length > 0 && static_cast<int>(text_.size()) >= max_length) return true;

    text_.insert(static_cast<std::size_t>(cursor_), 1, ev.text);
    ++cursor_;
    emit_changed();
    return true;
}

bool TextInput::on_mouse(const MouseEvent& ev) {
    if (ev.action != MouseEvent::Action::Press || ev.button != MouseEvent::Button::Left)
        return false;
    set_cursor_col(scroll_ + ev.pos.x);
    return true;
}

}  // namespace modcurses
