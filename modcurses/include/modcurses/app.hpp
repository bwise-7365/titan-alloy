#pragma once
//
// modcurses/app.hpp - the event loop and the application bootstrap.
//
// PUBLIC HEADER: no curses.
//
// One loop, one thread, no queues. Timers are part of the loop's wait rather
// than events, which removes the routing question entirely (lesson 4).
// Everything is synchronous: a handler mutates state and calls invalidate();
// the effect appears at the top of the next iteration.
//
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "modcurses/args.hpp"
#include "modcurses/core.hpp"
#include "modcurses/render.hpp"
#include "modcurses/terminal.hpp"
#include "modcurses/widget.hpp"

namespace modcurses {

class EventLoop final : public detail::LoopContext {
public:
    EventLoop(TerminalIO& term, ScreenBuffer& buffer);
    ~EventLoop() = default;  // LoopContext's dtor is protected, not virtual

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    // Takes the widget as the tree root: hands it the loop context, points
    // the focus chain at it, and marks layout dirty. Does not own it.
    void adopt_root(Widget& root);
    [[nodiscard]] Widget* root() const { return root_; }

    int run();                       // returns the exit code
    void quit(int code = 0);
    [[nodiscard]] bool quitting() const { return quitting_; }

    // Exactly one iteration of run()'s body. Returns false once the loop is
    // finished. This is what makes the whole loop testable against a
    // MockTerminal with no TTY in sight.
    bool pump_once();

    [[nodiscard]] FocusChain& focus() { return focus_; }
    [[nodiscard]] const FocusChain& focus() const { return focus_; }

    // App-level shortcuts are consulted before any widget sees the key.
    // Adding a shortcut for a key that already has one replaces it.
    void add_shortcut(KeyEvent match, std::function<void()> fn);
    bool remove_shortcut(KeyEvent match);

    // Diagnostics, mostly for tests.
    [[nodiscard]] int frames_rendered() const { return frames_; }
    [[nodiscard]] std::size_t timer_count() const { return timers_.size(); }
    [[nodiscard]] bool frame_dirty() const { return frame_dirty_; }
    [[nodiscard]] bool layout_invalid() const { return layout_invalid_; }

    // ---- detail::LoopContext ----
    void request_frame() override { frame_dirty_ = true; }
    void request_layout() override {
        layout_invalid_ = true;
        frame_dirty_ = true;
    }
    void set_focus(Widget* w) override;
    [[nodiscard]] Widget* focused() const override { return focus_.current(); }
    TimerHandle add_timer(std::chrono::milliseconds period, std::function<void()> fn,
                          Widget* owner) override;
    void schedule_destroy(Widget* w) override;

private:
    // Layout passes allowed per iteration before giving up on convergence.
    // Well-behaved widgets settle in two: one to learn their width, one to
    // report the height that follows from it.
    static constexpr int kMaxLayoutPasses = 8;

    struct Shortcut {
        KeyEvent match;
        std::function<void()> fn;
    };

    struct TimerEntry {
        std::chrono::steady_clock::time_point deadline;
        std::chrono::milliseconds period;
        std::function<void()> fn;
        std::shared_ptr<detail::TimerControl> ctl;
        Widget* owner;
    };

    void run_layout();
    void render();
    void update_cursor();

    void dispatch(const Event& ev);
    void dispatch_key(const KeyEvent& ev);
    void dispatch_mouse(const MouseEvent& ev);
    void dispatch_resize(const ResizeEvent& ev);

    void paint_widget(Widget* w, Rect clip);
    [[nodiscard]] static Widget* hit_test(Widget* w, Point p);
    [[nodiscard]] static bool in_tree(Widget* root, Widget* needle);

    [[nodiscard]] std::optional<std::chrono::milliseconds> time_until_next_timer() const;
    void fire_due_timers();
    void reap();

    TerminalIO* term_;
    ScreenBuffer* buffer_;
    Widget* root_ = nullptr;
    FocusChain focus_;

    std::vector<Shortcut> shortcuts_;
    std::vector<TimerEntry> timers_;  // min-heap by deadline
    std::vector<Widget*> destroy_list_;
    std::uint64_t next_timer_id_ = 1;

    bool layout_invalid_ = true;
    bool frame_dirty_ = true;
    bool quitting_ = false;
    int exit_code_ = 0;
    int frames_ = 0;
};

struct AppInfo {
    std::string name;
    std::string version;
    std::string description;
};

// Composes the whole thing. No static members: two Apps in one process are
// possible with mock terminals and rejected by the curses backend itself,
// which throws if a curses terminal already exists.
//
// Construction order is contractual: the terminal is created here, explicitly,
// on the main thread, BEFORE any widget exists. Nothing is lazily initialised
// anywhere in this library (lesson 1).
class App {
public:
    explicit App(AppInfo info = {});

    // The argument-parsing overload. The ORDER here is contractual, not
    // incidental: args.parse() runs first, and the terminal is constructed
    // only if parsing neither failed nor asked to exit. That is what
    // guarantees --help, --version and usage errors reach a normal stdout
    // instead of being painted into a curses screen that is then torn down.
    //
    //   ArgParser args{"notepad", "0.1.0", "a tiny editor"};
    //   auto& file = args.positional<std::string>("file", "file to open");
    //   App app{argc, argv, args, {"notepad", "0.1.0"}};
    //   if (app.should_exit()) return app.exit_code();
    //   ... build the UI, using file.value_or("") ...
    //   return app.run();
    App(int argc, char** argv, ArgParser& args, AppInfo info = {});

    // Takes an explicit backend - a MockTerminal in tests, or a custom one.
    explicit App(std::unique_ptr<TerminalIO> term, AppInfo info = {});
    ~App();

    // True when the command line asked the program to stop before it ever
    // starts: --help, --version, or a usage error. No terminal exists in that
    // case, and make_root/loop/terminal/buffer all throw rather than hand back
    // something unusable.
    [[nodiscard]] bool should_exit() const { return exit_early_; }
    [[nodiscard]] int exit_code() const { return exit_code_; }
    // The text already written to stdout (help/version) or stderr (an error).
    [[nodiscard]] const std::string& exit_message() const { return exit_message_; }
    [[nodiscard]] const ParseResult& parse_result() const { return parse_result_; }

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    template <typename W, typename... Args>
    W& make_root(Args&&... args) {
        require_terminal();
        auto owned = std::make_unique<W>(std::forward<Args>(args)...);
        W& ref = *owned;
        root_ = std::move(owned);
        loop_->adopt_root(*root_);
        return ref;
    }

    [[nodiscard]] Widget* root() const { return root_.get(); }

    void add_shortcut(KeyEvent match, std::function<void()> fn) {
        require_terminal();
        loop_->add_shortcut(match, std::move(fn));
    }

    [[nodiscard]] Palette& palette() { return palette_; }
    [[nodiscard]] EventLoop& loop() {
        require_terminal();
        return *loop_;
    }
    [[nodiscard]] TerminalIO& terminal() {
        require_terminal();
        return *term_;
    }
    [[nodiscard]] ScreenBuffer& buffer() {
        require_terminal();
        return buffer_;
    }
    [[nodiscard]] const AppInfo& info() const { return info_; }

    // Returns the parse exit code immediately when the command line already
    // decided the program should stop.
    int run() { return exit_early_ ? exit_code_ : loop_->run(); }
    void quit(int code = 0) {
        if (!exit_early_) loop_->quit(code);
    }
    bool pump_once() { return exit_early_ ? false : loop_->pump_once(); }

private:
    void bootstrap();
    void require_terminal() const;

    AppInfo info_;
    std::unique_ptr<TerminalIO> term_;
    ScreenBuffer buffer_;
    Palette palette_;
    std::unique_ptr<EventLoop> loop_;
    std::unique_ptr<Widget> root_;

    ParseResult parse_result_;
    std::string exit_message_;
    bool exit_early_ = false;
    int exit_code_ = 0;
};

}  // namespace modcurses
