# Copyright Ben Paul Wise. All Rights Reserved.
"""assemble.py -- stage 7: write the sheet XML from the catalogue, one to one, and validate it.

    python assemble.py MAP

Inputs: maps/MAP.json; work/MAP/calibration.json (the grid); work/MAP/catalogue/catalogue.json (merge.py);
work/MAP/catalogue/markers.json (the edge-marker pass, optional: entrance boxes and printed texts, each
written as a <label>); and the existing sheet, whose parts named in "sheet.keep" (palette, terrains, lines,
panels, and labels placed at x/y) are copied unchanged. Output: the sheet at "sheet.path", validated
against hexsheet.xsd.

No tidying. Every terrain, place, hexside line and link step of the catalogue is written as it is and
nothing is added: a hex takes its terrain from <hexes> lists (the grid default is not listed); a place is a
<hex> with a glyph and a name label on its first hex; a hexside line is an <edge>; the link steps of one
kind become <link> chains split at every end and junction. A step that leaves the map ("HEX:DIR") has no
<link> form: it is listed in an XML comment beside the links.
"""
import collections
import os
import sys
from xml.sax.saxutils import quoteattr

from lxml import etree

import common as C


def order_key(grid):
    return lambda pid: grid.ids[pid]


def chains(grid, steps):
    """Link steps [(a, b)] as hex chains, split at ends and junctions; closed loops start at their lowest hex."""
    adj = collections.defaultdict(list)
    for a, b in steps:
        adj[a].append(b)
        adj[b].append(a)
    used, out = set(), []

    def walk(a, b):
        chain = [a, b]
        used.add(frozenset((a, b)))
        while 2 == len(adj[b]):
            nxt = adj[b][0] if adj[b][1] == chain[-2] else adj[b][1]
            if frozenset((b, nxt)) in used:
                break
            used.add(frozenset((b, nxt)))
            chain.append(nxt)
            b = nxt
        return chain

    for pass_ends in (True, False):
        for n in sorted(adj, key=order_key(grid)):
            if pass_ends and 2 == len(adj[n]):
                continue
            for m in sorted(adj[n], key=order_key(grid)):
                if frozenset((n, m)) not in used:
                    out.append(walk(n, m))
    return out


def kept(old, keep):
    out = []
    for el in old.getroot():
        if not isinstance(el.tag, str):
            continue
        if el.tag in keep and not ("label" == el.tag and el.get("at")):
            el.tail = None
            out.append("  " + etree.tostring(el, encoding="unicode").strip())
    return out


def attrs(**kw):
    return " ".join("%s=%s" % (k.replace("_", "-"), quoteattr(str(v))) for k, v in kw.items() if v is not None)


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    cfg = C.load_config(argv[1])
    fit = C.load_fit(cfg)
    grid = C.make_grid(cfg, fit)
    # --catalogue PATH and --out PATH: a dry run on another catalogue, written beside the real sheet's name
    cat = C.read_json(argv[argv.index("--catalogue") + 1] if "--catalogue" in argv else C.work(cfg, "catalogue", "catalogue.json"))
    markers_path = C.work(cfg, "catalogue", "markers.json")
    markers = C.read_json(markers_path) if os.path.exists(markers_path) else []
    sheet_cfg = cfg["sheet"]
    path = os.path.join(C.XML_DIR, sheet_cfg["path"])
    old = etree.parse(path)
    root = old.getroot()
    missing = [pid for pid in grid.ids if pid not in cat["hexes"]]
    if missing:
        raise ValueError("catalogue has no terrain for %d hexes: %s" % (len(missing), " ".join(sorted(missing)[:20])))
    urban = sheet_cfg.get("urban")
    if urban not in ("buildings", "symbol"):
        raise ValueError("%s: sheet.urban must be 'buildings' or 'symbol' (how the renderer draws cities), not %r"
                         % (cfg["_path"], urban))
    carried = [(k, v) for k, v in root.attrib.items() if not k.startswith("{") and "urban" != k]
    x = ['<?xml version="1.0" encoding="UTF-8"?>',
         '<sheet xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="hexsheet.xsd"']
    x += ["       %s=%s" % (k, quoteattr(v)) for k, v in carried]
    x[-1] += " urban=%s>" % quoteattr(urban)
    x += [
         "  <!-- built by map_graphics/xml/tools/image2sheet/assemble.py from the %s catalogue; fix the catalogue, not this file -->" % cfg["map"],
         "  " + etree.tostring(C.grid_element(cfg, fit), encoding="unicode").strip()]
    keep = set(sheet_cfg["keep"])
    x += [line for line in kept(old, keep - {"label", "panel"})]
    by_terrain = collections.defaultdict(list)
    for pid in sorted(cat["hexes"], key=order_key(grid)):
        by_terrain[cat["hexes"][pid]["terrain"]].append(pid)
    # Every hex is listed, the grid's default terrain included: hexrules' PackageLoader knows a hex only from a
    # <hexes> list or a <hex> element, not from the grid.
    x.append("  <!-- terrain, one per hex, as read -->")
    for terrain in cfg["vocabulary"]["terrain"]:
        if by_terrain[terrain]:
            x.append("  <hexes %s/>" % attrs(terrain=terrain, ids=" ".join(by_terrain[terrain])))
    for line in cfg["vocabulary"]["hexside_lines"]:
        names = cat["rivers" if "river" == line else line]
        x.append("  <!-- %s: %d hexsides -->" % (line, len(names)))
        for name in names:
            pid, d = C.side_hexes(grid, name)
            x.append("  <edge %s/>" % attrs(at="%s:%s" % (pid, d), line=line))
    for kind in cfg["vocabulary"]["links"]:
        inner = [tuple(n.split("-")) for n in cat[kind] if ":" not in n]
        edge = [n for n in cat[kind] if ":" in n]
        parts = chains(grid, inner)
        x.append("  <!-- %s: %d steps in %d chains -->" % (kind, len(inner), len(parts)))
        if edge:
            x.append("  <!-- %s leaves the map across: %s -->" % (kind, " ".join(edge)))
        for chain in parts:
            x.append("  <link %s/>" % attrs(kind=kind, line=kind, hexes=" ".join(chain)))
    x.append("  <!-- places, as read -->")
    style = sheet_cfg["place_style"]
    named = set()
    # Region markers (Soviet/Axis mobilization hexes, resource hexes): a glyph on the hex, same as a place
    # glyph, from the catalogue's "markers" (kind "region"), never a new catalogue kind of their own.
    REGION_GLYPH = {"soviet-mobilization": ("star", "soviet"), "axis-mobilization": ("star", "axis"),
                    "resource": ("oil", "ink")}
    region_glyphs = {}
    for m in cat.get("markers", []):
        if "region" != m.get("kind"):
            continue
        sym, color = REGION_GLYPH[m["region"]]
        for pid in m["hexes"]:
            region_glyphs.setdefault(pid, []).append((sym, color))
    for pid in sorted(set(cat["hexes"]) | set(region_glyphs), key=order_key(grid)):
        place = cat["hexes"].get(pid, {}).get("place")
        glyphs = ([attrs(symbol=place["glyph"], color="ink")] if place else [])
        glyphs += [attrs(symbol=sym, color=color) for sym, color in region_glyphs.get(pid, [])]
        if not glyphs:
            continue
        hex_attrs = attrs(id=pid, name=place.get("name")) if place else attrs(id=pid)
        x.append("  <hex %s>%s</hex>" % (hex_attrs, "".join("<glyph %s/>" % g for g in glyphs)))
        if place and place.get("name") and place["name"] not in named:  # a place the reader has not named yet gets its glyph and no label
            named.add(place["name"])
            s = style[place["glyph"]]
            x.append("  <label %s/>" % attrs(text=place["name"], at=pid, slot="s", size=s["size"], weight=s["weight"]))
    if markers:
        x.append("  <!-- printed markers from the edge-marker pass -->")
    for m in markers:
        if m.get("label_xy"):
            lx, ly = m["label_xy"]
            x.append("  <label %s/>" % attrs(text=m["text"], x="%.0f" % lx, y="%.0f" % ly, size=m.get("size", 40),
                                             weight="bold"))
    x += kept(old, keep & {"label", "panel"})
    x.append("</sheet>")
    text = "\n".join(x) + "\n"
    doc = etree.fromstring(text.encode("utf-8"))
    schema = etree.XMLSchema(etree.parse(os.path.join(C.XML_DIR, "hexsheet.xsd")))
    if not schema.validate(doc):
        for e in schema.error_log:
            print("invalid: line %d: %s" % (e.line, e.message))
        return 1
    if "--out" in argv:
        path = argv[argv.index("--out") + 1]
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)
    counts = {k: len(v) for k, v in by_terrain.items()}
    print("wrote %s: terrain %s; %s" % (path, counts, "  ".join(
        "%s %d" % (k, len(cat["rivers" if "river" == k else k])) for k in list(cfg["vocabulary"]["hexside_lines"]) + list(cfg["vocabulary"]["links"]))))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
