#pragma once
//
// mamon - the editing operations nano provides on top of a text buffer.
//
// Deliberately UI-free, like modcurses' own TextBuffer: cut/paste, search,
// replace, justify and undo are all checkable with no terminal in sight.
//
#include <cstdint>
#include <string>
#include <vector>

#include "modcurses/text.hpp"

namespace mamon {

using modcurses::TextBuffer;

// What kind of edit produced an undo state. Consecutive edits of the same
// kind on the same line are coalesced into one undo step, so typing a word
// undoes as a word rather than a letter at a time - which is what nano does.
enum class EditKind { None, Insert, Erase, Cut, Paste, Replace, Justify, Other };

struct SearchOutcome {
    bool found = false;
    bool wrapped = false;  // the search ran off the end and resumed at the top
};

class Editor {
public:
    explicit Editor(TextBuffer& buffer) : buffer_(&buffer) {}

    [[nodiscard]] TextBuffer& buffer() const { return *buffer_; }

    // ---- the cut buffer ----
    //
    // nano's rule: consecutive ^K presses ACCUMULATE into the cut buffer, and
    // the first ^K after anything else starts it over. That is what makes
    // "cut three lines, paste them back" work.
    void cut_line();
    void paste();
    [[nodiscard]] const std::vector<std::u32string>& cut_buffer() const { return cut_; }
    void interrupt_cut_run() { cut_run_ = false; }

    // ---- search and replace ----
    //
    // Searching starts just past the cursor and wraps, exactly as nano does,
    // so repeating a search walks through every match and comes back round.
    SearchOutcome find(std::u32string_view needle, bool backwards, bool case_sensitive);
    // Replaces the match AT the cursor, if the text there matches.
    bool replace_at_cursor(std::u32string_view needle, std::u32string_view with,
                           bool case_sensitive);
    int replace_all(std::u32string_view needle, std::u32string_view with, bool case_sensitive);

    // ---- other operations ----
    void goto_line(int line, int column = 0);
    // Re-wraps the paragraph around the cursor to `width` columns. A
    // paragraph runs to the next blank line, as in nano.
    int justify(int width);

    // ---- undo ----
    //
    // Snapshot-based: modcurses' TextBuffer has no undo of its own until M6,
    // and whole-buffer snapshots are honest and cheap at the sizes an editor
    // like this handles.
    void begin_edit(EditKind kind);  // call BEFORE mutating
    bool undo();
    bool redo();
    [[nodiscard]] bool can_undo() const { return !undo_.empty(); }
    [[nodiscard]] bool can_redo() const { return !redo_.empty(); }
    void clear_history();

    // Cap on remembered states, so a long session cannot grow without bound.
    int history_limit = 200;

private:
    struct Snapshot {
        std::u32string text;
        TextBuffer::Cursor cursor;
        EditKind kind = EditKind::None;
        int line = -1;
    };

    [[nodiscard]] Snapshot capture(EditKind kind) const;
    void restore(const Snapshot& s);

    TextBuffer* buffer_;

    std::vector<std::u32string> cut_;
    bool cut_run_ = false;  // is the previous action also a ^K?

    std::vector<Snapshot> undo_;
    std::vector<Snapshot> redo_;
    EditKind last_kind_ = EditKind::None;
    int last_line_ = -1;
};

// Case-insensitive comparison is needed in several places; exposed for tests.
[[nodiscard]] char32_t to_lower(char32_t c);
[[nodiscard]] std::size_t find_in(std::u32string_view haystack, std::u32string_view needle,
                                  std::size_t from, bool case_sensitive);

}  // namespace mamon
