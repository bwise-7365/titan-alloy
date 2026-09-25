# Copyright Ben Paul Wise. All Rights Reserved.
"""sheet.py -- the reader's structure as a hexsheet document (B4 output), validated and ready to render.

    python sheet.py DIR [--id ID] [--title TITLE]

Reads DIR/lattice.json (grid geometry and printed numbering), DIR/legend/vocabulary.json (the declared
kinds and their colours) and DIR/chain/ (terrain.json, places.json, <kind>.json chains), writes
DIR/chain/sheet.xml in hexsheet.xsd's language and validates it. The grid element is the lattice:
size and origin from the fitted centres, cols/rows/offset/id-format/starts from the numbering, and the
unprinted cells of the rectangle in clip. Style is what the reader measured: each terrain's fill is
its swatch colour, each line's stroke its swatch colour; nothing about the drawing is invented, and
the point of the document is the structure, which an editor finishes. A glyph whose sheet id is not a
Symbol of the schema is written as text.
"""
import json
import math
import os
import sys
from xml.sax.saxutils import quoteattr

from lxml import etree

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lattice as L
from legend import option

XML_DIR = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
SQRT3 = math.sqrt(3.0)
SYMBOLS = {"position-badge", "fire-intense", "fire-steady", "fire-square", "lvt-wreck", "arrival-box", "artillery", "tank",
           "pier-head", "city-major", "city-minor", "city", "capital", "town", "port", "port-multi", "port-key", "oil",
           "range-dot", "stacking", "star", "aid", "ice", "strait", "strait-broken", "arrow", "junction", "dot"}
SYMBOL_OF = {"star-soviet": "star", "star-axis": "star", "moscow": "capital", "setup": "star", "trenches": "dot", "fortification": "dot"}


LINE_COLOURS = (("rail", "#111111"), ("river", "#1f6fa8"), ("border", "#b0281f"), ("front", "#d06000"), ("blocked", "#000000"), ("road", "#5a5a5a"))


def line_colour(sheet_id, measured):
    """A line kind draws in the conventional colour of its family (rail black, river blue, border red,
    front orange, blocked black, road grey) so the structure reads at a glance (Ben, 2026-09-21: the
    measured swatch colours were too pale to see on Target Leningrad). Fills keep their measured colour."""
    for key, colour in LINE_COLOURS:
        if key in sheet_id:
            return colour
    return measured


def attrs(**kw):
    return " ".join("%s=%s" % (k.replace("_", "-"), quoteattr(str(v))) for k, v in kw.items() if v is not None)


def grid_element(d):
    """The renderer's grid from the lattice: printed index ranges, the stagger the print uses, origin
    and size from the fitted centres. Returns the element text and the set of printed ids."""
    numbering = d["numbering"]
    ori = d["orientation"]
    printed = set(d["printed"]["cells"])
    cells = {tuple(int(t) for t in k[1:].split("r")): v for k, v in d["grown"]["cells"].items() if k in printed}
    cols = max(c for c, _ in cells) + 1
    rows = max(r for _, r in cells) + 1
    # cells the numbering cannot name (negative column or row: furniture that passed the printed test,
    # Tannenberg's unit display boxes) are outside the grid rectangle, since clip cannot name them either
    cells = {k: v for k, v in cells.items() if "?" != L.cell_label(k, numbering, cols, rows)}
    pidx = {k: L.printed_index(k, numbering["orientation"], numbering["offset"], numbering["parity"]) for k in cells}
    pc_min = min(pc for pc, _ in pidx.values())
    pr_min = min(pr for _, pr in pidx.values())
    zero = {k: (pc - pc_min, pr - pr_min) for k, (pc, pr) in pidx.items()}
    ids = {k: L.cell_label(k, numbering, cols, rows) for k in cells}
    s = d["size"]
    flat = "flat" == ori
    # which parity of the staggered axis sits half a step further: compare two cells in the same row
    # (flat: same pr, columns of different parity) or the same column (pointy)
    sample = None
    for k, (c, r) in zero.items():
        if "?" == ids[k]:
            continue
        for k2, (c2, r2) in zero.items():
            if flat and r == r2 and c % 2 == 0 and c2 % 2 == 1:
                sample = (cells[k][1] < cells[k2][1])  # odd column lower on the page -> shifted
                break
            if not flat and c == c2 and r % 2 == 0 and r2 % 2 == 1:
                sample = (cells[k][0] < cells[k2][0])
                break
        if sample is not None:
            break
    offset = "odd" if sample else "even"
    k0 = next(iter(zero))
    c, r = zero[k0]
    x, y = cells[k0][0], cells[k0][1]
    shifted = (c % 2 == 1) if flat else (r % 2 == 1)
    shifted = shifted if "odd" == offset else not shifted
    if flat:
        ox = x - c * 1.5 * s
        oy = y - r * SQRT3 * s - (SQRT3 / 2 * s if shifted else 0)
    else:
        ox = x - c * SQRT3 * s - (SQRT3 / 2 * s if shifted else 0)
        oy = y - r * 1.5 * s
    ncols = max(c for c, _ in zero.values()) + 1
    nrows = max(r for _, r in zero.values()) + 1
    printed_ids = {v for v in ids.values() if "?" != v}  # cells the numbering cannot name are off the printed grid
    # ids of the rectangle's unprinted cells, made the renderer's way
    clip = []
    fmt = numbering["id-format"]
    half = numbering["half"]
    for cc in range(ncols):
        for rr in range(nrows):
            col = numbering["col-start"] + numbering["col-step"] * (cc + pc_min)
            row = numbering["row-start"] + numbering["row-step"] * (rr + pr_min)
            pid = fmt.format(col=col, row=row)  # exactly as the renderer's make_id forms it, negatives included
            if pid not in printed_ids:
                clip.append(pid)
    el = "  <grid %s/>" % attrs(id="main", orientation=ori, offset=offset, cols=ncols, rows=nrows, size="%.3f" % s,
                               ox="%.2f" % ox, oy="%.2f" % oy, id_format="{row:0%d}{col:0%d}" % (half, half) if "row-col" == numbering["order"] else "{col:0%d}{row:0%d}" % (half, half),
                               col_start=numbering["col-start"] + numbering["col-step"] * pc_min,
                               row_start=numbering["row-start"] + numbering["row-step"] * pr_min,
                               col_step=numbering["col-step"], row_step=numbering["row-step"], terrain="__DEFAULT__",
                               id_side="n" if flat else "w",  # the renderer's flat geometry has no "w" side
                               clip=" ".join(clip) if clip else None)
    return el, printed_ids


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    folder = argv[1]
    name = os.path.basename(os.path.normpath(folder))
    with open(os.path.join(folder, "lattice.json"), encoding="utf-8") as fh:
        d = json.load(fh)
    if not d.get("numbering"):
        raise ValueError("%s: no printed numbering; the sheet needs printed ids (anchors.json)" % name)
    with open(os.path.join(folder, "legend", "vocabulary.json"), encoding="utf-8") as fh:
        vocab = json.load(fh)
    chain = os.path.join(folder, "chain")
    with open(os.path.join(chain, "terrain.json"), encoding="utf-8") as fh:
        terrain = json.load(fh)
    with open(os.path.join(chain, "places.json"), encoding="utf-8") as fh:
        places = json.load(fh)["places"]
    glyphs_path = os.path.join(chain, "glyphs.json")
    glyphs = json.load(open(glyphs_path, encoding="utf-8")) if os.path.exists(glyphs_path) else []
    rings = {}
    cells_path = os.path.join(folder, "cells.json")
    if os.path.exists(cells_path):
        with open(cells_path, encoding="utf-8") as fh:
            for h, row in json.load(fh)["hex"].items():
                if row["ring"] and max(row["ring"].values()) >= 0.5:
                    rings[h] = max(row["ring"], key=row["ring"].get)

    fills = [v for v in vocab if "hex" == v["kind"]]
    lines = [v for v in vocab if v["kind"].startswith("side-")]
    default = option(argv, "--default", "clear")
    if default not in {v["sheet"] for v in fills}:
        fills.append({"sheet": default, "name": default.title(), "colour": "#f4f1e6", "kind": "hex"})
    grid_el, printed_ids = grid_element(d)
    grid_el = grid_el.replace("__DEFAULT__", default)
    with_im = d.get("size_px")
    x = ['<?xml version="1.0" encoding="UTF-8"?>',
         '<sheet xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="hexsheet.xsd"',
         '       %s>' % attrs(id=option(argv, "--id", name.lower().replace(" ", "-")), title=option(argv, "--title", name.replace("_", " ")),
                              source=os.path.basename(d["source"]), width=int(d.get("width", 0) or Image_size(d)[0]), height=int(d.get("height", 0) or Image_size(d)[1]),
                              background="paper", font="Arial, Helvetica, sans-serif", urban="buildings", junctions="implicit"),
         "  <!-- built by map_graphics/xml/tools/reader/sheet.py from the reader's structure (cells.json, chain/); fix the reading, not this file -->",
         grid_el, "  <palette>", '    <color id="paper" value="#f4f1e6"/>', '    <color id="grid" value="#6f6f66"/>', '    <color id="ink" value="#111111"/>']
    for v in fills:
        x.append("    <color %s/>" % attrs(id="c-" + v["sheet"], value=v["colour"], name=v.get("name")))
    for v in lines:
        x.append("    <color %s/>" % attrs(id="c-" + v["sheet"], value=line_colour(v["sheet"], v["colour"]), name=v.get("name")))
    ring_kinds = [v for v in vocab if "ring" == v["kind"]]
    for v in ring_kinds:
        x.append("    <color %s/>" % attrs(id="c-" + v["sheet"], value=v["colour"], name=v.get("name")))
    x.append("  </palette>")
    x.append("  <terrains>")
    for v in fills:
        x.append("    <terrain %s/>" % attrs(id=v["sheet"], name=v.get("name") or v["sheet"], fill="c-" + v["sheet"], stroke="grid"))
    x.append("  </terrains>")
    if lines:
        x.append("  <lines>")
        for v in lines:
            width = 6 if "side-along" == v["kind"] else 4
            x.append("    <line %s/>" % attrs(id=v["sheet"], stroke="c-" + v["sheet"], width=width, dash="1,3" if "rail" in v["sheet"] else None))
        x.append("  </lines>")
    by_terrain = {}
    for h, t in terrain.items():
        if h in printed_ids and t != default:
            by_terrain.setdefault(t, []).append(h)
    for t, hs in sorted(by_terrain.items()):
        x.append("  <hexes %s/>" % attrs(terrain=t, ids=" ".join(sorted(hs))))
    for v in lines:
        path = os.path.join(chain, v["sheet"] + ".json")
        if not os.path.exists(path):
            continue
        with open(path, encoding="utf-8") as fh:
            rows = json.load(fh)["chains"]
        if "side-across" == v["kind"]:
            for r in rows:
                x.append("  <link %s/>" % attrs(kind=v["sheet"], line=v["sheet"], hexes=" ".join(r["hexes"]),
                                                 ends=" ".join(e["reason"] for e in r["ends"])))
        else:
            for r in rows:
                for side in r["sides"]:
                    x.append("  <edge %s/>" % attrs(at=edge_ref(side, d), line=v["sheet"]))
    glyph_at = {}
    for h, p in places.items():
        glyph_at.setdefault(h, []).append(p["glyph"])
    for g in glyphs:
        glyph_at.setdefault(g["hex"], []).append(g["glyph"])
    for h in sorted(set(glyph_at) | set(rings)):
        if h not in printed_ids:
            continue
        inner = []
        for gname in glyph_at.get(h, []):
            sym = gname if gname in SYMBOLS else SYMBOL_OF.get(gname)
            inner.append("<glyph %s/>" % (attrs(symbol=sym, color="ink") if sym else attrs(symbol="dot", text=gname, color="ink")))
        x.append("  <hex %s>%s</hex>" % (attrs(id=h, ring=("c-" + rings[h]) if h in rings else None), "".join(inner)))
    x.append("</sheet>")
    text = "\n".join(x) + "\n"
    doc = etree.fromstring(text.encode("utf-8"))
    schema = etree.XMLSchema(etree.parse(os.path.join(XML_DIR, "hexsheet.xsd")))
    if not schema.validate(doc):
        for e in schema.error_log:
            print("invalid: line %d: %s" % (e.line, e.message))
        return 1
    out = os.path.join(chain, "sheet.xml")
    with open(out, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(text)
    print("wrote %s: valid; terrain groups %d, edges %d, links %d, hexes with glyphs or rings %d" % (
        out, len(by_terrain), sum(1 for l in x if l.startswith("  <edge")), sum(1 for l in x if l.startswith("  <link")), sum(1 for l in x if l.startswith("  <hex "))))
    return 0


def edge_ref(side, d):
    """cells.py's hexside name as an EdgeRef: HEX:DIR stays; A-B becomes A:DIR from the lattice."""
    if ":" in side:
        return side
    a, b = side.split("-")
    return "%s:%s" % (a, DIR_OF(d)[(a, b)])


_dirs = {}


def DIR_OF(d):
    if not _dirs:
        from structure import Lattice
        lat = Lattice(d)
        for (h, dr), name in lat.by_hexdir.items():
            if "-" in name:
                a, b = name.split("-")
                _dirs[(h, b if a == h else a)] = dr
    return _dirs


def Image_size(d):
    from PIL import Image
    with Image.open(d["source"]) as im:
        return im.size


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
