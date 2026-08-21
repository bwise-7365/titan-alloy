#include <memory>
#include <string>
#include <vector>

#include "doctest.h"
#include "fixture.hpp"
#include "modcurses/app.hpp"
#include "modcurses/mock_terminal.hpp"
#include "modcurses/widgets.hpp"

using namespace modcurses;
using Fixture = modcurses::testing::AppFixture;

namespace {

// Appends its name to a shared log whenever it is offered an event, and can
// be told whether to claim it.
class Spy : public Widget {
public:
    Spy() = default;

    std::vector<std::string>* log = nullptr;
    std::string name = "?";
    bool claim_keys = false;
    bool claim_mouse = false;
    Point last_mouse_pos{-1, -1};
    int focus_gained = 0;
    int focus_lost = 0;
    std::optional<Point> cursor_pos;

    [[nodiscard]] SizeReq width_req() const override { return width; }
    [[nodiscard]] SizeReq height_req() const override { return height; }
    SizeReq width{};
    SizeReq height{};

protected:
    bool on_key(const KeyEvent&) override {
        if (log) log->push_back(name + ":key");
        return claim_keys;
    }
    bool on_mouse(const MouseEvent& ev) override {
        if (log) log->push_back(name + ":mouse");
        last_mouse_pos = ev.pos;
        return claim_mouse;
    }
    void on_focus(bool gained) override { gained ? ++focus_gained : ++focus_lost; }
    [[nodiscard]] std::optional<Point> cursor() const override { return cursor_pos; }
};

}  // namespace

// ------------------------------------------------------------- key routing

TEST_CASE("an unhandled key bubbles from the focused widget up to the root") {
    Fixture f;
    std::vector<std::string> log;

    auto& root = f.app.make_root<VBox>();
    auto& mid = root.emplace_child<VBox>();
    auto& leaf = mid.emplace_child<Spy>();
    leaf.log = &log;
    leaf.name = "leaf";
    leaf.focus_policy = FocusPolicy::Strong;
    f.pump();
    leaf.take_focus();

    f.term->feed(char_ev(U'x'));
    f.pump();

    // The Spy is the only widget that logs; the plain VBoxes above it just
    // decline. What matters is that the leaf saw it exactly once.
    CHECK(log == std::vector<std::string>{"leaf:key"});
}

TEST_CASE("a widget that claims a key stops the bubble") {
    Fixture f;
    std::vector<std::string> log;

    auto& root = f.app.make_root<VBox>();
    auto& outer = root.emplace_child<Spy>();
    outer.log = &log;
    outer.name = "outer";
    auto& inner = outer.emplace_child<Spy>();
    inner.log = &log;
    inner.name = "inner";
    inner.focus_policy = FocusPolicy::Strong;
    inner.claim_keys = true;
    f.pump();
    inner.take_focus();

    f.term->feed(char_ev(U'x'));
    f.pump();
    CHECK(log == std::vector<std::string>{"inner:key"});  // outer never saw it
}

TEST_CASE("the hook is consulted before the virtual") {
    Fixture f;
    std::vector<std::string> log;
    auto& root = f.app.make_root<VBox>();
    auto& s = root.emplace_child<Spy>();
    s.log = &log;
    s.name = "s";
    s.focus_policy = FocusPolicy::Strong;
    s.on_key_hook = [&](const KeyEvent&) {
        log.push_back("s:hook");
        return true;
    };
    f.pump();
    s.take_focus();

    f.term->feed(char_ev(U'x'));
    f.pump();
    CHECK(log == std::vector<std::string>{"s:hook"});  // virtual never ran
}

TEST_CASE("app shortcuts beat every widget") {
    Fixture f;
    std::vector<std::string> log;
    auto& root = f.app.make_root<VBox>();
    auto& s = root.emplace_child<Spy>();
    s.log = &log;
    s.name = "s";
    s.focus_policy = FocusPolicy::Strong;
    s.claim_keys = true;
    f.pump();
    s.take_focus();

    int fired = 0;
    f.app.add_shortcut(ctrl_ev(U'q'), [&] { ++fired; });

    f.term->feed(ctrl_ev(U'q'));
    f.pump();
    CHECK(fired == 1);
    CHECK(log.empty());
}

TEST_CASE("adding a shortcut for an existing key replaces it") {
    Fixture f;
    f.app.make_root<VBox>();
    int a = 0, b = 0;
    f.app.add_shortcut(ctrl_ev(U'x'), [&] { ++a; });
    f.app.add_shortcut(ctrl_ev(U'x'), [&] { ++b; });

    f.term->feed(ctrl_ev(U'x'));
    f.pump();
    CHECK(a == 0);
    CHECK(b == 1);
}

TEST_CASE("Ctrl-C quits by default, because raw() means it is not a signal") {
    Fixture f;
    f.app.make_root<VBox>();
    f.pump();
    CHECK_FALSE(f.app.loop().quitting());

    f.term->feed(ctrl_ev(U'c'));
    f.pump();
    CHECK(f.app.loop().quitting());
}

TEST_CASE("with nothing focused, keys still reach the root") {
    Fixture f;
    std::vector<std::string> log;
    auto& root = f.app.make_root<Spy>();
    root.log = &log;
    root.name = "root";
    f.pump();
    REQUIRE(f.app.loop().focus().current() == nullptr);

    f.term->feed(char_ev(U'z'));
    f.pump();
    CHECK(log == std::vector<std::string>{"root:key"});
}

// -------------------------------------------------------------- focus chain

TEST_CASE("Tab and Shift-Tab cycle the focus chain in tree order") {
    Fixture f;
    auto& root = f.app.make_root<VBox>();
    auto& a = root.emplace_child<Button>("a");
    auto& b = root.emplace_child<Button>("b");
    auto& c = root.emplace_child<Button>("c");
    f.pump();

    CHECK(f.app.loop().focus().order() == std::vector<Widget*>{&a, &b, &c});

    a.take_focus();
    CHECK(a.has_focus());

    f.term->feed(key_ev(Key::Tab));
    f.pump();
    CHECK(b.has_focus());

    f.term->feed(key_ev(Key::Tab));
    f.pump();
    CHECK(c.has_focus());

    SUBCASE("Tab wraps around the end") {
        f.term->feed(key_ev(Key::Tab));
        f.pump();
        CHECK(a.has_focus());
    }
    SUBCASE("BackTab walks the other way and wraps at the start") {
        f.term->feed(key_ev(Key::BackTab));
        f.pump();
        CHECK(b.has_focus());
        f.term->feed(key_ev(Key::BackTab));
        f.pump();
        CHECK(a.has_focus());
        f.term->feed(key_ev(Key::BackTab));
        f.pump();
        CHECK(c.has_focus());
    }
}

TEST_CASE("a widget may claim Tab for itself before the chain sees it") {
    Fixture f;
    auto& root = f.app.make_root<VBox>();
    auto& a = root.emplace_child<Spy>();
    auto& b = root.emplace_child<Button>("b");
    a.focus_policy = FocusPolicy::Strong;
    a.claim_keys = true;  // swallows everything, Tab included
    f.pump();
    a.take_focus();

    f.term->feed(key_ev(Key::Tab));
    f.pump();
    CHECK(a.has_focus());  // focus did not move
    CHECK_FALSE(b.has_focus());
}

TEST_CASE("the Tab order skips widgets that are hidden or unlaid-out") {
    Fixture f{Size{20, 2}};
    auto& root = f.app.make_root<VBox>();
    auto& a = root.emplace_child<Button>("a");
    auto& b = root.emplace_child<Button>("b");
    auto& c = root.emplace_child<Button>("c");
    b.visible = false;
    f.pump();

    // Only 2 rows: a and c are laid out, b is invisible.
    const auto order = f.app.loop().focus().order();
    CHECK(order.size() == 2);
    CHECK(order[0] == &a);
    CHECK(order[1] == &c);
}

TEST_CASE("focus notifications fire once each, on the right widgets") {
    Fixture f;
    auto& root = f.app.make_root<VBox>();
    auto& a = root.emplace_child<Spy>();
    auto& b = root.emplace_child<Spy>();
    a.focus_policy = FocusPolicy::Strong;
    b.focus_policy = FocusPolicy::Strong;
    f.pump();

    a.take_focus();
    CHECK(a.focus_gained == 1);
    CHECK(a.focus_lost == 0);

    b.take_focus();
    CHECK(a.focus_lost == 1);
    CHECK(b.focus_gained == 1);

    b.take_focus();  // already focused
    CHECK(b.focus_gained == 1);
}

TEST_CASE("focus is repaired when the focused widget is destroyed") {
    Fixture f;
    auto& root = f.app.make_root<VBox>();
    auto& a = root.emplace_child<Button>("a");
    auto& b = root.emplace_child<Button>("b");
    f.pump();
    b.take_focus();
    CHECK(b.has_focus());

    b.destroy_later();
    f.pump();
    CHECK(f.app.loop().focus().current() == &a);
}

TEST_CASE("a Click-only widget keeps focus across a repair") {
    // It is legitimately focusable by mouse but never appears in the Tab
    // order, so repair() must not treat its absence there as "gone".
    Fixture f;
    auto& root = f.app.make_root<VBox>();
    auto& clicky = root.emplace_child<Spy>();
    auto& tabby = root.emplace_child<Button>("t");
    clicky.focus_policy = FocusPolicy::Click;
    f.pump();

    clicky.take_focus();
    CHECK(f.app.loop().focus().current() == &clicky);
    CHECK(f.app.loop().focus().order() == std::vector<Widget*>{&tabby});

    tabby.destroy_later();
    f.pump();
    CHECK(f.app.loop().focus().current() == &clicky);
}

// ----------------------------------------------------------- mouse routing

TEST_CASE("a mouse event goes to the deepest widget under the point") {
    Fixture f{Size{20, 8}};
    std::vector<std::string> log;
    auto& root = f.app.make_root<VBox>();
    auto& outer = root.emplace_child<Spy>();
    outer.log = &log;
    outer.name = "outer";
    auto& inner = outer.emplace_child<Spy>();
    inner.log = &log;
    inner.name = "inner";
    inner.claim_mouse = true;
    f.pump();

    REQUIRE_FALSE(inner.rect().empty());
    f.term->feed(Event{MouseEvent{MouseEvent::Button::Left, MouseEvent::Action::Press, {3, 2}, {}}});
    f.pump();
    CHECK(log == std::vector<std::string>{"inner:mouse"});
}

TEST_CASE("mouse coordinates are translated to each widget's local space") {
    Fixture f{Size{20, 8}};
    auto& root = f.app.make_root<VBox>();
    auto& top = root.emplace_child<Spy>();
    auto& bottom = root.emplace_child<Spy>();
    top.height = SizeReq::fixed(3);
    bottom.height = SizeReq::expand();
    f.pump();
    REQUIRE(bottom.rect() == Rect{{0, 3}, {20, 5}});

    f.term->feed(Event{MouseEvent{MouseEvent::Button::Left, MouseEvent::Action::Press, {7, 5}, {}}});
    f.pump();
    CHECK(bottom.last_mouse_pos == Point{7, 2});  // 5 - 3
}

TEST_CASE("an unclaimed mouse event bubbles to the parent, retranslated") {
    Fixture f{Size{20, 8}};
    std::vector<std::string> log;
    auto& root = f.app.make_root<VBox>();
    auto& outer = root.emplace_child<Spy>();
    outer.log = &log;
    outer.name = "outer";
    auto& inner = outer.emplace_child<Spy>();
    inner.log = &log;
    inner.name = "inner";
    inner.claim_mouse = false;
    f.pump();

    f.term->feed(Event{MouseEvent{MouseEvent::Button::Left, MouseEvent::Action::Press, {3, 2}, {}}});
    f.pump();
    CHECK(log == std::vector<std::string>{"inner:mouse", "outer:mouse"});
}

TEST_CASE("a left press focuses the nearest click-capable widget at or above the hit") {
    Fixture f{Size{20, 8}};
    auto& root = f.app.make_root<VBox>();
    auto& button = root.emplace_child<Button>("press me");
    auto& label = button.emplace_child<Label>("inner");  // not click-capable
    f.pump();
    REQUIRE_FALSE(label.rect().empty());

    f.term->feed(Event{MouseEvent{MouseEvent::Button::Left, MouseEvent::Action::Press,
                                  label.rect().origin, {}}});
    f.pump();
    CHECK(button.has_focus());  // the label handed it upward
}

TEST_CASE("a click outside every widget is dropped without a crash") {
    Fixture f{Size{20, 8}};
    auto& root = f.app.make_root<VBox>();
    auto& b = root.emplace_child<Button>("b");
    f.pump();

    f.term->feed(
        Event{MouseEvent{MouseEvent::Button::Left, MouseEvent::Action::Press, {500, 500}, {}}});
    f.pump();
    CHECK_FALSE(b.has_focus());
}

// ---------------------------------------------------------------- lifecycle

TEST_CASE("a resize event resizes the buffer and re-lays out the tree") {
    Fixture f{Size{20, 8}};
    auto& root = f.app.make_root<VBox>();
    auto& child = root.emplace_child<Spy>();
    f.pump();
    CHECK(child.rect() == Rect{{0, 0}, {20, 8}});

    f.term->set_size(Size{10, 4});  // queues the ResizeEvent, as a real one would
    f.pump();
    f.pump();
    CHECK(f.app.buffer().size() == Size{10, 4});
    CHECK(child.rect() == Rect{{0, 0}, {10, 4}});
}

TEST_CASE("the first frame is painted before the loop ever blocks on input") {
    Fixture f{Size{12, 3}};
    auto& root = f.app.make_root<VBox>();
    root.emplace_child<Titlebar>("hi");
    CHECK(f.app.loop().frames_rendered() == 0);

    f.app.pump_once();
    CHECK(f.app.loop().frames_rendered() == 1);
    CHECK(f.term->row_text(0) == " hi         ");
}

TEST_CASE("a clean frame is not repainted") {
    Fixture f{Size{12, 3}};
    auto& root = f.app.make_root<VBox>();
    root.emplace_child<Titlebar>("hi");
    f.pump();
    CHECK(f.app.loop().frames_rendered() == 1);

    f.pump(3);
    CHECK(f.app.loop().frames_rendered() == 1);  // nothing changed, nothing drawn

    root.invalidate();
    f.pump();
    CHECK(f.app.loop().frames_rendered() == 2);
}

TEST_CASE("the hardware cursor follows the focused widget") {
    Fixture f{Size{20, 8}};
    auto& root = f.app.make_root<VBox>();
    auto& top = root.emplace_child<Spy>();
    auto& bottom = root.emplace_child<Spy>();
    top.height = SizeReq::fixed(3);
    bottom.height = SizeReq::expand();
    top.focus_policy = FocusPolicy::Strong;
    bottom.focus_policy = FocusPolicy::Strong;
    bottom.cursor_pos = Point{2, 1};
    f.pump();

    SUBCASE("hidden when the focused widget has no cursor") {
        top.take_focus();
        f.pump();
        CHECK_FALSE(f.term->cursor().has_value());
    }
    SUBCASE("placed in absolute coordinates when it does") {
        bottom.take_focus();
        f.pump();
        REQUIRE(f.term->cursor().has_value());
        CHECK(*f.term->cursor() == Point{2, 4});  // local (2,1) inside a rect at y=3
    }
    SUBCASE("a cursor outside its own widget is refused") {
        bottom.cursor_pos = Point{99, 99};
        bottom.take_focus();
        f.pump();
        CHECK_FALSE(f.term->cursor().has_value());
    }
}

TEST_CASE("quit stops the loop and run() returns the code") {
    Fixture f;
    f.app.make_root<VBox>();
    f.app.quit(3);
    CHECK_FALSE(f.app.pump_once());
    CHECK(f.app.run() == 3);
}

TEST_CASE("focus is released when the focused widget's page is hidden") {
    // Regression, and the other half of the pager freeze: a Stack page change
    // leaves the old page's widgets with an empty rect. Focus stranded on one
    // of them means an invisible widget goes on eating the keyboard.
    Fixture f{Size{20, 6}};
    auto& stack = f.app.make_root<Stack>();
    auto& page0 = stack.emplace_child<VBox>();
    auto& page1 = stack.emplace_child<VBox>();
    page0.emplace_child<Label>("visible page");
    auto& button = page1.emplace_child<Button>("on the hidden page");
    f.sync();

    stack.set_active(1);
    f.sync();
    button.take_focus();
    REQUIRE(button.has_focus());

    stack.set_active(0);
    f.sync();
    CHECK_FALSE(button.has_focus());
    CHECK(f.app.loop().focus().current() == nullptr);

    SUBCASE("and keys then reach the application again") {
        std::string seen;
        auto& root = *f.app.root();
        root.on_key_hook = [&](const KeyEvent& ev) {
            if (ev.key == Key::Char) seen += static_cast<char>(ev.text);
            return true;
        };
        f.term->feed_text(U"q");
        f.sync();
        CHECK(seen == "q");
    }
}

TEST_CASE("hiding a widget outright also releases its focus") {
    Fixture f{Size{20, 6}};
    auto& root = f.app.make_root<VBox>();
    auto& a = root.emplace_child<Button>("a");
    auto& b = root.emplace_child<Button>("b");
    f.sync();
    b.take_focus();
    REQUIRE(b.has_focus());

    b.visible = false;
    b.invalidate_layout();
    f.sync();
    CHECK_FALSE(b.has_focus());
    CHECK_FALSE(a.has_focus());  // released, not silently handed on
}
