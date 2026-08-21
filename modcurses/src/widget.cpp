#include "modcurses/widget.hpp"

#include <numeric>

namespace modcurses {

// ------------------------------------------------------------------ widget

Widget::~Widget() {
    // Timers outlive their handles but never their owner.
    for (auto& t : timers_) {
        if (t) {
            t->cancelled = true;
            t->live = false;
        }
    }
}

void Widget::attach_context(detail::LoopContext* ctx) {
    ctx_ = ctx;
    // A widget may build its own subtree in its constructor, before it is
    // attached to anything; those descendants get the context here.
    for (auto& c : children_) c->attach_context(ctx);
}

std::unique_ptr<Widget> Widget::remove_child(Widget& child) {
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if (it->get() != &child) continue;
        std::unique_ptr<Widget> owned = std::move(*it);
        children_.erase(it);
        owned->parent_ = nullptr;
        owned->attach_context(nullptr);
        invalidate_layout();
        return owned;
    }
    return nullptr;
}

void Widget::destroy_later() {
    if (destroy_pending_) return;
    destroy_pending_ = true;
    if (ctx_) ctx_->schedule_destroy(this);
}

void Widget::invalidate() {
    if (ctx_) ctx_->request_frame();
}

void Widget::invalidate_layout() {
    if (ctx_) ctx_->request_layout();
}

bool Widget::has_focus() const { return ctx_ != nullptr && ctx_->focused() == this; }

bool Widget::focusable() const {
    // An invisible widget, or one the layout gave no room to, cannot be
    // focused - otherwise Tab would stop on something the user cannot see.
    return visible && !rect_.empty() && accepts_tab(focus_policy);
}

void Widget::take_focus() {
    if (ctx_) ctx_->set_focus(this);
}

TimerHandle Widget::add_timer(std::chrono::milliseconds period, std::function<void()> fn) {
    if (!ctx_) return TimerHandle{};
    TimerHandle h = ctx_->add_timer(period, std::move(fn), this);
    return h;
}

void Widget::layout_children() {
    // A plain widget is not a box, but it may still have children - a button
    // with a label inside it, say. The useful default is that each child
    // fills its parent, the way Stack's active page does. Leaving them at an
    // empty rect instead would make them invisible to paint, hit-test and
    // focus alike, which is never what the caller meant by adding them.
    for (const auto& c : children_) assign_rect(*c, c->visible ? rect_ : Rect{});
}

void Widget::set_rect(Rect r) {
    const Rect old = rect_;
    rect_ = r;
    if (old != r) on_geometry(old, r);
    // Children are placed even when this widget's own rect did not change:
    // a child's SizeReq may have.
    layout_children();
}

// ------------------------------------------------------------- distribution

std::vector<int> distribute(std::span<const SizeReq> reqs, int total) {
    const auto n = static_cast<int>(reqs.size());
    std::vector<int> out(static_cast<std::size_t>(n), 0);
    if (n == 0) return out;
    if (total < 0) total = 0;

    const auto lo = [&](int i) { return std::max(0, reqs[static_cast<std::size_t>(i)].min); };
    const auto hi = [&](int i) {
        return std::max(lo(i), reqs[static_cast<std::size_t>(i)].max);
    };
    const auto weight = [&](int i) {
        return std::max(0, reqs[static_cast<std::size_t>(i)].weight);
    };
    const auto at = [&](int i) -> int& { return out[static_cast<std::size_t>(i)]; };

    // 1. preferred, clamped.
    long long sum = 0;
    for (int i = 0; i < n; ++i) {
        at(i) = std::clamp(reqs[static_cast<std::size_t>(i)].preferred, lo(i), hi(i));
        sum += at(i);
    }

    // 2. grow into leftover space, proportionally to weight.
    while (sum < total) {
        long long total_weight = 0;
        for (int i = 0; i < n; ++i)
            if (at(i) < hi(i) && weight(i) > 0) total_weight += weight(i);
        if (total_weight == 0) break;  // nobody can take more

        const long long space = total - sum;
        bool progressed = false;
        for (int i = 0; i < n && sum < total; ++i) {
            if (at(i) >= hi(i) || weight(i) == 0) continue;
            // At least 1 per pass, so a rounded-down share cannot stall.
            long long share = std::max<long long>(1, (space * weight(i)) / total_weight);
            share = std::min<long long>(share, hi(i) - at(i));
            share = std::min<long long>(share, total - sum);
            if (share <= 0) continue;
            at(i) += static_cast<int>(share);
            sum += share;
            progressed = true;
        }
        if (!progressed) break;
    }

    // 3. shrink out of an overdraft, proportionally to weight.
    while (sum > total) {
        long long total_weight = 0;
        for (int i = 0; i < n; ++i)
            if (at(i) > lo(i) && weight(i) > 0) total_weight += weight(i);
        if (total_weight == 0) break;  // everyone is at their minimum

        const long long excess = sum - total;
        bool progressed = false;
        for (int i = 0; i < n && sum > total; ++i) {
            if (at(i) <= lo(i) || weight(i) == 0) continue;
            long long take = std::max<long long>(1, (excess * weight(i)) / total_weight);
            take = std::min<long long>(take, at(i) - lo(i));
            take = std::min<long long>(take, sum - total);
            if (take <= 0) continue;
            at(i) -= static_cast<int>(take);
            sum -= take;
            progressed = true;
        }
        if (!progressed) break;
    }

    // 4. Still over budget: everyone is at their minimum and the minimums do
    //    not fit. Hide from the last child backwards rather than squeezing
    //    anyone below their stated minimum.
    for (int i = n - 1; i >= 0 && sum > total; --i) {
        if (at(i) == 0) continue;
        sum -= at(i);
        at(i) = 0;
    }

    return out;
}

namespace {

// Visible children only; a hidden child takes part in neither layout nor
// painting, but keeps all of its state.
std::vector<Widget*> visible_children(const Widget& w) {
    std::vector<Widget*> out;
    for (const auto& c : w.children())
        if (c->visible) out.push_back(c.get());
    return out;
}

// Combines requirements along the axis the box stacks on: minimums and
// preferences add up, weights add up, and a max saturates instead of wrapping.
SizeReq sum_reqs(const std::vector<SizeReq>& reqs) {
    if (reqs.empty()) return SizeReq::fixed(0);
    constexpr int kMax = std::numeric_limits<int>::max();
    SizeReq out{0, 0, 0, 0};
    long long max_sum = 0;
    for (const auto& r : reqs) {
        out.min += r.min;
        out.preferred += r.preferred;
        out.weight += std::max(0, r.weight);
        max_sum += r.max;
    }
    out.max = max_sum > kMax ? kMax : static_cast<int>(max_sum);
    if (out.weight == 0) out.max = out.preferred;  // nothing here can stretch
    return out;
}

// Combines requirements across the axis the box does NOT stack on: it must be
// wide enough for its widest child, and prefers its widest preference.
//
// The maximum is the LARGEST child maximum, not the smallest. Taking the
// smallest looks superficially safe and is badly wrong: layout_children
// already clamps each child to its own [min, max] within the box, so one
// narrow child capping the whole container just starves every other child.
// (A single Checkbox once held an entire page to 14 columns this way.) Past
// the widest child's maximum there is genuinely nothing left to use the
// space, which is what makes that the right ceiling.
SizeReq span_reqs(const std::vector<SizeReq>& reqs) {
    if (reqs.empty()) return SizeReq::fixed(0);
    SizeReq out{0, 0, 0, 0};
    for (const auto& r : reqs) {
        out.min = std::max(out.min, r.min);
        out.preferred = std::max(out.preferred, r.preferred);
        out.max = std::max(out.max, std::max(r.min, r.max));
        out.weight = std::max(out.weight, std::max(0, r.weight));
    }
    out.max = std::max(out.max, out.min);
    return out;
}

}  // namespace

// -------------------------------------------------------------------- VBox

SizeReq VBox::height_req() const {
    std::vector<SizeReq> reqs;
    for (Widget* c : visible_children(*this)) reqs.push_back(c->effective_height_req());
    return sum_reqs(reqs);
}

SizeReq VBox::width_req() const {
    std::vector<SizeReq> reqs;
    for (Widget* c : visible_children(*this)) reqs.push_back(c->effective_width_req());
    return span_reqs(reqs);
}

void VBox::layout_children() {
    const auto kids = visible_children(*this);
    std::vector<SizeReq> reqs;
    reqs.reserve(kids.size());
    for (Widget* c : kids) reqs.push_back(c->effective_height_req());

    const std::vector<int> heights = distribute(reqs, rect().size.height);

    int y = rect().top();
    for (std::size_t i = 0; i < kids.size(); ++i) {
        Widget* c = kids[i];
        const int h = heights[i];
        if (h <= 0) {
            // No room this pass. An empty rect is skipped by paint, hit-test
            // and focus alike, and the widget keeps every bit of its state.
            assign_rect(*c, Rect{{rect().left(), y}, {0, 0}});
            continue;
        }
        const SizeReq wr = c->effective_width_req();
        const int w = std::clamp(rect().size.width, std::max(0, wr.min), std::max(wr.min, wr.max));
        assign_rect(*c, Rect{{rect().left(), y}, {w, h}});
        y += h;
    }
    // Children hidden with `visible = false` still need a defined rect.
    for (const auto& c : children())
        if (!c->visible) assign_rect(*c, Rect{});
}

// -------------------------------------------------------------------- HBox

SizeReq HBox::width_req() const {
    std::vector<SizeReq> reqs;
    for (Widget* c : visible_children(*this)) reqs.push_back(c->effective_width_req());
    return sum_reqs(reqs);
}

SizeReq HBox::height_req() const {
    std::vector<SizeReq> reqs;
    for (Widget* c : visible_children(*this)) reqs.push_back(c->effective_height_req());
    return span_reqs(reqs);
}

void HBox::layout_children() {
    const auto kids = visible_children(*this);
    std::vector<SizeReq> reqs;
    reqs.reserve(kids.size());
    for (Widget* c : kids) reqs.push_back(c->effective_width_req());

    const std::vector<int> widths = distribute(reqs, rect().size.width);

    int x = rect().left();
    for (std::size_t i = 0; i < kids.size(); ++i) {
        Widget* c = kids[i];
        const int w = widths[i];
        if (w <= 0) {
            assign_rect(*c, Rect{{x, rect().top()}, {0, 0}});
            continue;
        }
        const SizeReq hr = c->effective_height_req();
        const int h = std::clamp(rect().size.height, std::max(0, hr.min), std::max(hr.min, hr.max));
        assign_rect(*c, Rect{{x, rect().top()}, {w, h}});
        x += w;
    }
    for (const auto& c : children())
        if (!c->visible) assign_rect(*c, Rect{});
}

// ------------------------------------------------------------------- Stack

Widget* Stack::active() const {
    const auto kids = children();
    if (active_ < 0 || active_ >= static_cast<int>(kids.size())) return nullptr;
    return kids[static_cast<std::size_t>(active_)].get();
}

void Stack::set_active(int index) {
    const auto n = static_cast<int>(children().size());
    if (index < 0 || index >= n || index == active_) return;
    active_ = index;
    invalidate_layout();
    page_changed.emit(index);
}

void Stack::set_active(Widget& child) {
    const auto kids = children();
    for (std::size_t i = 0; i < kids.size(); ++i) {
        if (kids[i].get() == &child) {
            set_active(static_cast<int>(i));
            return;
        }
    }
}

SizeReq Stack::width_req() const {
    Widget* a = active();
    return a ? a->effective_width_req() : SizeReq{};
}

SizeReq Stack::height_req() const {
    Widget* a = active();
    return a ? a->effective_height_req() : SizeReq{};
}

void Stack::layout_children() {
    Widget* a = active();
    for (const auto& c : children())
        assign_rect(*c, c.get() == a && c->visible ? rect() : Rect{});
}

// ------------------------------------------------------------------- focus

void FocusChain::collect(Widget* w, std::vector<Widget*>& out) {
    if (w == nullptr || !w->visible || w->rect().empty()) return;
    if (w->focusable()) out.push_back(w);
    for (const auto& c : w->children()) collect(c.get(), out);
}

std::vector<Widget*> FocusChain::order() const {
    std::vector<Widget*> out;
    collect(root_, out);
    return out;
}

void FocusChain::set(Widget* w) {
    if (w == current_) return;
    Widget* old = current_;
    current_ = w;
    if (old) old->on_focus(false);
    if (current_) current_->on_focus(true);
}

void FocusChain::next() {
    const auto chain = order();
    if (chain.empty()) return set(nullptr);
    const auto it = std::find(chain.begin(), chain.end(), current_);
    if (it == chain.end() || std::next(it) == chain.end()) return set(chain.front());
    set(*std::next(it));
}

void FocusChain::previous() {
    const auto chain = order();
    if (chain.empty()) return set(nullptr);
    const auto it = std::find(chain.begin(), chain.end(), current_);
    if (it == chain.end() || it == chain.begin()) return set(chain.back());
    set(*std::prev(it));
}

namespace {

// Identity search only: `needle` may already be destroyed, so it must never
// be dereferenced here.
bool still_present(Widget* w, Widget* needle) {
    if (w == nullptr) return false;
    if (w == needle) return true;
    for (const auto& c : w->children())
        if (still_present(c.get(), needle)) return true;
    return false;
}

}  // namespace

bool FocusChain::release_if_unreachable() {
    if (current_ == nullptr) return false;
    const bool reachable = still_present(root_, current_) && current_->visible &&
                           !current_->rect().empty() &&
                           current_->focus_policy != FocusPolicy::None;
    if (reachable) return false;

    Widget* was = current_;
    current_ = nullptr;
    // Safe to notify: still_present confirmed it is in the tree, so it is a
    // live object that merely became unreachable.
    if (still_present(root_, was)) was->on_focus(false);
    return true;
}

void FocusChain::repair() {
    // Keep the current widget if it is still in the tree and still takes
    // focus at all. Note this is deliberately NOT order() membership: a
    // Click-only widget is legitimately focused by a mouse press but never
    // appears in the Tab order, and must not be repaired away.
    if (current_ != nullptr && still_present(root_, current_) && current_->visible &&
        !current_->rect().empty() && current_->focus_policy != FocusPolicy::None) {
        return;
    }

    // Otherwise the focused widget is gone, hidden, or no longer focusable.
    // Drop it WITHOUT firing on_focus(false) - it may already be destroyed.
    current_ = nullptr;
    const auto chain = order();
    if (!chain.empty()) set(chain.front());
}

}  // namespace modcurses
