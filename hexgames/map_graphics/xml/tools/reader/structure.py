# Copyright Ben Paul Wise. All Rights Reserved.
"""structure.py -- from cell scores to structure: terrain per hex, chains per line kind with end
reasons, places, doubts (B4). Ranks, never invents: nothing is written without a score behind it.

    python structure.py DIR [--high 0.6] [--low 0.3] [--rail-high 0.4] [--rail-low 0.2]
                            [--oracle SHEET.xml] [--overlay]

Reads DIR/cells.json and DIR/lattice.json, writes DIR/chain/terrain.json, DIR/chain/<kind>.json for
every side-along and side-across kind in chain2catalogue.py's schema, DIR/chain/places.json in
merge.py's shape, DIR/chain/doubts.json, and a report. Chains are grown by hysteresis: a hexside with a
score of at least --high seeds a chain, which continues through hexsides scoring at least --low that
share a vertex with it (rail: that share a hex), and splits at every junction. Ends carry a reason from
the EndReason set: edge (the chain reaches the map edge), sea (a river meets sea terrain), junction,
place (a rail ends at a city or port), or unexplained. --oracle compares the chains with an existing
sheet's edges and links, kind by kind. --overlay draws the chains and their end reasons on the scan.
"""
import collections
import json
import math
import os
import re
import sys
import xml.etree.ElementTree as ET

import numpy as np
from PIL import Image, ImageDraw
from scipy.spatial import cKDTree

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lattice as L
from legend import option
from cells import DIRS, _normal, _side_ends

RED = {"border", "front"}  # kinds the oracle compare unites (both are the sheet's border line)


# ---------------------------------------------------------------- geometry from the lattice


class Lattice:
    """Printed cells, their hexsides named as cells.py names them, hexside vertices keyed so that
    hexsides sharing a corner share a key, and the map-edge hexsides."""

    def __init__(self, d):
        size, ori, rot, spacing = d["size"], d["orientation"], float(d.get("rotation_deg", 0.0)), d["spacing"]
        printed = set(d["printed"]["cells"])
        cells = {tuple(int(t) for t in k[1:].split("r")): (v[0], v[1]) for k, v in d["grown"]["cells"].items() if k in printed}
        cols = max(c for c, _ in cells) + 1
        rows = max(r for _, r in cells) + 1
        keys = list(cells)
        self.label = {k: L.cell_label(k, d.get("numbering"), cols, rows) for k in keys}
        keys = [k for k in keys if "?" != self.label[k]]  # cells the numbering cannot name are off the printed grid
        self.centre = {self.label[k]: cells[k] for k in keys}
        tree = cKDTree([cells[k] for k in keys])
        self.size, self.ori, self.rot = size, ori, rot
        self.sides = {}  # name -> (hex a, hex b or None, direction from a, vertex keys)
        self.by_hexdir = {}
        self.neighbours = collections.defaultdict(set)
        q = max(2.0, size / 4)
        for k in keys:
            x, y = cells[k]
            for s in range(6):
                nx, ny = _normal(rot, ori, s)
                dist, j = tree.query((x + nx * spacing, y + ny * spacing))
                other = self.label[keys[j]] if dist < 0.3 * spacing else None
                me = self.label[k]
                name = "%s:%s" % (me, DIRS[ori][s]) if other is None else "-".join(sorted((me, other)))
                self.by_hexdir[(me, DIRS[ori][s])] = name
                if name in self.sides:
                    continue
                (x0, y0), (x1, y1) = _side_ends(size, rot, ori, s)
                va = (round((x + x0) / q), round((y + y0) / q))
                vb = (round((x + x1) / q), round((y + y1) / q))
                self.sides[name] = (me, other, DIRS[ori][s], (va, vb))
                if other is not None:
                    self.neighbours[me].add(other)
                    self.neighbours[other].add(me)

    def edge_side_p(self, name):
        return self.sides[name][1] is None

    def hexes_of(self, name):
        a, b, _, _ = self.sides[name]
        return [h for h in (a, b) if h is not None]


# ---------------------------------------------------------------- chains by hysteresis


def accepted(scores, high, low):
    """Hexsides in chains: seeds at or above high, grown through neighbours at or above low."""
    return {n for n, v in scores.items() if v >= high}, {n for n, v in scores.items() if v >= low}


def grow(seeds, candidates, adjacent):
    out = set(seeds)
    frontier = list(seeds)
    while frontier:
        n = frontier.pop()
        for m in adjacent(n):
            if m in candidates and m not in out:
                out.add(m)
                frontier.append(m)
    return out


def side_adjacency(lat, names):
    """Hexsides sharing a vertex, among the given names."""
    by_vertex = collections.defaultdict(set)
    for n in names:
        for v in lat.sides[n][3]:
            by_vertex[v].add(n)
    return by_vertex


def chains_of(nodes_of, links, degree_split=True):
    """Ordered chains through a graph given as link -> (node, node): connected pieces split at every
    node whose degree is not 2 (mirrors chain2catalogue.graph_chains)."""
    at = collections.defaultdict(list)
    for link, (a, b) in links.items():
        at[a].append(link)
        at[b].append(link)
    seen = set()
    chains = []

    def walk(link, node):
        chain = [link]
        seen.add(link)
        while True:
            nxt = [l for l in at[node] if l not in seen]
            if 2 != len(at[node]) or not nxt:
                return chain, node
            link = nxt[0]
            seen.add(link)
            chain.append(link)
            a, b = links[link]
            node = b if a == node else a

    starts = sorted((l for l in links if any(2 != len(at[n]) for n in links[l])), key=str)
    for link in starts:
        if link in seen:
            continue
        a, b = links[link]
        start = a if 2 != len(at[a]) else b
        chain, end = walk(link, b if start == a else a)
        chains.append((chain, start, end))
    for link in sorted(links, key=str):  # closed loops
        if link not in seen:
            a, b = links[link]
            chain, end = walk(link, b)
            chains.append((chain, a, end))
    return chains, at


# ---------------------------------------------------------------- structural rules


def drop_lone(rows, member, kind, doubts):
    """A lone step or hexside with both ends unexplained is not a line: a line kind is something that
    goes somewhere. It is recorded as a doubt, not silently removed."""
    kept = []
    for r in rows:
        length = len(r[member]) - (1 if "hexes" == member else 0)
        if 1 == length and all("unexplained" == e["reason"] for e in r["ends"]):
            doubts.append({"at": r[member][0] if "sides" == member else "-".join(r["hexes"]), "kind": kind, "chose": "none",
                           "alternatives": [kind], "why": "a lone %s with both ends unexplained" % ("hexside" if "sides" == member else "step"),
                           "look": None, "stage": "B4-" + kind})
        else:
            kept.append(r)
    for i, r in enumerate(kept, 1):
        r["id"] = i
    return kept


def rail_rules(lat, chosen, places, kind, doubts):
    """Steps that survive two structural rules: no hex carries four or more steps (a star of false
    spokes over mottle or text is not a junction), and a connected piece of fewer than three steps that
    reaches neither a place nor the map edge goes nowhere. Both are recorded as doubts."""
    degree = collections.Counter(h for n in chosen for h in lat.hexes_of(n))
    stars = {h for h, k in degree.items() if k >= 4}
    kept = set()
    for n in chosen:
        if any(h in stars for h in lat.hexes_of(n)):
            doubts.append({"at": n, "kind": kind, "chose": "none", "alternatives": [kind], "look": None, "stage": "B4-" + kind,
                           "why": "one of %d steps at hex %s; four or more is a false star" % (max(degree[h] for h in lat.hexes_of(n)), max(lat.hexes_of(n), key=degree.get))})
        else:
            kept.add(n)
    by_hex = collections.defaultdict(set)
    for n in kept:
        for h in lat.hexes_of(n):
            by_hex[h].add(n)
    seen = set()
    out = set()
    for n in sorted(kept):
        if n in seen:
            continue
        comp, frontier = {n}, [n]
        while frontier:
            m = frontier.pop()
            for h in lat.hexes_of(m):
                for q in by_hex[h]:
                    if q not in comp:
                        comp.add(q)
                        frontier.append(q)
        seen |= comp
        hexes = {h for m in comp for h in lat.hexes_of(m)}
        anchored = any(h in places for h in hexes) or any(lat.edge_side_p(lat.by_hexdir[(h, d)]) for h in hexes for d in DIRS[lat.ori])
        if len(comp) >= 3 or anchored:
            out |= comp
        else:
            for m in comp:
                doubts.append({"at": m, "kind": kind, "chose": "none", "alternatives": [kind], "look": None, "stage": "B4-" + kind,
                               "why": "piece of %d step(s) reaching no place and no map edge" % len(comp)})
    return out


# ---------------------------------------------------------------- end reasons


def side_end_reason(lat, name, vertex, at, terrain, kind):
    """Why a side chain stops at this hexside: edge, sea (river beside sea terrain), junction, else
    unexplained."""
    if len(at[vertex]) > 2:
        return "junction"
    if lat.edge_side_p(name) or any(lat.edge_side_p(s) for s in lat.sides if vertex in lat.sides[s][3] and lat.edge_side_p(s)):
        return "edge"
    if "river" == kind and any("sea" == terrain.get(h) for h in lat.hexes_of(name)):
        return "sea"
    return "unexplained"


def hex_end_reason(lat, hex_id, at, places):
    if len(at[hex_id]) > 2:
        return "junction"
    if hex_id in places:
        return "place"
    if any(lat.edge_side_p(lat.by_hexdir[(hex_id, d)]) for d in DIRS[lat.ori]):
        return "edge"
    return "unexplained"


# ---------------------------------------------------------------- main


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    folder = argv[1]
    high, low = float(option(argv, "--high", 0.6)), float(option(argv, "--low", 0.3))
    rail_high, rail_low = float(option(argv, "--rail-high", 0.4)), float(option(argv, "--rail-low", 0.2))
    with open(os.path.join(folder, "lattice.json"), encoding="utf-8") as fh:
        d = json.load(fh)
    with open(os.path.join(folder, "cells.json"), encoding="utf-8") as fh:
        c = json.load(fh)
    lat = Lattice(d)
    c["hex"] = {h: r for h, r in c["hex"].items() if "?" != h}
    c["side"] = {n: r for n, r in c["side"].items() if "?" not in n}  # cells off the printed numbering
    out = os.path.join(folder, "chain")
    os.makedirs(out, exist_ok=True)

    # terrain: the best fill, or the default when nothing scores; doubt = runner-up share
    default = option(argv, "--default", "clear")
    terrain, doubts = {}, []
    for h, row in c["hex"].items():
        fills = row["hex"]
        if not fills or max(fills.values()) < 0.3:
            terrain[h] = default
            continue
        best = max(fills, key=fills.get)
        terrain[h] = best
        rest = sorted(fills.values(), reverse=True)
        if len(rest) > 1 and rest[0] > 0 and rest[1] / rest[0] > 0.7:
            runner = [k for k in fills if k != best and fills[k] == rest[1]][0]
            doubts.append({"at": h, "kind": "terrain", "chose": best, "alternatives": [runner],
                           "why": "runner-up scores %.2f against %.2f" % (rest[1], rest[0]), "look": None, "stage": "B4-terrain"})

    # places: glyphs scoring, in merge.py's shape
    places = {}
    for h, row in c["hex"].items():
        for sheet, v in row["glyph"].items():
            if v >= 0.5 and sheet in ("city", "port", "town"):
                places.setdefault(h, {"glyph": sheet})  # no name, no vp: the label pass is not written yet and nothing is invented
    markers = [{"hex": h, "glyph": sheet} for h, row in c["hex"].items() for sheet, v in row["glyph"].items() if v >= 0.5 and sheet not in ("city", "port", "town")]

    report = {}
    written = {}
    # side-along kinds: chains of hexsides
    along_kinds = sorted({k for s in c["side"].values() for k in s["along"]})
    for kind in along_kinds:
        scores = {n: s["along"].get(kind, 0.0) for n, s in c["side"].items()}
        seeds, cands = accepted(scores, high, low)
        by_vertex = side_adjacency(lat, cands)
        chosen = grow(seeds, cands, lambda n: {m for v in lat.sides[n][3] for m in by_vertex[v]})
        links = {n: lat.sides[n][3] for n in chosen}
        chains, at = chains_of(None, links)
        rows = []
        for i, (chain, start, end) in enumerate(chains, 1):
            rows.append({"id": i, "sides": chain,
                         "ends": [{"at": chain[0], "reason": side_end_reason(lat, chain[0], start, at, terrain, kind), "look": None},
                                  {"at": chain[-1], "reason": side_end_reason(lat, chain[-1], end, at, terrain, kind), "look": None}],
                         "seen": True})
        written[kind] = rows
        rows = drop_lone(rows, "sides", kind, doubts)
        written[kind] = rows
        for n, v in scores.items():
            if low <= v < high and n not in chosen:
                doubts.append({"at": n, "kind": kind, "chose": "none", "alternatives": [kind],
                               "why": "scores %.2f, middle band, no chain reached it" % v, "look": None, "stage": "B4-" + kind})
        report[kind] = (sum(len(r["sides"]) for r in rows), len(rows), collections.Counter(e["reason"] for r in rows for e in r["ends"]))
    # side-across kinds: chains of hexes
    across_kinds = sorted({k for s in c["side"].values() for k in s["across"]})
    for kind in across_kinds:
        scores = {n: s["across"].get(kind, 0.0) for n, s in c["side"].items() if not lat.edge_side_p(n)}
        seeds, cands = accepted(scores, rail_high, rail_low)
        by_hex = collections.defaultdict(set)
        for n in cands:
            for h in lat.hexes_of(n):
                by_hex[h].add(n)
        chosen = grow(seeds, cands, lambda n: {m for h in lat.hexes_of(n) for m in by_hex[h]})
        chosen = rail_rules(lat, chosen, places, kind, doubts)
        links = {n: tuple(lat.hexes_of(n)) for n in chosen}
        chains, at = chains_of(None, links)
        rows = []
        for i, (chain, start, end) in enumerate(chains, 1):
            hexes = [start]
            node = start
            for n in chain:
                a, b = links[n]
                node = b if a == node else a
                hexes.append(node)
            rows.append({"id": i, "hexes": hexes,
                         "ends": [{"at": hexes[0], "reason": hex_end_reason(lat, hexes[0], at, places), "look": None},
                                  {"at": hexes[-1], "reason": hex_end_reason(lat, hexes[-1], at, places), "look": None}],
                         "seen": True})
        written[kind] = rows
        rows = drop_lone(rows, "hexes", kind, doubts)
        written[kind] = rows
        for n, v in scores.items():
            if rail_low <= v < rail_high and n not in chosen:
                doubts.append({"at": n, "kind": kind, "chose": "none", "alternatives": [kind],
                               "why": "scores %.2f, middle band, no chain reached it" % v, "look": None, "stage": "B4-" + kind})
        report[kind] = (sum(len(r["hexes"]) - 1 for r in rows), len(rows), collections.Counter(e["reason"] for r in rows for e in r["ends"]))

    with open(os.path.join(out, "terrain.json"), "w", encoding="utf-8") as fh:
        json.dump(terrain, fh, indent=0)
    with open(os.path.join(out, "places.json"), "w", encoding="utf-8") as fh:
        json.dump({"places": places, "markers": []}, fh, indent=1)
    with open(os.path.join(out, "glyphs.json"), "w", encoding="utf-8") as fh:
        json.dump(markers, fh, indent=1)
    for kind, rows in written.items():
        with open(os.path.join(out, kind + ".json"), "w", encoding="utf-8") as fh:
            json.dump({"kind": kind, "chains": rows, "todo": [], "done": [], "notes": ["structure.py from cells.json; thresholds high %.2f low %.2f (rail %.2f %.2f)" % (high, low, rail_high, rail_low)]}, fh, indent=1)
    with open(os.path.join(out, "doubts.json"), "w", encoding="utf-8") as fh:
        json.dump(doubts, fh, indent=1)

    name = os.path.basename(os.path.normpath(folder))
    print("%s: terrain %s; places %d, markers %d; doubts %d" % (
        name, dict(collections.Counter(terrain.values())), len(places), len(markers), len(doubts)))
    for kind, (n, chains, reasons) in report.items():
        print("  %-8s %3d hexsides/steps in %2d chains; ends %s" % (kind, n, chains, dict(reasons)))
    if "--oracle" in argv:
        oracle(option(argv, "--oracle"), lat, written, terrain)
    if "--overlay" in argv:
        overlay(d, lat, written, terrain, os.path.join(folder, "structure-overlay.jpg"))
    return 0


# ---------------------------------------------------------------- oracle and overlay


def oracle(sheet_path, lat, written, terrain):
    """Compare with an existing sheet: hexsides per line kind, rail steps, terrain per hex."""
    root = ET.parse(sheet_path).getroot()
    ns = {"h": root.tag.split("}")[0].strip("{")} if "}" in root.tag else {}
    tag = lambda t: ("{%s}%s" % (ns["h"], t)) if ns else t
    sheet_sides = collections.defaultdict(set)
    for e in root.iter(tag("edge")):
        hex_id, dr = e.get("at").split(":")
        name = lat.by_hexdir.get((hex_id, dr))
        if name:
            sheet_sides[e.get("line")].add(name)
    sheet_steps = set()
    for l in root.iter(tag("link")):
        hs = l.get("hexes").split()
        for a, b in zip(hs, hs[1:]):
            sheet_steps.add("-".join(sorted((a, b))))
    sheet_terrain = {}
    for hx in root.iter(tag("hexes")):
        for h in hx.get("ids").split():
            sheet_terrain[h] = hx.get("terrain")
    mine_sides = collections.defaultdict(set)
    for kind, rows in written.items():
        for r in rows:
            if "sides" in r:
                mine_sides["border" if kind in RED else kind].update(r["sides"])
            else:
                mine_sides[kind].update("-".join(sorted(p)) for p in zip(r["hexes"], r["hexes"][1:]))
    print("  oracle %s:" % os.path.basename(sheet_path))
    for kind in sorted(set(sheet_sides) | set(mine_sides)):
        want = sheet_sides.get(kind, sheet_steps if "rail" == kind else set())
        have = mine_sides.get(kind, set())
        hit = len(want & have)
        print("    %-8s sheet %3d  read %3d  both %3d  missed %3d  extra %3d" % (kind, len(want), len(have), hit, len(want - have), len(have - want)))
    both = [h for h in terrain if h in sheet_terrain]
    agree = sum(1 for h in both if terrain[h] == sheet_terrain[h])
    conf = collections.Counter((sheet_terrain[h], terrain[h]) for h in both if terrain[h] != sheet_terrain[h])
    print("    terrain agree %d of %d; disagreements (sheet, read) %s" % (agree, len(both), dict(conf.most_common(6))))


def overlay(d, lat, written, terrain, path):
    im = Image.open(d["source"]).convert("RGB")
    dr = ImageDraw.Draw(im)
    colours = {"river": (0, 90, 255), "river-major": (0, 40, 160), "border": (200, 0, 0), "front": (255, 120, 0),
               "rail": (0, 0, 0), "rail-double": (0, 0, 0), "blocked": (30, 30, 30), "road": (90, 90, 90)}
    letters = {"edge": "E", "sea": "S", "junction": "J", "place": "P", "unexplained": "?"}
    for kind, rows in written.items():
        col = colours.get(kind, (255, 0, 255))
        for r in rows:
            if "sides" in r:
                for n in r["sides"]:
                    a, _, dname, _ = lat.sides[n]
                    x, y = lat.centre[a]
                    s = DIRS[lat.ori].index(dname)
                    (x0, y0), (x1, y1) = _side_ends(lat.size, lat.rot, lat.ori, s)
                    dr.line((x + x0, y + y0, x + x1, y + y1), fill=col, width=4)
                pts = [lat.centre[lat.hexes_of(r["sides"][i])[0]] for i in (0, -1)]
            else:
                pts = [lat.centre[h] for h in r["hexes"]]
                dr.line([tuple(p) for p in pts], fill=col, width=3)
                pts = [pts[0], pts[-1]]
            for (x, y), e in zip(pts, r["ends"]):
                if "junction" != e["reason"]:
                    dr.text((x - 4, y - 6), letters[e["reason"]], fill=col)
    im.save(path, quality=85)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
