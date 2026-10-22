#pragma once
//
// modcurses/text.hpp - the editable-text model, its keymap, and its view.
//
// PUBLIC HEADER: no curses.
//
// TextBuffer is a MODEL, not a widget: it holds lines and a cursor and knows
// nothing about screens. TextArea is a view onto one. Keeping them apart is
// what lets a buffer be edited, tested and saved with no terminal in sight.
//
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "modcurses/core.hpp"
#include "modcurses/render.hpp"
#include "modcurses/widget.hpp"

namespace modcurses {

// ------------------------------------------------------------- TextBuffer

class TextBuffer {
public:
    struct Cursor {
        int line = 0;
        int col = 0;
        constexpr auto operator<=>(const Cursor&) const = default;
    };

    // How the file this buffer came from terminates its lines. Preserved
    // across a load/save round trip so editing a file does not rewrite every
    // line of it on a machine with the other convention.
    enum class LineEnding { Lf, CrLf };

    TextBuffer();
    explicit TextBuffer(std::u32string_view text);

    Signal<> changed;        // the content changed
    Signal<> cursor_moved;   // only the cursor moved

    // ---- content ----
    [[nodiscard]] int line_count() const { return static_cast<int>(lines_.size()); }
    [[nodiscard]] const std::u32string& line(int index) const;
    [[nodiscard]] int line_length(int index) const;
    [[nodiscard]] std::u32string text() const;   // joined with '\n'
    void set_text(std::u32string_view text);
    void clear();
    [[nodiscard]] bool empty() const;

    // ---- cursor ----
    [[nodiscard]] Cursor cursor() const { return cursor_; }
    void set_cursor(Cursor c);   // clamped into the buffer

    // ---- editing (at the cursor, which moves with the edit) ----
    void insert(char32_t c);
    void insert(std::u32string_view s);
    void insert_newline();
    bool backspace();       // false at the very start of the buffer
    bool erase_forward();   // false at the very end
    void erase_line();

    // ---- movement ----
    void move_left();
    void move_right();
    void move_up();
    void move_down();
    void move_line_start();
    void move_line_end();
    void move_buffer_start();
    void move_buffer_end();
    void move_word_left();
    void move_word_right();

    // ---- dirty tracking ----
    [[nodiscard]] bool dirty() const { return dirty_; }
    void clear_dirty() { dirty_ = false; }

    // ---- file I/O ----
    //
    // Streams are opened in binary and line endings handled explicitly, so a
    // file round-trips byte-for-byte on both platforms. Malformed UTF-8 is
    // replaced rather than rejected, so a stray byte cannot make a file
    // unopenable.
    bool load(const std::filesystem::path& p, std::string* error = nullptr);
    bool save(const std::filesystem::path& p, std::string* error = nullptr);
    bool save(std::string* error = nullptr);  // to path()

    [[nodiscard]] const std::filesystem::path& path() const { return path_; }
    void set_path(std::filesystem::path p) { path_ = std::move(p); }
    [[nodiscard]] LineEnding line_ending() const { return line_ending_; }
    void set_line_ending(LineEnding e) { line_ending_ = e; }
    // True when the last loaded file had no terminating newline; save()
    // reproduces that rather than silently appending one.
    [[nodiscard]] bool final_newline() const { return final_newline_; }
    void set_final_newline(bool v) { final_newline_ = v; }

private:
    void clamp_cursor();
    void touch();  // marks dirty and emits changed

    std::vector<std::u32string> lines_{1};  // always at least one line
    std::u32string empty_line_;             // returned for out-of-range line()
    Cursor cursor_;
    bool dirty_ = false;
    std::filesystem::path path_;
    LineEnding line_ending_ = LineEnding::Lf;
    bool final_newline_ = true;
};

// ----------------------------------------------------------------- Keymap

// The editing verbs a key can be bound to. Deliberately a closed enum rather
// than std::function: a table of these is comparable, printable, and can be
// swapped wholesale for a different editing style.
enum class EditAction {
    None,
    InsertNewline,
    Backspace,
    DeleteForward,
    DeleteLine,
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,
    MoveLineStart,
    MoveLineEnd,
    MoveBufferStart,
    MoveBufferEnd,
    MoveWordLeft,
    MoveWordRight,
    PageUp,
    PageDown,
};

[[nodiscard]] const char* to_string(EditAction a);

struct KeyBinding {
    KeyEvent match;
    EditAction action = EditAction::None;
};

// Answering TUI_DESIGN section 13's open question in the affirmative: the
// keymap is a swappable table from day one, because retrofitting one later is
// the painful direction.
class Keymap {
public:
    void bind(KeyEvent match, EditAction action);  // rebinding replaces
    bool unbind(KeyEvent match);
    [[nodiscard]] EditAction lookup(const KeyEvent& ev) const;
    [[nodiscard]] std::span<const KeyBinding> bindings() const { return bindings_; }
    void clear() { bindings_.clear(); }

    // Arrows, Home/End, PageUp/PageDown, Backspace, Delete, Enter. Both named
    // maps below start from this.
    [[nodiscard]] static Keymap basic();
    // nano: Ctrl-A/E for line ends, Ctrl-Y/V for pages, Ctrl-K cuts a line.
    [[nodiscard]] static Keymap nano();
    // emacs: Ctrl-B/F/P/N to move, Ctrl-A/E, Ctrl-D deletes forward.
    [[nodiscard]] static Keymap emacs();

private:
    std::vector<KeyBinding> bindings_;
};

// --------------------------------------------------------------- TextArea

// A scrolling view onto a TextBuffer. It does not own the buffer: several
// views onto one buffer are legal, and the buffer outlives them.
class TextArea : public Widget {
public:
    explicit TextArea(TextBuffer& buffer);

    [[nodiscard]] TextBuffer& buffer() const { return *buffer_; }
    void set_buffer(TextBuffer& buffer);

    Signal<int, int> scrolled;  // (first visible line, total lines)

    bool read_only = false;
    int tab_width = 4;
    // With this on, Tab indents instead of moving focus (Shift-Tab still
    // leaves). Dispatch consults widgets before the focus chain so that this
    // is possible at all; turn it off for a read-only pager.
    bool capture_tab = true;
    Keymap keymap = Keymap::basic();

    [[nodiscard]] Point scroll() const { return scroll_; }
    void scroll_to(Point p);
    void ensure_cursor_visible();
    [[nodiscard]] int visible_lines() const { return size().height; }

    // Column arithmetic lives behind these two so that tab expansion - and,
    // in M6, double-width glyphs - stay in one place.
    [[nodiscard]] int display_column(int line, int col) const;
    [[nodiscard]] int column_at_display(int line, int display_col) const;

protected:
    void paint(Canvas& c) override;
    bool on_key(const KeyEvent& ev) override;
    bool on_mouse(const MouseEvent& ev) override;
    [[nodiscard]] std::optional<Point> cursor() const override;
    void on_geometry(Rect old_rect, Rect new_rect) override;

private:
    bool apply(EditAction action);
    void rewatch();
    void emit_scrolled();

    TextBuffer* buffer_;
    Point scroll_;  // x = first visible display column, y = first visible line
    ScopedConnection changed_conn_;
    ScopedConnection cursor_conn_;
};

}  // namespace modcurses
