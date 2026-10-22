#include <algorithm>

#include "modcurses/widgets.hpp"

namespace modcurses {

GridCanvas::GridCanvas(int columns, int rows, int cell_width) {
    cell_width_ = std::max(1, cell_width);
    resize_grid(columns, rows);
}

void GridCanvas::resize_grid(int columns, int rows) {
    columns_ = std::max(0, columns);
    rows_ = std::max(0, rows);
    cells_.assign(static_cast<std::size_t>(columns_) * static_cast<std::size_t>(rows_),
                  Glyph{U' ', style});
    invalidate_layout();
}

void GridCanvas::set_cell_width(int width) {
    width = std::max(1, width);
    if (width == cell_width_) return;
    cell_width_ = width;
    invalidate_layout();
}

void GridCanvas::set_cell(int x, int y, Glyph g) {
    if (!in_grid(x, y)) return;  // out-of-grid writes are dropped, not clamped
    Glyph& target = cells_[index(x, y)];
    if (target == g) return;
    target = g;
    invalidate();
}

Glyph GridCanvas::cell(int x, int y) const {
    if (!in_grid(x, y)) return outside_;
    return cells_[index(x, y)];
}

void GridCanvas::fill_grid(Glyph g) {
    if (std::all_of(cells_.begin(), cells_.end(), [&](const Glyph& c) { return c == g; })) return;
    cells_.assign(cells_.size(), g);
    invalidate();
}

std::optional<Point> GridCanvas::cell_at(Point local) const {
    if (local.x < 0 || local.y < 0) return std::nullopt;
    const int x = local.x / cell_width_;
    const int y = local.y;
    if (!in_grid(x, y)) return std::nullopt;
    return Point{x, y};
}

SizeReq GridCanvas::width_req() const { return SizeReq::fixed(columns_ * cell_width_); }
SizeReq GridCanvas::height_req() const { return SizeReq::fixed(rows_); }

void GridCanvas::paint(Canvas& c) {
    c.fill(Glyph{U' ', style});
    for (int y = 0; y < rows_ && y < c.size().height; ++y) {
        for (int x = 0; x < columns_; ++x) {
            const Glyph g = cells_[index(x, y)];
            const int sx = x * cell_width_;
            if (sx >= c.size().width) break;
            // A cell wider than one column is painted as the glyph followed by
            // blanks in its own style, which is what makes 2x1 cells read as
            // square blocks rather than as stretched characters.
            c.put({sx, y}, g);
            for (int i = 1; i < cell_width_; ++i) c.put({sx + i, y}, Glyph{U' ', g.style});
        }
    }
}

}  // namespace modcurses
