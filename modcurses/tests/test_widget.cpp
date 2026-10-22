#include <string>
#include <vector>

#include "doctest.h"
#include "modcurses/app.hpp"
#include "modcurses/mock_terminal.hpp"
#include "modcurses/widget.hpp"
#include "modcurses/widgets.hpp"

using namespace modcurses;

namespace {

// Records its own lifecycle so tests can assert on destruction and geometry
// notifications.
class Probe : public Widget {
public:
    explicit Probe(std::vector<std::string>* log = nullptr, std::string name = "p")
        : log_(log), name_(std::move(name)) {}
    ~Probe() override {
        if (log_) log_->push_back(name_ + ":destroyed");
    }

    int geometry_calls = 0;
    Rect last_new_rect;

    [[nodiscard]] SizeReq width_req() const override { return width; }
    [[nodiscard]] SizeReq height_req() const override { return height; }

    SizeReq width{};
    SizeReq height{};

protected:
    void on_geometry(Rect, Rect new_rect) override {
        ++geometry_calls;
        last_new_rect = new_rect;
    }

private:
    std::vector<std::string>* log_;
    std::string name_;
};

}  // namespace

TEST_CASE("a parent owns its children and destroys them with it") {
    std::vector<std::string> log;
    {
        VBox root;
        auto& a = root.emplace_child<Probe>(&log, "a");
        a.emplace_child<Probe>(&log, "a1");
        root.emplace_child<Probe>(&log, "b");
        CHECK(root.children().size() == 2);
        CHECK(a.parent() == &root);
        CHECK(a.children().size() == 1);
    }
    CHECK(log.size() == 3);  // every descendant went with it
}

TEST_CASE("remove_child hands ownership back and detaches the parent link") {
    std::vector<std::string> log;
    VBox root;
    auto& a = root.emplace_child<Probe>(&log, "a");

    auto owned = root.remove_child(a);
    REQUIRE(owned != nullptr);
    CHECK(owned.get() == &a);
    CHECK(a.parent() == nullptr);
    CHECK(root.children().empty());
    CHECK(log.empty());  // still alive - we hold it

    owned.reset();
    CHECK(log == std::vector<std::string>{"a:destroyed"});
}

TEST_CASE("remove_child on a widget that is not a child returns null") {
    VBox root;
    VBox stranger;
    CHECK(root.remove_child(stranger) == nullptr);
}

TEST_CASE("VBox stacks children top to bottom and gives them its width") {
    // Layout is driven through a real loop rather than by poking rects.
    App app{std::make_unique<MockTerminal>(Size{20, 10})};
    auto& r = app.make_root<VBox>();
    auto& p1 = r.emplace_child<Probe>();
    auto& p2 = r.emplace_child<Probe>();
    p1.height = SizeReq::fixed(2);
    p2.height = SizeReq::expand();
    app.pump_once();

    CHECK(p1.rect() == Rect{{0, 0}, {20, 2}});
    CHECK(p2.rect() == Rect{{0, 2}, {20, 8}});
}

TEST_CASE("HBox lays children out left to right") {
    App app{std::make_unique<MockTerminal>(Size{20, 6})};
    auto& r = app.make_root<HBox>();
    auto& a = r.emplace_child<Probe>();
    auto& b = r.emplace_child<Probe>();
    a.width = SizeReq::fixed(5);
    b.width = SizeReq::expand();
    app.pump_once();

    CHECK(a.rect() == Rect{{0, 0}, {5, 6}});
    CHECK(b.rect() == Rect{{5, 0}, {15, 6}});
}

TEST_CASE("a child with no room gets an empty rect and keeps its state") {
    App app{std::make_unique<MockTerminal>(Size{20, 4})};
    auto& r = app.make_root<VBox>();
    auto& a = r.emplace_child<Probe>();
    auto& b = r.emplace_child<Probe>();
    auto& c = r.emplace_child<Probe>();
    a.height = SizeReq::fixed(3);
    b.height = SizeReq::fixed(3);
    c.height = SizeReq::fixed(3);
    app.pump_once();

    CHECK(a.rect().size.height == 3);
    CHECK(b.rect().empty());  // 3 + 3 > 4
    CHECK(c.rect().empty());
    CHECK(b.visible);         // hidden by the layout, not by its own flag
}

TEST_CASE("an invisible child takes part in neither layout nor its siblings' share") {
    App app{std::make_unique<MockTerminal>(Size{10, 10})};
    auto& r = app.make_root<VBox>();
    auto& a = r.emplace_child<Probe>();
    auto& b = r.emplace_child<Probe>();
    a.height = SizeReq::expand();
    b.height = SizeReq::expand();
    b.visible = false;
    app.pump_once();

    CHECK(a.rect() == Rect{{0, 0}, {10, 10}});  // takes the whole box
    CHECK(b.rect().empty());
}

TEST_CASE("on_geometry fires only when the rect actually changes") {
    App app{std::make_unique<MockTerminal>(Size{10, 4})};
    auto& r = app.make_root<VBox>();
    auto& p = r.emplace_child<Probe>();
    app.pump_once();

    const int after_first = p.geometry_calls;
    CHECK(after_first >= 1);
    CHECK(p.last_new_rect == Rect{{0, 0}, {10, 4}});

    app.loop().request_layout();
    app.pump_once();
    CHECK(p.geometry_calls == after_first);  // same rect, no notification
}

TEST_CASE("nested boxes compose") {
    App app{std::make_unique<MockTerminal>(Size{20, 10})};
    auto& r = app.make_root<VBox>();
    auto& header = r.emplace_child<Probe>();
    header.height = SizeReq::fixed(1);
    auto& body = r.emplace_child<HBox>();
    auto& left = body.emplace_child<Probe>();
    auto& right = body.emplace_child<Probe>();
    left.width = SizeReq::fixed(4);
    right.width = SizeReq::expand();
    app.pump_once();

    CHECK(header.rect() == Rect{{0, 0}, {20, 1}});
    CHECK(body.rect() == Rect{{0, 1}, {20, 9}});
    CHECK(left.rect() == Rect{{0, 1}, {4, 9}});
    CHECK(right.rect() == Rect{{4, 1}, {16, 9}});
}

TEST_CASE("Stack shows exactly one page at a time") {
    App app{std::make_unique<MockTerminal>(Size{12, 5})};
    auto& stack = app.make_root<Stack>();
    auto& p0 = stack.emplace_child<Probe>();
    auto& p1 = stack.emplace_child<Probe>();
    auto& p2 = stack.emplace_child<Probe>();
    app.pump_once();

    CHECK(stack.active() == &p0);
    CHECK(p0.rect() == Rect{{0, 0}, {12, 5}});
    CHECK(p1.rect().empty());
    CHECK(p2.rect().empty());

    int page_signal = -1;
    auto conn = stack.page_changed.connect([&](int i) { page_signal = i; });

    stack.set_active(2);
    app.pump_once();
    CHECK(page_signal == 2);
    CHECK(stack.active() == &p2);
    CHECK(p2.rect() == Rect{{0, 0}, {12, 5}});
    CHECK(p0.rect().empty());

    SUBCASE("set_active by reference") {
        stack.set_active(p1);
        app.pump_once();
        CHECK(stack.active_index() == 1);
    }
    SUBCASE("an out-of-range index is ignored") {
        page_signal = -1;
        stack.set_active(99);
        stack.set_active(-1);
        CHECK(page_signal == -1);
        CHECK(stack.active_index() == 2);
    }
    SUBCASE("re-selecting the active page does not re-signal") {
        page_signal = -1;
        stack.set_active(2);
        CHECK(page_signal == -1);
    }
}

TEST_CASE("destroy_later defers until a safe point in the loop") {
    std::vector<std::string> log;
    App app{std::make_unique<MockTerminal>(Size{10, 6})};
    auto& r = app.make_root<VBox>();
    auto& a = r.emplace_child<Probe>(&log, "a");
    r.emplace_child<Probe>(&log, "b");
    app.pump_once();

    a.destroy_later();
    CHECK(log.empty());          // not yet - we are not at a safe point
    CHECK(r.children().size() == 2);

    app.pump_once();
    CHECK(log == std::vector<std::string>{"a:destroyed"});
    CHECK(r.children().size() == 1);
}

TEST_CASE("destroying a widget from inside its own handler is safe") {
    // This is exactly what destroy_later exists for: the handler is running
    // inside the widget it wants to delete.
    App app{std::make_unique<MockTerminal>(Size{10, 6})};
    auto& r = app.make_root<VBox>();
    auto& victim = r.emplace_child<Button>("x");
    auto conn = victim.pressed.connect([&victim] { victim.destroy_later(); });
    victim.take_focus();
    app.pump_once();

    auto& term = static_cast<MockTerminal&>(app.terminal());
    term.feed(key_ev(Key::Enter));
    app.pump_once();

    CHECK(r.children().empty());
}

TEST_CASE("destroying a parent and child in the same pass does not double-free") {
    std::vector<std::string> log;
    App app{std::make_unique<MockTerminal>(Size{10, 6})};
    auto& r = app.make_root<VBox>();
    auto& parent = r.emplace_child<Probe>(&log, "parent");
    auto& child = parent.emplace_child<Probe>(&log, "child");
    app.pump_once();

    child.destroy_later();
    parent.destroy_later();
    app.pump_once();

    CHECK(r.children().empty());
    CHECK(log.size() == 2);
}

TEST_CASE("a widget's timers die with it even if the handle was released") {
    App app{std::make_unique<MockTerminal>(Size{10, 6})};
    auto& r = app.make_root<VBox>();
    auto& p = r.emplace_child<Probe>();

    int fires = 0;
    auto h = p.add_timer(std::chrono::milliseconds{1}, [&] { ++fires; });
    h.release();  // fire and forget
    CHECK(app.loop().timer_count() == 1);

    p.destroy_later();
    app.pump_once();
    CHECK(app.loop().timer_count() == 0);
}

TEST_CASE("dropping a TimerHandle cancels the timer") {
    App app{std::make_unique<MockTerminal>(Size{10, 6})};
    auto& r = app.make_root<VBox>();
    {
        auto h = r.add_timer(std::chrono::milliseconds{1}, [] {});
        CHECK(h.active());
    }
    // Cancelled entries are purged at the next housekeeping pass.
    r.emplace_child<Probe>().destroy_later();
    app.pump_once();
    CHECK(app.loop().timer_count() == 0);
}

TEST_CASE("a detached widget is inert rather than a crash") {
    Probe p;  // no parent, no loop
    CHECK_FALSE(p.has_focus());
    p.invalidate();
    p.invalidate_layout();
    p.take_focus();
    p.destroy_later();
    auto h = p.add_timer(std::chrono::milliseconds{5}, [] {});
    CHECK_FALSE(h.active());
}

TEST_CASE("one narrow child does not cap the width of its whole container") {
    // Regression: a VBox reported max-width as the SMALLEST of its children's
    // maxima, so a single fixed-width child (a Checkbox, say) held an entire
    // page to its own width and starved every sibling. layout_children
    // already clamps each child individually; the container itself must be
    // free to be as wide as its widest child can use.
    App app{std::make_unique<MockTerminal>(Size{80, 6})};
    auto& row = app.make_root<HBox>();
    auto& sidebar = row.emplace_child<Probe>();
    sidebar.width = SizeReq::fixed(14);

    auto& page = row.emplace_child<VBox>();
    auto& narrow = page.emplace_child<Probe>();
    auto& wide = page.emplace_child<Probe>();
    narrow.width = SizeReq::fixed(10);          // a checkbox-shaped child
    wide.width = SizeReq::expand();
    app.pump_once();

    CHECK(sidebar.rect().size.width == 14);
    CHECK(page.rect().size.width == 66);        // not 10
    CHECK(narrow.rect().size.width == 10);      // still clamped to its own max
    CHECK(wide.rect().size.width == 66);
}

TEST_CASE("a container of fixed-width children stays that wide") {
    // The other half of the same rule: with nothing able to use more space,
    // the box must not grab any.
    App app{std::make_unique<MockTerminal>(Size{80, 6})};
    auto& row = app.make_root<HBox>();
    auto& page = row.emplace_child<VBox>();
    auto& filler = row.emplace_child<Probe>();
    page.emplace_child<Probe>().width = SizeReq::fixed(10);
    page.emplace_child<Probe>().width = SizeReq::fixed(6);
    filler.width = SizeReq::expand();
    app.pump_once();

    CHECK(page.rect().size.width == 10);   // the widest child, no more
    CHECK(filler.rect().size.width == 70);
}

TEST_CASE("size hints override a widget's own request without subclassing") {
    App app{std::make_unique<MockTerminal>(Size{40, 10})};
    auto& row = app.make_root<HBox>();
    auto& sidebar = row.emplace_child<Probe>();
    auto& rest = row.emplace_child<Probe>();
    sidebar.width = SizeReq::expand();  // would otherwise take half
    rest.width = SizeReq::expand();

    sidebar.width_hint = SizeReq::fixed(14);
    app.pump_once();
    CHECK(sidebar.rect().size.width == 14);
    CHECK(rest.rect().size.width == 26);

    SUBCASE("clearing the hint restores the widget's own request") {
        sidebar.width_hint.reset();
        app.loop().request_layout();
        app.pump_once();
        CHECK(sidebar.rect().size.width == 20);
    }
    SUBCASE("a hint on one axis leaves the other alone") {
        CHECK(sidebar.rect().size.height == 10);
    }
}

TEST_CASE("a height hint stops a row of one-line controls swallowing the page") {
    // A plain Widget used as a spacer expands on BOTH axes, so an HBox of
    // one-line controls reports unbounded height unless told otherwise.
    App app{std::make_unique<MockTerminal>(Size{20, 10})};
    auto& page = app.make_root<VBox>();
    auto& row = page.emplace_child<HBox>();
    row.emplace_child<Widget>();                                  // spacer
    row.emplace_child<Probe>().height = SizeReq::fixed(1);        // a button
    auto& below = page.emplace_child<Probe>();
    below.height = SizeReq::fixed(1);
    page.emplace_child<Widget>();

    app.pump_once();
    CHECK(row.rect().size.height > 1);   // unbounded, as the spacer asked

    row.height_hint = SizeReq::fixed(1);
    app.loop().request_layout();
    app.pump_once();
    CHECK(row.rect().size.height == 1);
    CHECK(below.rect().origin.y == 1);   // the next control sits right under it
}

TEST_CASE("layout settles before the frame is painted, not after the next key") {
    // A wrapping label needs two passes: one to be given a width, one to
    // report the height that follows. The loop blocks on input straight after
    // painting, so if it painted between those passes the stale geometry would
    // stay on screen until the user happened to press a key. ONE pump must be
    // enough.
    App app{std::make_unique<MockTerminal>(Size{11, 6})};
    auto& root = app.make_root<VBox>();
    auto& label = root.emplace_child<Label>("hello world foo");
    label.set_wrap(true);
    root.emplace_child<Widget>();

    app.pump_once();
    CHECK(label.rect().size.height == 2);
    CHECK_FALSE(app.loop().layout_invalid());

    auto& term = static_cast<MockTerminal&>(app.terminal());
    CHECK(term.row_text(0) == "hello world");
    CHECK(term.row_text(1) == "foo        ");
}
