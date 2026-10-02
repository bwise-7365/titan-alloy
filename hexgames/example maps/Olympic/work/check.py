# Copyright Ben Paul Wise. All Rights Reserved.
"""Check road chains: python check.py out/roads-west.txt
Each non-comment line: `road: 1003 1104 1203 | ends: place unexplained` (hex ids in order).
Reports any step between hexes that are not neighbours, and ids not on the lattice."""
import sys, os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
from reg import G, ALL


def neighbours(h):
    c, r = ALL[h]
    out = set()
    for d in ("n", "ne", "se", "s", "sw", "nw"):
        q = G.neighbour(c, r, d)
        if q in G.cells or (0 <= q[0] < G.cols and 0 <= q[1] < G.rows):
            out.add(G.make_id(*q))
    return out


bad = 0
for n, line in enumerate(open(sys.argv[1]), 1):
    line = line.split("#")[0].strip()
    if not line.startswith("road:"):
        continue
    hexes = line[5:].split("|")[0].split()
    for h in hexes:
        if h not in ALL:
            print(f"line {n}: {h} is not a lattice hex"); bad += 1
    for a, b in zip(hexes, hexes[1:]):
        if a in ALL and b in ALL and b not in neighbours(a):
            print(f"line {n}: {a} -> {b} not adjacent"); bad += 1
print("OK" if not bad else f"{bad} problems")
# Copyright Ben Paul Wise. All Rights Reserved.
