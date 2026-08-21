#include <memory>
#include <string>

#include "doctest.h"
#include "fixture.hpp"
#include "modcurses/app.hpp"
#include "modcurses/mock_terminal.hpp"
#include "modcurses/widgets.hpp"

using namespace modcurses;
using Fixture = modcurses::testing::AppFixture;


// ------------------------------------------------------------------- Label

TEST_CASE("Label alignment") {
    Fixture f{Size{11, 1}};
    auto& label = f.app.make_root<Label>("abc");

    SUBCASE("left") {
        f.pump();
        CHECK(f.term->row_text(0) == "abc        ");
    }
    SUBCASE("center") {
        label.set_align(Align::Center);
        f.pump();
        CHECK(f.term->row_text(0) == "    abc    ");
    }
    SUBCASE("right") {
        label.set_align(Align::Right);
        f.pump();
        CHECK(f.term->row_text(0) == "        abc");
    }
}

TEST_CASE("Label text round-trips through UTF-8 and measures in columns") {
    Fixture f{Size{8, 1}};
    auto& label = f.app.make_root<Label>("héllo");
    CHECK(label.text() == "héllo");
    CHECK(label.width_req().preferred == 5);  // 5 columns, not 6 bytes
    f.pump();
    CHECK(f.term->row_text(0) == "héllo   ");
}

TEST_CASE("Label text longer than its box is clipped, never wrapped") {
    Fixture f{Size{5, 2}};
    f.app.make_root<Label>("abcdefghij");
    f.pump();
    CHECK(f.term->row_text(0) == "abcde");
    CHECK(f.term->row_text(1) == "     ");  // nothing spilled downward
}

TEST_CASE("Label prefers its text width but accepts any width it is given") {
    Label l{"abcd"};
    const SizeReq w = l.width_req();
    CHECK(w.preferred == 4);
    CHECK(w.min == 0);   // yields rather than push a sibling off the screen
    CHECK(w.max > 4);    // but stretches, so alignment has room to work
    CHECK(l.height_req() == SizeReq::fixed(1));
}

TEST_CASE("setting the same text again does not dirty the frame") {
    Fixture f{Size{10, 1}};
    auto& label = f.app.make_root<Label>("same");
    f.pump();
    const int frames = f.app.loop().frames_rendered();

    label.set_text("same");
    f.pump();
    CHECK(f.app.loop().frames_rendered() == frames);

    label.set_text("different");
    f.pump();
    CHECK(f.app.loop().frames_rendered() == frames + 1);
}

// ------------------------------------------------------------------ Button

TEST_CASE("Button renders its label in brackets, centred") {
    Fixture f{Size{11, 1}};
    f.app.make_root<Button>("Go");
    f.pump();
    CHECK(f.term->row_text(0) == "  [ Go ]   ");
}

TEST_CASE("Button is focusable by both routes and starts unfocused") {
    Button b{"x"};
    CHECK(b.focus_policy == FocusPolicy::Strong);
    CHECK(accepts_tab(b.focus_policy));
    CHECK(accepts_click(b.focus_policy));
    CHECK_FALSE(b.has_focus());
}

TEST_CASE("Enter and Space press the button; other keys do not") {
    Fixture f{Size{12, 1}};
    auto& b = f.app.make_root<Button>("Go");
    int pressed = 0;
    auto conn = b.pressed.connect([&] { ++pressed; });
    f.pump();
    b.take_focus();

    f.term->feed(key_ev(Key::Enter));
    f.pump();
    CHECK(pressed == 1);

    f.term->feed(char_ev(U' '));
    f.pump();
    CHECK(pressed == 2);

    f.term->feed(char_ev(U'x'));
    f.term->feed(key_ev(Key::Down));
    f.pump(2);
    CHECK(pressed == 2);

    SUBCASE("a modified Enter is not a press") {
        f.term->feed(key_ev(Key::Enter, Mods{true, false, false}));
        f.pump();
        CHECK(pressed == 2);
    }
}

TEST_CASE("clicking a button focuses and presses it") {
    Fixture f{Size{12, 1}};
    auto& b = f.app.make_root<Button>("Go");
    int pressed = 0;
    auto conn = b.pressed.connect([&] { ++pressed; });
    f.pump();

    f.term->feed(Event{MouseEvent{MouseEvent::Button::Left, MouseEvent::Action::Press, {4, 0}, {}}});
    f.pump();
    CHECK(pressed == 1);
    CHECK(b.has_focus());

    SUBCASE("a release does not press it a second time") {
        f.term->feed(
            Event{MouseEvent{MouseEvent::Button::Left, MouseEvent::Action::Release, {4, 0}, {}}});
        f.pump();
        CHECK(pressed == 1);
    }
    SUBCASE("the right button does nothing") {
        f.term->feed(
            Event{MouseEvent{MouseEvent::Button::Right, MouseEvent::Action::Press, {4, 0}, {}}});
        f.pump();
        CHECK(pressed == 1);
    }
}

TEST_CASE("a focused button is visibly distinct") {
    Fixture f{Size{12, 1}};
    auto& b = f.app.make_root<Button>("Go");
    f.pump();
    const Style unfocused = f.term->cell_at({4, 0}).style;

    b.take_focus();
    f.pump();
    const Style focused = f.term->cell_at({4, 0}).style;

    CHECK(focused != unfocused);
    CHECK(focused.has(Trait::Reverse));
}

TEST_CASE("Button width accounts for its brackets") {
    Button b{"Quit"};
    CHECK(b.width_req().preferred == 8);  // "[ Quit ]"
    CHECK(b.height_req() == SizeReq::fixed(1));
}

// ---------------------------------------------------------------- Titlebar

TEST_CASE("Titlebar shows the title on the left and the hint on the right") {
    Fixture f{Size{20, 1}};
    f.app.make_root<Titlebar>("Title", "hint");
    f.pump();
    // Title starts one column in; the hint ends one column from the right.
    CHECK(f.term->row_text(0) == " Title         hint ");
}

TEST_CASE("Titlebar drops the hint rather than let it collide with the title") {
    Fixture f{Size{12, 1}};
    f.app.make_root<Titlebar>("A long title", "hint");
    f.pump();
    CHECK(f.term->row_text(0) == " A long titl");
}

TEST_CASE("Titlebar defaults to reverse video and is one row tall") {
    Fixture f{Size{10, 3}};
    auto& t = f.app.make_root<VBox>().emplace_child<Titlebar>("T");
    f.pump();
    CHECK(t.height_req() == SizeReq::fixed(1));
    CHECK(t.rect().size.height == 1);
    CHECK(f.term->cell_at({0, 0}).style.has(Trait::Reverse));
    CHECK_FALSE(f.term->cell_at({0, 1}).style.has(Trait::Reverse));
}

// ------------------------------------------------------- composed together

TEST_CASE("the hello layout composes into the expected screen") {
    Fixture f{Size{20, 5}};
    auto& root = f.app.make_root<VBox>();
    root.emplace_child<Titlebar>("hello", "M2");
    auto& body = root.emplace_child<VBox>();
    body.emplace_child<Label>("Nothing yet.", Align::Center);
    body.emplace_child<Widget>();  // spacer
    auto& buttons = root.emplace_child<HBox>();
    buttons.emplace_child<Widget>();
    auto& go = buttons.emplace_child<Button>("Go");
    buttons.emplace_child<Widget>();
    f.pump();

    CHECK(f.term->row_text(0) == " hello           M2 ");
    CHECK(f.term->row_text(1) == "    Nothing yet.    ");
    CHECK(f.term->row_text(4) == "       [ Go ]       ");
    CHECK(go.rect().size.height == 1);

    SUBCASE("Tab lands on the only focusable widget") {
        f.term->feed(key_ev(Key::Tab));
        f.pump();
        CHECK(go.has_focus());
    }
}
