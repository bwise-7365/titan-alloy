#include "board.hpp"

#include <algorithm>

namespace mtetris {

Board::Board(int rows, int clms, Prng& rng)
    : rows_(std::clamp(rows, kMinRows, kMaxRows)),
      clms_(std::clamp(clms, kMinClms, kMaxClms)),
      rng_(&rng) {
    frags_.assign(static_cast<std::size_t>(rows_) * static_cast<std::size_t>(clms_), N);
    curr_ = Shape{N};
    next_ = Shape{N};
    reset_current_piece();
}

TCode Board::at(int i, int j) const {
    if (!in_bounds(i, j)) return N;
    return frags_[static_cast<std::size_t>(index(i, j))];
}

bool Board::test_shape(const Shape& s, int i, int j) const {
    // frags_ holds only settled cells, so a collision test is a plain lookup.
    if (j + s.min_x() < 0 || j + s.max_x() >= clms_) return false;
    if (i + s.min_y() < 0 || i + s.max_y() >= rows_) return false;
    for (int k = 0; k < 4; ++k)
        if (frags_[static_cast<std::size_t>(index(i + s.y(k), j + s.x(k)))] != N) return false;
    return true;
}

void Board::place_shape(const Shape& s, int i, int j) {
    for (int k = 0; k < 4; ++k)
        frags_[static_cast<std::size_t>(index(i + s.y(k), j + s.x(k)))] = s.code();
}

bool Board::try_lrot() {
    // The O piece is rotationally symmetric; FTetris skipped the work and so
    // do we, which also stops it drifting inside its 2x2 box.
    if (curr_.code() == O) return true;
    const Shape rotated = curr_.lrot();
    if (!test_shape(rotated, curr_i_, curr_j_)) return false;
    curr_ = rotated;
    return true;
}

bool Board::try_rrot() {
    if (curr_.code() == O) return true;
    const Shape rotated = curr_.rrot();
    if (!test_shape(rotated, curr_i_, curr_j_)) return false;
    curr_ = rotated;
    return true;
}

bool Board::try_lmove() {
    if (!test_shape(curr_, curr_i_, curr_j_ - 1)) return false;
    --curr_j_;
    return true;
}

bool Board::try_rmove() {
    if (!test_shape(curr_, curr_i_, curr_j_ + 1)) return false;
    ++curr_j_;
    return true;
}

bool Board::test_sdrop() const { return test_shape(curr_, curr_i_ - 1, curr_j_); }

bool Board::try_sdrop() {
    if (!test_sdrop()) return false;
    --curr_i_;
    return true;
}

void Board::try_hdrop() {
    while (try_sdrop()) {
    }
}

bool Board::reset_current_piece() {
    while (curr_.code() == N) {
        curr_ = next_;
        curr_i_ = rows_ - 1;  // row 0 is the bottom, so the top row is rows-1
        curr_j_ = clms_ / 2;
        if (random_placement) {
            // Two columns of clearance on the left for the I piece, one on
            // the right for everything else - FTetris' arithmetic exactly.
            curr_j_ = 2 + rng_->below(clms_ - 3);
        }
        next_.set_random_shape(*rng_);
        // One re-roll if the next piece repeats the current one: it makes the
        // sequence feel fairer without making it uniform.
        if (next_.code() == curr_.code()) next_.set_random_shape(*rng_);
    }
    return test_shape(curr_, curr_i_, curr_j_);
}

bool Board::clear_one_line(int i) {
    for (int j = 0; j < clms_; ++j)
        if (frags_[static_cast<std::size_t>(index(i, j))] == N) return false;

    // Slide everything above down one row...
    for (int i2 = i + 1; i2 < rows_; ++i2)
        for (int j = 0; j < clms_; ++j)
            frags_[static_cast<std::size_t>(index(i2 - 1, j))] =
                frags_[static_cast<std::size_t>(index(i2, j))];
    // ...and empty the top row.
    for (int j = 0; j < clms_; ++j) frags_[static_cast<std::size_t>(index(rows_ - 1, j))] = N;
    return true;
}

int Board::clear_lines() {
    int count = 0;
    int i = 0;
    while (i < rows_) {
        // Do not advance after a clear: the row that slid down into i may be
        // full as well.
        if (clear_one_line(i)) {
            ++count;
        } else {
            ++i;
        }
    }
    return count;
}

StepResult Board::step() {
    StepResult result;
    try_sdrop();
    if (test_sdrop()) return result;  // still falling; nothing else to do

    place_shape(curr_, curr_i_, curr_j_);
    curr_ = Shape{N};
    if (!reset_current_piece()) {
        result.game_over = true;
        return result;
    }
    result.lines_cleared = clear_lines();
    return result;
}

}  // namespace mtetris
