#include "shape.hpp"

#include <algorithm>

namespace mtetris {

void Shape::set_shape(TCode p) {
    // Transcribed unchanged from FTetris, so the pieces have exactly the same
    // geometry and therefore the same feel.
    static constexpr int kCoords[8][4][2] = {
        {{0, 0}, {0, 0},   {0, 0},  {0, 0}},    // N, occupying just one cell
        {{0, 0}, {-1, 0},  {-2, 0}, {1, 0}},    // I
        {{0, 0}, {-1, 0},  {1, 0},  {1, -1}},   // J
        {{0, 0}, {-1, 0},  {1, 0},  {-1, -1}},  // L
        {{0, 0}, {-1, 0},  {0, -1}, {-1, -1}},  // O
        {{0, 0}, {0, -1},  {1, 0},  {-1, -1}},  // S
        {{0, 0}, {-1, 0},  {1, 0},  {0, -1}},   // T
        {{0, 0}, {0, -1},  {-1, 0}, {1, -1}},   // Z
    };
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 2; ++j) coords_[i][j] = kCoords[p][i][j];
    code_ = p;
}

int Shape::min_x() const {
    return std::min({coords_[0][0], coords_[1][0], coords_[2][0], coords_[3][0]});
}
int Shape::max_x() const {
    return std::max({coords_[0][0], coords_[1][0], coords_[2][0], coords_[3][0]});
}
int Shape::min_y() const {
    return std::min({coords_[0][1], coords_[1][1], coords_[2][1], coords_[3][1]});
}
int Shape::max_y() const {
    return std::max({coords_[0][1], coords_[1][1], coords_[2][1], coords_[3][1]});
}

Shape Shape::lrot() const {
    Shape out;
    out.code_ = code_;  // rotation never changes which tetromino this is
    for (int i = 0; i < 4; ++i) {
        out.coords_[i][0] = -coords_[i][1];
        out.coords_[i][1] = coords_[i][0];
    }
    return out;
}

Shape Shape::rrot() const {
    Shape out;
    out.code_ = code_;
    for (int i = 0; i < 4; ++i) {
        out.coords_[i][0] = coords_[i][1];
        out.coords_[i][1] = -coords_[i][0];
    }
    return out;
}

}  // namespace mtetris
