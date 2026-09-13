# Copyright Ben Paul Wise. All Rights Reserved.
"""reference-centres.py -- print the reference hex centres that GridTest compares against.

The renderer is the specification, so the expected pixels of GridTest.PixelMatchesRendererFourSheets
come from hexsheet2svg.py itself: this script loads its Grid class, walks the <grid> elements of the
four sheets, and prints the sampled centres as C++ initialisers to paste into GridTest.cpp.

    python reference-centres.py
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
XML = os.path.normpath(os.path.join(HERE, "..", "..", "map_graphics", "xml"))
sys.path.insert(0, XML)

from lxml import etree  # noqa: E402  (the renderer's own parser, found beside it)

import hexsheet2svg  # noqa: E402

SAMPLES = [
    ("the-russian-campaign.xml", "main", ["A33", "KK19", "KK20", "QQ1"]),
    ("d-day-at-tarawa.xml", "main", ["2336", "2502", "0143"]),
    ("dai-senso.xml", "west", ["w5227", "w6101", "w5201"]),
    ("dai-senso.xml", "east", ["e3411", "e5201", "e6101"]),
    ("panzergruppe-guderian.xml", "main", ["0101", "0109", "5631"]),
]


def main():
    for filename, grid_id, ids in SAMPLES:
        tree = etree.parse(os.path.join(XML, filename))
        el = [g for g in tree.getroot().iter("grid") if g.get("id") == grid_id][0]
        grid = hexsheet2svg.Grid(el)
        print("// %s, grid '%s'" % (filename, grid_id))
        for pid in ids:
            cell = grid.ids.get(pid)
            if cell is None:
                print('    {"%s", 0, 0, 0, 0},  // NOT FOUND' % pid)
                continue
            c, r = cell
            x, y = grid.centre(c, r)
            print('    {"%s", %d, %d, %.6f, %.6f},' % (pid, c, r, x, y))
    return 0


if __name__ == "__main__":
    sys.exit(main())
# Copyright Ben Paul Wise. All Rights Reserved.
