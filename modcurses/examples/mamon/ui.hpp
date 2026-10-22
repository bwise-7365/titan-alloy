#pragma once
//
// mamon - nano's screen furniture.
//
// nano's layout, from the manual: a title bar showing "the version number,
// the current filename, and whether or not the file has been modified"; the
// text window; a status bar for messages and prompts; and "the two lines at
// the bottom of the screen show some important commands".
//
//     [ mamon 1.0            filename                    Modified ]  <- reverse
//     text ...
//     [ status / prompt line                                      ]
//     ^G Help      ^O Write Out ^W Where Is  ^K Cut  ...             <- keys reversed
//     ^X Exit      ^R Read File ^\ Replace   ^U Paste ...
//
#include <string>
#include <vector>

#include "modcurses/text.hpp"   // TextArea, which LineNumbers follows
#include "modcurses/widget.hpp"
#include "modcurses/widgets.hpp"

namespace mamon {

using namespace modcurses;

// Title bar: version left, filename centred, "Modified" right.
//
// White on blue, as nano draws it when colour is configured (titlecolor).
// These are colour SLOTS, not redefined RGB - see the MTetris notes on why
// that distinction matters: a slot change is visible on every terminal.
class TitleBar : public Widget {
public:
    TitleBar() { style = Style{}.with_fg(Color::BrightWhite).with_bg(Color::Blue); }

    void set_version(std::string v);
    void set_filename(std::string name);  // empty renders as "New Buffer"
    void set_modified(bool m);

    [[nodiscard]] SizeReq height_req() const override { return SizeReq::fixed(1); }

protected:
    void paint(Canvas& c) override;

private:
    std::u32string version_ = U"mamon 1.0";
    std::u32string filename_;
    bool modified_ = false;
};

// One entry of the bottom help bar: the key, then what it does.
struct HelpEntry {
    std::u32string key;    // "^G", "M-U"
    std::u32string label;  // "Help", "Undo"
};

// The two help lines. The key is drawn in reverse video and the description
// plain, which is how nano distinguishes them.
class HelpBar : public Widget {
public:
    HelpBar();

    // nano's keycolor and functioncolor: the key in blue, what it does in
    // green.
    Style key_style = Style{}.with_fg(Color::Blue);
    Style label_style = Style{}.with_fg(Color::Green);

    // Column-major: entries[i] is the upper row, entries[i+1] the lower.
    void set_entries(std::vector<HelpEntry> entries);
    void set_default_entries();
    // The reduced bar nano shows while a prompt is up.
    void set_prompt_entries();
    // ...and for a yes/no question, where the answers are the shortcuts.
    void set_yesno_entries();

    [[nodiscard]] SizeReq height_req() const override { return SizeReq::fixed(2); }

    // How many columns fit at the current width; nano packs as many as it can.
    [[nodiscard]] int columns_shown() const;

protected:
    void paint(Canvas& c) override;

private:
    std::vector<HelpEntry> entries_;
};

// The message / prompt line. Messages are centred in reverse video the way
// nano shows "[ Read 5 lines ]"; a prompt is left-aligned and editable.
class MessageBar : public Widget {
public:
    MessageBar() = default;

    void show_message(std::string text);
    void clear();
    [[nodiscard]] bool has_message() const { return !message_.empty(); }

    // nano's statuscolor: white on green, always.
    Style message_style = Style{}.with_fg(Color::BrightWhite).with_bg(Color::Green);

    [[nodiscard]] SizeReq height_req() const override { return SizeReq::fixed(1); }

protected:
    void paint(Canvas& c) override;

private:
    std::u32string message_;
};

// The line-number gutter for -l / --linenumbers. It follows a TextArea's
// scroll position, so the numbers stay lined up with the text beside them.
class LineNumbers : public Widget {
public:
    LineNumbers() = default;

    // Attached after construction: the gutter has to be created before the
    // TextArea so that it sits to its left, which means it cannot take the
    // view as a constructor argument.
    void set_view(const TextArea& view) { view_ = &view; }

    [[nodiscard]] SizeReq width_req() const override;

protected:
    void paint(Canvas& c) override;

private:
    const TextArea* view_ = nullptr;
};

}  // namespace mamon
