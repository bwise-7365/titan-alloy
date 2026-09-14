# Copyright Ben Paul Wise. All Rights Reserved.
"""common.py -- shared pieces of the image2sheet stages.

The map configuration (maps/<map>.json), the work folder (work/<map>/), the fitted grid (built with the
reference renderer's own Grid class, so every stage uses exactly the geometry the render will use), the
images, and the names of hexsides.

Hexside names. A reader names an inner hexside by its two hexes, "0419-0420", in either order; merge
writes the canonical form, the two ids in grid order (column index, then row index). A hexside on the
map edge has one hex and is named "HEX:DIR". A link step (rail, road) is named the same way as the
hexside it crosses, so every feature a reader records is a hex or a hexside.
"""
import json
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
XML_DIR = os.path.normpath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, XML_DIR)

import hexsheet2svg as H  # noqa: E402
from lxml import etree  # noqa: E402
from PIL import Image  # noqa: E402

Image.MAX_IMAGE_PIXELS = None
SQRT3 = math.sqrt(3)
MAX_JPEG_BYTES = 300_000


def load_config(name):
    """maps/<name>.json, or a path to a configuration file."""
    path = name if name.endswith(".json") else os.path.join(HERE, "maps", name + ".json")
    with open(path, encoding="utf-8") as f:
        cfg = json.load(f)
    cfg["_path"] = os.path.abspath(path)
    return cfg


def work(cfg, *parts):
    """A path under work/<map>/, its folder created."""
    path = os.path.join(HERE, "work", cfg["map"], *parts)
    os.makedirs(os.path.dirname(path) if os.path.splitext(path)[1] else path, exist_ok=True)
    return path


def read_json(path):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def write_json(path, data):
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=1)
        f.write("\n")


def source(cfg, name):
    if name not in cfg["sources"]:
        raise ValueError("%s: no source '%s' (have %s)" % (cfg["_path"], name, ", ".join(cfg["sources"])))
    return cfg["sources"][name]


def open_image(path):
    return Image.open(path).convert("RGB")


def grid_element(cfg, fit):
    """The <grid> element for a fit {size, ox, oy}, from the configuration's grid block."""
    g = cfg["grid"]
    attrs = {"id": g.get("id", "main"), "orientation": g["orientation"], "offset": g["offset"],
             "cols": str(g["cols"]), "rows": str(g["rows"]), "size": "%.3f" % fit["size"],
             "ox": "%.2f" % fit["ox"], "oy": "%.2f" % fit["oy"], "id-format": g["id-format"],
             "col-start": str(g.get("col-start", 1)), "row-start": str(g.get("row-start", 1)),
             "terrain": g["terrain"]}
    for key in ("col-step", "row-step", "id-side", "clip"):
        if key in g:
            attrs[key] = str(g[key])
    return etree.Element("grid", **attrs)


def make_grid(cfg, fit):
    return H.Grid(grid_element(cfg, fit))


def load_fit(cfg, name="primary"):
    path = work(cfg, "calibration.json")
    if not os.path.exists(path):
        raise ValueError("no calibration yet: run calibrate.py %s fit" % cfg["map"])
    fits = read_json(path)
    if name not in fits or not fits[name]["passed"]:
        raise ValueError("source '%s' has not passed the calibrate gate (%s)" % (name, path))
    return fits[name]


def spacing(grid):
    """Centre-to-centre distance of neighbouring hexes: the 'hex' unit of residuals."""
    return SQRT3 * grid.size


def directions(grid):
    return list(grid.geom["edges"])


def known(grid, pid, ctx):
    if pid not in grid.ids:
        raise ValueError("%s: hex '%s' is not on the grid" % (ctx, pid))
    return pid


def step_dir(grid, a, b):
    """The direction from hex a to neighbouring hex b, or None."""
    ca = grid.ids[a]
    for d in directions(grid):
        if grid.cells.get(grid.neighbour(*ca, d)) == b:
            return d
    return None


def side_name(grid, pid, d):
    """Canonical name of hexside pid:d: 'A-B' in grid order, or 'HEX:DIR' on the map edge."""
    other = grid.cells.get(grid.neighbour(*grid.ids[pid], d))
    if other is None:
        return "%s:%s" % (pid, d)
    a, b = sorted((pid, other), key=lambda h: grid.ids[h])
    return "%s-%s" % (a, b)


def canonical_side(grid, token, ctx):
    """A reader's hexside token ('A-B', 'B-A' or 'HEX:DIR') in canonical form; throws on a bad token."""
    if ":" in token:
        pid, d = token.split(":", 1)
        known(grid, pid, ctx)
        if d not in directions(grid):
            raise ValueError("%s: '%s' is not a direction of this grid" % (ctx, token))
        return side_name(grid, pid, d)
    parts = token.split("-")
    if 2 != len(parts):
        raise ValueError("%s: hexside '%s' is neither 'A-B' nor 'HEX:DIR'" % (ctx, token))
    a, b = (known(grid, p, ctx) for p in parts)
    d = step_dir(grid, a, b)
    if d is None:
        raise ValueError("%s: hexes %s and %s are not neighbours" % (ctx, a, b))
    return side_name(grid, a, d)


def side_hexes(grid, name):
    """(hex, direction) for the hexsheet <edge at="HEX:DIR">: the first hex of a pair, or the edge hex."""
    if ":" in name:
        pid, d = name.split(":")
        return pid, d
    a, b = name.split("-")
    return a, step_dir(grid, a, b)


def all_sides(grid):
    """Every hexside of the grid, canonical names, in grid order."""
    out = []
    seen = set()
    for (c, r), pid in sorted(grid.cells.items()):
        for d in directions(grid):
            name = side_name(grid, pid, d)
            if name not in seen:
                seen.add(name)
                out.append(name)
    return out


def all_sides_within(grid, area):
    """Canonical names of every hexside whose hexes (one on the map edge, else both) lie in area."""
    out = []
    for pid in area:
        for d in directions(grid):
            name = side_name(grid, pid, d)
            if set(name.split(":")[0].split("-")) <= area:
                out.append(name)
    return sorted(set(out))


def side_segment(grid, name):
    pid, d = side_hexes(grid, name)
    return grid.edge_ends(*grid.ids[pid], d)


def save_jpeg(img, path, quality=82):
    """Save a JPEG no larger than MAX_JPEG_BYTES, lowering quality (then size) until it fits."""
    while True:
        img.save(path, "JPEG", quality=quality)
        if os.path.getsize(path) <= MAX_JPEG_BYTES:
            return path
        if quality > 50:
            quality -= 8
        else:
            img = img.resize((int(img.width * 0.85), int(img.height * 0.85)), Image.LANCZOS)
# Copyright Ben Paul Wise. All Rights Reserved.
