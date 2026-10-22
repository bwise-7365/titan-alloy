#pragma once
//
// MTetris - the seven tetrominoes, ported from FTetris' shape.h/cpp.
//
// The coordinate convention comes straight across: coords are (x, y) offsets
// from the piece's origin cell, x increasing to the RIGHT and y increasing
// UPWARD, matching Board's row index which is 0 at the bottom.
//
#include "rng.hpp"

namespace mtetris {

// Character codes for the canonical tetrominoes, used as indices into the
// colour table as well as naming the shape. N is "no shape".
enum TCode { N = 0, I, J, L, O, S, T, Z };

inline constexpr char kShapeNames[] = "NIJLOSTZ";

class Shape {
public:
    Shape() { set_shape(N); }
    explicit Shape(TCode p) { set_shape(p); }

    void set_shape(TCode p);
    void set_random_shape(Prng& rng) { set_shape(static_cast<TCode>(1 + rng.below(7))); }

    [[nodiscard]] TCode code() const { return code_; }
    [[nodiscard]] char name() const { return kShapeNames[code_]; }

    [[nodiscard]] int x(int index) const { return coords_[index][0]; }
    [[nodiscard]] int y(int index) const { return coords_[index][1]; }

    [[nodiscard]] int min_x() const;
    [[nodiscard]] int max_x() const;
    [[nodiscard]] int min_y() const;
    [[nodiscard]] int max_y() const;

    [[nodiscard]] Shape lrot() const;  // counter-clockwise
    [[nodiscard]] Shape rrot() const;  // clockwise

private:
    TCode code_ = N;
    int coords_[4][2] = {};
};

}  // namespace mtetris
