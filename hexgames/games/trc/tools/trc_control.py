# Copyright Ben Paul Wise. All Rights Reserved.
"""trc_control.py -- regenerate the rail control list of games/trc/scenario/trc-1941.xml from the sheet.

    python trc_control.py SHEET.xml SCENARIO.xml            compare with the scenario's current list
    python trc_control.py SHEET.xml SCENARIO.xml --write    replace the list in the scenario

The rule, as the scenario's notes state it: every hex takes the side of its nearest anchor city by hop
distance over the whole grid (Axis anchors win ties), unless the scenario's own <control><hex> names its
side; a rail link whose two hexes have the same side starts under that side. Written for M6c, when the
map networks were re-authored; rerun it whenever the TRC sheet's rail changes, then re-record the TRC
goldens. It reads the sheet with the reference renderer's Grid, so it follows the sheet's geometry
exactly and depends on no other tool.
"""
import collections
import os
import re
import sys

from lxml import etree

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "..", "map_graphics", "xml"))
import hexsheet2svg as H  # noqa: E402

AXIS = ["BERLIN", "Posen", "Breslau", "WARSAW", "Königsberg", "BUCHAREST", "Ploesti Oil Fields", "HELSINKI"]
RUSSIAN = ["RIGA", "Kaunas", "Brest", "Lvov", "MINSK", "Tallinn", "LENINGRAD", "KIEV", "ODESSA", "Vitebsk", "MOSCOW"]
LINK = '<link network="rail"'


class Sheet:
    """The hexes of a sheet and their neighbours, by the renderer's grid geometry."""

    def __init__(self, path):
        self.root = etree.parse(path).getroot()
        self.cell = {}
        for g in (H.Grid(el) for el in self.root.findall("grid")):
            for pid, cr in g.ids.items():
                self.cell[pid] = (g, cr)

    def neighbours(self, pid):
        g, (c, r) = self.cell[pid]
        for d in g.geom["nbr_plain"]:
            nb = g.neighbour(c, r, d)
            if nb is not None and nb in g.cells:
                yield d, g.cells[nb]


def hops_from(sheet, start):
    dist = {start: 0}
    queue = collections.deque([start])
    while queue:
        h = queue.popleft()
        for _, n in sheet.neighbours(h):
            if n not in dist:
                dist[n] = dist[h] + 1
                queue.append(n)
    return dist


def sides(sheet, scenario_text):
    names = {h.get("name"): h.get("id") for h in sheet.root.findall("hex") if h.get("name")}
    missing = [n for n in AXIS + RUSSIAN if n not in names]
    if missing:
        raise SystemExit("anchor cities not named on the sheet: " + ", ".join(missing))
    anchors = [(names[n], "axis") for n in AXIS] + [(names[n], "russian") for n in RUSSIAN]
    dists = [hops_from(sheet, a) for a, _ in anchors]
    fixed = dict(re.findall(r'<hex id="([A-Z]+[0-9]+)" side="([a-z]+)"/>', scenario_text))
    return {h: fixed.get(h, anchors[min(range(len(anchors)), key=lambda k: (dists[k][h], k))][1])
            for h in sheet.cell}


def links(sheet, side, indent):
    out, seen = [], set()
    for lk in sheet.root.findall("link"):
        if lk.get("kind") != "rail":
            continue
        hexes = lk.get("hexes").split()
        for a, b in zip(hexes, hexes[1:]):
            if side[a] == side[b] and frozenset((a, b)) not in seen:
                seen.add(frozenset((a, b)))
                out.append('%s%s hexes="%s %s" side="%s"/>' % (indent, LINK, a, b, side[a]))
    return out


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    sheet = Sheet(argv[1])
    text = open(argv[2], encoding="utf-8").read()
    lines = text.split("\n")
    current = [l for l in lines if LINK in l]
    if not current:
        raise SystemExit("no rail control list in " + argv[2])
    indent = current[0][:len(current[0]) - len(current[0].lstrip())]
    new = links(sheet, sides(sheet, text), indent)
    print("generated %d, scenario %d, identical %s; %d kept, %d dropped, %d added" % (
        len(new), len(current), new == current, len(set(new) & set(current)),
        len(set(current) - set(new)), len(set(new) - set(current))))
    if "--write" in argv:
        first = next(i for i, l in enumerate(lines) if LINK in l)
        lines = [l for l in lines if LINK not in l]
        lines[first:first] = new
        with open(argv[2], "w", encoding="utf-8", newline="\n") as f:
            f.write("\n".join(lines))
        print("wrote", argv[2])
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
