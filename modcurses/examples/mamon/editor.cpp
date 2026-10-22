#include "editor.hpp"

#include <algorithm>

#include "modcurses/widgets.hpp"

namespace mamon {

char32_t to_lower(char32_t c) {
    // ASCII only, deliberately: a correct Unicode fold needs tables this
    // example has no business carrying, and nano's own default search is
    // case-insensitive in the same shallow way.
    return (c >= U'A' && c <= U'Z') ? c + (U'a' - U'A') : c;
}

std::size_t find_in(std::u32string_view haystack, std::u32string_view needle, std::size_t from,
                    bool case_sensitive) {
    if (needle.empty() || needle.size() > haystack.size()) return std::u32string_view::npos;
    if (case_sensitive) return haystack.find(needle, from);

    for (std::size_t i = from; i + needle.size() <= haystack.size(); ++i) {
        bool match = true;
        for (std::size_t k = 0; k < needle.size() && match; ++k)
            match = to_lower(haystack[i + k]) == to_lower(needle[k]);
        if (match) return i;
    }
    return std::u32string_view::npos;
}

// ------------------------------------------------------------------- undo

Editor::Snapshot Editor::capture(EditKind kind) const {
    return Snapshot{buffer_->text(), buffer_->cursor(), kind, buffer_->cursor().line};
}

void Editor::restore(const Snapshot& s) {
    buffer_->set_text(s.text);
    buffer_->set_cursor(s.cursor);
}

void Editor::begin_edit(EditKind kind) {
    // Coalesce a run of the same kind of edit on the same line into one undo
    // step. Without this, undoing a typed word would take a keystroke per
    // letter, which is not what anyone means by "undo".
    const int line = buffer_->cursor().line;
    const bool same_run = kind == last_kind_ && line == last_line_ &&
                          (kind == EditKind::Insert || kind == EditKind::Erase);
    last_kind_ = kind;
    last_line_ = line;
    if (same_run && !undo_.empty()) return;

    undo_.push_back(capture(kind));
    if (static_cast<int>(undo_.size()) > history_limit) undo_.erase(undo_.begin());
    redo_.clear();  // a fresh edit invalidates anything that was undone
}

bool Editor::undo() {
    if (undo_.empty()) return false;
    redo_.push_back(capture(EditKind::Other));
    const Snapshot s = undo_.back();
    undo_.pop_back();
    restore(s);
    last_kind_ = EditKind::None;  // the next edit starts a new run
    last_line_ = -1;
    return true;
}

bool Editor::redo() {
    if (redo_.empty()) return false;
    undo_.push_back(capture(EditKind::Other));
    const Snapshot s = redo_.back();
    redo_.pop_back();
    restore(s);
    last_kind_ = EditKind::None;
    last_line_ = -1;
    return true;
}

void Editor::clear_history() {
    undo_.clear();
    redo_.clear();
    last_kind_ = EditKind::None;
    last_line_ = -1;
}

// -------------------------------------------------------------- cut/paste

void Editor::cut_line() {
    begin_edit(EditKind::Cut);
    // A run of consecutive cuts appends; anything else starts a new buffer.
    if (!cut_run_) cut_.clear();
    cut_run_ = true;
    last_kind_ = EditKind::Cut;

    const auto cursor = buffer_->cursor();
    cut_.push_back(buffer_->line(cursor.line));
    buffer_->erase_line();
    buffer_->set_cursor({cursor.line, 0});
}

void Editor::paste() {
    if (cut_.empty()) return;
    begin_edit(EditKind::Paste);
    cut_run_ = false;
    last_kind_ = EditKind::Paste;

    // Whole lines went in, so whole lines come out - above the current line,
    // leaving the cursor after them, as nano does.
    std::u32string text;
    for (const auto& line : cut_) {
        text += line;
        text.push_back(U'\n');
    }
    const auto cursor = buffer_->cursor();
    buffer_->set_cursor({cursor.line, 0});
    buffer_->insert(text);
}

// ----------------------------------------------------------------- search

SearchOutcome Editor::find(std::u32string_view needle, bool backwards, bool case_sensitive) {
    SearchOutcome out;
    if (needle.empty()) return out;

    const int lines = buffer_->line_count();
    const auto start = buffer_->cursor();

    const auto probe = [&](int line, std::size_t from, bool reverse) -> std::size_t {
        const std::u32string& text = buffer_->line(line);
        if (!reverse) return find_in(text, needle, from, case_sensitive);
        // Backwards: the last match at or before `from`.
        std::size_t best = std::u32string_view::npos;
        std::size_t at = find_in(text, needle, 0, case_sensitive);
        while (at != std::u32string_view::npos && at < from) {
            best = at;
            at = find_in(text, needle, at + 1, case_sensitive);
        }
        return best;
    };

    // Walk every line once, starting from the cursor and wrapping - which is
    // what makes repeating a search cycle through all the matches.
    for (int step = 0; step <= lines; ++step) {
        int line;
        std::size_t from;
        if (!backwards) {
            line = (start.line + step) % lines;
            from = step == 0 ? static_cast<std::size_t>(start.col) + 1 : 0;
            if (step == lines) from = 0;  // final pass over the starting line
        } else {
            line = ((start.line - step) % lines + lines) % lines;
            from = step == 0 ? static_cast<std::size_t>(start.col)
                             : buffer_->line(line).size() + 1;
        }
        if (from > buffer_->line(line).size() + 1) continue;

        const std::size_t at = probe(line, from, backwards);
        if (at == std::u32string_view::npos) continue;

        out.found = true;
        out.wrapped = backwards ? line > start.line : line < start.line;
        if (step == lines) out.wrapped = true;
        buffer_->set_cursor({line, static_cast<int>(at)});
        return out;
    }
    return out;
}

bool Editor::replace_at_cursor(std::u32string_view needle, std::u32string_view with,
                               bool case_sensitive) {
    const auto cursor = buffer_->cursor();
    const std::u32string& line = buffer_->line(cursor.line);
    const auto at = static_cast<std::size_t>(cursor.col);
    if (at + needle.size() > line.size()) return false;

    const std::u32string_view here{line.data() + at, needle.size()};
    if (find_in(here, needle, 0, case_sensitive) != 0) return false;

    begin_edit(EditKind::Replace);
    last_kind_ = EditKind::Replace;
    for (std::size_t i = 0; i < needle.size(); ++i) buffer_->erase_forward();
    buffer_->insert(with);
    return true;
}

int Editor::replace_all(std::u32string_view needle, std::u32string_view with,
                        bool case_sensitive) {
    if (needle.empty()) return 0;
    begin_edit(EditKind::Replace);
    last_kind_ = EditKind::Replace;

    int count = 0;
    for (int line = 0; line < buffer_->line_count(); ++line) {
        std::size_t at = 0;
        while (true) {
            const std::u32string& text = buffer_->line(line);
            at = find_in(text, needle, at, case_sensitive);
            if (at == std::u32string_view::npos) break;
            buffer_->set_cursor({line, static_cast<int>(at)});
            for (std::size_t i = 0; i < needle.size(); ++i) buffer_->erase_forward();
            buffer_->insert(with);
            at += with.size();
            ++count;
        }
    }
    return count;
}

// ------------------------------------------------------------------ other

void Editor::goto_line(int line, int column) {
    buffer_->set_cursor({line, column});  // TextBuffer clamps for us
}

int Editor::justify(int width) {
    if (width < 8) width = 8;
    const int lines = buffer_->line_count();
    const int here = buffer_->cursor().line;

    const auto blank = [&](int i) {
        const std::u32string& s = buffer_->line(i);
        return std::all_of(s.begin(), s.end(), [](char32_t c) { return c == U' ' || c == U'\t'; });
    };
    if (blank(here)) return 0;

    int first = here;
    while (first > 0 && !blank(first - 1)) --first;
    int last = here;
    while (last + 1 < lines && !blank(last + 1)) ++last;

    // Join the paragraph, then re-break it. Label::wrap_text is the library's
    // own word wrapper, so mamon and a wrapping Label agree on where lines go.
    std::u32string joined;
    for (int i = first; i <= last; ++i) {
        if (i > first) joined.push_back(U' ');
        joined += buffer_->line(i);
    }
    const auto wrapped = modcurses::Label::wrap_text(joined, width);
    if (wrapped.empty()) return 0;

    begin_edit(EditKind::Justify);
    last_kind_ = EditKind::Justify;

    std::u32string replacement;
    for (std::size_t i = 0; i < wrapped.size(); ++i) {
        if (i > 0) replacement.push_back(U'\n');
        replacement += wrapped[i];
    }

    // Rebuild the whole buffer with the paragraph swapped out: simpler than
    // splicing, and undo is a snapshot anyway.
    //
    // Chunks are JOINED with newlines rather than each appending one. Adding
    // a separator after the replacement put a stray blank line at the end
    // whenever the paragraph ran to the bottom of the buffer.
    std::u32string out;
    bool first_chunk = true;
    for (int i = 0; i < lines; ++i) {
        if (i > first && i <= last) continue;  // absorbed into the replacement
        if (!first_chunk) out.push_back(U'\n');
        first_chunk = false;
        out += (i == first) ? replacement : buffer_->line(i);
    }
    buffer_->set_text(out);
    buffer_->set_cursor({first, 0});
    return static_cast<int>(wrapped.size());
}

}  // namespace mamon
