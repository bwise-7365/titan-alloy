#pragma once
//
// modcurses/widget.hpp - the widget base, the layout system, focus.
//
// PUBLIC HEADER: no curses.
//
// Ownership is unambiguous: a parent owns its children by unique_ptr, and
// emplace_child is the only way to make a non-root widget. Geometry is
// assigned by the layout pass and never by a widget to itself.
//
#include <algorithm>
#include <chrono>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "modcurses/core.hpp"
#include "modcurses/render.hpp"

namespace modcurses {

class Widget;
class EventLoop;
class FocusChain;

namespace detail {

// The services a widget needs from the loop it lives under. EventLoop
// implements it. A widget that is not attached to a loop has a null context
// and every request becomes a silent no-op, so detached widgets (in tests,
// or mid-construction) are safe to use.
class LoopContext {
public:
    virtual void request_frame() = 0;
    virtual void request_layout() = 0;
    virtual void set_focus(Widget* w) = 0;
    [[nodiscard]] virtual Widget* focused() const = 0;
    virtual TimerHandle add_timer(std::chrono::milliseconds period, std::function<void()> fn,
                                  Widget* owner) = 0;
    virtual void schedule_destroy(Widget* w) = 0;

protected:
    ~LoopContext() = default;
};

}  // namespace detail

// ------------------------------------------------------------------ layout

struct SizeReq {
    int min = 0;
    int preferred = 1;
    int max = std::numeric_limits<int>::max();
    int weight = 1;  // share of any leftover space

    constexpr auto operator<=>(const SizeReq&) const = default;

    [[nodiscard]] static constexpr SizeReq fixed(int n) { return {n, n, n, 0}; }
    [[nodiscard]] static constexpr SizeReq expand(int w = 1) {
        return {0, 1, std::numeric_limits<int>::max(), w};
    }
    [[nodiscard]] static constexpr SizeReq at_least(int n, int w = 1) {
        return {n, n, std::numeric_limits<int>::max(), w};
    }
};

// Distributes `total` across `reqs`, per TUI_DESIGN section 6:
//   1. everyone gets `preferred`, clamped to [min, max]
//   2. leftover space goes out proportionally to weight, skipping anyone at max
//   3. an overdraft is taken back proportionally to weight, skipping anyone at min
//   4. if it still does not fit, trailing entries are dropped to 0 ("hidden
//      for this pass") rather than being squeezed below their minimum
// A returned length of 0 means the widget has no room this pass.
[[nodiscard]] std::vector<int> distribute(std::span<const SizeReq> reqs, int total);

enum class FocusPolicy : std::uint8_t {
    None = 0,
    Click = 1,
    Tab = 2,
    Strong = 3,  // Click | Tab
};

[[nodiscard]] constexpr bool accepts_click(FocusPolicy p) {
    return (static_cast<std::uint8_t>(p) & static_cast<std::uint8_t>(FocusPolicy::Click)) != 0;
}
[[nodiscard]] constexpr bool accepts_tab(FocusPolicy p) {
    return (static_cast<std::uint8_t>(p) & static_cast<std::uint8_t>(FocusPolicy::Tab)) != 0;
}

// ------------------------------------------------------------------ widget

class Widget {
    friend class EventLoop;
    friend class FocusChain;

public:
    Widget() = default;
    virtual ~Widget();

    Widget(const Widget&) = delete;             // a widget is an identity, not a value
    Widget& operator=(const Widget&) = delete;

    // ---- tree (a parent owns its children) ----

    template <typename W, typename... Args>
    W& emplace_child(Args&&... args) {
        auto owned = std::make_unique<W>(std::forward<Args>(args)...);
        W& ref = *owned;
        Widget& as_widget = ref;
        as_widget.parent_ = this;
        as_widget.attach_context(ctx_);
        children_.push_back(std::move(owned));
        invalidate_layout();
        return ref;
    }

    // Detaches a direct child and hands back ownership. Returns nullptr if
    // `child` is not a direct child of this widget.
    std::unique_ptr<Widget> remove_child(Widget& child);

    // Destroys this widget at the end of the current loop iteration. This is
    // the ONLY deferred mechanism in the library - it exists so a handler can
    // safely delete the widget it is running inside (lesson 4).
    void destroy_later();

    [[nodiscard]] Widget* parent() const { return parent_; }
    [[nodiscard]] std::span<const std::unique_ptr<Widget>> children() const { return children_; }

    // ---- geometry (assigned by layout; never self-assigned) ----

    [[nodiscard]] Rect rect() const { return rect_; }
    [[nodiscard]] Size size() const { return rect_.size; }
    [[nodiscard]] virtual SizeReq width_req() const { return {}; }
    [[nodiscard]] virtual SizeReq height_req() const { return {}; }

    // Per-instance overrides of the two above. Subclassing purely to pin a
    // size is a lot of ceremony for "this row is one line tall", and the need
    // comes up constantly: a plain Widget used as a spacer expands on BOTH
    // axes, so a row of one-line controls otherwise claims unbounded height.
    // Layout consults effective_*_req(), never the virtuals directly.
    std::optional<SizeReq> width_hint;
    std::optional<SizeReq> height_hint;

    [[nodiscard]] SizeReq effective_width_req() const {
        return width_hint.value_or(width_req());
    }
    [[nodiscard]] SizeReq effective_height_req() const {
        return height_hint.value_or(height_req());
    }

    // ---- appearance & state ----

    Style style;         // background and default text style
    bool visible = true;

    void invalidate();          // repaint this frame
    void invalidate_layout();   // re-run layout, then repaint

    // ---- focus ----

    FocusPolicy focus_policy = FocusPolicy::None;
    [[nodiscard]] bool has_focus() const;
    [[nodiscard]] bool focusable() const;
    void take_focus();

    // ---- simple-case hooks (consulted BEFORE the virtuals) ----

    std::function<bool(const KeyEvent&)> on_key_hook;
    std::function<bool(const MouseEvent&)> on_mouse_hook;

    // ---- timers ----
    //
    // Repeating. Hold the handle to control the timer's life; drop it and the
    // timer dies immediately; call release() on it for fire-and-forget. Either
    // way the timer is cancelled when this widget is destroyed.
    [[nodiscard]] TimerHandle add_timer(std::chrono::milliseconds period,
                                        std::function<void()> fn);

protected:
    // ---- overridables; the defaults do the sane minimal thing ----

    virtual void paint(Canvas& c) { c.fill(Glyph{U' ', style}); }
    virtual bool on_key(const KeyEvent&) { return false; }     // true == handled
    virtual bool on_mouse(const MouseEvent&) { return false; }
    virtual void on_focus(bool /*gained*/) {}
    virtual void on_geometry(Rect /*old_rect*/, Rect /*new_rect*/) {}
    [[nodiscard]] virtual std::optional<Point> cursor() const { return std::nullopt; }

    // Places this widget's children; runs immediately after its own rect is
    // assigned. The boxes override it. The default gives every visible child
    // the parent's whole rect.
    virtual void layout_children();

    // Layout helper for containers. Static so that a container may assign a
    // rect to a child held as a Widget& - a non-static protected member could
    // only be reached through an object of the container's own type.
    static void assign_rect(Widget& w, Rect r) { w.set_rect(r); }

private:
    void set_rect(Rect r);
    void attach_context(detail::LoopContext* ctx);

    Widget* parent_ = nullptr;
    std::vector<std::unique_ptr<Widget>> children_;
    detail::LoopContext* ctx_ = nullptr;
    Rect rect_;
    bool destroy_pending_ = false;

    // Controls for timers this widget created, so they die with it even if the
    // caller released the handle.
    std::vector<std::shared_ptr<detail::TimerControl>> timers_;
};

// -------------------------------------------------------------- containers

// Stacks children top to bottom, distributing height by SizeReq. Each child
// gets the container's width, clamped to its own [min, max].
class VBox : public Widget {
public:
    [[nodiscard]] SizeReq width_req() const override;
    [[nodiscard]] SizeReq height_req() const override;

protected:
    void layout_children() override;
};

// The mirror image: children left to right, width distributed by SizeReq.
class HBox : public Widget {
public:
    [[nodiscard]] SizeReq width_req() const override;
    [[nodiscard]] SizeReq height_req() const override;

protected:
    void layout_children() override;
};

// Shows exactly one child at a time; the active child fills the container.
// This is the page / menu-navigation primitive.
class Stack : public Widget {
public:
    Signal<int> page_changed;

    void set_active(int index);
    void set_active(Widget& child);
    [[nodiscard]] int active_index() const { return active_; }
    [[nodiscard]] Widget* active() const;

    [[nodiscard]] SizeReq width_req() const override;
    [[nodiscard]] SizeReq height_req() const override;

protected:
    void layout_children() override;

private:
    int active_ = 0;
};

// ------------------------------------------------------------------- focus

// Owned by the loop. Tab order is tree pre-order over visible, laid-out,
// Tab-capable widgets.
class FocusChain {
public:
    void set_root(Widget* root) { root_ = root; }
    [[nodiscard]] Widget* current() const { return current_; }

    void set(Widget* w);   // fires on_focus(false) then on_focus(true)
    void next();           // Tab
    void previous();       // BackTab

    // Called after widgets are destroyed: if the focused widget is gone (or no
    // longer focusable), move focus to the first widget that will take it.
    void repair();

    // Releases focus if the focused widget is no longer something the user
    // can see or reach - hidden, unlaid-out, or gone from the tree. Unlike
    // repair() it never ACQUIRES focus, which makes it safe to run after
    // every layout pass. That is where it belongs: hiding a Stack page leaves
    // its widgets with an empty rect, and focus stranded on one of them means
    // an invisible widget goes on eating the keyboard.
    bool release_if_unreachable();

    [[nodiscard]] std::vector<Widget*> order() const;

private:
    static void collect(Widget* w, std::vector<Widget*>& out);

    Widget* root_ = nullptr;
    Widget* current_ = nullptr;
};

}  // namespace modcurses
