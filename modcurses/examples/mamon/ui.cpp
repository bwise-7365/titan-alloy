#include "ui.hpp"

#include <algorithm>

#include "modcurses/text.hpp"
#include "modcurses/utf8.hpp"

namespace mamon {
namespace {

int width_of(std::u32string_view s) {
    int w = 0;
    for (char32_t c : s) w += col_width(c);
    return w;
}

}  // namespace

// --------------------------------------------------------------- TitleBar

void TitleBar::set_version(std::string v) {
    version_ = utf8_decode(v);
    invalidate();
}

void TitleBar::set_filename(std::string name) {
    auto decoded = utf8_decode(name);
    if (decoded == filename_) return;
    filename_ = std::move(decoded);
    invalidate();
}

void TitleBar::set_modified(bool m) {
    if (m == modified_) return;
    modified_ = m;
    invalidate();
}

void TitleBar::paint(Canvas& c) {
    c.fill(Glyph{U' ', style});
    const int width = c.size().width;
    if (width <= 0 || c.size().height <= 0) return;

    // nano's own wording for a buffer with no file behind it.
    const std::u32string name = filename_.empty() ? U"New Buffer" : filename_;
    const std::u32string modified = U"Modified";

    // Two spaces of indent, as nano has.
    c.print({2, 0}, version_, style);

    // The filename is centred on the whole bar, not on what is left of it -
    // that is what makes nano's title look balanced.
    int name_x = (width - width_of(name)) / 2;
    const int version_end = 2 + width_of(version_);
    if (name_x < version_end + 2) name_x = version_end + 2;
    if (name_x + width_of(name) <= width) c.print({name_x, 0}, name, style);

    if (modified_) {
        const int x = width - width_of(modified) - 1;
        // Drop it rather than let it collide with the filename.
        if (x > name_x + width_of(name)) c.print({x, 0}, modified, style);
    }
}

// ---------------------------------------------------------------- HelpBar

HelpBar::HelpBar() { set_default_entries(); }

void HelpBar::set_entries(std::vector<HelpEntry> entries) {
    entries_ = std::move(entries);
    invalidate();
}

void HelpBar::set_default_entries() {
    // nano's default bar, column by column: the upper row entry then the
    // lower one. The last pair only appears on a wide enough terminal.
    set_entries({
        {U"^G", U"Help"},      {U"^X", U"Exit"},
        {U"^O", U"Write Out"}, {U"^R", U"Read File"},
        {U"^W", U"Where Is"},  {U"^\\", U"Replace"},
        {U"^K", U"Cut"},       {U"^U", U"Paste"},
        {U"^T", U"Execute"},   {U"^J", U"Justify"},
        {U"^C", U"Location"},  {U"^/", U"Go To Line"},
        {U"M-U", U"Undo"},     {U"M-E", U"Redo"},
    });
}

void HelpBar::set_prompt_entries() {
    set_entries({
        {U"^G", U"Help"},   {U"^C", U"Cancel"},
        {U"M-F", U"New Buffer"}, {U"", U""},
    });
}

void HelpBar::set_yesno_entries() {
    set_entries({
        {U"Y", U"Yes"},
        {U"N", U"No"},
        {U"^C", U"Cancel"},
        {U"", U""},
    });
}

int HelpBar::columns_shown() const {
    const int pairs = static_cast<int>(entries_.size() + 1) / 2;
    if (pairs <= 0 || size().width <= 0) return 0;
    // nano fits as many 13-column cells as the width allows.
    return std::clamp(size().width / 13, 1, pairs);
}

void HelpBar::paint(Canvas& c) {
    c.fill(Glyph{U' ', style});
    const int width = c.size().width;
    if (width <= 0 || c.size().height <= 0) return;

    const int columns = columns_shown();
    if (columns <= 0) return;
    const int cell = width / columns;

    for (int col = 0; col < columns; ++col) {
        for (int row = 0; row < 2 && row < c.size().height; ++row) {
            const auto index = static_cast<std::size_t>(col * 2 + row);
            if (index >= entries_.size()) continue;
            const HelpEntry& e = entries_[index];
            if (e.key.empty()) continue;

            const int x = col * cell;
            c.print({x, row}, e.key, key_style);
            const int label_x = x + width_of(e.key) + 1;
            // Clip to the cell so a long label cannot bleed into its neighbour.
            const int room = std::min(cell - (label_x - x), width - label_x);
            if (room > 0)
                c.print({label_x, row}, std::u32string_view{e.label}.substr(
                                            0, static_cast<std::size_t>(room)),
                        label_style);
        }
    }
}

// ------------------------------------------------------------- MessageBar

void MessageBar::show_message(std::string text) {
    message_ = utf8_decode(text);
    invalidate();
}

void MessageBar::clear() {
    if (message_.empty()) return;
    message_.clear();
    invalidate();
}

void MessageBar::paint(Canvas& c) {
    c.fill(Glyph{U' ', style});
    if (message_.empty() || c.size().height <= 0) return;
    // nano centres its status messages and paints them in the status colour.
    // The padding either side is highlighted too, so the message reads as a
    // block rather than as loose text.
    const int x = std::max(0, (c.size().width - width_of(message_)) / 2);
    const int pad = 1;
    c.fill(Rect{{std::max(0, x - pad), 0}, {width_of(message_) + 2 * pad, 1}},
           Glyph{U' ', message_style});
    c.print({x, 0}, message_, message_style);
}

// ------------------------------------------------------------ LineNumbers

SizeReq LineNumbers::width_req() const {
    if (view_ == nullptr) return SizeReq::fixed(4);
    int digits = 1;
    for (int n = view_->buffer().line_count(); n >= 10; n /= 10) ++digits;
    return SizeReq::fixed(std::max(4, digits + 2));
}

void LineNumbers::paint(Canvas& c) {
    c.fill(Glyph{U' ', style});
    if (view_ == nullptr) return;
    const int rows = c.size().height;
    const int first = view_->scroll().y;
    const int total = view_->buffer().line_count();
    const int current = view_->buffer().cursor().line;

    for (int row = 0; row < rows; ++row) {
        const int line = first + row;
        if (line >= total) break;
        const std::string label = std::to_string(line + 1);
        const int x = c.size().width - 1 - static_cast<int>(label.size());
        // The cursor's own line is highlighted, as nano does with linenumbers.
        c.print({std::max(0, x), row}, label,
                line == current ? style.with(Trait::Reverse) : style);
    }
}

}  // namespace mamon
