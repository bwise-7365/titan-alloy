#pragma once
//
// MTetris - the board model, ported from FTetris' board.h/cpp.
//
// This is a MODEL and nothing else: it knows about rows, columns, fragments
// and a falling shape, and not one thing about how any of it is drawn. That
// is what makes the game rules testable with no terminal in sight, and it
// mirrors how modcurses separates TextBuffer from TextArea.
//
#include <vector>

#include "rng.hpp"
#include "shape.hpp"

namespace mtetris {

// FTetris' board-size limits, taken from the Fl_Counter widgets that set them.
inline constexpr int kMinRows = 16;
inline constexpr int kMaxRows = 32;
inline constexpr int kMinClms = 8;
inline constexpr int kMaxClms = 16;
inline constexpr int kDefaultRows = 24;
inline constexpr int kDefaultClms = 12;

struct StepResult {
    int lines_cleared = 0;
    bool game_over = false;  // the new piece had nowhere to go
};

class Board {
public:
    // `frags` holds the leftover cells of shapes that have stopped falling;
    // the falling shape is never in it. Row 0 is the BOTTOM, column 0 the
    // left - the same orientation FTetris used.
    Board(int rows, int clms, Prng& rng);

    [[nodiscard]] int rows() const { return rows_; }
    [[nodiscard]] int clms() const { return clms_; }

    [[nodiscard]] TCode at(int i, int j) const;
    [[nodiscard]] bool in_bounds(int i, int j) const {
        return i >= 0 && j >= 0 && i < rows_ && j < clms_;
    }

    [[nodiscard]] const Shape& current() const { return curr_; }
    [[nodiscard]] const Shape& next() const { return next_; }
    [[nodiscard]] int current_row() const { return curr_i_; }
    [[nodiscard]] int current_col() const { return curr_j_; }

    // Centred start (clms/2) or random. FTetris' random start leaves two
    // columns clear on the left for the I piece and one on the right.
    bool random_placement = false;

    bool try_lrot();
    bool try_rrot();
    bool try_lmove();
    bool try_rmove();
    [[nodiscard]] bool test_sdrop() const;
    bool try_sdrop();
    void try_hdrop();

    // One gravity tick: drop if possible, otherwise land the piece, start the
    // next one and clear any full lines.
    StepResult step();

    // True if the piece fits; false means game over when starting a new one.
    bool reset_current_piece();
    int clear_lines();

private:
    [[nodiscard]] int index(int i, int j) const { return i * clms_ + j; }
    [[nodiscard]] bool test_shape(const Shape& s, int i, int j) const;
    void place_shape(const Shape& s, int i, int j);
    bool clear_one_line(int i);

    int rows_;
    int clms_;
    Prng* rng_;
    std::vector<TCode> frags_;
    Shape curr_{N};
    Shape next_{N};
    int curr_i_ = 0;
    int curr_j_ = 0;
};

}  // namespace mtetris
