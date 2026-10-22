#include "modcurses/app.hpp"

#include <algorithm>
#include <cstdio>
#include <variant>

namespace modcurses {
namespace {

// Ordered so that std::*_heap gives us the EARLIEST deadline at front().
struct LaterFirst {
    bool operator()(const auto& a, const auto& b) const { return a.deadline > b.deadline; }
};

}  // namespace

// --------------------------------------------------------------- EventLoop

EventLoop::EventLoop(TerminalIO& term, ScreenBuffer& buffer) : term_(&term), buffer_(&buffer) {}

void EventLoop::adopt_root(Widget& root) {
    root_ = &root;
    root.attach_context(this);
    focus_.set_root(&root);
    request_layout();
}

void EventLoop::quit(int code) {
    quitting_ = true;
    exit_code_ = code;
}

int EventLoop::run() {
    while (pump_once()) {
    }
    return exit_code_;
}

bool EventLoop::pump_once() {
    if (quitting_) return false;

    // 1. Paint first, so the opening frame is on screen before we block on
    //    input.
    //
    //    Layout runs to a FIXED POINT before painting, and the flag is cleared
    //    before each pass rather than after. A pass may legitimately request
    //    another: a wrapping label only learns its width once layout assigns
    //    it, and only then can it report an honest height. Clearing afterwards
    //    would discard that request; painting between the passes would leave
    //    stale geometry on screen until the next keypress, because the loop
    //    blocks on input immediately below. The bound means a widget that
    //    always re-requests degrades to a wrong size rather than hanging.
    for (int pass = 0; layout_invalid_ && pass < kMaxLayoutPasses; ++pass) {
        layout_invalid_ = false;
        run_layout();
        // A layout pass may have moved anything, so the frame must follow it.
        // The diff makes this free when nothing actually moved.
        frame_dirty_ = true;
    }
    // Layout may have hidden whatever had focus (a Stack page change does
    // exactly that). Releasing it here, before the frame, keeps an invisible
    // widget from going on consuming the keyboard.
    if (focus_.release_if_unreachable()) frame_dirty_ = true;

    if (frame_dirty_) {
        frame_dirty_ = false;
        render();
    }
    update_cursor();

    // 2. Wait for input or for the next timer, whichever comes first.
    if (auto ev = term_->poll_event(time_until_next_timer())) dispatch(*ev);

    // 3. Timer callbacks run here, on the loop's own thread, like everything
    //    else.
    fire_due_timers();

    // 4. Housekeeping, at a point where nothing is mid-dispatch.
    reap();

    return !quitting_;
}

void EventLoop::run_layout() {
    if (root_ == nullptr) return;
    root_->set_rect(Rect{{0, 0}, buffer_->size()});
}

void EventLoop::render() {
    if (root_ == nullptr) return;
    // Always repaint the whole tree into the back buffer; the diff in
    // flush_to() is what keeps terminal I/O small. There are no partial
    // repaints anywhere in this library, by design (lesson 6).
    buffer_->clear_back(Glyph{U' ', root_->style});
    paint_widget(root_, Rect{{0, 0}, buffer_->size()});
    buffer_->flush_to(*term_);
    ++frames_;
}

void EventLoop::paint_widget(Widget* w, Rect clip) {
    if (w == nullptr || !w->visible || w->rect().empty()) return;
    const Rect visible = clip.intersect(w->rect());
    if (visible.empty()) return;

    Canvas canvas{*buffer_, w->rect(), visible};
    w->paint(canvas);
    // Parents first, then children, so a child always composites onto a
    // surface its parent has already defined.
    for (const auto& child : w->children()) paint_widget(child.get(), visible);
}

void EventLoop::update_cursor() {
    std::optional<Point> pos;
    if (Widget* f = focus_.current(); f != nullptr && f->visible && !f->rect().empty()) {
        if (const auto local = f->cursor()) {
            const Point absolute{f->rect().left() + local->x, f->rect().top() + local->y};
            if (f->rect().contains(absolute)) pos = absolute;
        }
    }
    term_->set_cursor(pos);
}

// ---------------------------------------------------------------- dispatch

void EventLoop::dispatch(const Event& ev) {
    std::visit(
        [this](const auto& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, KeyEvent>) dispatch_key(e);
            else if constexpr (std::is_same_v<T, MouseEvent>) dispatch_mouse(e);
            else dispatch_resize(e);
        },
        ev);
}

void EventLoop::dispatch_key(const KeyEvent& ev) {
    // 1. App-level shortcuts, so a global quit key always works no matter
    //    which widget has focus.
    for (const auto& s : shortcuts_) {
        if (s.match == ev) {
            if (s.fn) s.fn();
            return;
        }
    }

    // 2/3. The focused widget, hook before virtual, then bubble to the root.
    //      With nothing focused we start at the root, so a keyboard-driven
    //      app with no focusable widgets still receives keys.
    Widget* start = focus_.current() != nullptr ? focus_.current() : root_;
    for (Widget* w = start; w != nullptr; w = w->parent()) {
        if (w->on_key_hook && w->on_key_hook(ev)) return;
        if (w->on_key(ev)) return;
    }

    // 4. Focus traversal comes LAST, so a widget that wants Tab for itself
    //    (a text area inserting an indent) gets first refusal.
    if (ev.key == Key::Tab && !ev.mods.ctrl && !ev.mods.alt) {
        focus_.next();
        request_frame();
    } else if (ev.key == Key::BackTab) {
        focus_.previous();
        request_frame();
    }
}

void EventLoop::dispatch_mouse(const MouseEvent& ev) {
    Widget* target = hit_test(root_, ev.pos);
    if (target == nullptr) return;

    // A press focuses the nearest click-capable widget at or above the hit,
    // so clicking a label inside a button focuses the button.
    if (ev.action == MouseEvent::Action::Press && ev.button == MouseEvent::Button::Left) {
        for (Widget* w = target; w != nullptr; w = w->parent()) {
            if (w->visible && accepts_click(w->focus_policy)) {
                set_focus(w);
                break;
            }
        }
    }

    for (Widget* w = target; w != nullptr; w = w->parent()) {
        MouseEvent local = ev;
        local.pos = Point{ev.pos.x - w->rect().left(), ev.pos.y - w->rect().top()};
        if (w->on_mouse_hook && w->on_mouse_hook(local)) return;
        if (w->on_mouse(local)) return;
    }
}

void EventLoop::dispatch_resize(const ResizeEvent& ev) {
    // The backend has already resynced curses by this point (including
    // PDCurses' resize_term). Widgets learn about it only through the layout
    // pass and on_geometry - they never see the event.
    buffer_->resize(ev.size);
    request_layout();
}

Widget* EventLoop::hit_test(Widget* w, Point p) {
    if (w == nullptr || !w->visible || !w->rect().contains(p)) return nullptr;
    // Children paint after their parent, so later children sit on top:
    // search back to front.
    const auto kids = w->children();
    for (auto it = kids.rbegin(); it != kids.rend(); ++it)
        if (Widget* deeper = hit_test(it->get(), p)) return deeper;
    return w;
}

bool EventLoop::in_tree(Widget* root, Widget* needle) {
    if (root == nullptr) return false;
    if (root == needle) return true;
    for (const auto& c : root->children())
        if (in_tree(c.get(), needle)) return true;
    return false;
}

// --------------------------------------------------------------- shortcuts

void EventLoop::add_shortcut(KeyEvent match, std::function<void()> fn) {
    for (auto& s : shortcuts_) {
        if (s.match == match) {
            s.fn = std::move(fn);
            return;
        }
    }
    shortcuts_.push_back(Shortcut{match, std::move(fn)});
}

bool EventLoop::remove_shortcut(KeyEvent match) {
    const auto it = std::find_if(shortcuts_.begin(), shortcuts_.end(),
                                 [&](const Shortcut& s) { return s.match == match; });
    if (it == shortcuts_.end()) return false;
    shortcuts_.erase(it);
    return true;
}

// ------------------------------------------------------------------ focus

void EventLoop::set_focus(Widget* w) {
    if (w == focus_.current()) return;
    focus_.set(w);
    request_frame();
}

// ------------------------------------------------------------------ timers

TimerHandle EventLoop::add_timer(std::chrono::milliseconds period, std::function<void()> fn,
                                 Widget* owner) {
    // A zero or negative period would spin the loop; one millisecond is the
    // floor.
    if (period <= std::chrono::milliseconds{0}) period = std::chrono::milliseconds{1};

    auto ctl = std::make_shared<detail::TimerControl>();
    ctl->id = next_timer_id_++;
    ctl->live = true;

    timers_.push_back(TimerEntry{std::chrono::steady_clock::now() + period, period, std::move(fn),
                                 ctl, owner});
    std::push_heap(timers_.begin(), timers_.end(), LaterFirst{});

    if (owner != nullptr) owner->timers_.push_back(ctl);
    return TimerHandle{std::move(ctl)};
}

std::optional<std::chrono::milliseconds> EventLoop::time_until_next_timer() const {
    if (timers_.empty()) return std::nullopt;  // block until input arrives
    const auto now = std::chrono::steady_clock::now();
    const auto delta = timers_.front().deadline - now;
    if (delta <= std::chrono::steady_clock::duration::zero()) return std::chrono::milliseconds{0};
    return std::chrono::duration_cast<std::chrono::milliseconds>(delta) +
           std::chrono::milliseconds{1};  // round up, never wake a tick early
}

void EventLoop::fire_due_timers() {
    const auto now = std::chrono::steady_clock::now();
    while (!timers_.empty() && timers_.front().deadline <= now) {
        std::pop_heap(timers_.begin(), timers_.end(), LaterFirst{});
        TimerEntry entry = std::move(timers_.back());
        timers_.pop_back();

        if (!entry.ctl || entry.ctl->cancelled) {
            if (entry.ctl) entry.ctl->live = false;
            continue;
        }

        // The callback may cancel this timer, add others, or destroy widgets.
        // The entry is out of the heap while it runs, so none of that can
        // disturb the walk.
        if (entry.fn) entry.fn();

        if (entry.ctl->cancelled) {
            entry.ctl->live = false;
            continue;
        }

        // Re-arm from the SCHEDULED deadline, not from now, so a repeating
        // timer keeps its phase and gravity does not drift. If we fell behind,
        // skip the missed ticks rather than firing a catch-up burst.
        do {
            entry.deadline += entry.period;
        } while (entry.deadline <= now);

        timers_.push_back(std::move(entry));
        std::push_heap(timers_.begin(), timers_.end(), LaterFirst{});
    }
}

// ---------------------------------------------------------------- teardown

void EventLoop::schedule_destroy(Widget* w) {
    if (w != nullptr) destroy_list_.push_back(w);
}

void EventLoop::reap() {
    if (destroy_list_.empty()) return;
    const std::vector<Widget*> pending = std::move(destroy_list_);
    destroy_list_.clear();

    for (Widget* w : pending) {
        // Never dereference w before confirming it is still in the tree: an
        // earlier entry may have been its ancestor and already freed it.
        if (w == root_ || !in_tree(root_, w)) continue;
        if (Widget* parent = w->parent()) parent->remove_child(*w);  // freed here
    }

    // Drop timers whose owning widget is gone.
    const auto dead = std::remove_if(timers_.begin(), timers_.end(), [](const TimerEntry& t) {
        return !t.ctl || t.ctl->cancelled;
    });
    if (dead != timers_.end()) {
        timers_.erase(dead, timers_.end());
        std::make_heap(timers_.begin(), timers_.end(), LaterFirst{});
    }

    focus_.repair();
    request_layout();
}

// --------------------------------------------------------------------- App

App::App(AppInfo info) : info_(std::move(info)) {
    term_ = make_curses_terminal();
    bootstrap();
}

App::App(std::unique_ptr<TerminalIO> term, AppInfo info) : info_(std::move(info)) {
    if (!term) throw TerminalError{"App was given a null terminal"};
    term_ = std::move(term);
    bootstrap();
}

App::App(int argc, char** argv, ArgParser& args, AppInfo info) : info_(std::move(info)) {
    // THE contractual ordering of this library's bootstrap, and the reason
    // this overload exists at all: parse first, and construct the terminal
    // only if the command line did not already decide we are done. Do it the
    // other way round and --help paints into a curses screen that is then
    // torn down, leaving the user with nothing.
    parse_result_ = args.parse(argc, argv);

    if (parse_result_.done()) {
        exit_early_ = true;
        exit_code_ = parse_result_.exit_code;
        exit_message_ = parse_result_.message;

        // Help and version are output; a usage error is a diagnostic. They go
        // to different streams for the same reason every other tool does it:
        // so `prog --help | less` works and `prog --bogus` does not pollute a
        // pipeline.
        std::FILE* stream = parse_result_.ok ? stdout : stderr;
        std::fputs(exit_message_.c_str(), stream);
        if (!exit_message_.empty() && exit_message_.back() != '\n') std::fputc('\n', stream);
        if (!parse_result_.ok)
            std::fprintf(stderr, "try '%s --help' for more information\n", args.program().c_str());
        std::fflush(stream);
        return;  // no terminal, no loop, no widgets
    }

    term_ = make_curses_terminal();
    bootstrap();
}

void App::require_terminal() const {
    if (exit_early_)
        throw TerminalError{
            "this App has no terminal: the command line asked to exit (--help, "
            "--version, or a usage error). Check should_exit() before building "
            "a widget tree."};
}

void App::bootstrap() {
    buffer_.resize(term_->size());
    palette_ = Palette{term_.get()};
    loop_ = std::make_unique<EventLoop>(*term_, buffer_);

    // The backend runs in raw() mode, so Ctrl-C arrives as a key rather than a
    // signal. Without a default way out, a tester has to kill the window -
    // that was observed, not theorised. Applications may override this by
    // registering their own Ctrl-C shortcut.
    loop_->add_shortcut(ctrl_ev(U'c'), [this] { quit(0); });
}

// Declared out of line so the header does not need EventLoop or Widget to be
// complete for the destructor.
App::~App() {
    // Tear the tree down before the loop, so widget destructors can still
    // reach a live context, and before the terminal, so endwin() is last.
    root_.reset();
    loop_.reset();
}

}  // namespace modcurses
