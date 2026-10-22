#include <algorithm>

#include "modcurses/text.hpp"
#include "modcurses/utf8.hpp"

namespace modcurses {

const char* to_string(EditAction a) {
    switch (a) {
        case EditAction::None: return "None";
        case EditAction::InsertNewline: return "InsertNewline";
        case EditAction::Backspace: return "Backspace";
        case EditAction::DeleteForward: return "DeleteForward";
        case EditAction::DeleteLine: return "DeleteLine";
        case EditAction::MoveLeft: return "MoveLeft";
        case EditAction::MoveRight: return "MoveRight";
        case EditAction::MoveUp: return "MoveUp";
        case EditAction::MoveDown: return "MoveDown";
        case EditAction::MoveLineStart: return "MoveLineStart";
        case EditAction::MoveLineEnd: return "MoveLineEnd";
        case EditAction::MoveBufferStart: return "MoveBufferStart";
        case EditAction::MoveBufferEnd: return "MoveBufferEnd";
        case EditAction::MoveWordLeft: return "MoveWordLeft";
        case EditAction::MoveWordRight: return "MoveWordRight";
        case EditAction::PageUp: return "PageUp";
        case EditAction::PageDown: return "PageDown";
    }
    return "?";
}

// ----------------------------------------------------------------- Keymap

void Keymap::bind(KeyEvent match, EditAction action) {
    for (auto& b : bindings_) {
        if (b.match == match) {
            b.action = action;
            return;
        }
    }
    bindings_.push_back(KeyBinding{match, action});
}

bool Keymap::unbind(KeyEvent match) {
    const auto it = std::find_if(bindings_.begin(), bindings_.end(),
                                 [&](const KeyBinding& b) { return b.match == match; });
    if (it == bindings_.end()) return false;
    bindings_.erase(it);
    return true;
}

EditAction Keymap::lookup(const KeyEvent& ev) const {
    for (const auto& b : bindings_)
        if (b.match == ev) return b.action;
    return EditAction::None;
}

Keymap Keymap::basic() {
    Keymap k;
    constexpr Mods ctrl{true, false, false};

    k.bind(key_ev(Key::Left), EditAction::MoveLeft);
    k.bind(key_ev(Key::Right), EditAction::MoveRight);
    k.bind(key_ev(Key::Up), EditAction::MoveUp);
    k.bind(key_ev(Key::Down), EditAction::MoveDown);
    k.bind(key_ev(Key::Home), EditAction::MoveLineStart);
    k.bind(key_ev(Key::End), EditAction::MoveLineEnd);
    k.bind(key_ev(Key::PageUp), EditAction::PageUp);
    k.bind(key_ev(Key::PageDown), EditAction::PageDown);
    k.bind(key_ev(Key::Enter), EditAction::InsertNewline);
    k.bind(key_ev(Key::Backspace), EditAction::Backspace);
    k.bind(key_ev(Key::Delete), EditAction::DeleteForward);

    // Chords the design calls robust across terminals: plain keys and
    // Ctrl+key. Nothing here relies on Ctrl+Shift+arrow being distinguishable.
    k.bind(key_ev(Key::Left, ctrl), EditAction::MoveWordLeft);
    k.bind(key_ev(Key::Right, ctrl), EditAction::MoveWordRight);
    k.bind(key_ev(Key::Home, ctrl), EditAction::MoveBufferStart);
    k.bind(key_ev(Key::End, ctrl), EditAction::MoveBufferEnd);
    return k;
}

Keymap Keymap::nano() {
    Keymap k = basic();
    k.bind(ctrl_ev(U'a'), EditAction::MoveLineStart);
    k.bind(ctrl_ev(U'e'), EditAction::MoveLineEnd);
    k.bind(ctrl_ev(U'y'), EditAction::PageUp);
    k.bind(ctrl_ev(U'v'), EditAction::PageDown);
    k.bind(ctrl_ev(U'k'), EditAction::DeleteLine);
    k.bind(ctrl_ev(U'd'), EditAction::DeleteForward);
    return k;
}

Keymap Keymap::emacs() {
    Keymap k = basic();
    k.bind(ctrl_ev(U'b'), EditAction::MoveLeft);
    k.bind(ctrl_ev(U'f'), EditAction::MoveRight);
    k.bind(ctrl_ev(U'p'), EditAction::MoveUp);
    k.bind(ctrl_ev(U'n'), EditAction::MoveDown);
    k.bind(ctrl_ev(U'a'), EditAction::MoveLineStart);
    k.bind(ctrl_ev(U'e'), EditAction::MoveLineEnd);
    k.bind(ctrl_ev(U'd'), EditAction::DeleteForward);
    k.bind(ctrl_ev(U'k'), EditAction::DeleteLine);
    k.bind(ctrl_ev(U'v'), EditAction::PageDown);
    k.bind(alt_ev(U'v'), EditAction::PageUp);
    k.bind(alt_ev(U'<'), EditAction::MoveBufferStart);
    k.bind(alt_ev(U'>'), EditAction::MoveBufferEnd);
    return k;
}

// --------------------------------------------------------------- TextArea

TextArea::TextArea(TextBuffer& buffer) : buffer_(&buffer) {
    focus_policy = FocusPolicy::Strong;
    rewatch();
}

void TextArea::set_buffer(TextBuffer& buffer) {
    buffer_ = &buffer;
    scroll_ = Point{};
    rewatch();
    invalidate();
    emit_scrolled();
}

void TextArea::rewatch() {
    changed_conn_ = buffer_->changed.connect([this] {
        ensure_cursor_visible();
        invalidate();
        emit_scrolled();
    });
    cursor_conn_ = buffer_->cursor_moved.connect([this] {
        ensure_cursor_visible();
        invalidate();
    });
}

void TextArea::emit_scrolled() { scrolled.emit(scroll_.y, buffer_->line_count()); }

// Tab expansion lives here and only here, so that M6's double-width glyphs
// land in the same two functions rather than being scattered through paint.
int TextArea::display_column(int line_index, int col) const {
    const std::u32string& s = buffer_->line(line_index);
    const int n = std::min(col, static_cast<int>(s.size()));
    int display = 0;
    for (int i = 0; i < n; ++i) {
        if (s[static_cast<std::size_t>(i)] == U'\t')
            display += std::max(1, tab_width) - (display % std::max(1, tab_width));
        else
            display += col_width(s[static_cast<std::size_t>(i)]);
    }
    return display;
}

int TextArea::column_at_display(int line_index, int target) const {
    const std::u32string& s = buffer_->line(line_index);
    int display = 0;
    for (int i = 0; i < static_cast<int>(s.size()); ++i) {
        if (display >= target) return i;
        if (s[static_cast<std::size_t>(i)] == U'\t')
            display += std::max(1, tab_width) - (display % std::max(1, tab_width));
        else
            display += col_width(s[static_cast<std::size_t>(i)]);
    }
    return static_cast<int>(s.size());
}

void TextArea::scroll_to(Point p) {
    const int max_y = std::max(0, buffer_->line_count() - std::max(1, size().height));
    const Point clamped{std::max(0, p.x), std::clamp(p.y, 0, max_y)};
    if (clamped == scroll_) return;
    scroll_ = clamped;
    invalidate();
    emit_scrolled();
}

void TextArea::ensure_cursor_visible() {
    const Size s = size();
    if (s.width <= 0 || s.height <= 0) return;

    const auto cur = buffer_->cursor();
    Point wanted = scroll_;
    if (cur.line < wanted.y) wanted.y = cur.line;
    if (cur.line > wanted.y + s.height - 1) wanted.y = cur.line - s.height + 1;

    const int cx = display_column(cur.line, cur.col);
    if (cx < wanted.x) wanted.x = cx;
    if (cx > wanted.x + s.width - 1) wanted.x = cx - s.width + 1;

    wanted.x = std::max(0, wanted.x);
    wanted.y = std::max(0, wanted.y);
    if (wanted == scroll_) return;
    scroll_ = wanted;
    emit_scrolled();
}

void TextArea::on_geometry(Rect /*old_rect*/, Rect /*new_rect*/) {
    ensure_cursor_visible();
    emit_scrolled();
}

void TextArea::paint(Canvas& c) {
    c.fill(Glyph{U' ', style});
    const int rows = c.size().height;
    if (rows <= 0 || c.size().width <= 0) return;

    const int tab = std::max(1, tab_width);
    std::u32string expanded;
    for (int y = 0; y < rows; ++y) {
        const int index = scroll_.y + y;
        if (index >= buffer_->line_count()) break;

        expanded.clear();
        int display = 0;
        for (char32_t ch : buffer_->line(index)) {
            if (ch == U'\t') {
                const int advance = tab - (display % tab);
                expanded.append(static_cast<std::size_t>(advance), U' ');
                display += advance;
            } else {
                expanded.push_back(ch);
                display += col_width(ch);
            }
        }
        if (scroll_.x < static_cast<int>(expanded.size()))
            c.print({0, y}, std::u32string_view{expanded}.substr(static_cast<std::size_t>(scroll_.x)),
                    style);
    }
}

std::optional<Point> TextArea::cursor() const {
    const Size s = size();
    if (s.width <= 0 || s.height <= 0) return std::nullopt;
    const auto cur = buffer_->cursor();
    const Point p{display_column(cur.line, cur.col) - scroll_.x, cur.line - scroll_.y};
    if (p.x < 0 || p.y < 0 || p.x >= s.width || p.y >= s.height) return std::nullopt;
    return p;
}

bool TextArea::apply(EditAction action) {
    TextBuffer& b = *buffer_;
    const bool edits = action == EditAction::InsertNewline || action == EditAction::Backspace ||
                       action == EditAction::DeleteForward || action == EditAction::DeleteLine;
    // A read-only view does NOT consume an edit key - it declines it, so the
    // key bubbles to whatever else might want it.
    //
    // This used to consume, on the theory that an edit attempt should not be
    // mistaken for a shortcut. That was backwards, and it froze a real
    // application: a read-only TextArea used as a help pager swallowed every
    // key, so once it had focus there was no way to dismiss it or even quit.
    // A widget that cannot act on a key has not handled it. Read-only means
    // "I do not edit", and a pager is expected to let 'q' through.
    if (edits && read_only) return false;

    const int page = std::max(1, visible_lines() - 1);
    switch (action) {
        case EditAction::None: return false;
        case EditAction::InsertNewline: b.insert_newline(); break;
        case EditAction::Backspace: b.backspace(); break;
        case EditAction::DeleteForward: b.erase_forward(); break;
        case EditAction::DeleteLine: b.erase_line(); break;
        case EditAction::MoveLeft: b.move_left(); break;
        case EditAction::MoveRight: b.move_right(); break;
        case EditAction::MoveUp: b.move_up(); break;
        case EditAction::MoveDown: b.move_down(); break;
        case EditAction::MoveLineStart: b.move_line_start(); break;
        case EditAction::MoveLineEnd: b.move_line_end(); break;
        case EditAction::MoveBufferStart: b.move_buffer_start(); break;
        case EditAction::MoveBufferEnd: b.move_buffer_end(); break;
        case EditAction::MoveWordLeft: b.move_word_left(); break;
        case EditAction::MoveWordRight: b.move_word_right(); break;
        case EditAction::PageUp:
            b.set_cursor(TextBuffer::Cursor{b.cursor().line - page, b.cursor().col});
            break;
        case EditAction::PageDown:
            b.set_cursor(TextBuffer::Cursor{b.cursor().line + page, b.cursor().col});
            break;
    }
    ensure_cursor_visible();
    invalidate();
    return true;
}

bool TextArea::on_key(const KeyEvent& ev) {
    if (const EditAction action = keymap.lookup(ev); action != EditAction::None)
        return apply(action);

    // Tab indents rather than moving focus. Dispatch consults widgets before
    // the focus chain precisely so this is possible; Shift-Tab still leaves.
    // A read-only view has nothing to indent, so Tab stays focus traversal.
    if (ev.key == Key::Tab && capture_tab && !read_only && !ev.mods.ctrl && !ev.mods.alt) {
        buffer_->insert(U'\t');
        ensure_cursor_visible();
        return true;
    }

    if (ev.key != Key::Char || ev.mods.ctrl || ev.mods.alt) return false;
    if (ev.text < U' ' && ev.text != U'\t') return false;
    // Text this view will not insert is not text it has handled: decline it so
    // it reaches whatever else might want it. See the note in apply().
    if (read_only) return false;

    buffer_->insert(ev.text);
    ensure_cursor_visible();
    return true;
}

bool TextArea::on_mouse(const MouseEvent& ev) {
    if (ev.button == MouseEvent::Button::WheelUp) {
        scroll_to({scroll_.x, scroll_.y - 3});
        return true;
    }
    if (ev.button == MouseEvent::Button::WheelDown) {
        scroll_to({scroll_.x, scroll_.y + 3});
        return true;
    }
    if (ev.action != MouseEvent::Action::Press || ev.button != MouseEvent::Button::Left)
        return false;

    const int line = std::clamp(scroll_.y + ev.pos.y, 0, buffer_->line_count() - 1);
    buffer_->set_cursor(TextBuffer::Cursor{line, column_at_display(line, scroll_.x + ev.pos.x)});
    return true;
}

}  // namespace modcurses
