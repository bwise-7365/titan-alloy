//
// MTetris - FTetris ported from FLTK to modcurses.
//
// "Because the game was invented in 1984 at the Soviet Academy of Sciences in
// Moscow, it is worth knowing how to write Tetris with proper Cyrillic
// letters: Тетрис."  -- FTetris' README, and the reason the title bar says
// what it says. (Every Cyrillic letter here is in the BMP, so PDCurses'
// 16-bit character cell renders them; see BUILD_NOTES on the Windows limit.)
//
// Carried across from FTetris: the five piece schemes and three backgrounds
// with their exact RGB values, the centred-or-random starting column, the
// explanatory dialogs, the board-size and level controls, and the whole key
// map. Added here: run-time control of drop speed and of the PRNG seed, so a
// game can be replayed exactly.
//
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

#include "board.hpp"
#include "modcurses/app.hpp"
#include "modcurses/args.hpp"
#include "modcurses/text.hpp"
#include "modcurses/utf8.hpp"
#include "modcurses/widgets.hpp"
#include "rng.hpp"
#include "scheme.hpp"

using namespace modcurses;
namespace mt = mtetris;

namespace {

// ------------------------------------------------------------------ colours

// Which terminal colour each scheme colour ends up in. Two ways to get there:
// redefine the palette slots (exact), or pick the nearest standard colour
// (approximate). Everything downstream just reads this.
struct ColorMap {
    Color piece[8]{};
    Color even_column = Color::Black;
    Color odd_column = Color::BrightBlack;
};

ColorMap build_color_map(const mt::Scheme& s, Palette& palette, bool exact_colors) {
    // The SLOTS are the mechanism. Each scheme lands on a different set of
    // them, so the cell attributes change and the board repaints in different
    // colours on every terminal.
    const mt::SlotMap slots = mt::assign_slots(s);

    ColorMap map;
    for (int p = mt::I; p <= mt::Z; ++p)
        map.piece[p] = slots.piece[static_cast<std::size_t>(p)];
    map.even_column = slots.even_column;
    map.odd_column = slots.odd_column;

    // Redefining the RGB behind those slots is a BONUS, and off by default.
    // It reproduces FTetris' exact colours where the terminal honours it, but
    // plenty do not - and the console offers no way to ask, so an application
    // that relies on it silently shows the wrong thing. It also mutates the
    // terminal's own palette, which is not a side effect to have by default.
    if (!exact_colors || !palette.can_redefine()) return map;

    for (int p = mt::I; p <= mt::Z; ++p)
        palette.set(slots.piece[static_cast<std::size_t>(p)],
                    s.piece[static_cast<std::size_t>(p)]);
    palette.set(slots.even_column, s.even_column);
    palette.set(slots.odd_column, s.odd_column);
    return map;
}

// ---------------------------------------------------------------- the views

// The playing field. Two screen columns per cell, so the cells read as square
// rather than as tall thin slots - which is exactly what GridCanvas' cell
// width is for.
class BoardView : public GridCanvas {
public:
    BoardView() : GridCanvas(mt::kDefaultClms, mt::kDefaultRows, 2) {}

    void render(const mt::Board& board, const ColorMap& map) {
        if (columns() != board.clms() || rows() != board.rows())
            resize_grid(board.clms(), board.rows());

        // Row 0 of the model is the BOTTOM; row 0 of the grid is the top.
        const auto to_grid_y = [&](int i) { return board.rows() - 1 - i; };

        for (int i = 0; i < board.rows(); ++i) {
            for (int j = 0; j < board.clms(); ++j) {
                const mt::TCode p = board.at(i, j);
                const Color c = p == mt::N ? (j % 2 == 0 ? map.even_column : map.odd_column)
                                           : map.piece[p];
                set_cell(j, to_grid_y(i), Glyph{U' ', bg(c)});
            }
        }

        const mt::Shape& s = board.current();
        if (s.code() == mt::N) return;
        for (int k = 0; k < 4; ++k) {
            const int i = board.current_row() + s.y(k);
            const int j = board.current_col() + s.x(k);
            if (board.in_bounds(i, j)) set_cell(j, to_grid_y(i), Glyph{U' ', bg(map.piece[s.code()])});
        }
    }
};

// The next-piece preview: a fixed 4x4 window with the shape centred in it.
class PreviewView : public GridCanvas {
public:
    PreviewView() : GridCanvas(4, 4, 2) {}

    void render(const mt::Shape& s, const ColorMap& map) {
        fill_grid(Glyph{U' ', bg(map.even_column)});
        if (s.code() == mt::N) return;
        // Centre the piece's bounding box inside the 4x4 window.
        const int ox = 1 - s.min_x() - (s.max_x() - s.min_x()) / 2;
        const int oy = 2 + s.min_y() + (s.max_y() - s.min_y()) / 2;
        for (int k = 0; k < 4; ++k)
            set_cell(ox + s.x(k), oy - s.y(k), Glyph{U' ', bg(map.piece[s.code()])});
    }
};

std::string mmss(double seconds) {
    if (seconds < 0) seconds = 0;
    const int total = static_cast<int>(seconds);
    char buf[32];
    std::snprintf(buf, sizeof buf, "%d:%02d", total / 60, total % 60);
    return buf;
}

std::string hex64(std::uint64_t v) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "0x%016llX", static_cast<unsigned long long>(v));
    return buf;
}

// FTetris' scoring table, unchanged.
int score_for(int lines) {
    switch (lines) {
        case 0: return 0;
        case 1: return 100;
        case 2: return 250;
        case 3: return 475;
        case 4: return 800;
        default: return lines * 250;  // if chain-reaction clearing ever lands
    }
}

const char* kAboutText =
    "MTetris is a classic Тетрис game.\n"
    "\n"
    "It is a port of FTetris, which was implemented with FLTK as a testbed\n"
    "for FLCPP2. This version runs on modcurses instead. The original code\n"
    "is copyright Ben Wise, all rights reserved.\n"
    "\n"
    "The game was invented in 1984 at the Soviet Academy of Sciences in\n"
    "Moscow, which is why the title is spelled with Cyrillic letters.\n"
    "\n"
    "PLAYING\n"
    "  left:            4, left arrow\n"
    "  right:           6, right arrow\n"
    "  hard drop:       8, up arrow, space bar\n"
    "  soft drop:       2, down arrow\n"
    "  rotate R (CW):   3, 9, x\n"
    "  rotate L (CCW):  1, 7, z\n"
    "\n"
    "  pause/resume:        5\n"
    "  pause:               p\n"
    "  resume with delay:   r\n"
    "  new game (same seed):    n\n"
    "  new game (fresh seed):   N\n"
    "  quit:                q\n"
    "\n"
    "SETTINGS (all take effect immediately)\n"
    "  drop speed / level:   - and +      (1 slowest, 10 fastest)\n"
    "  background scheme:    b            (Beige, Black, White)\n"
    "  piece scheme:         c            (Game Boy, Gerasimov, Sega,\n"
    "                                      Soviet Mind Game, Tetris Company)\n"
    "  game length:          t            (1, 5 or 10 minutes)\n"
    "  start position:       s            (centred or random column)\n"
    "  board rows:           [ and ]      (16 to 32)\n"
    "  board columns:        , and .      (8 to 16; rows snap to 2x columns)\n"
    "\n"
    "  this dialog:          a or F1      dismiss: d, Escape or Enter\n"
    "\n"
    "The PRNG seed is shown in the sidebar. Passing it back with --seed\n"
    "replays exactly the same sequence of pieces and starting columns.\n";

}  // namespace

// ----------------------------------------------------------------- the game

namespace {

enum class Page { Game = 0, About = 1, GameOver = 2 };

class Game : public VBox {
public:
    Game(App& app, mt::Prng& rng) : app_(&app), rng_(&rng) {}

    // Everything below is wired up by build(), which runs once the widget is
    // attached to the loop (so add_timer has a context to talk to).
    void build();

    // ---- settings, all changeable at run time ----
    int level = 3;                       // 1 slowest, 10 fastest
    int rows = mt::kDefaultRows;
    int clms = mt::kDefaultClms;
    double max_play_time = 5 * 60.0;
    bool random_placement = false;
    bool dialogs_enabled = true;
    bool exact_colors = false;  // redefine the terminal's palette; see build_color_map
    mt::Background background = mt::Background::Black;
    mt::Pieces pieces = mt::Pieces::Sega;
    std::uint64_t seed = 0;

protected:
    bool on_key(const KeyEvent& ev) override;

private:
    void new_game(bool fresh_seed);
    void apply_scheme();
    void rearm_timer();
    void tick();
    void refresh_sidebar();
    void set_page(Page p);
    void pause();
    void resume(double delay_seconds);
    void set_level(int lvl);
    void set_board(int r, int c);
    [[nodiscard]] double step_seconds() const;

    App* app_;
    mt::Prng* rng_;
    std::unique_ptr<mt::Board> board_;
    ColorMap colors_;

    Stack* pages_ = nullptr;
    BoardView* board_view_ = nullptr;
    PreviewView* preview_ = nullptr;
    Titlebar* title_ = nullptr;
    StatusBar* status_ = nullptr;
    Label* score_label_ = nullptr;
    Label* lines_label_ = nullptr;
    Label* level_label_ = nullptr;
    Label* time_label_ = nullptr;
    Label* size_label_ = nullptr;
    Label* start_label_ = nullptr;
    Label* bg_label_ = nullptr;
    Label* pc_label_ = nullptr;
    Label* seed_label_ = nullptr;
    Label* over_label_ = nullptr;
    TextArea* about_text_ = nullptr;

    TextBuffer about_buffer_;
    ScopedConnection about_scroll_;
    TimerHandle gravity_;
    TimerHandle resume_timer_;

    double play_time = 0.0;
    int score_ = 0;
    int line_count_ = 0;
    bool paused_ = true;
    bool was_running_ = false;  // play state to restore when a dialog closes
};

void Game::build() {
    title_ = &emplace_child<Titlebar>("Тетрис", "MTetris");

    pages_ = &emplace_child<Stack>();

    // ---- page 0: the game ------------------------------------------------
    auto& game_page = pages_->emplace_child<HBox>();
    board_view_ = &game_page.emplace_child<BoardView>();
    game_page.emplace_child<Divider>(Divider::Orientation::Vertical);

    auto& sidebar = game_page.emplace_child<VBox>();
    sidebar.width_hint = SizeReq::fixed(26);
    sidebar.emplace_child<Label>(" Next");
    auto& preview_row = sidebar.emplace_child<HBox>();
    preview_row.height_hint = SizeReq::fixed(4);
    preview_row.emplace_child<Widget>().width_hint = SizeReq::fixed(1);
    preview_ = &preview_row.emplace_child<PreviewView>();
    preview_row.emplace_child<Widget>();

    sidebar.emplace_child<Divider>();
    score_label_ = &sidebar.emplace_child<Label>();
    lines_label_ = &sidebar.emplace_child<Label>();
    level_label_ = &sidebar.emplace_child<Label>();
    time_label_ = &sidebar.emplace_child<Label>();
    sidebar.emplace_child<Divider>();
    size_label_ = &sidebar.emplace_child<Label>();
    start_label_ = &sidebar.emplace_child<Label>();
    bg_label_ = &sidebar.emplace_child<Label>();
    pc_label_ = &sidebar.emplace_child<Label>();
    sidebar.emplace_child<Divider>();
    seed_label_ = &sidebar.emplace_child<Label>();
    sidebar.emplace_child<Widget>();  // spacer

    // ---- page 1: the About dialog ---------------------------------------
    auto& about_page = pages_->emplace_child<VBox>();
    about_page.emplace_child<Label>("About MTetris", Align::Center);
    about_page.emplace_child<Divider>();
    about_buffer_.set_text(utf8_decode(kAboutText));
    about_buffer_.clear_dirty();
    auto& about_body = about_page.emplace_child<HBox>();
    about_text_ = &about_body.emplace_child<TextArea>(about_buffer_);
    TextArea& about_text = *about_text_;
    about_text.read_only = true;
    about_text.capture_tab = false;  // a pager, not an editor
    auto& about_bar = about_body.emplace_child<ScrollBar>();
    // Held as a member: both widgets are owned by this tree, so the
    // connection must die with it rather than at the end of build().
    about_scroll_ = about_text.scrolled.connect([&about_bar, &about_text](int top, int total) {
        about_bar.set_range(total, about_text.visible_lines());
        about_bar.set_position(top);
    });

    // ---- page 2: the Game Over notice -----------------------------------
    auto& over_page = pages_->emplace_child<VBox>();
    over_page.emplace_child<Widget>();
    over_page.emplace_child<Label>("GAME OVER", Align::Center).style =
        Style{}.with(Trait::Bold).with(Trait::Reverse);
    over_label_ = &over_page.emplace_child<Label>("", Align::Center);
    over_page.emplace_child<Label>("n: new game (same seed)   N: fresh seed   q: quit",
                                   Align::Center);
    over_page.emplace_child<Widget>();

    status_ = &emplace_child<StatusBar>(
        "5 pause/resume   a help   n new game   q quit");

    apply_scheme();
    set_board(rows, clms);
    set_page(Page::Game);
    if (dialogs_enabled) set_page(Page::About);
    rearm_timer();
    refresh_sidebar();
}

double Game::step_seconds() const {
    // FTetris' timing, transcribed. At level 1 a piece takes ~15 s to fall the
    // height of the board; at level 10, ~1 s; the speed-up is geometric so
    // each level is the same percentage faster than the last.
    const double t_slow = 15.0;
    const double t_fast = 1.0;
    const double s0 = t_slow / static_cast<double>(board_ ? board_->rows() : rows);
    const double a = std::exp(std::log(t_fast / t_slow) / 9.0);
    const double acc = std::exp(level * std::log(a));
    return s0 * (1.0 / a) * acc;
}

void Game::rearm_timer() {
    gravity_.cancel();
    const auto period =
        std::chrono::milliseconds{std::max<long long>(10, static_cast<long long>(step_seconds() * 1000.0))};
    gravity_ = add_timer(period, [this] { tick(); });
}

void Game::tick() {
    if (paused_) return;

    const mt::StepResult r = board_->step();
    line_count_ += r.lines_cleared;
    score_ += score_for(r.lines_cleared);
    play_time += step_seconds();

    const bool out_of_time = play_time >= max_play_time;
    if (r.game_over || out_of_time) {
        pause();
        if (over_label_)
            over_label_->set_text("Score " + std::to_string(score_) + "   Lines " +
                                  std::to_string(line_count_) +
                                  (out_of_time ? "   (time expired)" : "   (board full)"));
        if (dialogs_enabled) {
            set_page(Page::GameOver);
        } else {
            new_game(false);
        }
        return;
    }
    board_view_->render(*board_, colors_);
    refresh_sidebar();
}

void Game::apply_scheme() {
    colors_ = build_color_map(mt::make_scheme(background, pieces), app_->palette(), exact_colors);
    if (board_) board_view_->render(*board_, colors_);
    if (board_) preview_->render(board_->next(), colors_);
    // With --exact-colors, only the RGB behind a slot changed and no cell
    // differs, so the diff would write nothing. Force the frame out.
    app_->buffer().force_full_redraw();
    invalidate();
}

void Game::set_board(int r, int c) {
    rows = std::clamp(r, mt::kMinRows, mt::kMaxRows);
    clms = std::clamp(c, mt::kMinClms, mt::kMaxClms);
    board_ = std::make_unique<mt::Board>(rows, clms, *rng_);
    board_->random_placement = random_placement;
    play_time = 0.0;
    score_ = 0;
    line_count_ = 0;
    board_view_->render(*board_, colors_);
    preview_->render(board_->next(), colors_);
    rearm_timer();
    refresh_sidebar();
    invalidate_layout();
}

void Game::new_game(bool fresh_seed) {
    if (fresh_seed) seed = mt::Prng::random_seed();
    // Restarting the stream is what makes "same seed, same game" true. FTetris
    // let the stream run on between games; replaying exactly was the point of
    // asking for seed control, so here a new game rewinds it.
    *rng_ = mt::Prng{seed};
    set_board(rows, clms);
    set_page(Page::Game);
    paused_ = true;
    status_->flash("New game - press 5 to start", std::chrono::seconds{3});
}

void Game::set_level(int lvl) {
    level = std::clamp(lvl, 1, 10);
    rearm_timer();
    refresh_sidebar();
    status_->flash("Level " + std::to_string(level) + " - " +
                       mmss(step_seconds() * board_->rows()) + " to fall",
                   std::chrono::seconds{2});
}

void Game::pause() {
    paused_ = true;
    resume_timer_.cancel();
    refresh_sidebar();
}

void Game::resume(double delay_seconds) {
    resume_timer_.cancel();
    if (delay_seconds <= 0.01) {
        paused_ = false;
        refresh_sidebar();
        return;
    }
    status_->flash("Resuming...", std::chrono::milliseconds{
                                      static_cast<long long>(delay_seconds * 1000)});
    // A one-shot built from the repeating primitive: cancel on first fire.
    resume_timer_ = add_timer(
        std::chrono::milliseconds{static_cast<long long>(delay_seconds * 1000)}, [this] {
            paused_ = false;
            resume_timer_.cancel();
            refresh_sidebar();
        });
}

void Game::set_page(Page p) {
    const auto previous = static_cast<Page>(pages_->active_index());
    if (p == previous) return;

    if (p != Page::Game) {
        // Remember whether we were actually playing, so dismissing a dialog
        // puts things back rather than silently leaving the game stopped -
        // which reads exactly like a freeze.
        if (previous == Page::Game) was_running_ = !paused_;
        pause();
    }
    pages_->set_active(static_cast<int>(p));

    if (p == Page::About && about_text_ != nullptr) {
        // Give the pager the keyboard so arrows and PageUp/PageDown scroll it.
        // Anything it will not act on now bubbles back here, so 'd' and 'q'
        // still work; and the loop releases this focus as soon as the page is
        // hidden again.
        about_text_->take_focus();
    }
    if (p == Page::Game && was_running_) {
        was_running_ = false;
        resume(0.001);
    }
    refresh_sidebar();
}

void Game::refresh_sidebar() {
    score_label_->set_text(" Score   " + std::to_string(score_));
    lines_label_->set_text(" Lines   " + std::to_string(line_count_));
    level_label_->set_text(" Level   " + std::to_string(level) + (paused_ ? "  [PAUSED]" : ""));
    time_label_->set_text(" Time    " + mmss(play_time) + " / " + mmss(max_play_time));
    size_label_->set_text(" Board   " + std::to_string(rows) + " x " + std::to_string(clms));
    start_label_->set_text(std::string{" Start   "} + (random_placement ? "Random" : "Centred"));
    bg_label_->set_text(std::string{" Back    "} + mt::name_of(background));
    pc_label_->set_text(std::string{" Pieces  "} + mt::name_of(pieces));
    seed_label_->set_text(" Seed " + hex64(seed));
    if (preview_ && board_) preview_->render(board_->next(), colors_);
    invalidate();
}

bool Game::on_key(const KeyEvent& ev) {
    const auto page = static_cast<Page>(pages_->active_index());
    const bool playing = page == Page::Game;

    // Dialogs swallow the game keys, so a stray keypress cannot move a piece
    // you cannot see.
    if (!playing) {
        if (ev.key == Key::Escape || ev.key == Key::Enter ||
            (ev.key == Key::Char && (ev.text == U'd' || ev.text == U'a'))) {
            set_page(Page::Game);
            return true;
        }
        if (ev.key == Key::Char && ev.text == U'n') {
            new_game(false);
            return true;
        }
        if (ev.key == Key::Char && ev.text == U'N') {
            new_game(true);
            return true;
        }
        if (ev.key == Key::Char && ev.text == U'q') {
            app_->quit(0);
            return true;
        }
        return false;  // let the About page's TextArea scroll
    }

    const auto act = [&](void (mt::Board::*fn)()) {
        if (!paused_) {
            (board_.get()->*fn)();
            board_view_->render(*board_, colors_);
            invalidate();
        }
    };
    const auto act_try = [&](bool (mt::Board::*fn)()) {
        if (!paused_) {
            (board_.get()->*fn)();
            board_view_->render(*board_, colors_);
            invalidate();
        }
    };

    // FTetris' key map, transcribed. The digits are the numeric keypad in
    // spirit: 4/6 left/right, 2/8 down/drop, 1/7 and 3/9 the two rotations.
    switch (ev.key) {
        case Key::Left: act_try(&mt::Board::try_lmove); return true;
        case Key::Right: act_try(&mt::Board::try_rmove); return true;
        case Key::Down: act_try(&mt::Board::try_sdrop); return true;
        case Key::Up: act(&mt::Board::try_hdrop); return true;
        case Key::F1: set_page(Page::About); return true;
        default: break;
    }
    if (ev.key != Key::Char) return false;

    switch (ev.text) {
        case U'4': act_try(&mt::Board::try_lmove); return true;
        case U'6': act_try(&mt::Board::try_rmove); return true;
        case U'2': act_try(&mt::Board::try_sdrop); return true;
        case U' ':
        case U'8': act(&mt::Board::try_hdrop); return true;
        case U'1':
        case U'7':
        case U'z': act_try(&mt::Board::try_lrot); return true;
        case U'3':
        case U'9':
        case U'x': act_try(&mt::Board::try_rrot); return true;

        case U'5':
            paused_ ? resume(0.001) : pause();
            return true;
        case U'p': pause(); return true;
        case U'r':
            if (paused_) resume(1.0);
            return true;
        case U'n': new_game(false); return true;
        case U'N': new_game(true); return true;
        case U'q': app_->quit(0); return true;
        case U'a': set_page(Page::About); return true;

        // ---- run-time settings ----
        case U'-': set_level(level - 1); return true;
        case U'+':
        case U'=': set_level(level + 1); return true;
        case U'b':
            background = static_cast<mt::Background>((static_cast<int>(background) + 1) %
                                                     mt::kBackgroundCount);
            apply_scheme();
            refresh_sidebar();
            status_->flash(std::string{"Background: "} + mt::name_of(background),
                           std::chrono::seconds{2});
            return true;
        case U'c':
            pieces = static_cast<mt::Pieces>((static_cast<int>(pieces) + 1) % mt::kPiecesCount);
            apply_scheme();
            refresh_sidebar();
            status_->flash(std::string{"Pieces: "} + mt::name_of(pieces),
                           std::chrono::seconds{2});
            return true;
        case U't':
            max_play_time = max_play_time <= 60.5    ? 5 * 60.0
                            : max_play_time <= 300.5 ? 10 * 60.0
                                                     : 60.0;
            refresh_sidebar();
            status_->flash("Game length: " + mmss(max_play_time), std::chrono::seconds{2});
            return true;
        case U's':
            random_placement = !random_placement;
            board_->random_placement = random_placement;
            refresh_sidebar();
            status_->flash(std::string{"Start position: "} +
                               (random_placement ? "Random" : "Centred"),
                           std::chrono::seconds{2});
            return true;
        case U'[': set_board(rows - 1, clms); return true;
        case U']': set_board(rows + 1, clms); return true;
        // FTetris snapped rows to twice the column count when the column
        // counter moved, keeping the field's aspect. Preserved.
        case U',': set_board(2 * (clms - 1), clms - 1); return true;
        case U'.': set_board(2 * (clms + 1), clms + 1); return true;
        default: break;
    }
    return false;
}

}  // namespace

// ---------------------------------------------------------------------- main

int main(int argc, char** argv) {
    try {
        ArgParser args{"mtetris", "1.0", "Тетрис - FTetris ported to modcurses"};

        auto& seed_arg = args.option<std::string>('S', "seed", "PRNG seed; 0 picks a random one")
                             .default_value("0")
                             .metavar("N");
        auto& level_arg =
            args.option<int>('l', "level", "drop speed, 1 slowest to 10 fastest").default_value(3);
        auto& rows_arg = args.option<int>("rows", "board rows").default_value(mt::kDefaultRows);
        auto& clms_arg = args.option<int>("clms", "board columns; rows default to twice this")
                             .default_value(mt::kDefaultClms);
        auto& time_arg =
            args.option<int>('t', "minutes", "game length: 1, 5 or 10").default_value(5);
        auto& bg_arg = args.option<std::string>('b', "background", "beige, black or white")
                           .default_value("black");
        auto& pc_arg = args.option<std::string>('c', "pieces",
                                                "gameboy, gerasimov, sega, soviet or tetrisco")
                           .default_value("sega");
        auto& random_arg = args.flag('r', "random", "random starting column instead of centred");
        auto& nodialog_arg = args.flag("no-dialogs", "skip the explanatory dialogs");
        auto& exact_arg =
            args.flag('x', "exact-colors",
                      "redefine the terminal palette for FTetris' exact RGB "
                      "(many terminals ignore this)");

        level_arg.validate([](const int& v, std::string& why) {
            if (v >= 1 && v <= 10) return true;
            why = "must be 1 to 10, got " + std::to_string(v);
            return false;
        });
        rows_arg.validate([](const int& v, std::string& why) {
            if (v >= mt::kMinRows && v <= mt::kMaxRows) return true;
            why = "must be 16 to 32, got " + std::to_string(v);
            return false;
        });
        clms_arg.validate([](const int& v, std::string& why) {
            if (v >= mt::kMinClms && v <= mt::kMaxClms) return true;
            why = "must be 8 to 16, got " + std::to_string(v);
            return false;
        });
        time_arg.validate([](const int& v, std::string& why) {
            if (v == 1 || v == 5 || v == 10) return true;
            why = "must be 1, 5 or 10, got " + std::to_string(v);
            return false;
        });

        std::uint64_t seed_value = 0;
        seed_arg.validate([&seed_value](const std::string& text, std::string& why) {
            // Accepts decimal or 0x-prefixed hex, so the value printed in the
            // sidebar can be pasted straight back in.
            std::string_view sv{text};
            int base = 10;
            if (sv.size() > 2 && sv[0] == '0' && (sv[1] == 'x' || sv[1] == 'X')) {
                base = 16;
                sv.remove_prefix(2);
            }
            std::uint64_t parsed = 0;
            const auto* end = sv.data() + sv.size();
            const auto res = std::from_chars(sv.data(), end, parsed, base);
            if (res.ec != std::errc{} || res.ptr != end) {
                why = "expected a 64-bit number, got '" + text + "'";
                return false;
            }
            seed_value = parsed;
            return true;
        });

        mt::Background background = mt::Background::Black;
        bg_arg.validate([&background](const std::string& text, std::string& why) {
            if (text == "beige") background = mt::Background::Beige;
            else if (text == "black") background = mt::Background::Black;
            else if (text == "white") background = mt::Background::White;
            else {
                why = "expected beige, black or white, got '" + text + "'";
                return false;
            }
            return true;
        });

        mt::Pieces pieces = mt::Pieces::Sega;
        pc_arg.validate([&pieces](const std::string& text, std::string& why) {
            if (text == "gameboy") pieces = mt::Pieces::GameBoy;
            else if (text == "gerasimov") pieces = mt::Pieces::Gerasimov;
            else if (text == "sega") pieces = mt::Pieces::Sega;
            else if (text == "soviet") pieces = mt::Pieces::SovietMindGame;
            else if (text == "tetrisco") pieces = mt::Pieces::TetrisCompany;
            else {
                why = "expected gameboy, gerasimov, sega, soviet or tetrisco, got '" + text + "'";
                return false;
            }
            return true;
        });

        // Defaults never run through validate(), so seed the derived values
        // the same way the validators would.
        App app{argc, argv, args, AppInfo{"mtetris", "1.0", "Тетрис on modcurses"}};
        if (app.should_exit()) return app.exit_code();

        // FTetris' convention: a seed of 0 means "pick a real one", and the
        // value chosen is reported so the game can be replayed.
        if (seed_value == 0) seed_value = mt::Prng::random_seed();
        mt::Prng rng{seed_value};

        auto& game = app.make_root<Game>(app, rng);
        game.seed = seed_value;
        game.level = level_arg.value();
        game.clms = clms_arg.value();
        // Matching the FLTK counters: moving the column counter snapped rows
        // to twice the columns, while the row counter left columns alone.
        game.rows = rows_arg.present() || !clms_arg.present() ? rows_arg.value()
                                                              : 2 * clms_arg.value();
        game.max_play_time = time_arg.value() * 60.0;
        game.random_placement = random_arg.value();
        game.dialogs_enabled = !nodialog_arg.value();
        game.exact_colors = exact_arg.value();
        game.background = background;
        game.pieces = pieces;
        game.build();

        return app.run();
    } catch (const TerminalError& e) {
        std::fprintf(stderr, "mtetris: %s\n", e.what());
        return 1;
    }
}
