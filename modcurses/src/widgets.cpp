#include "modcurses/widgets.hpp"

#include <algorithm>
#include <limits>

#include "modcurses/utf8.hpp"

namespace modcurses {
namespace {

// Column count, not codepoint count - the two diverge once M6 lands
// double-width glyphs, and every widget must be measuring columns.
int text_width(std::u32string_view s) {
    int w = 0;
    for (char32_t c : s) w += col_width(c);
    return w;
}

int align_offset(Align a, int content, int available) {
    switch (a) {
        case Align::Center: return std::max(0, (available - content) / 2);
        case Align::Right: return std::max(0, available - content);
        case Align::Left: break;
    }
    return 0;
}

}  // namespace

// ------------------------------------------------------------------- Label

Label::Label(std::string text, Align align) : text_(utf8_decode(text)), align_(align) {}

void Label::set_text(std::string text) {
    auto decoded = utf8_decode(text);
    if (decoded == text_) return;
    text_ = std::move(decoded);
    invalidate_layout();  // the preferred size changed with the text
}

std::string Label::text() const { return utf8_encode(text_); }

void Label::set_align(Align a) {
    if (a == align_) return;
    align_ = a;
    invalidate();
}

void Label::set_wrap(bool on) {
    if (on == wrap_) return;
    wrap_ = on;
    invalidate_layout();
}

std::vector<std::u32string> Label::wrap_text(std::u32string_view text, int width) {
    std::vector<std::u32string> lines;
    if (width <= 0) return lines;

    std::u32string current;
    int current_w = 0;
    std::size_t i = 0;
    while (i < text.size()) {
        // Take the next word plus the run of spaces that follows it, so a
        // break lands between words rather than inside one.
        std::size_t word_end = i;
        while (word_end < text.size() && text[word_end] != U' ') ++word_end;
        std::u32string_view word = text.substr(i, word_end - i);
        const int word_w = text_width(word);

        if (current_w > 0 && current_w + 1 + word_w > width) {
            lines.push_back(current);
            current.clear();
            current_w = 0;
        }
        if (current_w > 0) {
            current.push_back(U' ');
            ++current_w;
        }

        if (word_w <= width) {
            current += word;
            current_w += word_w;
        } else {
            // A single word too long for any line: break it mid-word rather
            // than emit a line that overflows.
            for (char32_t c : word) {
                if (current_w + col_width(c) > width) {
                    lines.push_back(current);
                    current.clear();
                    current_w = 0;
                }
                current.push_back(c);
                current_w += col_width(c);
            }
        }

        i = word_end;
        while (i < text.size() && text[i] == U' ') ++i;  // collapse the gap
    }
    if (!current.empty() || lines.empty()) lines.push_back(current);
    return lines;
}

SizeReq Label::width_req() const {
    // Prefers exactly its text but will take whatever it is given: a label is
    // an ALIGNED piece of text, and align_ can only mean something if the
    // widget is wider than the string. Capping max at the text width would
    // silently turn every Align::Center into Align::Left inside a VBox.
    return SizeReq{0, text_width(text_), std::numeric_limits<int>::max(), 1};
}

SizeReq Label::height_req() const {
    if (!wrap_) return SizeReq::fixed(1);
    // A wrapped label's height depends on its width, which the layout pass has
    // not decided at the point it asks. So measure against the width we
    // currently have, and let on_geometry re-run layout once that width
    // changes - see the note there.
    const int width = size().width > 0 ? size().width : text_width(text_);
    const int lines = static_cast<int>(wrap_text(text_, width).size());
    return SizeReq::fixed(std::max(1, lines));
}

void Label::on_geometry(Rect old_rect, Rect new_rect) {
    // Width-dependent height is inherently two-pass: pass one hands us a
    // width, and only then can height_req answer honestly. Requesting another
    // layout here is what makes the second pass happen. It terminates because
    // the third pass computes the same rect, so on_geometry does not fire
    // again.
    if (wrap_ && old_rect.size.width != new_rect.size.width) invalidate_layout();
}

void Label::paint(Canvas& c) {
    c.fill(Glyph{U' ', style});
    if (c.size().height <= 0 || c.size().width <= 0) return;

    if (!wrap_) {
        c.print({align_offset(align_, text_width(text_), c.size().width), 0}, text_, style);
        return;
    }
    const auto lines = wrap_text(text_, c.size().width);
    for (int y = 0; y < static_cast<int>(lines.size()) && y < c.size().height; ++y) {
        const auto& line = lines[static_cast<std::size_t>(y)];
        c.print({align_offset(align_, text_width(line), c.size().width), y}, line, style);
    }
}

// ------------------------------------------------------------------ Button

Button::Button(std::string label) : label_(utf8_decode(label)) {
    focus_policy = FocusPolicy::Strong;
}

void Button::set_label(std::string label) {
    auto decoded = utf8_decode(label);
    if (decoded == label_) return;
    label_ = std::move(decoded);
    invalidate_layout();
}

std::string Button::label() const { return utf8_encode(label_); }

SizeReq Button::width_req() const {
    const int w = text_width(label_) + 4;  // "[ " + label + " ]"
    return SizeReq{std::min(w, 2), w, w, 1};
}

SizeReq Button::height_req() const { return SizeReq::fixed(1); }

namespace {
// Merges a focus style over a base style rather than replacing it, so an
// application's own colours survive being focused.
Style merge_focus(Style base, const Style& focus) {
    base.traits = static_cast<std::uint8_t>(base.traits | focus.traits);
    if (focus.fg != Color::Default) base.fg = focus.fg;
    if (focus.bg != Color::Default) base.bg = focus.bg;
    return base;
}
}  // namespace

void Button::paint(Canvas& c) {
    const Style s = has_focus() ? merge_focus(style, focus_style) : style;
    c.fill(Glyph{U' ', s});
    if (c.size().height <= 0) return;

    const std::u32string rendered = U"[ " + label_ + U" ]";
    c.print({align_offset(Align::Center, text_width(rendered), c.size().width), 0}, rendered, s);
}

bool Button::on_key(const KeyEvent& ev) {
    if (ev.mods.ctrl || ev.mods.alt) return false;
    const bool activate = ev.key == Key::Enter || (ev.key == Key::Char && ev.text == U' ');
    if (!activate) return false;
    pressed.emit();
    return true;
}

bool Button::on_mouse(const MouseEvent& ev) {
    // Fires on press, not release: a release can land outside the widget after
    // a drag, and there is no drag tracking until M6.
    if (ev.action != MouseEvent::Action::Press || ev.button != MouseEvent::Button::Left)
        return false;
    pressed.emit();
    return true;
}

void Button::on_focus(bool /*gained*/) { invalidate(); }

// ---------------------------------------------------------------- Checkbox

Checkbox::Checkbox(std::string label, bool checked)
    : label_(utf8_decode(label)), checked_(checked) {
    focus_policy = FocusPolicy::Strong;
}

void Checkbox::set_checked(bool value) {
    if (value == checked_) return;
    checked_ = value;
    invalidate();
    toggled.emit(checked_);
}

void Checkbox::set_label(std::string label) {
    auto decoded = utf8_decode(label);
    if (decoded == label_) return;
    label_ = std::move(decoded);
    invalidate_layout();
}

std::string Checkbox::label() const { return utf8_encode(label_); }

SizeReq Checkbox::width_req() const {
    const int w = text_width(label_) + 4;  // "[x] " + label
    return SizeReq{std::min(w, 3), w, w, 1};
}

SizeReq Checkbox::height_req() const { return SizeReq::fixed(1); }

void Checkbox::paint(Canvas& c) {
    const Style s = has_focus() ? merge_focus(style, focus_style) : style;
    c.fill(Glyph{U' ', s});
    if (c.size().height <= 0) return;

    std::u32string rendered = checked_ ? U"[x] " : U"[ ] ";
    rendered += label_;
    c.print({0, 0}, rendered, s);
}

bool Checkbox::on_key(const KeyEvent& ev) {
    if (ev.mods.ctrl || ev.mods.alt) return false;
    const bool activate = ev.key == Key::Enter || (ev.key == Key::Char && ev.text == U' ');
    if (!activate) return false;
    toggle();
    return true;
}

bool Checkbox::on_mouse(const MouseEvent& ev) {
    if (ev.action != MouseEvent::Action::Press || ev.button != MouseEvent::Button::Left)
        return false;
    toggle();
    return true;
}

void Checkbox::on_focus(bool /*gained*/) { invalidate(); }

// ---------------------------------------------------------------- Titlebar

Titlebar::Titlebar(std::string title, std::string hint)
    : title_(utf8_decode(title)), hint_(utf8_decode(hint)) {
    style = Style{}.with(Trait::Reverse);
}

void Titlebar::set_title(std::string title) {
    auto decoded = utf8_decode(title);
    if (decoded == title_) return;
    title_ = std::move(decoded);
    invalidate();
}

void Titlebar::set_hint(std::string hint) {
    auto decoded = utf8_decode(hint);
    if (decoded == hint_) return;
    hint_ = std::move(decoded);
    invalidate();
}

std::string Titlebar::title() const { return utf8_encode(title_); }
std::string Titlebar::hint() const { return utf8_encode(hint_); }

void Titlebar::paint(Canvas& c) {
    c.fill(Glyph{U' ', style});
    if (c.size().height <= 0) return;

    c.print({1, 0}, title_, style);

    // The hint is a nicety: drop it rather than let it collide with the title.
    const int hint_w = text_width(hint_);
    const int title_w = text_width(title_);
    if (hint_w > 0 && c.size().width - hint_w - 1 > title_w + 2)
        c.print({c.size().width - hint_w - 1, 0}, hint_, style);
}

// --------------------------------------------------------------- StatusBar

StatusBar::StatusBar(std::string text) : text_(utf8_decode(text)) {
    style = Style{}.with(Trait::Reverse);
}

void StatusBar::set_text(std::string text) {
    auto decoded = utf8_decode(text);
    if (decoded == text_) return;
    text_ = std::move(decoded);
    if (!flashing_) invalidate();
}

std::string StatusBar::text() const { return utf8_encode(text_); }

void StatusBar::flash(std::string text, std::chrono::milliseconds duration) {
    flash_text_ = utf8_decode(text);
    flashing_ = true;
    invalidate();

    // The library's only timer primitive is a repeating one, so a one-shot is
    // a repeating timer that cancels itself on its first tick. The loop drops
    // the entry as soon as it sees the cancellation.
    flash_timer_ = add_timer(duration, [this] {
        flashing_ = false;
        flash_text_.clear();
        flash_timer_.cancel();
        invalidate();
    });
}

void StatusBar::clear_flash() {
    if (!flashing_) return;
    flashing_ = false;
    flash_text_.clear();
    flash_timer_.cancel();
    invalidate();
}

void StatusBar::paint(Canvas& c) {
    const Style s = flashing_ ? merge_focus(style, flash_style) : style;
    c.fill(Glyph{U' ', s});
    if (c.size().height <= 0) return;

    const std::u32string& shown = flashing_ ? flash_text_ : text_;
    const int x = align == Align::Left
                      ? 1
                      : align_offset(align, text_width(shown), c.size().width);
    c.print({x, 0}, shown, s);
}

// ----------------------------------------------------------------- Divider

Divider::Divider(Orientation o, BoxStyle box_style) : orientation_(o), box_style_(box_style) {}

SizeReq Divider::width_req() const {
    return orientation_ == Orientation::Vertical ? SizeReq::fixed(1) : SizeReq{};
}

SizeReq Divider::height_req() const {
    return orientation_ == Orientation::Horizontal ? SizeReq::fixed(1) : SizeReq{};
}

void Divider::paint(Canvas& c) {
    c.fill(Glyph{U' ', style});
    if (orientation_ == Orientation::Horizontal)
        c.draw_hline({0, 0}, c.size().width, style, box_style_);
    else
        c.draw_vline({0, 0}, c.size().height, style, box_style_);
}

// ---------------------------------------------------------------- ScrollBar

ScrollBar::ScrollBar() { focus_policy = FocusPolicy::None; }

void ScrollBar::set_range(int total, int visible) {
    total = std::max(0, total);
    visible = std::max(0, visible);
    if (total == total_ && visible == visible_) return;
    total_ = total;
    visible_ = visible;
    set_position(position_);  // re-clamp against the new range
    invalidate();
}

void ScrollBar::set_position(int first_visible) {
    const int max_pos = std::max(0, total_ - visible_);
    const int clamped = std::clamp(first_visible, 0, max_pos);
    if (clamped == position_) return;
    position_ = clamped;
    invalidate();
}

std::pair<int, int> ScrollBar::thumb_span(int track) const {
    if (track <= 0 || total_ <= 0) return {0, 0};
    if (!scrollable()) return {0, track};  // everything is visible: full track

    // At least one row of thumb, however long the content is.
    const int thumb = std::max(1, (visible_ * track) / total_);
    const int max_pos = total_ - visible_;
    const int travel = track - thumb;
    const int first = max_pos > 0 ? (position_ * travel) / max_pos : 0;
    return {std::clamp(first, 0, travel), thumb};
}

void ScrollBar::paint(Canvas& c) {
    c.fill(Glyph{U' ', style});
    const int track = c.size().height;
    if (track <= 0 || c.size().width <= 0) return;

    // A visible track even when there is nothing to scroll, so the layout does
    // not jump around as content grows.
    for (int y = 0; y < track; ++y) c.put({0, y}, U'│', style);

    const auto [first, length] = thumb_span(track);
    for (int y = first; y < first + length && y < track; ++y)
        c.put({0, y}, U' ', merge_focus(style, thumb_style));
}

bool ScrollBar::on_mouse(const MouseEvent& ev) {
    if (!interactive || !scrollable()) return false;

    if (ev.button == MouseEvent::Button::WheelUp) {
        position_changed.emit(std::max(0, position_ - 1));
        return true;
    }
    if (ev.button == MouseEvent::Button::WheelDown) {
        position_changed.emit(std::min(total_ - visible_, position_ + 1));
        return true;
    }
    if (ev.action != MouseEvent::Action::Press || ev.button != MouseEvent::Button::Left)
        return false;

    // Jump so that the clicked row becomes the middle of the thumb.
    const int track = size().height;
    if (track <= 0) return false;
    const int max_pos = total_ - visible_;
    const int wanted = (ev.pos.y * max_pos) / std::max(1, track - 1);
    position_changed.emit(std::clamp(wanted, 0, max_pos));
    return true;
}

}  // namespace modcurses
