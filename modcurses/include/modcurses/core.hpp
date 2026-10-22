#pragma once
//
// modcurses/core.hpp - geometry, signals, input events, timer handles.
//
// PUBLIC HEADER. It must never include a curses header, directly or
// transitively (TUI_DESIGN section 2, lesson 5: the curses firewall).
// Nothing in modcurses has static state, and nothing here is thread-safe by
// design - the library is single-threaded by contract.
//
#include <algorithm>
#include <chrono>
#include <compare>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace modcurses {

// ---------------------------------------------------------------- geometry
//
// RULE: every coordinate and dimension in this library is a signed int.
// Unsigned geometry is what turned a curses ERR (-1) into SIZE_MAX in
// CPPurses (lesson 2). Backend values are range-checked before they get here.

struct Point {
    int x = 0;
    int y = 0;
    constexpr auto operator<=>(const Point&) const = default;
};

struct Size {
    int width = 0;
    int height = 0;
    constexpr auto operator<=>(const Size&) const = default;
    [[nodiscard]] constexpr bool empty() const { return width <= 0 || height <= 0; }
    [[nodiscard]] constexpr int area() const { return empty() ? 0 : width * height; }
};

struct Rect {
    Point origin;
    Size size;

    constexpr auto operator<=>(const Rect&) const = default;

    [[nodiscard]] constexpr int left() const { return origin.x; }
    [[nodiscard]] constexpr int top() const { return origin.y; }
    // Exclusive edges: a rect at x=2 of width 3 covers columns 2, 3, 4.
    [[nodiscard]] constexpr int right() const { return origin.x + size.width; }
    [[nodiscard]] constexpr int bottom() const { return origin.y + size.height; }

    [[nodiscard]] constexpr bool empty() const { return size.empty(); }

    [[nodiscard]] constexpr bool contains(Point p) const {
        return !empty() && p.x >= left() && p.x < right() && p.y >= top() && p.y < bottom();
    }

    [[nodiscard]] constexpr Rect intersect(const Rect& other) const {
        const int l = left() > other.left() ? left() : other.left();
        const int t = top() > other.top() ? top() : other.top();
        const int r = right() < other.right() ? right() : other.right();
        const int b = bottom() < other.bottom() ? bottom() : other.bottom();
        if (r <= l || b <= t) return {};  // one canonical empty value
        return Rect{{l, t}, {r - l, b - t}};
    }
};

// ------------------------------------------------------------------ signal
//
// Single-threaded, and safe to connect or disconnect from inside an emit.
// Connections may outlive the Signal (they go dead rather than dangle),
// which is what the shared control block is for.

namespace detail {

class SignalBase {
public:
    virtual void disconnect_id(std::uint64_t id) = 0;
    [[nodiscard]] virtual bool has_id(std::uint64_t id) const = 0;

protected:
    ~SignalBase() = default;
};

// Lives as long as any Connection to the signal does. The owner pointer is
// nulled by the Signal's destructor.
struct SignalControl {
    SignalBase* owner = nullptr;
};

}  // namespace detail

class Connection {
public:
    Connection() = default;
    Connection(std::weak_ptr<detail::SignalControl> ctl, std::uint64_t id)
        : ctl_(std::move(ctl)), id_(id) {}

    void disconnect() {
        if (auto c = ctl_.lock(); c && c->owner && id_ != 0) c->owner->disconnect_id(id_);
        ctl_.reset();
        id_ = 0;
    }

    [[nodiscard]] bool connected() const {
        auto c = ctl_.lock();
        return c && c->owner && id_ != 0 && c->owner->has_id(id_);
    }

private:
    std::weak_ptr<detail::SignalControl> ctl_;
    std::uint64_t id_ = 0;
};

// RAII wrapper: disconnects on destruction. Movable, non-copyable.
class ScopedConnection {
public:
    ScopedConnection() = default;
    ScopedConnection(Connection c) : conn_(std::move(c)) {}  // implicit on purpose
    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;
    ScopedConnection(ScopedConnection&& other) noexcept
        : conn_(std::exchange(other.conn_, Connection{})) {}
    ScopedConnection& operator=(ScopedConnection&& other) noexcept {
        if (this != &other) {
            conn_.disconnect();
            conn_ = std::exchange(other.conn_, Connection{});
        }
        return *this;
    }
    ~ScopedConnection() { conn_.disconnect(); }

    void disconnect() { conn_.disconnect(); }
    [[nodiscard]] bool connected() const { return conn_.connected(); }

private:
    Connection conn_;
};

template <typename... Args>
class Signal final : private detail::SignalBase {
public:
    using Slot = std::function<void(Args...)>;

    Signal() : ctl_(std::make_shared<detail::SignalControl>()) { ctl_->owner = this; }
    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

    Signal(Signal&& other) noexcept
        : entries_(std::move(other.entries_)), next_id_(other.next_id_),
          ctl_(std::move(other.ctl_)) {
        if (ctl_) ctl_->owner = this;  // existing Connections now point at us
        other.entries_.clear();
        other.ctl_ = std::make_shared<detail::SignalControl>();
        other.ctl_->owner = &other;
    }

    Signal& operator=(Signal&& other) noexcept {
        if (this != &other) {
            if (ctl_) ctl_->owner = nullptr;
            entries_ = std::move(other.entries_);
            next_id_ = other.next_id_;
            ctl_ = std::move(other.ctl_);
            if (ctl_) ctl_->owner = this;
            other.entries_.clear();
            other.ctl_ = std::make_shared<detail::SignalControl>();
            other.ctl_->owner = &other;
        }
        return *this;
    }

    ~Signal() {
        if (ctl_) ctl_->owner = nullptr;
    }

    [[nodiscard]] Connection connect(Slot s) {
        const std::uint64_t id = next_id_++;
        entries_.push_back(std::make_unique<Entry>(Entry{id, std::move(s)}));
        return Connection{ctl_, id};
    }

    void emit(Args... args) {
        ++emit_depth_;
        // Snapshot the count: slots connected during this emit are not called
        // by it.
        const std::size_t n = entries_.size();
        for (std::size_t i = 0; i < n; ++i) {
            // entries_[i] is re-read every iteration because a slot may call
            // connect() and reallocate the vector mid-walk. That is also why
            // the entries are indirect: with a vector<Entry>, reallocation
            // would destroy the very std::function whose operator() is still
            // running further up this stack. Off-heap slots crash here.
            Entry* e = entries_[i].get();
            if (e->id != 0 && e->slot) e->slot(args...);
        }
        if (--emit_depth_ == 0) compact();
    }

    void operator()(Args... args) { emit(args...); }

    [[nodiscard]] std::size_t slot_count() const {
        std::size_t n = 0;
        for (const auto& e : entries_)
            if (e->id != 0) ++n;
        return n;
    }

    void clear() {
        if (emit_depth_ > 0) {
            for (auto& e : entries_) e->id = 0;  // tombstone; compacted after emit
        } else {
            entries_.clear();
        }
    }

private:
    struct Entry {
        std::uint64_t id;  // 0 == tombstone
        Slot slot;
    };

    void disconnect_id(std::uint64_t id) override {
        for (auto& e : entries_) {
            if (e->id == id) {
                e->id = 0;  // tombstone now, erase later: an emit may be walking us
                if (emit_depth_ == 0) compact();
                return;
            }
        }
    }

    [[nodiscard]] bool has_id(std::uint64_t id) const override {
        for (const auto& e : entries_)
            if (e->id == id) return true;
        return false;
    }

    void compact() {
        entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
                                      [](const std::unique_ptr<Entry>& e) { return e->id == 0; }),
                       entries_.end());
    }

    // Indirect on purpose - see the note in emit(). One allocation per
    // connect (rare); none per emit (the hot path); none at all for the many
    // signals in a widget tree that never get a slot.
    std::vector<std::unique_ptr<Entry>> entries_;
    std::uint64_t next_id_ = 1;
    int emit_depth_ = 0;
    std::shared_ptr<detail::SignalControl> ctl_;
};

// ------------------------------------------------------------------- input

enum class Key : int {
    None = 0,
    Char,  // printable input; the codepoint is in KeyEvent::text
    Enter, Escape, Tab, BackTab, Backspace, Delete, Insert,
    Up, Down, Left, Right, Home, End, PageUp, PageDown,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
};

[[nodiscard]] const char* to_string(Key k);

struct Mods {
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    constexpr auto operator<=>(const Mods&) const = default;
    [[nodiscard]] constexpr bool any() const { return ctrl || alt || shift; }
};

struct KeyEvent {
    Key key = Key::None;
    char32_t text = 0;  // meaningful when key == Key::Char
    Mods mods;
    constexpr auto operator<=>(const KeyEvent&) const = default;
};

// Shorthand for shortcut tables and tests.
[[nodiscard]] constexpr KeyEvent key_ev(Key k, Mods m = {}) { return KeyEvent{k, 0, m}; }
[[nodiscard]] constexpr KeyEvent char_ev(char32_t c, Mods m = {}) { return KeyEvent{Key::Char, c, m}; }
[[nodiscard]] constexpr KeyEvent ctrl_ev(char32_t c) {
    return KeyEvent{Key::Char, c, Mods{true, false, false}};
}
[[nodiscard]] constexpr KeyEvent alt_ev(char32_t c) {
    return KeyEvent{Key::Char, c, Mods{false, true, false}};
}

struct MouseEvent {
    enum class Button { None, Left, Middle, Right, WheelUp, WheelDown };
    enum class Action { Press, Release, DoubleClick, Move };

    Button button = Button::None;
    Action action = Action::Press;
    Point pos;  // screen coords from the backend; local coords at delivery
    Mods mods;
    constexpr auto operator<=>(const MouseEvent&) const = default;
};

struct ResizeEvent {
    Size size;
    constexpr auto operator<=>(const ResizeEvent&) const = default;
};

using Event = std::variant<KeyEvent, MouseEvent, ResizeEvent>;

// ------------------------------------------------------------------ timers
//
// Timers are callbacks owned by their creator, not events - so there is no
// queue and therefore no routing question (lesson 4). The EventLoop (M2)
// owns the heap; this handle is the user-facing RAII grip on one entry.

namespace detail {
struct TimerControl {
    std::uint64_t id = 0;
    bool cancelled = false;
    bool live = false;  // false once the loop has dropped the entry
};
}  // namespace detail

class TimerHandle {
public:
    TimerHandle() = default;
    explicit TimerHandle(std::shared_ptr<detail::TimerControl> ctl) : ctl_(std::move(ctl)) {}
    TimerHandle(const TimerHandle&) = delete;
    TimerHandle& operator=(const TimerHandle&) = delete;
    TimerHandle(TimerHandle&& other) noexcept : ctl_(std::move(other.ctl_)) {}
    TimerHandle& operator=(TimerHandle&& other) noexcept {
        if (this != &other) {
            cancel();
            ctl_ = std::move(other.ctl_);
        }
        return *this;
    }
    ~TimerHandle() { cancel(); }

    void cancel() {
        if (ctl_) ctl_->cancelled = true;
        ctl_.reset();
    }

    [[nodiscard]] bool active() const { return ctl_ && ctl_->live && !ctl_->cancelled; }

    // Give up ownership without cancelling (fire-and-forget timers).
    void release() { ctl_.reset(); }

private:
    std::shared_ptr<detail::TimerControl> ctl_;
};

}  // namespace modcurses
