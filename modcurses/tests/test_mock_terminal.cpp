#include <variant>

#include "doctest.h"
#include "modcurses/mock_terminal.hpp"
#include "modcurses/render.hpp"

using namespace modcurses;

TEST_CASE("a fresh mock is a blank screen with an empty script") {
    MockTerminal term{Size{6, 2}};
    CHECK(term.size() == Size{6, 2});
    CHECK(term.pending_events() == 0);
    CHECK(term.row_text(0) == "      ");
    CHECK(term.screen_text() == "      \n      ");
    CHECK_FALSE(term.cursor().has_value());
}

TEST_CASE("the event script is replayed in order, then starves") {
    MockTerminal term;
    term.feed(char_ev(U'a'));
    term.feed(key_ev(Key::Enter));
    CHECK(term.pending_events() == 2);

    auto e1 = term.poll_event(std::nullopt);
    REQUIRE(e1.has_value());
    REQUIRE(std::holds_alternative<KeyEvent>(*e1));
    CHECK(std::get<KeyEvent>(*e1).text == U'a');

    auto e2 = term.poll_event(std::nullopt);
    REQUIRE(e2.has_value());
    CHECK(std::get<KeyEvent>(*e2).key == Key::Enter);

    CHECK_FALSE(term.poll_event(std::nullopt).has_value());
    CHECK(term.starved_polls() == 1);
}

TEST_CASE("feed_text queues one Char event per codepoint") {
    MockTerminal term;
    term.feed_text(U"hé");
    CHECK(term.pending_events() == 2);
    auto e = term.poll_event(std::nullopt);
    CHECK(std::get<KeyEvent>(*e).text == U'h');
    e = term.poll_event(std::nullopt);
    CHECK(std::get<KeyEvent>(*e).text == U'é');
}

TEST_CASE("draw_run lands in the grid with its style, and counts") {
    MockTerminal term{Size{8, 2}};
    const Style s = fg(Color::Yellow).with(Trait::Bold);
    term.draw_run({2, 1}, U"abc", s);

    CHECK(term.draw_run_count() == 1);
    CHECK(term.row_text(1) == "  abc   ");
    CHECK(term.cell_at({2, 1}).ch == U'a');
    CHECK(term.cell_at({2, 1}).style == s);
    CHECK(term.cell_at({2, 1}).style.has(Trait::Bold));
    CHECK(term.cell_at({1, 1}).style == Style{});
}

TEST_CASE("draw_run past the right edge is truncated, never wrapped") {
    MockTerminal term{Size{4, 2}};
    term.draw_run({2, 0}, U"abcdef", {});
    CHECK(term.row_text(0) == "  ab");
    CHECK(term.row_text(1) == "    ");  // nothing spilled onto the next row
}

TEST_CASE("out-of-bounds reads answer with a blank cell instead of crashing") {
    MockTerminal term{Size{2, 2}};
    CHECK(term.cell_at({-1, 0}).ch == U' ');
    CHECK(term.cell_at({99, 99}).ch == U' ');
    CHECK(term.row_text(-1).empty());
    CHECK(term.row_text(5).empty());
}

TEST_CASE("cursor and beep are observable") {
    MockTerminal term;
    term.set_cursor(Point{3, 4});
    REQUIRE(term.cursor().has_value());
    CHECK(*term.cursor() == Point{3, 4});
    term.set_cursor(std::nullopt);
    CHECK_FALSE(term.cursor().has_value());

    term.beep();
    term.beep();
    CHECK(term.beep_count() == 2);
}

TEST_CASE("set_size resizes the grid and announces it through the event stream") {
    MockTerminal term{Size{4, 1}};
    term.draw_run({0, 0}, U"abcd", {});
    term.set_size(Size{2, 2});

    CHECK(term.size() == Size{2, 2});
    CHECK(term.row_text(0) == "  ");  // contents dropped
    auto e = term.poll_event(std::nullopt);
    REQUIRE(e.has_value());
    REQUIRE(std::holds_alternative<ResizeEvent>(*e));
    CHECK(std::get<ResizeEvent>(*e).size == Size{2, 2});
}

TEST_CASE("colour definition can be made to fail, as a real terminal may") {
    MockTerminal term;
    CHECK(term.can_define_colors());
    Palette pal{&term};
    CHECK(pal.can_redefine());
    CHECK(pal.set(Color::Red, Rgb{255, 0, 0}));
    CHECK(pal.apply_dawnbringer16());

    term.set_colors_definable(false);
    CHECK_FALSE(pal.can_redefine());
    CHECK_FALSE(pal.set(Color::Red, Rgb{255, 0, 0}));
    CHECK_FALSE(pal.apply_dawnbringer16());
}

TEST_CASE("Color::Default is not a redefinable slot") {
    MockTerminal term;
    Palette pal{&term};
    CHECK_FALSE(pal.set(Color::Default, Rgb{1, 2, 3}));
}

TEST_CASE("a Palette with no terminal is inert rather than a crash") {
    Palette pal;
    CHECK_FALSE(pal.can_redefine());
    CHECK_FALSE(pal.set(Color::Red, Rgb{}));
    CHECK_FALSE(pal.apply_dawnbringer16());
}

TEST_CASE("a full render pipeline runs headlessly, with no TTY and no curses") {
    MockTerminal term{Size{12, 3}};
    ScreenBuffer buf{term.size()};

    const Rect all{{0, 0}, buf.size()};
    Canvas c{buf, all, all};
    c.fill(Glyph{U' ', bg(Color::Blue)});
    c.draw_box(Rect{{0, 0}, {12, 3}}, fg(Color::White).with_bg(Color::Blue));
    c.print({2, 1}, "hi", fg(Color::BrightYellow).with_bg(Color::Blue));

    buf.flush_to(term);
    CHECK(term.row_text(0) == "┌──────────┐");
    CHECK(term.row_text(1) == "│ hi       │");
    CHECK(term.row_text(2) == "└──────────┘");
    CHECK(term.cell_at({2, 1}).style.fg == Color::BrightYellow);
    CHECK(term.flush_count() == 1);

    SUBCASE("a second identical frame is free") {
        term.reset_counters();
        CHECK(buf.flush_to(term) == 0);
        CHECK(term.draw_run_count() == 0);
    }
}
