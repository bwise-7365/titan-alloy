#include <memory>
#include <string>

#include "doctest.h"
#include "fixture.hpp"
#include "modcurses/app.hpp"
#include "modcurses/mock_terminal.hpp"
#include "modcurses/text.hpp"
#include "modcurses/utf8.hpp"
#include "modcurses/widgets.hpp"

using namespace modcurses;
using Fixture = modcurses::testing::AppFixture;
using Cursor = TextBuffer::Cursor;


// ----------------------------------------------------------------- Keymap

TEST_CASE("a keymap maps keys to editing verbs") {
    Keymap k;
    CHECK(k.lookup(key_ev(Key::Left)) == EditAction::None);

    k.bind(key_ev(Key::Left), EditAction::MoveLeft);
    CHECK(k.lookup(key_ev(Key::Left)) == EditAction::MoveLeft);
    CHECK(k.bindings().size() == 1);

    SUBCASE("rebinding replaces rather than shadows") {
        k.bind(key_ev(Key::Left), EditAction::MoveWordLeft);
        CHECK(k.bindings().size() == 1);
        CHECK(k.lookup(key_ev(Key::Left)) == EditAction::MoveWordLeft);
    }
    SUBCASE("unbind") {
        CHECK(k.unbind(key_ev(Key::Left)));
        CHECK(k.lookup(key_ev(Key::Left)) == EditAction::None);
        CHECK_FALSE(k.unbind(key_ev(Key::Left)));
    }
    SUBCASE("modifiers are part of the match") {
        k.bind(ctrl_ev(U'a'), EditAction::MoveLineStart);
        CHECK(k.lookup(ctrl_ev(U'a')) == EditAction::MoveLineStart);
        CHECK(k.lookup(char_ev(U'a')) == EditAction::None);  // plain 'a' is text
    }
}

TEST_CASE("the shipped keymaps differ where they are supposed to") {
    const Keymap basic = Keymap::basic();
    const Keymap nano = Keymap::nano();
    const Keymap emacs = Keymap::emacs();

    // All three agree on the plain navigation keys.
    for (const Keymap* k : {&basic, &nano, &emacs}) {
        CHECK(k->lookup(key_ev(Key::Left)) == EditAction::MoveLeft);
        CHECK(k->lookup(key_ev(Key::Enter)) == EditAction::InsertNewline);
        CHECK(k->lookup(key_ev(Key::Backspace)) == EditAction::Backspace);
    }
    // basic leaves Ctrl-letter chords to the application.
    CHECK(basic.lookup(ctrl_ev(U'a')) == EditAction::None);
    CHECK(basic.lookup(ctrl_ev(U'k')) == EditAction::None);

    CHECK(nano.lookup(ctrl_ev(U'a')) == EditAction::MoveLineStart);
    CHECK(nano.lookup(ctrl_ev(U'k')) == EditAction::DeleteLine);
    CHECK(nano.lookup(ctrl_ev(U'v')) == EditAction::PageDown);

    CHECK(emacs.lookup(ctrl_ev(U'f')) == EditAction::MoveRight);
    CHECK(emacs.lookup(ctrl_ev(U'p')) == EditAction::MoveUp);
    CHECK(emacs.lookup(alt_ev(U'<')) == EditAction::MoveBufferStart);
}

TEST_CASE("to_string covers every action") {
    CHECK(std::string{to_string(EditAction::None)} == "None");
    CHECK(std::string{to_string(EditAction::MoveWordRight)} == "MoveWordRight");
}

// --------------------------------------------------------------- TextArea

TEST_CASE("a TextArea renders its buffer from the top") {
    Fixture f{Size{12, 4}};
    TextBuffer buf{U"one\ntwo\nthree"};
    f.app.make_root<TextArea>(buf);
    f.pump();

    CHECK(f.term->row_text(0) == "one         ");
    CHECK(f.term->row_text(1) == "two         ");
    CHECK(f.term->row_text(2) == "three       ");
    CHECK(f.term->row_text(3) == "            ");
}

TEST_CASE("typing goes into the buffer and moves the cursor") {
    Fixture f{Size{12, 3}};
    TextBuffer buf;
    auto& area = f.app.make_root<TextArea>(buf);
    f.pump();
    area.take_focus();

    f.term->feed_text(U"hi");
    f.sync();
    CHECK(buf.text() == U"hi");
    CHECK(buf.cursor() == Cursor{0, 2});
    CHECK(f.term->row_text(0) == "hi          ");
    REQUIRE(f.term->cursor().has_value());
    CHECK(*f.term->cursor() == Point{2, 0});
}

TEST_CASE("Enter splits the line, Backspace joins it back") {
    Fixture f{Size{12, 3}};
    TextBuffer buf{U"ab"};
    auto& area = f.app.make_root<TextArea>(buf);
    f.pump();
    area.take_focus();
    buf.set_cursor({0, 1});

    f.term->feed(key_ev(Key::Enter));
    f.pump();
    CHECK(buf.text() == U"a\nb");

    f.term->feed(key_ev(Key::Backspace));
    f.pump();
    CHECK(buf.text() == U"ab");
}

TEST_CASE("a read-only area ignores edit keys") {
    Fixture f{Size{12, 3}};
    TextBuffer buf{U"fixed"};
    auto& area = f.app.make_root<TextArea>(buf);
    area.read_only = true;
    f.pump();
    area.take_focus();

    f.term->feed_text(U"x");
    f.term->feed(key_ev(Key::Enter));
    f.term->feed(key_ev(Key::Backspace));
    f.pump(3);
    CHECK(buf.text() == U"fixed");

    SUBCASE("movement still works in a read-only area") {
        f.term->feed(key_ev(Key::End));
        f.pump();
        CHECK(buf.cursor() == Cursor{0, 5});
    }
}

TEST_CASE("a swapped keymap changes which keys edit") {
    Fixture f{Size{12, 3}};
    TextBuffer buf{U"hello world"};
    auto& area = f.app.make_root<TextArea>(buf);
    area.keymap = Keymap::emacs();
    f.pump();
    area.take_focus();
    buf.set_cursor({0, 0});

    f.term->feed(ctrl_ev(U'f'));  // emacs: forward char
    f.pump();
    CHECK(buf.cursor() == Cursor{0, 1});

    f.term->feed(ctrl_ev(U'e'));  // emacs: end of line
    f.pump();
    CHECK(buf.cursor() == Cursor{0, 11});

    SUBCASE("under the basic map the same chord is not an edit at all") {
        area.keymap = Keymap::basic();
        buf.set_cursor({0, 0});
        f.term->feed(ctrl_ev(U'f'));
        f.pump();
        CHECK(buf.cursor() == Cursor{0, 0});
        CHECK(buf.text() == U"hello world");  // and it did not get typed either
    }
}

TEST_CASE("the view scrolls vertically to keep the cursor visible") {
    Fixture f{Size{12, 3}};
    TextBuffer buf{U"l0\nl1\nl2\nl3\nl4\nl5"};
    auto& area = f.app.make_root<TextArea>(buf);
    f.pump();
    area.take_focus();
    CHECK(area.scroll().y == 0);

    buf.set_cursor({5, 0});
    f.pump();
    CHECK(area.scroll().y == 3);  // last three lines
    CHECK(f.term->row_text(0) == "l3          ");
    CHECK(f.term->row_text(2) == "l5          ");

    buf.set_cursor({0, 0});
    f.pump();
    CHECK(area.scroll().y == 0);
}

TEST_CASE("the view scrolls horizontally too") {
    Fixture f{Size{6, 2}};
    TextBuffer buf{U"abcdefghijkl"};
    auto& area = f.app.make_root<TextArea>(buf);
    f.pump();
    area.take_focus();

    buf.set_cursor({0, 12});
    f.pump();
    CHECK(area.scroll().x == 7);
    CHECK(f.term->row_text(0) == "hijkl ");
}

TEST_CASE("the scrolled signal reports position and total, for a ScrollBar") {
    Fixture f{Size{12, 3}};
    TextBuffer buf{U"a\nb\nc\nd\ne"};
    auto& area = f.app.make_root<TextArea>(buf);
    int top = -1, total = -1;
    auto conn = area.scrolled.connect([&](int t, int n) {
        top = t;
        total = n;
    });
    f.pump();

    buf.set_cursor({4, 0});
    f.pump();
    CHECK(top == 2);
    CHECK(total == 5);
}

TEST_CASE("tabs expand for display without changing the stored text") {
    Fixture f{Size{16, 2}};
    TextBuffer buf{U"\tx"};
    auto& area = f.app.make_root<TextArea>(buf);
    area.tab_width = 4;
    f.pump();

    CHECK(buf.line(0) == U"\tx");           // one tab, still one character
    CHECK(f.term->row_text(0) == "    x           ");
    CHECK(area.display_column(0, 0) == 0);
    CHECK(area.display_column(0, 1) == 4);  // the tab occupies four columns
    CHECK(area.display_column(0, 2) == 5);

    SUBCASE("a tab mid-line advances to the next stop, not by a full width") {
        buf.set_text(U"ab\tc");
        f.pump();
        CHECK(area.display_column(0, 3) == 4);  // 'ab' then tab -> column 4
        CHECK(f.term->row_text(0) == "ab  c           ");
    }
    SUBCASE("column_at_display is the inverse") {
        CHECK(area.column_at_display(0, 0) == 0);
        CHECK(area.column_at_display(0, 4) == 1);
        CHECK(area.column_at_display(0, 99) == 2);
    }
}

TEST_CASE("Tab indents instead of moving focus, unless told otherwise") {
    Fixture f{Size{16, 3}};
    TextBuffer buf;
    auto& area = f.app.make_root<TextArea>(buf);
    f.pump();
    area.take_focus();

    f.term->feed(key_ev(Key::Tab));
    f.pump();
    CHECK(buf.line(0) == U"\t");
    CHECK(area.has_focus());

    SUBCASE("with capture_tab off it is focus traversal again") {
        buf.clear();
        area.capture_tab = false;
        f.term->feed(key_ev(Key::Tab));
        f.pump();
        CHECK(buf.empty());
    }
}

TEST_CASE("clicking positions the cursor, wheel scrolls the view") {
    Fixture f{Size{12, 3}};
    TextBuffer buf{U"aaaa\nbbbb\ncccc\ndddd\neeee\nffff"};
    auto& area = f.app.make_root<TextArea>(buf);
    f.pump();

    f.term->feed(
        Event{MouseEvent{MouseEvent::Button::Left, MouseEvent::Action::Press, {2, 1}, {}}});
    f.pump();
    CHECK(buf.cursor() == Cursor{1, 2});

    f.term->feed(Event{MouseEvent{MouseEvent::Button::WheelDown, MouseEvent::Action::Press,
                                  {0, 0}, {}}});
    f.pump();
    CHECK(area.scroll().y == 3);
    CHECK(buf.cursor() == Cursor{1, 2});  // scrolling does not move the cursor
}

TEST_CASE("the cursor is hidden when it scrolls out of view") {
    Fixture f{Size{12, 3}};
    TextBuffer buf{U"a\nb\nc\nd\ne\nf"};
    auto& area = f.app.make_root<TextArea>(buf);
    f.pump();
    area.take_focus();
    buf.set_cursor({0, 0});
    f.pump();
    REQUIRE(f.term->cursor().has_value());

    area.scroll_to({0, 3});
    f.pump();
    CHECK_FALSE(f.term->cursor().has_value());
}

TEST_CASE("two views can share one buffer") {
    Fixture f{Size{12, 4}};
    TextBuffer buf{U"shared"};
    auto& root = f.app.make_root<VBox>();
    auto& top = root.emplace_child<TextArea>(buf);
    auto& bottom = root.emplace_child<TextArea>(buf);
    f.pump();

    CHECK(&top.buffer() == &bottom.buffer());
    CHECK(f.term->row_text(0) == "shared      ");
    CHECK(f.term->row_text(2) == "shared      ");

    top.take_focus();
    f.term->feed_text(U"!");
    f.sync();
    // The edit went through the buffer, so BOTH views show it.
    CHECK(f.term->row_text(0) == "!shared     ");
    CHECK(f.term->row_text(2) == "!shared     ");
}

TEST_CASE("a read-only TextArea DECLINES keys it cannot use, so they reach the app") {
    // Regression. A read-only TextArea used as a help pager used to CONSUME
    // every printable key on the theory that an edit attempt should not be
    // mistaken for a shortcut. That froze a real application: once the pager
    // had focus - one mouse click was enough - there was no way to dismiss it
    // or even quit, because every key died inside a widget that could not act
    // on any of them. A widget that cannot act on a key has not handled it.
    Fixture f{Size{20, 5}};
    TextBuffer buf{U"help text"};
    auto& root = f.app.make_root<VBox>();

    std::string seen;
    root.on_key_hook = [&](const KeyEvent& ev) {
        if (ev.key == Key::Char) seen += utf8_encode(ev.text);
        if (ev.key == Key::Enter) seen += "<enter>";
        if (ev.key == Key::Backspace) seen += "<bs>";
        return true;
    };
    auto& area = root.emplace_child<TextArea>(buf);
    area.read_only = true;
    f.sync();
    area.take_focus();
    REQUIRE(area.has_focus());

    f.term->feed_text(U"dq");
    f.term->feed(key_ev(Key::Enter));
    f.term->feed(key_ev(Key::Backspace));
    f.sync();

    CHECK(seen == "dq<enter><bs>");
    CHECK(buf.text() == U"help text");  // and none of it edited anything
}

TEST_CASE("a read-only TextArea still handles the keys that make it a pager") {
    Fixture f{Size{12, 2}};
    TextBuffer buf{U"l0\nl1\nl2\nl3\nl4\nl5"};
    auto& root = f.app.make_root<VBox>();
    int reached_app = 0;
    root.on_key_hook = [&](const KeyEvent&) {
        ++reached_app;
        return true;
    };
    auto& area = root.emplace_child<TextArea>(buf);
    area.read_only = true;
    f.sync();
    area.take_focus();

    f.term->feed(key_ev(Key::Down));
    f.term->feed(key_ev(Key::PageDown));
    f.sync();
    CHECK(reached_app == 0);            // navigation is the pager's own business
    CHECK(buf.cursor().line > 0);
}

TEST_CASE("a writable TextArea still consumes the text it inserts") {
    Fixture f{Size{20, 3}};
    TextBuffer buf;
    auto& root = f.app.make_root<VBox>();
    int reached_app = 0;
    root.on_key_hook = [&](const KeyEvent&) {
        ++reached_app;
        return true;
    };
    auto& area = root.emplace_child<TextArea>(buf);
    f.sync();
    area.take_focus();

    f.term->feed_text(U"hi");
    f.sync();
    CHECK(buf.text() == U"hi");
    CHECK(reached_app == 0);  // typed text belongs to the editor, not the app
}

TEST_CASE("Tab is focus traversal in a read-only area, indentation in a writable one") {
    Fixture f{Size{20, 3}};
    TextBuffer buf;
    auto& root = f.app.make_root<VBox>();
    auto& area = root.emplace_child<TextArea>(buf);
    auto& button = root.emplace_child<Button>("b");
    button.height_hint = SizeReq::fixed(1);
    area.read_only = true;
    f.sync();
    area.take_focus();

    f.term->feed(key_ev(Key::Tab));
    f.sync();
    CHECK(buf.empty());          // nothing indented
    CHECK(button.has_focus());   // focus moved on instead
}
