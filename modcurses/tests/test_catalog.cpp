#include <memory>
#include <string>
#include <vector>

#include "doctest.h"
#include "fixture.hpp"
#include "modcurses/app.hpp"
#include "modcurses/mock_terminal.hpp"
#include "modcurses/utf8.hpp"
#include "modcurses/widgets.hpp"

using namespace modcurses;
using Fixture = modcurses::testing::AppFixture;


// -------------------------------------------------------------- Label wrap

TEST_CASE("wrap_text breaks on spaces") {
    const auto lines = Label::wrap_text(U"hello world foo", 11);
    REQUIRE(lines.size() == 2);
    CHECK(lines[0] == U"hello world");
    CHECK(lines[1] == U"foo");
}

TEST_CASE("wrap_text breaks mid-word only when a word cannot fit at all") {
    const auto lines = Label::wrap_text(U"abcdefghij", 4);
    CHECK(lines == std::vector<std::u32string>{U"abcd", U"efgh", U"ij"});
}

TEST_CASE("wrap_text degenerate inputs") {
    CHECK(Label::wrap_text(U"anything", 0).empty());
    CHECK(Label::wrap_text(U"anything", -1).empty());
    CHECK(Label::wrap_text(U"", 10) == std::vector<std::u32string>{U""});
}

TEST_CASE("a wrapping Label asks for the height it needs and paints every line") {
    Fixture f{Size{11, 4}};
    auto& root = f.app.make_root<VBox>();
    auto& label = root.emplace_child<Label>("hello world foo");
    label.set_wrap(true);
    f.sync();

    CHECK(label.rect().size.height == 2);
    CHECK(f.term->row_text(0) == "hello world");
    CHECK(f.term->row_text(1) == "foo        ");

    SUBCASE("without wrap it is one clipped line") {
        label.set_wrap(false);
        f.sync();
        CHECK(label.rect().size.height == 1);
        CHECK(f.term->row_text(0) == "hello world");
        CHECK(f.term->row_text(1) == "           ");
    }
}

// ---------------------------------------------------------------- Checkbox

TEST_CASE("Checkbox renders its state and toggles on Space, Enter and click") {
    Fixture f{Size{16, 1}};
    auto& box = f.app.make_root<Checkbox>("Enabled");
    std::vector<bool> seen;
    auto conn = box.toggled.connect([&](bool v) { seen.push_back(v); });
    f.pump();
    CHECK(f.term->row_text(0) == "[ ] Enabled     ");

    box.take_focus();
    f.term->feed(char_ev(U' '));
    f.sync();
    CHECK(box.checked());
    CHECK(f.term->row_text(0) == "[x] Enabled     ");

    f.term->feed(key_ev(Key::Enter));
    f.pump();
    CHECK_FALSE(box.checked());

    f.term->feed(Event{MouseEvent{MouseEvent::Button::Left, MouseEvent::Action::Press, {1, 0}, {}}});
    f.pump();
    CHECK(box.checked());
    CHECK(seen == std::vector<bool>{true, false, true});
}

TEST_CASE("setting a Checkbox to its current value signals nothing") {
    Checkbox box{"x", true};
    int signals = 0;
    auto conn = box.toggled.connect([&](bool) { ++signals; });
    box.set_checked(true);
    CHECK(signals == 0);
    box.set_checked(false);
    CHECK(signals == 1);
}

// --------------------------------------------------------------- TextInput

TEST_CASE("TextInput accepts typing and reports changes") {
    Fixture f{Size{12, 1}};
    auto& input = f.app.make_root<TextInput>();
    std::string last;
    auto conn = input.changed.connect([&](const std::u32string& s) { last = utf8_encode(s); });
    f.pump();
    input.take_focus();

    f.term->feed_text(U"abc");
    f.sync();
    CHECK(input.text() == "abc");
    CHECK(last == "abc");
    CHECK(f.term->row_text(0) == "abc         ");
    CHECK(*f.term->cursor() == Point{3, 0});
}

TEST_CASE("TextInput editing keys") {
    Fixture f{Size{12, 1}};
    auto& input = f.app.make_root<TextInput>("hello");
    f.pump();
    input.take_focus();

    SUBCASE("Backspace at the cursor") {
        f.term->feed(key_ev(Key::Backspace));
        f.pump();
        CHECK(input.text() == "hell");
    }
    SUBCASE("Home, then Delete") {
        f.term->feed(key_ev(Key::Home));
        f.term->feed(key_ev(Key::Delete));
        f.pump(2);
        CHECK(input.text() == "ello");
        CHECK(input.cursor_col() == 0);
    }
    SUBCASE("arrows move within the text and stop at the ends") {
        f.term->feed(key_ev(Key::Home));
        f.term->feed(key_ev(Key::Left));
        f.pump(2);
        CHECK(input.cursor_col() == 0);
        f.term->feed(key_ev(Key::End));
        f.term->feed(key_ev(Key::Right));
        f.pump(2);
        CHECK(input.cursor_col() == 5);
    }
    SUBCASE("Enter submits without clearing") {
        std::string submitted;
        auto conn = input.submitted.connect(
            [&](const std::u32string& s) { submitted = utf8_encode(s); });
        f.term->feed(key_ev(Key::Enter));
        f.pump();
        CHECK(submitted == "hello");
        CHECK(input.text() == "hello");
    }
}

TEST_CASE("TextInput scrolls horizontally rather than overflowing") {
    Fixture f{Size{5, 1}};
    auto& input = f.app.make_root<TextInput>();
    f.pump();
    input.take_focus();

    f.term->feed_text(U"abcdefgh");
    f.sync();
    CHECK(input.text() == "abcdefgh");
    CHECK(f.term->row_text(0) == "efgh ");   // the tail, with room for the cursor
    CHECK(*f.term->cursor() == Point{4, 0});
}

TEST_CASE("TextInput respects read_only and max_length") {
    Fixture f{Size{12, 1}};
    auto& input = f.app.make_root<TextInput>("ab");
    f.pump();
    input.take_focus();

    SUBCASE("read_only swallows edits but still allows movement") {
        input.read_only = true;
        f.term->feed_text(U"z");
        f.term->feed(key_ev(Key::Backspace));
        f.pump(2);
        CHECK(input.text() == "ab");
        f.term->feed(key_ev(Key::Home));
        f.pump();
        CHECK(input.cursor_col() == 0);
    }
    SUBCASE("max_length caps insertion") {
        input.max_length = 3;
        f.term->feed_text(U"xyz");
        f.pump(3);
        CHECK(input.text() == "abx");
    }
}

TEST_CASE("TextInput shows a placeholder only when empty and unfocused") {
    Fixture f{Size{12, 1}};
    auto& input = f.app.make_root<TextInput>();
    input.set_placeholder("type here");
    f.pump();
    CHECK(f.term->row_text(0) == "type here   ");
    CHECK(f.term->cell_at({0, 0}).style.has(Trait::Dim));

    input.take_focus();
    f.pump();
    CHECK(f.term->row_text(0) == "            ");
}

// --------------------------------------------------------------- StatusBar

TEST_CASE("StatusBar shows its text, and a flash on top of it") {
    Fixture f{Size{16, 1}};
    auto& bar = f.app.make_root<StatusBar>("ready");
    f.pump();
    CHECK(f.term->row_text(0) == " ready          ");
    CHECK_FALSE(bar.flashing());

    bar.flash("saved!", std::chrono::milliseconds{1});
    f.pump();
    CHECK(bar.flashing());
    CHECK(f.term->row_text(0) == " saved!         ");
    CHECK(f.term->cell_at({1, 0}).style.has(Trait::Bold));

    // The flash is a self-cancelling repeating timer. Pump until it expires;
    // the bound keeps a broken timer from hanging the suite.
    for (int i = 0; i < 2000000 && bar.flashing(); ++i) f.app.pump_once();
    CHECK_FALSE(bar.flashing());
    f.pump();
    CHECK(f.term->row_text(0) == " ready          ");
    CHECK(f.app.loop().timer_count() == 0);
}

TEST_CASE("clear_flash reverts immediately") {
    Fixture f{Size{16, 1}};
    auto& bar = f.app.make_root<StatusBar>("base");
    bar.flash("temp", std::chrono::seconds{60});
    f.pump();
    CHECK(f.term->row_text(0) == " temp           ");

    bar.clear_flash();
    f.pump();
    CHECK(f.term->row_text(0) == " base           ");
    CHECK_FALSE(bar.flashing());
}

// ----------------------------------------------------------------- Divider

TEST_CASE("Divider draws a one-cell rule on its thin axis") {
    SUBCASE("horizontal") {
        Fixture f{Size{6, 3}};
        auto& root = f.app.make_root<VBox>();
        auto& d = root.emplace_child<Divider>();
        root.emplace_child<Widget>();
        f.pump();
        CHECK(d.rect().size.height == 1);
        CHECK(f.term->row_text(0) == "──────");
    }
    SUBCASE("vertical") {
        Fixture f{Size{3, 2}};
        auto& root = f.app.make_root<HBox>();
        auto& d = root.emplace_child<Divider>(Divider::Orientation::Vertical);
        root.emplace_child<Widget>();
        f.pump();
        CHECK(d.rect().size.width == 1);
        CHECK(f.term->row_text(0) == "│  ");
        CHECK(f.term->row_text(1) == "│  ");
    }
}

// ---------------------------------------------------------------- ScrollBar

TEST_CASE("ScrollBar thumb tracks position and size") {
    Fixture f{Size{1, 10}};
    auto& bar = f.app.make_root<ScrollBar>();
    bar.set_range(100, 10);
    f.pump();
    CHECK(bar.scrollable());

    // 10 of 100 rows visible in a 10-row track: a one-row thumb at the top.
    CHECK(f.term->cell_at({0, 0}).style.has(Trait::Reverse));
    CHECK_FALSE(f.term->cell_at({0, 5}).style.has(Trait::Reverse));

    bar.set_position(90);  // fully scrolled
    f.pump();
    CHECK(f.term->cell_at({0, 9}).style.has(Trait::Reverse));
    CHECK_FALSE(f.term->cell_at({0, 0}).style.has(Trait::Reverse));
}

TEST_CASE("ScrollBar clamps position to the range") {
    ScrollBar bar;
    bar.set_range(20, 5);
    bar.set_position(999);
    CHECK(bar.position() == 15);
    bar.set_position(-5);
    CHECK(bar.position() == 0);

    SUBCASE("shrinking the content re-clamps an out-of-range position") {
        bar.set_position(15);
        bar.set_range(6, 5);
        CHECK(bar.position() == 1);
    }
    SUBCASE("content that fits is not scrollable") {
        bar.set_range(3, 5);
        CHECK_FALSE(bar.scrollable());
        CHECK(bar.position() == 0);
    }
}

TEST_CASE("ScrollBar requests a new position when clicked or scrolled") {
    Fixture f{Size{1, 10}};
    auto& bar = f.app.make_root<ScrollBar>();
    bar.set_range(100, 10);
    int requested = -1;
    auto conn = bar.position_changed.connect([&](int p) { requested = p; });
    f.pump();

    f.term->feed(Event{MouseEvent{MouseEvent::Button::Left, MouseEvent::Action::Press, {0, 9}, {}}});
    f.pump();
    CHECK(requested == 90);

    f.term->feed(
        Event{MouseEvent{MouseEvent::Button::WheelDown, MouseEvent::Action::Press, {0, 0}, {}}});
    f.pump();
    CHECK(requested == 1);

    SUBCASE("a non-interactive bar ignores the mouse") {
        bar.interactive = false;
        requested = -1;
        f.term->feed(
            Event{MouseEvent{MouseEvent::Button::Left, MouseEvent::Action::Press, {0, 4}, {}}});
        f.pump();
        CHECK(requested == -1);
    }
}

// ----------------------------------------------------------------- ListView

TEST_CASE("ListView renders rows and moves the selection with the keyboard") {
    Fixture f{Size{10, 3}};
    auto& list = f.app.make_root<ListView>();
    list.set_items({"alpha", "beta", "gamma", "delta"});
    f.pump();
    list.take_focus();
    f.pump();

    CHECK(f.term->row_text(0) == "alpha     ");
    CHECK(f.term->row_text(2) == "gamma     ");
    CHECK(list.selected() == 0);
    CHECK(f.term->cell_at({0, 0}).style.has(Trait::Reverse));

    f.term->feed(key_ev(Key::Down));
    f.sync();
    CHECK(list.selected() == 1);
    CHECK(f.term->cell_at({0, 1}).style.has(Trait::Reverse));

    SUBCASE("End jumps to the last item and scrolls to it") {
        f.term->feed(key_ev(Key::End));
        f.sync();
        CHECK(list.selected() == 3);
        CHECK(list.scroll() == 1);
        CHECK(f.term->row_text(2) == "delta     ");
    }
    SUBCASE("the selection cannot leave the list") {
        f.term->feed(key_ev(Key::Up));
        f.term->feed(key_ev(Key::Up));
        f.pump(2);
        CHECK(list.selected() == 0);
    }
}

TEST_CASE("ListView activation") {
    Fixture f{Size{10, 3}};
    auto& list = f.app.make_root<ListView>();
    list.set_items({"a", "b"});
    std::vector<int> activated;
    auto conn = list.activated.connect([&](int i) { activated.push_back(i); });
    f.pump();
    list.take_focus();

    f.term->feed(key_ev(Key::Enter));
    f.pump();
    CHECK(activated == std::vector<int>{0});

    SUBCASE("clicking an unselected row selects it without activating") {
        f.term->feed(
            Event{MouseEvent{MouseEvent::Button::Left, MouseEvent::Action::Press, {0, 1}, {}}});
        f.pump();
        CHECK(list.selected() == 1);
        CHECK(activated.size() == 1);

        // Clicking it again activates: double-click reporting is unreliable
        // across backends, so re-clicking the selection is the sure path.
        f.term->feed(
            Event{MouseEvent{MouseEvent::Button::Left, MouseEvent::Action::Press, {0, 1}, {}}});
        f.pump();
        CHECK(activated == std::vector<int>{0, 1});
    }
}

TEST_CASE("ListView wheel scrolls without moving the selection") {
    Fixture f{Size{10, 2}};
    auto& list = f.app.make_root<ListView>();
    list.set_items({"0", "1", "2", "3", "4"});
    f.pump();

    f.term->feed(
        Event{MouseEvent{MouseEvent::Button::Left, MouseEvent::Action::Press, {0, 0}, {}}});
    f.pump();
    const int before = list.selected();

    f.term->feed(
        Event{MouseEvent{MouseEvent::Button::WheelDown, MouseEvent::Action::Press, {0, 0}, {}}});
    f.sync();
    CHECK(list.scroll() == 1);
    CHECK(list.selected() == before);
    CHECK(f.term->row_text(0) == "1         ");
}

TEST_CASE("ListView reports scroll position for a ScrollBar to follow") {
    Fixture f{Size{10, 2}};
    auto& list = f.app.make_root<ListView>();
    int top = -1, total = -1;
    auto conn = list.scrolled.connect([&](int t, int n) {
        top = t;
        total = n;
    });
    list.set_items({"0", "1", "2", "3"});
    f.pump();
    list.take_focus();

    f.term->feed(key_ev(Key::End));
    f.pump();
    CHECK(top == 2);
    CHECK(total == 4);
}

TEST_CASE("an empty ListView is inert rather than a crash") {
    Fixture f{Size{10, 3}};
    auto& list = f.app.make_root<ListView>();
    f.pump();
    list.take_focus();
    f.term->feed(key_ev(Key::Down));
    f.term->feed(key_ev(Key::Enter));
    f.pump(2);
    CHECK(list.item_count() == 0);
    CHECK(list.item(0).empty());
}

// -------------------------------------------------------------------- Menu

TEST_CASE("Menu entries invoke their callbacks when activated") {
    Fixture f{Size{12, 3}};
    auto& menu = f.app.make_root<Menu>();
    std::string chosen;
    menu.add_entry("Open", [&] { chosen = "open"; });
    menu.add_entry("Save", [&] { chosen = "save"; });
    menu.add_entry("Quit", [&] { chosen = "quit"; });
    f.pump();
    menu.take_focus();

    CHECK(menu.item_count() == 3);
    CHECK(f.term->row_text(1) == "Save        ");

    f.term->feed(key_ev(Key::Down));
    f.term->feed(key_ev(Key::Enter));
    f.pump(2);
    CHECK(chosen == "save");

    SUBCASE("clear_entries drops labels and callbacks together") {
        menu.clear_entries();
        CHECK(menu.item_count() == 0);
        f.term->feed(key_ev(Key::Enter));
        f.pump();
        CHECK(chosen == "save");  // nothing new fired
    }
}

// -------------------------------------------------------------- GridCanvas

TEST_CASE("GridCanvas holds a logical grid and paints it") {
    Fixture f{Size{8, 3}};
    auto& grid = f.app.make_root<GridCanvas>(4, 2, 1);
    grid.set_cell(0, 0, Glyph{U'#', fg(Color::Red)});
    grid.set_cell(3, 1, Glyph{U'@', {}});
    f.pump();

    CHECK(grid.columns() == 4);
    CHECK(grid.rows() == 2);
    CHECK(f.term->row_text(0) == "#       ");
    CHECK(f.term->row_text(1) == "   @    ");
    CHECK(f.term->cell_at({0, 0}).style.fg == Color::Red);
    CHECK(grid.cell(0, 0).ch == U'#');
}

TEST_CASE("a cell wider than one column paints as a block") {
    Fixture f{Size{8, 2}};
    auto& grid = f.app.make_root<GridCanvas>(3, 1, 2);
    grid.set_cell(1, 0, Glyph{U'X', bg(Color::Blue)});
    f.pump();

    CHECK(grid.width_req() == SizeReq::fixed(6));
    CHECK(f.term->row_text(0) == "  X     ");
    // The pad cell carries the block's style, which is what makes 2x1 cells
    // read as square rather than as a stretched character.
    CHECK(f.term->cell_at({3, 0}).style.bg == Color::Blue);
}

TEST_CASE("out-of-grid access is dropped rather than clamped") {
    GridCanvas grid{2, 2, 1};
    grid.set_cell(-1, 0, Glyph{U'!', {}});
    grid.set_cell(5, 5, Glyph{U'!', {}});
    CHECK(grid.cell(0, 0).ch == U' ');
    CHECK(grid.cell(99, 99).ch == U' ');
    CHECK_FALSE(grid.in_grid(2, 0));
    CHECK(grid.in_grid(1, 1));
}

TEST_CASE("cell_at maps a local point back to a grid cell") {
    GridCanvas grid{3, 2, 2};
    CHECK(grid.cell_at({0, 0}) == Point{0, 0});
    CHECK(grid.cell_at({1, 0}) == Point{0, 0});  // second column of cell 0
    CHECK(grid.cell_at({2, 0}) == Point{1, 0});
    CHECK(grid.cell_at({5, 1}) == Point{2, 1});
    CHECK_FALSE(grid.cell_at({6, 0}).has_value());
    CHECK_FALSE(grid.cell_at({0, 2}).has_value());
    CHECK_FALSE(grid.cell_at({-1, 0}).has_value());
}

TEST_CASE("resizing the grid clears it and updates the size request") {
    GridCanvas grid{2, 2, 1};
    grid.set_cell(0, 0, Glyph{U'#', {}});
    grid.resize_grid(10, 20);
    CHECK(grid.columns() == 10);
    CHECK(grid.rows() == 20);
    CHECK(grid.cell(0, 0).ch == U' ');
    CHECK(grid.width_req() == SizeReq::fixed(10));
    CHECK(grid.height_req() == SizeReq::fixed(20));
}

TEST_CASE("set_text leaves the cursor at the end, like construction does") {
    // Regression: set_text used to CLAMP the cursor, so a field seeded from
    // empty kept it at column 0. Pre-filling a prompt with a suggested value
    // then meant typing was prepended to it and backspace did nothing.
    Fixture f{Size{20, 1}};
    auto& input = f.app.make_root<TextInput>();
    f.pump();
    input.take_focus();

    input.set_text("suggested.txt");
    CHECK(input.cursor_col() == 13);

    f.term->feed(key_ev(Key::Backspace));
    f.term->feed_text(U"!");
    f.sync();
    CHECK(input.text() == "suggested.tx!");  // appended, not prepended

    SUBCASE("a field constructed with text agrees") {
        TextInput other{"abc"};
        CHECK(other.cursor_col() == 3);
    }
    SUBCASE("clearing puts it back to zero") {
        input.set_text("");
        CHECK(input.cursor_col() == 0);
    }
}
