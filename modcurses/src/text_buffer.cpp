#include <algorithm>
#include <fstream>
#include <iterator>

#include "modcurses/text.hpp"
#include "modcurses/utf8.hpp"

namespace modcurses {
namespace {

bool is_word_char(char32_t c) {
    return (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z') || (c >= U'0' && c <= U'9') ||
           c == U'_' || c > 127;
}

}  // namespace

TextBuffer::TextBuffer() = default;

TextBuffer::TextBuffer(std::u32string_view text) { set_text(text); }

const std::u32string& TextBuffer::line(int index) const {
    // Out-of-range reads answer with an empty line rather than UB. (Held as a
    // member, not a function-local static: the library keeps zero static
    // state, and that rule has no useful exceptions.)
    if (index < 0 || index >= line_count()) return empty_line_;
    return lines_[static_cast<std::size_t>(index)];
}

int TextBuffer::line_length(int index) const { return static_cast<int>(line(index).size()); }

bool TextBuffer::empty() const { return lines_.size() == 1 && lines_[0].empty(); }

std::u32string TextBuffer::text() const {
    std::u32string out;
    for (std::size_t i = 0; i < lines_.size(); ++i) {
        if (i > 0) out.push_back(U'\n');
        out += lines_[i];
    }
    return out;
}

void TextBuffer::set_text(std::u32string_view text) {
    lines_.clear();
    std::u32string current;
    for (char32_t c : text) {
        if (c == U'\n') {
            lines_.push_back(std::move(current));
            current.clear();
        } else if (c != U'\r') {  // a stray CR is a line-ending artefact, not content
            current.push_back(c);
        }
    }
    lines_.push_back(std::move(current));
    cursor_ = Cursor{};
    touch();
}

void TextBuffer::clear() {
    lines_.assign(1, std::u32string{});
    cursor_ = Cursor{};
    touch();
}

void TextBuffer::set_cursor(Cursor c) {
    const Cursor before = cursor_;
    cursor_ = c;
    clamp_cursor();
    if (cursor_ != before) cursor_moved.emit();
}

void TextBuffer::clamp_cursor() {
    if (lines_.empty()) lines_.emplace_back();
    cursor_.line = std::clamp(cursor_.line, 0, line_count() - 1);
    cursor_.col = std::clamp(cursor_.col, 0, line_length(cursor_.line));
}

void TextBuffer::touch() {
    clamp_cursor();
    dirty_ = true;
    changed.emit();
}

// ------------------------------------------------------------------ editing

void TextBuffer::insert(char32_t c) {
    if (c == U'\n') return insert_newline();
    lines_[static_cast<std::size_t>(cursor_.line)].insert(static_cast<std::size_t>(cursor_.col), 1,
                                                          c);
    ++cursor_.col;
    touch();
}

void TextBuffer::insert(std::u32string_view s) {
    for (char32_t c : s) {
        if (c == U'\n') {
            insert_newline();
        } else if (c != U'\r') {
            lines_[static_cast<std::size_t>(cursor_.line)].insert(
                static_cast<std::size_t>(cursor_.col), 1, c);
            ++cursor_.col;
        }
    }
    touch();
}

void TextBuffer::insert_newline() {
    auto& line = lines_[static_cast<std::size_t>(cursor_.line)];
    std::u32string tail = line.substr(static_cast<std::size_t>(cursor_.col));
    line.erase(static_cast<std::size_t>(cursor_.col));
    lines_.insert(lines_.begin() + cursor_.line + 1, std::move(tail));
    ++cursor_.line;
    cursor_.col = 0;
    touch();
}

bool TextBuffer::backspace() {
    if (cursor_.col > 0) {
        lines_[static_cast<std::size_t>(cursor_.line)].erase(
            static_cast<std::size_t>(cursor_.col - 1), 1);
        --cursor_.col;
        touch();
        return true;
    }
    if (cursor_.line == 0) return false;  // start of buffer

    // Join with the previous line; the cursor lands at the seam.
    const std::u32string tail = lines_[static_cast<std::size_t>(cursor_.line)];
    lines_.erase(lines_.begin() + cursor_.line);
    --cursor_.line;
    cursor_.col = line_length(cursor_.line);
    lines_[static_cast<std::size_t>(cursor_.line)] += tail;
    touch();
    return true;
}

bool TextBuffer::erase_forward() {
    auto& line = lines_[static_cast<std::size_t>(cursor_.line)];
    if (cursor_.col < static_cast<int>(line.size())) {
        line.erase(static_cast<std::size_t>(cursor_.col), 1);
        touch();
        return true;
    }
    if (cursor_.line + 1 >= line_count()) return false;  // end of buffer

    line += lines_[static_cast<std::size_t>(cursor_.line + 1)];
    lines_.erase(lines_.begin() + cursor_.line + 1);
    touch();
    return true;
}

void TextBuffer::erase_line() {
    if (line_count() == 1) {
        lines_[0].clear();
    } else {
        lines_.erase(lines_.begin() + cursor_.line);
    }
    cursor_.col = 0;
    touch();
}

// ---------------------------------------------------------------- movement

void TextBuffer::move_left() {
    if (cursor_.col > 0) {
        --cursor_.col;
    } else if (cursor_.line > 0) {
        --cursor_.line;
        cursor_.col = line_length(cursor_.line);
    } else {
        return;
    }
    cursor_moved.emit();
}

void TextBuffer::move_right() {
    if (cursor_.col < line_length(cursor_.line)) {
        ++cursor_.col;
    } else if (cursor_.line + 1 < line_count()) {
        ++cursor_.line;
        cursor_.col = 0;
    } else {
        return;
    }
    cursor_moved.emit();
}

void TextBuffer::move_up() {
    if (cursor_.line == 0) return;
    --cursor_.line;
    clamp_cursor();
    cursor_moved.emit();
}

void TextBuffer::move_down() {
    if (cursor_.line + 1 >= line_count()) return;
    ++cursor_.line;
    clamp_cursor();
    cursor_moved.emit();
}

void TextBuffer::move_line_start() {
    if (cursor_.col == 0) return;
    cursor_.col = 0;
    cursor_moved.emit();
}

void TextBuffer::move_line_end() {
    const int end = line_length(cursor_.line);
    if (cursor_.col == end) return;
    cursor_.col = end;
    cursor_moved.emit();
}

void TextBuffer::move_buffer_start() { set_cursor(Cursor{0, 0}); }

void TextBuffer::move_buffer_end() {
    set_cursor(Cursor{line_count() - 1, line_length(line_count() - 1)});
}

void TextBuffer::move_word_left() {
    const Cursor before = cursor_;
    if (cursor_.col == 0) {
        if (cursor_.line == 0) return;
        --cursor_.line;
        cursor_.col = line_length(cursor_.line);
    } else {
        const auto& line = lines_[static_cast<std::size_t>(cursor_.line)];
        int i = cursor_.col;
        while (i > 0 && !is_word_char(line[static_cast<std::size_t>(i - 1)])) --i;
        while (i > 0 && is_word_char(line[static_cast<std::size_t>(i - 1)])) --i;
        cursor_.col = i;
    }
    if (cursor_ != before) cursor_moved.emit();
}

void TextBuffer::move_word_right() {
    const Cursor before = cursor_;
    const auto& line = lines_[static_cast<std::size_t>(cursor_.line)];
    if (cursor_.col >= static_cast<int>(line.size())) {
        if (cursor_.line + 1 >= line_count()) return;
        ++cursor_.line;
        cursor_.col = 0;
    } else {
        int i = cursor_.col;
        const int n = static_cast<int>(line.size());
        while (i < n && is_word_char(line[static_cast<std::size_t>(i)])) ++i;
        while (i < n && !is_word_char(line[static_cast<std::size_t>(i)])) ++i;
        cursor_.col = i;
    }
    if (cursor_ != before) cursor_moved.emit();
}

// ---------------------------------------------------------------- file I/O

bool TextBuffer::load(const std::filesystem::path& p, std::string* error) {
    // Binary, always: letting the runtime translate line endings is how a
    // CRLF file silently becomes an LF file on save.
    std::ifstream in(p, std::ios::binary);
    if (!in) {
        if (error) *error = "cannot open " + p.string();
        return false;
    }
    const std::string raw{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    if (in.bad()) {
        if (error) *error = "error reading " + p.string();
        return false;
    }

    // Whatever the first terminator is, that is the file's convention.
    line_ending_ = raw.find("\r\n") != std::string::npos ? LineEnding::CrLf : LineEnding::Lf;
    final_newline_ = raw.empty() || raw.back() == '\n';

    std::string body = raw;
    // A trailing newline terminates the last line; it does not start a new
    // empty one. Dropping it here keeps load/save symmetric.
    if (!body.empty() && body.back() == '\n') {
        body.pop_back();
        if (!body.empty() && body.back() == '\r') body.pop_back();
    }
    // Strip a UTF-8 BOM if present rather than treating it as content.
    if (body.size() >= 3 && static_cast<unsigned char>(body[0]) == 0xEF &&
        static_cast<unsigned char>(body[1]) == 0xBB && static_cast<unsigned char>(body[2]) == 0xBF)
        body.erase(0, 3);

    set_text(utf8_decode(body));
    path_ = p;
    dirty_ = false;
    return true;
}

bool TextBuffer::save(const std::filesystem::path& p, std::string* error) {
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    if (!out) {
        if (error) *error = "cannot write " + p.string();
        return false;
    }
    const std::string eol = line_ending_ == LineEnding::CrLf ? "\r\n" : "\n";
    for (std::size_t i = 0; i < lines_.size(); ++i) {
        out << utf8_encode(lines_[i]);
        const bool last = i + 1 == lines_.size();
        if (!last || final_newline_) out << eol;
    }
    out.flush();
    if (!out) {
        if (error) *error = "error writing " + p.string();
        return false;
    }
    path_ = p;
    dirty_ = false;
    return true;
}

bool TextBuffer::save(std::string* error) {
    if (path_.empty()) {
        if (error) *error = "no filename";
        return false;
    }
    return save(path_, error);
}

}  // namespace modcurses
