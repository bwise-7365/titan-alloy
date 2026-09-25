# Copyright Ben Paul Wise. All Rights Reserved.
"""svg-golden.py -- the Python side of hexview's golden tests. hexsheet2svg.py is the reference.

    python svg-golden.py reference SHEET.xml OUT.svg      render the reference SVG of a sheet
    python svg-golden.py centres SHEET.xml OUT.txt        every printed id: centre and six corners
    python svg-golden.py compare SHEET.xml REF.svg MINE.svg

compare normalises both SVGs to the same form and compares them element for element, layer for layer:
  - groups are flattened: each drawing element becomes one leaf, with its group transforms applied
    to its geometry and the presentation attributes it inherits written on it (SVG's defaults where
    nothing is inherited); class, id and data-* attributes are dropped (they carry no paint);
  - polygon, rect, circle and ellipse become path commands in one canonical spelling (hexview's
    Shapes.h: a rectangle clockwise from its top-left corner, a circle as two half-circle arcs from
    its east point); path data becomes absolute commands;
  - colours are lower-case #rrggbb; numbers compare to within one unit of the second decimal (0.011),
    angles to within 0.051 degree (the reference prints glyph rotations with one decimal);
  - <defs>: every reference symbol and pattern must be in MINE and equal (to 1e-6); MINE may define
    no others (the reference draws legend marks inline as
    groups, hexview as placed paths, so neither has them in <defs>);
  - a reference <g class="... buildings"> (the scattered buildings of a city hex on an
    urban="buildings" sheet) is matched by ONE path in MINE, since the C++ generator is the reference
    for their layout (Ben, 2026-09-23): both must hold 4 or 5 axis-aligned rectangles of the
    reference's sizes, in the same fill, and all their centres must fit in one glyph's scatter box
    (1.1 x 0.96 hex sizes).
It prints each layer's counts and the first differences, and exits 1 on any difference.
"""
import math
import os
import re
import sys

from lxml import etree

HERE = os.path.dirname(os.path.abspath(__file__))
XML = os.path.normpath(os.path.join(HERE, "..", "..", "map_graphics", "xml"))
sys.path.insert(0, XML)

SVG = "{http://www.w3.org/2000/svg}"
XLINK = "{http://www.w3.org/1999/xlink}href"
TOL = 0.011
ANGLE_TOL = 0.051
DEFS_TOL = 1e-6
BUILDING_SHAPES = [(0.44, 0.24), (0.32, 0.22), (0.24, 0.38), (0.30, 0.30), (0.38, 0.20), (0.22, 0.22)]

INHERITED = ["fill", "fill-opacity", "stroke", "stroke-width", "stroke-opacity", "stroke-dasharray",
             "stroke-linecap", "stroke-linejoin", "color", "font-family", "font-size", "font-weight",
             "font-style", "text-anchor", "letter-spacing", "paint-order"]
DEFAULTS = {"fill": "#000000", "fill-opacity": "1", "stroke": "none", "stroke-width": "1",
            "stroke-opacity": "1", "stroke-dasharray": "none", "stroke-linecap": "butt",
            "stroke-linejoin": "miter", "color": "#000000", "font-family": "", "font-size": "16",
            "font-weight": "normal", "font-style": "normal", "text-anchor": "start",
            "letter-spacing": "0", "paint-order": "normal"}


# ---------------------------------------------------------------- affine transforms
def mul(m, n):
    a, b, c, d, e, f = m
    a2, b2, c2, d2, e2, f2 = n
    return (a * a2 + c * b2, b * a2 + d * b2, a * c2 + c * d2, b * c2 + d * d2,
            a * e2 + c * f2 + e, b * e2 + d * f2 + f)


IDENTITY = (1.0, 0.0, 0.0, 1.0, 0.0, 0.0)


def parse_transform(text):
    m = IDENTITY
    for name, args in re.findall(r"(\w+)\s*\(([^)]*)\)", text or ""):
        v = [float(x) for x in re.split(r"[\s,]+", args.strip()) if x]
        if name == "translate":
            t = (1, 0, 0, 1, v[0], v[1] if len(v) > 1 else 0)
        elif name == "scale":
            t = (v[0], 0, 0, v[1] if len(v) > 1 else v[0], 0, 0)
        elif name == "rotate":
            a = math.radians(v[0])
            t = (math.cos(a), math.sin(a), -math.sin(a), math.cos(a), 0, 0)
            if len(v) == 3:
                t = mul(mul((1, 0, 0, 1, v[1], v[2]), t), (1, 0, 0, 1, -v[1], -v[2]))
        else:
            raise ValueError("unsupported transform " + name)
        m = mul(m, t)
    return m


def apply(m, x, y):
    return (m[0] * x + m[2] * y + m[4], m[1] * x + m[3] * y + m[5])


def scale_of(m):
    return math.hypot(m[0], m[1])


def angle_of(m):
    return math.degrees(math.atan2(m[1], m[0])) % 360.0


# ---------------------------------------------------------------- geometry to canonical commands
def rect_cmds(x, y, w, h, r=0.0):
    if r <= 0:
        return [("M", x, y), ("L", x + w, y), ("L", x + w, y + h), ("L", x, y + h), ("Z",)]
    return [("M", x + r, y), ("L", x + w - r, y), ("A", r, r, 0, 0, 1, x + w, y + r),
            ("L", x + w, y + h - r), ("A", r, r, 0, 0, 1, x + w - r, y + h), ("L", x + r, y + h),
            ("A", r, r, 0, 0, 1, x, y + h - r), ("L", x, y + r), ("A", r, r, 0, 0, 1, x + r, y), ("Z",)]


def ellipse_cmds(cx, cy, rx, ry):
    return [("M", cx + rx, cy), ("A", rx, ry, 0, 0, 1, cx - rx, cy), ("A", rx, ry, 0, 0, 1, cx + rx, cy), ("Z",)]


def path_cmds(d):
    toks = re.findall(r"[A-Za-z]|[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?", d)
    out = []
    i = 0
    cur = (0.0, 0.0)
    start = (0.0, 0.0)
    cmd = None
    while i < len(toks):
        if re.match(r"[A-Za-z]", toks[i]):
            cmd = toks[i]
            i += 1
            first = True
            if cmd in "Zz":
                out.append(("Z",))
                cur = start
                continue
        rel = cmd.islower()
        up = cmd.upper()

        def num():
            nonlocal i
            v = float(toks[i])
            i += 1
            return v

        def pt():
            x, y = num(), num()
            return (cur[0] + x, cur[1] + y) if rel else (x, y)
        if up == "M":
            p = pt()
            out.append(("M" if first else "L",) + p)
            if first:
                start = p
            cur = p
        elif up == "L":
            cur = pt()
            out.append(("L",) + cur)
        elif up == "H":
            x = num()
            cur = (cur[0] + x if rel else x, cur[1])
            out.append(("L",) + cur)
        elif up == "V":
            y = num()
            cur = (cur[0], cur[1] + y if rel else y)
            out.append(("L",) + cur)
        elif up == "Q":
            c = pt()
            p = pt()
            out.append(("Q",) + c + p)
            cur = p
        elif up == "A":
            rx, ry, rot, large, sweep = num(), num(), num(), num(), num()
            p = pt()
            out.append(("A", rx, ry, rot, large, sweep) + p)
            cur = p
        else:
            raise ValueError("unsupported path command " + cmd)
        first = False
    return out


def transform_cmds(m, cmds):
    s = scale_of(m)
    rot = angle_of(m)
    out = []
    for c in cmds:
        if c[0] in "ML":
            out.append((c[0],) + apply(m, c[1], c[2]))
        elif c[0] == "Q":
            out.append(("Q",) + apply(m, c[1], c[2]) + apply(m, c[3], c[4]))
        elif c[0] == "A":
            rx, ry = c[1] * s, c[2] * s
            r = 0.0 if abs(rx - ry) < 1e-9 else (c[3] + rot) % 180.0
            out.append(("A", rx, ry, r, int(c[4]), int(c[5])) + apply(m, c[6], c[7]))
        else:
            out.append(("Z",))
    return out


# ---------------------------------------------------------------- flattening
def color(v, props):
    v = v.strip()
    if v == "currentColor":
        v = props.get("color", "currentColor")
        if v == "currentColor":
            return v
    v = v.lower()
    if re.fullmatch(r"#[0-9a-f]{3}", v):
        v = "#" + "".join(ch * 2 for ch in v[1:])
    return v


def own_props(el):
    p = {k: el.get(k) for k in INHERITED if el.get(k) is not None}
    for decl in (el.get("style") or "").split(";"):
        if ":" in decl:
            k, v = decl.split(":", 1)
            p[k.strip()] = v.strip()
    return p


def paint(props, m):
    s = scale_of(m)
    fill = color(props["fill"], props)
    out = {"fill": fill}
    if fill != "none":
        out["fill-opacity"] = float(props["fill-opacity"])
    stroke = color(props["stroke"], props)
    out["stroke"] = stroke
    if stroke != "none":
        out["stroke-width"] = float(props["stroke-width"]) * s
        out["stroke-opacity"] = float(props["stroke-opacity"])
        dash = props["stroke-dasharray"]
        out["stroke-dasharray"] = [] if dash == "none" else [float(x) * s for x in re.split(r"[\s,]+", dash.strip()) if x]
        out["stroke-linecap"] = props["stroke-linecap"]
        out["stroke-linejoin"] = props["stroke-linejoin"]
    return out


class Leaf:
    def __init__(self, kind, geom, props, text="", group=None):
        self.kind, self.geom, self.props, self.text, self.group = kind, geom, props, text, group

    def __repr__(self):
        g = self.geom if len(str(self.geom)) < 300 else str(self.geom)[:300] + "..."
        return "%s %s %s %r" % (self.kind, g, self.props, self.text)


def flatten(el, m, props, out, group=None):
    tag = etree.QName(el).localname if isinstance(el.tag, str) else None
    if tag is None or tag in ("title", "defs", "symbol", "pattern"):
        return
    props = dict(props)
    props.update(own_props(el))
    m = mul(m, parse_transform(el.get("transform")))
    f = lambda k: float(el.get(k) or 0)
    if tag == "g":
        if "buildings" in (el.get("class") or "").split():
            group = id(el)
        for ch in el:
            flatten(ch, m, props, out, group)
        return
    if tag == "text":
        x, y = apply(m, f("x"), f("y"))
        t = {"fill": color(props["fill"], props), "fill-opacity": float(props["fill-opacity"]),
             "font-family": props["font-family"], "font-size": float(props["font-size"]) * scale_of(m),
             "font-weight": props["font-weight"], "font-style": props["font-style"],
             "text-anchor": props["text-anchor"],
             "dominant-baseline": {"auto": "alphabetic", "alphabetic": "alphabetic"}.get(
                 el.get("dominant-baseline") or "auto", el.get("dominant-baseline")),
             "letter-spacing": float(props["letter-spacing"])}
        p = paint(props, m)
        if p["stroke"] != "none":
            t.update({k: v for k, v in p.items() if k.startswith("stroke")})
            t["paint-order"] = props["paint-order"]
        text = " ".join("".join(el.itertext()).split())
        out.append(Leaf("text", [x, y, angle_of(m)], t, text, group))
        return
    if tag == "use":
        x, y = apply(m, 0, 0)
        out.append(Leaf("use", [x, y, angle_of(m), scale_of(m)],
                        {"href": el.get(XLINK), "color": color(props["color"], props)}, "", group))
        return
    if tag == "path":
        cmds = path_cmds(el.get("d"))
    elif tag == "polygon":
        nums = [float(v) for v in re.split(r"[\s,]+", el.get("points").strip())]
        pts = list(zip(nums[0::2], nums[1::2]))
        cmds = [("M",) + pts[0]] + [("L",) + p for p in pts[1:]] + [("Z",)]
    elif tag == "rect":
        rx = el.get("rx") or el.get("ry")
        cmds = rect_cmds(f("x"), f("y"), f("width"), f("height"), float(rx) if rx else 0.0)
    elif tag == "circle":
        cmds = ellipse_cmds(f("cx"), f("cy"), f("r"), f("r"))
    elif tag == "ellipse":
        cmds = ellipse_cmds(f("cx"), f("cy"), f("rx"), f("ry"))
    else:
        raise ValueError("unsupported element <%s>" % tag)
    out.append(Leaf("path", transform_cmds(m, cmds), paint(props, m), "", group))


def normalise(path):
    root = etree.parse(path).getroot()
    base = dict(DEFAULTS)
    base.update(own_props(root))
    doc = {"size": (float(root.get("width")), float(root.get("height"))), "title": "",
           "symbols": {}, "patterns": {}, "layers": {}}
    for el in root:
        if not isinstance(el.tag, str):
            continue
        tag = etree.QName(el).localname
        if tag == "title":
            doc["title"] = " ".join((el.text or "").split())
        elif tag == "defs":
            for d in el:
                if not isinstance(d.tag, str):
                    continue
                leaves = []
                dprops = dict(DEFAULTS, color="currentColor")
                for ch in d:
                    flatten(ch, IDENTITY, dprops, leaves)
                name = etree.QName(d).localname
                if name == "symbol":
                    doc["symbols"][d.get("id")] = leaves
                else:
                    doc["patterns"][d.get("id")] = ((d.get("width"), d.get("height")), leaves)
        elif tag == "g" and "layer" in (el.get("class") or "").split():
            name = el.get("class").split()[1]
            leaves = doc["layers"].setdefault(name, [])
            for ch in el:
                flatten(ch, IDENTITY, dict(base, **own_props(el)), leaves)
        else:
            flatten(el, IDENTITY, base, doc["layers"].setdefault("background", []))
    return doc


# ---------------------------------------------------------------- comparison
def close(a, b, tol):
    return abs(a - b) <= tol


def angle_close(a, b):
    d = abs(a - b) % 360.0
    return min(d, 360.0 - d) <= ANGLE_TOL


def same_value(k, a, b, tol):
    if isinstance(a, (int, float)) and isinstance(b, (int, float)):
        return close(a, b, tol)
    if isinstance(a, list) and isinstance(b, list):
        return len(a) == len(b) and all(close(x, y, tol) for x, y in zip(a, b))
    return a == b


def leaf_diff(a, b, tol):
    """None if equal, else a short reason."""
    if a.kind != b.kind:
        return "kind %s vs %s" % (a.kind, b.kind)
    if a.kind == "path":
        if len(a.geom) != len(b.geom):
            return "%d vs %d path commands" % (len(a.geom), len(b.geom))
        for i, (c, d) in enumerate(zip(a.geom, b.geom)):
            if c[0] != d[0] or len(c) != len(d):
                return "command %d: %s vs %s" % (i, c[0], d[0])
            for x, y in zip(c[1:], d[1:]):
                if not close(x, y, tol):
                    return "command %d: %s vs %s" % (i, fmt(c), fmt(d))
    else:
        n = len(a.geom)
        for i, (x, y) in enumerate(zip(a.geom, b.geom)):
            ok = angle_close(x, y) if i == 2 else close(x, y, tol)
            if not ok:
                return "geometry %s vs %s" % (fmt(a.geom), fmt(b.geom))
        if a.text != b.text:
            return "text %r vs %r" % (a.text, b.text)
    keys = set(a.props) | set(b.props)
    for k in sorted(keys):
        if k not in a.props or k not in b.props or not same_value(k, a.props[k], b.props[k], tol):
            return "%s: %r vs %r" % (k, a.props.get(k), b.props.get(k))
    return None


def fmt(c):
    return "(" + " ".join(x if isinstance(x, str) else "%.3f" % x for x in c) + ")"


def rects_of(leaf):
    """The axis-aligned rectangles of a path leaf, as (x0, y0, x1, y1), or None."""
    out = []
    cur = []
    for c in leaf.geom:
        if c[0] in "ML":
            cur.append((c[1], c[2]))
        elif c[0] == "Z":
            if len(cur) != 4:
                return None
            xs = sorted({round(p[0], 3) for p in cur})
            ys = sorted({round(p[1], 3) for p in cur})
            if len(xs) != 2 or len(ys) != 2:
                return None
            out.append((xs[0], ys[0], xs[1], ys[1]))
            cur = []
        else:
            return None
    return out


def buildings_diff(ref_leaves, mine, size):
    ref_rects = [r for leaf in ref_leaves for r in (rects_of(leaf) or [])]
    my_rects = rects_of(mine) if mine.kind == "path" else None
    if my_rects is None:
        return "buildings: MINE is not a path of rectangles: %r" % mine
    if len(ref_rects) not in (4, 5) or len(my_rects) not in (4, 5):
        return "buildings: %d reference and %d C++ rectangles (want 4 or 5 each)" % (len(ref_rects), len(my_rects))
    if ref_leaves[0].props["fill"] != mine.props["fill"] or mine.props["stroke"] != "none":
        return "buildings: paint %r vs %r" % (ref_leaves[0].props, mine.props)
    for r in my_rects:
        w, h = (r[2] - r[0]) / size, (r[3] - r[1]) / size
        if not any(abs(w - a) < 0.01 and abs(h - b) < 0.01 for a, b in BUILDING_SHAPES):
            return "buildings: rectangle %s is not one of the reference's sizes" % fmt(r)
    centres = [((r[0] + r[2]) / 2, (r[1] + r[3]) / 2) for r in ref_rects + my_rects]
    xs = [c[0] for c in centres]
    ys = [c[1] for c in centres]
    if max(xs) - min(xs) > 1.1 * size + 0.02 or max(ys) - min(ys) > 0.96 * size + 0.02:
        return "buildings: the C++ rectangles lie outside the reference's scatter box"
    return None


def compare_layer(name, ref, mine, size, report):
    diffs = 0
    i = j = 0
    while i < len(ref) and j < len(mine):
        if ref[i].group is not None:
            k = i
            while k < len(ref) and ref[k].group == ref[i].group:
                k += 1
            why = buildings_diff(ref[i:k], mine[j], size)
            i = k
        else:
            why = leaf_diff(ref[i], mine[j], TOL)
            i += 1
        j += 1
        if why:
            diffs += 1
            if diffs <= 8:
                report.append("  %s #%d: %s\n      ref : %r\n      mine: %r" % (name, j - 1, why, ref[i - 1], mine[j - 1]))
    missing = len(ref) - i
    extra = len(mine) - j
    return diffs, missing, extra


def compare(sheet, ref_path, mine_path):
    size = float(etree.parse(sheet).getroot().find("grid").get("size"))
    ref = normalise(ref_path)
    mine = normalise(mine_path)
    report = []
    bad = 0
    if any(not close(a, b, TOL) for a, b in zip(ref["size"], mine["size"])):
        report.append("svg size %s vs %s" % (ref["size"], mine["size"]))
        bad += 1
    if ref["title"] != mine["title"]:
        report.append("title %r vs %r" % (ref["title"], mine["title"]))
        bad += 1
    for sid, leaves in ref["symbols"].items():
        mine_leaves = mine["symbols"].get(sid)
        if mine_leaves is None:
            report.append("defs: symbol %s missing" % sid)
            bad += 1
            continue
        why = None
        if len(leaves) != len(mine_leaves):
            why = "%d vs %d parts" % (len(leaves), len(mine_leaves))
        else:
            for a, b in zip(leaves, mine_leaves):
                why = why or leaf_diff(a, b, DEFS_TOL)
        if why:
            report.append("defs: symbol %s: %s" % (sid, why))
            bad += 1
    for sid in mine["symbols"]:
        if sid not in ref["symbols"]:
            report.append("defs: symbol %s is not the reference's" % sid)
            bad += 1
    for pid, (wh, leaves) in ref["patterns"].items():
        other = mine["patterns"].get(pid)
        if other is None or other[0] != wh or len(other[1]) != len(leaves) or \
                any(leaf_diff(a, b, DEFS_TOL) for a, b in zip(leaves, other[1])):
            report.append("defs: pattern %s differs" % pid)
            bad += 1
    names = list(ref["layers"]) + [n for n in mine["layers"] if n not in ref["layers"]]
    summary = []
    for name in names:
        r = ref["layers"].get(name, [])
        m = mine["layers"].get(name, [])
        diffs, missing, extra = compare_layer(name, r, m, size, report)
        if missing:
            report.append("  %s: %d reference elements missing from MINE" % (name, missing))
        if extra:
            report.append("  %s: %d extra elements in MINE" % (name, extra))
        bad += diffs + (1 if missing else 0) + (1 if extra else 0)
        summary.append("%-11s ref %5d  mine %5d  differing %5d  missing %5d  extra %5d" % (name, len(r), len(m), diffs, missing, extra))
    print("%s vs %s" % (os.path.basename(ref_path), os.path.basename(mine_path)))
    print("\n".join(summary))
    if report:
        print("\n".join(report))
    print("EQUAL" if not bad else "DIFFERENT (%d)" % bad)
    return 0 if not bad else 1


# ---------------------------------------------------------------- reference data
def reference(sheet, out):
    import hexsheet2svg
    r = hexsheet2svg.Renderer(sheet)
    with open(out, "w", encoding="utf-8") as f:
        f.write(r.render())
    return 0


def centres(sheet, out):
    import hexsheet2svg
    root = etree.parse(sheet).getroot()
    with open(out, "w", encoding="utf-8") as f:
        for el in root.findall("grid"):
            g = hexsheet2svg.Grid(el)
            for (c, r), pid in g.cells.items():
                vals = list(g.centre(c, r))
                for p in g.polygon(c, r):
                    vals.extend(p)
                f.write(pid + " " + " ".join("%.6f" % v for v in vals) + "\n")
    return 0


def main(argv):
    if len(argv) == 4 and argv[1] == "reference":
        return reference(argv[2], argv[3])
    if len(argv) == 4 and argv[1] == "centres":
        return centres(argv[2], argv[3])
    if len(argv) == 5 and argv[1] == "compare":
        return compare(argv[2], argv[3], argv[4])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
