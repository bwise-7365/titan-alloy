#pragma once
//
// modcurses/widgets.hpp - the widget catalog.
//
// PUBLIC HEADER: no curses.
//
// The text-editing half of the catalog (TextBuffer, TextArea, Keymap) lives in
// modcurses/text.hpp, because it is a model plus a view rather than a widget.
//
#include <chrono>
#include <functional>
#include <utility>
#include <optional>
#include <string>
#include <vector>

#include "modcurses/core.hpp"
#include "modcurses/render.hpp"
#include "modcurses/widget.hpp"

namespace modcurses {

enum class Align { Left, Center, Right };

// ------------------------------------------------------------------- Label

// Static text. With wrap off it occupies one line and clips; with wrap on it
// breaks on word boundaries and asks for as many lines as it needs.
class Label : public Widget {
public:
    explicit Label(std::string text = "", Align align = Align::Left);

    void set_text(std::string text);
    [[nodiscard]] std::string text() const;
    void set_align(Align a);
    [[nodiscard]] Align align() const { return align_; }
    void set_wrap(bool on);
    [[nodiscard]] bool wrap() const { return wrap_; }

    [[nodiscard]] SizeReq width_req() const override;
    [[nodiscard]] SizeReq height_req() const override;

    // Breaks `text` into lines of at most `width` columns, on spaces where it
    // can and mid-word where a single word is too long to fit.
    [[nodiscard]] static std::vector<std::u32string> wrap_text(std::u32string_view text,
                                                               int width);

protected:
    void paint(Canvas& c) override;
    void on_geometry(Rect old_rect, Rect new_rect) override;

private:
    std::u32string text_;
    Align align_ = Align::Left;
    bool wrap_ = false;
};

// ------------------------------------------------------------------ Button

// Renders as "[ label ]" and highlights while focused.
class Button : public Widget {
public:
    explicit Button(std::string label = "");

    Signal<> pressed;

    void set_label(std::string label);
    [[nodiscard]] std::string label() const;

    // Applied on top of `style` while the button holds focus.
    Style focus_style = Style{}.with(Trait::Reverse);

    [[nodiscard]] SizeReq width_req() const override;
    [[nodiscard]] SizeReq height_req() const override;

protected:
    void paint(Canvas& c) override;
    bool on_key(const KeyEvent& ev) override;
    bool on_mouse(const MouseEvent& ev) override;
    void on_focus(bool gained) override;

private:
    std::u32string label_;
};

// ---------------------------------------------------------------- Checkbox

// Renders as "[x] label" / "[ ] label". Space or Enter toggles, as does a click.
class Checkbox : public Widget {
public:
    explicit Checkbox(std::string label = "", bool checked = false);

    Signal<bool> toggled;

    void set_checked(bool value);
    [[nodiscard]] bool checked() const { return checked_; }
    void toggle() { set_checked(!checked_); }

    void set_label(std::string label);
    [[nodiscard]] std::string label() const;

    Style focus_style = Style{}.with(Trait::Reverse);

    [[nodiscard]] SizeReq width_req() const override;
    [[nodiscard]] SizeReq height_req() const override;

protected:
    void paint(Canvas& c) override;
    bool on_key(const KeyEvent& ev) override;
    bool on_mouse(const MouseEvent& ev) override;
    void on_focus(bool gained) override;

private:
    std::u32string label_;
    bool checked_ = false;
};

// --------------------------------------------------------------- TextInput

// One line of editable text with a cursor and horizontal scrolling. Unlike
// TextArea it owns its own content rather than viewing a TextBuffer: a prompt
// line is not a document.
class TextInput : public Widget {
public:
    explicit TextInput(std::string text = "");

    Signal<std::u32string> submitted;  // Enter
    Signal<std::u32string> changed;    // any edit

    void set_text(std::string text);
    [[nodiscard]] std::string text() const;
    void set_placeholder(std::string text);
    [[nodiscard]] std::string placeholder() const;
    void clear();

    [[nodiscard]] int cursor_col() const { return cursor_; }
    void set_cursor_col(int col);

    bool read_only = false;
    int max_length = 0;  // 0 means unlimited
    Style placeholder_style = Style{}.with(Trait::Dim);

    [[nodiscard]] SizeReq height_req() const override;

protected:
    void paint(Canvas& c) override;
    bool on_key(const KeyEvent& ev) override;
    bool on_mouse(const MouseEvent& ev) override;
    [[nodiscard]] std::optional<Point> cursor() const override;
    void on_geometry(Rect old_rect, Rect new_rect) override;

private:
    void ensure_cursor_visible();
    void emit_changed();

    std::u32string text_;
    std::u32string placeholder_;
    int cursor_ = 0;  // column index into text_
    int scroll_ = 0;  // first visible column
};

// ---------------------------------------------------------------- Titlebar

// One-line header: title on the left, an optional right-aligned hint.
class Titlebar : public Widget {
public:
    explicit Titlebar(std::string title = "", std::string hint = "");

    void set_title(std::string title);
    void set_hint(std::string hint);
    [[nodiscard]] std::string title() const;
    [[nodiscard]] std::string hint() const;

    [[nodiscard]] SizeReq height_req() const override { return SizeReq::fixed(1); }

protected:
    void paint(Canvas& c) override;

private:
    std::u32string title_;
    std::u32string hint_;
};

// --------------------------------------------------------------- StatusBar

// One-line footer with a transient flash message layered over the base text.
class StatusBar : public Widget {
public:
    explicit StatusBar(std::string text = "");

    void set_text(std::string text);
    [[nodiscard]] std::string text() const;

    // Shows `text` for `duration`, then reverts to whatever set_text last
    // said. A second flash replaces the first rather than queueing.
    void flash(std::string text, std::chrono::milliseconds duration);
    void clear_flash();
    [[nodiscard]] bool flashing() const { return flashing_; }

    Align align = Align::Left;
    Style flash_style = Style{}.with(Trait::Bold);

    [[nodiscard]] SizeReq height_req() const override { return SizeReq::fixed(1); }

protected:
    void paint(Canvas& c) override;

private:
    std::u32string text_;
    std::u32string flash_text_;
    bool flashing_ = false;
    TimerHandle flash_timer_;
};

// ----------------------------------------------------------------- Divider

class Divider : public Widget {
public:
    enum class Orientation { Horizontal, Vertical };

    explicit Divider(Orientation o = Orientation::Horizontal,
                     BoxStyle box_style = BoxStyle::Light);

    [[nodiscard]] SizeReq width_req() const override;
    [[nodiscard]] SizeReq height_req() const override;

protected:
    void paint(Canvas& c) override;

private:
    Orientation orientation_;
    BoxStyle box_style_;
};

// ---------------------------------------------------------------- ScrollBar

// A vertical position indicator. It holds no opinion about what it is
// scrolling: pair it with a TextArea or ListView through their `scrolled`
// signal in one direction and `position_changed` in the other.
class ScrollBar : public Widget {
public:
    ScrollBar();

    Signal<int> position_changed;  // a new first-visible row is being requested

    // total = rows of content, visible = rows the viewport can show.
    void set_range(int total, int visible);
    void set_position(int first_visible);
    [[nodiscard]] int position() const { return position_; }
    [[nodiscard]] int total() const { return total_; }
    [[nodiscard]] int visible() const { return visible_; }
    [[nodiscard]] bool scrollable() const { return total_ > visible_; }

    bool interactive = true;
    Style thumb_style = Style{}.with(Trait::Reverse);

    [[nodiscard]] SizeReq width_req() const override { return SizeReq::fixed(1); }

protected:
    void paint(Canvas& c) override;
    bool on_mouse(const MouseEvent& ev) override;

private:
    // Thumb extent in rows, as [first, last) within the track.
    [[nodiscard]] std::pair<int, int> thumb_span(int track) const;

    int position_ = 0;
    int total_ = 0;
    int visible_ = 0;
};

// ----------------------------------------------------------------- ListView

// Homogeneous rows with a single selection, keyboard and wheel scrolling.
class ListView : public Widget {
public:
    ListView();

    Signal<int> activated;          // Enter, or a double click
    Signal<int> selection_changed;
    Signal<int, int> scrolled;      // (first visible row, total rows)

    void set_items(std::vector<std::string> items);
    void add_item(std::string item);
    void clear_items();
    [[nodiscard]] int item_count() const { return static_cast<int>(items_.size()); }
    [[nodiscard]] std::string item(int index) const;

    [[nodiscard]] int selected() const { return selected_; }
    void set_selected(int index);
    void activate_selected();

    [[nodiscard]] int scroll() const { return scroll_; }
    void scroll_to(int first_visible);
    [[nodiscard]] int visible_rows() const { return size().height; }
    void ensure_selection_visible();

    Style selected_style = Style{}.with(Trait::Reverse);

    [[nodiscard]] SizeReq width_req() const override;
    [[nodiscard]] SizeReq height_req() const override;

protected:
    void paint(Canvas& c) override;
    bool on_key(const KeyEvent& ev) override;
    bool on_mouse(const MouseEvent& ev) override;
    void on_geometry(Rect old_rect, Rect new_rect) override;

    // Menu overrides this to render its own row text.
    [[nodiscard]] virtual std::u32string row_text(int index) const;

private:
    void emit_scrolled();

    std::vector<std::u32string> items_;
    int selected_ = 0;
    int scroll_ = 0;
};

// -------------------------------------------------------------------- Menu

// A ListView whose entries carry callbacks.
class Menu : public ListView {
public:
    Menu();

    void add_entry(std::string label, std::function<void()> on_select);
    void clear_entries();

private:
    std::vector<std::function<void()>> callbacks_;
    ScopedConnection activated_conn_;
};

// -------------------------------------------------------------- GridCanvas

// A fixed logical grid of cells, mapped onto the screen at `cell_width`
// columns per cell. Two columns per cell gives roughly square cells, which is
// what a Tetris board or a chessboard wants.
class GridCanvas : public Widget {
public:
    GridCanvas(int columns = 1, int rows = 1, int cell_width = 1);

    void resize_grid(int columns, int rows);
    [[nodiscard]] int columns() const { return columns_; }
    [[nodiscard]] int rows() const { return rows_; }

    void set_cell_width(int width);
    [[nodiscard]] int cell_width() const { return cell_width_; }

    void set_cell(int x, int y, Glyph g);
    [[nodiscard]] Glyph cell(int x, int y) const;
    void fill_grid(Glyph g);
    void clear_grid() { fill_grid(Glyph{U' ', style}); }
    [[nodiscard]] bool in_grid(int x, int y) const {
        return x >= 0 && y >= 0 && x < columns_ && y < rows_;
    }

    // Maps a widget-local point to the grid cell under it, if any.
    [[nodiscard]] std::optional<Point> cell_at(Point local) const;

    [[nodiscard]] SizeReq width_req() const override;
    [[nodiscard]] SizeReq height_req() const override;

protected:
    void paint(Canvas& c) override;

private:
    [[nodiscard]] std::size_t index(int x, int y) const {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(columns_) +
               static_cast<std::size_t>(x);
    }

    int columns_ = 1;
    int rows_ = 1;
    int cell_width_ = 1;
    std::vector<Glyph> cells_;
    Glyph outside_{};  // returned for out-of-grid reads
};

}  // namespace modcurses
