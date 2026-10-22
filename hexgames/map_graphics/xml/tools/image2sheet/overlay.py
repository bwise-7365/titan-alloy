# Copyright Ben Paul Wise. All Rights Reserved.
"""overlay.py -- stage 2: draw the fitted grid, the hex ids and the hexside midpoints over an image.

    python overlay.py MAP [SOURCE]

writes work/MAP/overview-SOURCE.jpg, a shrunken whole-map view under 300 KB with the grid and every
fifth hex id, for a person's orientation only (never read it to place a feature). The drawing
functions here are used by tile.py and crop.py, so every tile and crop carries the same overlay:
thin magenta hex outlines, the hex id in magenta (core hexes) or orange (margin hexes) placed on the
side opposite the printed id, and a small dot at every hexside midpoint.
"""
import math
import sys

from PIL import ImageDraw, ImageFont

import common as C

OUTLINE = (235, 0, 170, 160)
CORE = (225, 0, 160, 255)
MARGIN = (255, 110, 0, 255)
DOT = (235, 0, 170, 200)


def font(px):
    try:
        return ImageFont.truetype("arial.ttf", px)
    except OSError:
        return ImageFont.load_default()


def label_offset(grid):
    """Where our id label goes: toward the CORNER just clockwise of the hexside opposite the printed id.
    A label on the ray from the centre to a hexside midpoint hides every line that crosses that hexside
    (Ben, 2026-09-17: a horizontal railway through 1831 vanished under the id); a corner lies on none of
    the six midpoint rays, and only a line through that one vertex passes near it."""
    ang = math.radians(grid.edge_angle(C.H.OPPOSITE[grid.printed_id_side]) + 30)
    return 0.62 * grid.size * math.cos(ang), 0.62 * grid.size * math.sin(ang)


def hexes_in_box(grid, box):
    x0, y0, x1, y1 = box
    out = []
    for pid, (c, r) in grid.ids.items():
        cx, cy = grid.centre(c, r)
        if x0 - grid.size <= cx <= x1 + grid.size and y0 - grid.size <= cy <= y1 + grid.size:
            out.append(pid)
    return out


def draw_grid(img, grid, box, scale, core=None, ids=True, dots=True):
    """Draw the overlay on img, a copy of source pixels box (x0, y0, x1, y1) enlarged by scale."""
    x0, y0 = box[0], box[1]
    d = ImageDraw.Draw(img, "RGBA")
    f = font(max(11, int(grid.size * 0.24 * scale)))
    lx, ly = label_offset(grid)

    def at(x, y):
        return (x - x0) * scale, (y - y0) * scale

    for pid in hexes_in_box(grid, box):
        c, r = grid.ids[pid]
        poly = [at(x, y) for x, y in grid.polygon(c, r)]
        d.line(poly + [poly[0]], fill=OUTLINE, width=1)
        cx, cy = grid.centre(c, r)
        if ids:
            colour = CORE if core is None or pid in core else MARGIN
            d.text(at(cx + lx, cy + ly), pid, font=f, fill=colour[:3] + (170,), anchor="mm", stroke_width=1,
                   stroke_fill=(255, 255, 255, 110))
        if dots:
            for direction in C.directions(grid):
                mx, my = at(*grid.edge_mid(c, r, direction))
                d.ellipse((mx - 2, my - 2, mx + 2, my + 2), fill=DOT)
    return img


def overview(cfg, name):
    src = C.source(cfg, name)
    grid = C.make_grid(cfg, C.load_fit(cfg, name))
    img = C.open_image(src["path"])
    scale = 1400.0 / img.width
    small = img.resize((1400, int(img.height * scale)))
    d = ImageDraw.Draw(small, "RGBA")
    f = font(11)
    for pid, (c, r) in grid.ids.items():
        poly = [(x * scale, y * scale) for x, y in grid.polygon(c, r)]
        d.line(poly + [poly[0]], fill=OUTLINE, width=1)
        if 0 == c % 5 and 0 == r % 5:
            cx, cy = grid.centre(c, r)
            d.text((cx * scale, cy * scale), pid, font=f, fill=CORE[:3] + (170,), anchor="mm", stroke_width=1,
                   stroke_fill=(255, 255, 255, 110))
    out = C.work(cfg, "overview-%s.jpg" % name)
    C.save_jpeg(small, out, 70)
    print("wrote", out)
    return


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    cfg = C.load_config(argv[1])
    overview(cfg, argv[2] if len(argv) > 2 else "primary")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
