# Copyright Ben Paul Wise. All Rights Reserved.
"""Merge the three road strips into one road graph, split it into links, and print the <link> lines.

Each hex-to-hex step is taken from the strip that owns it (by the mean column of its two hexes:
west <= 20.75 < middle <= 40.75 < east), so the overlap columns are never read twice.
Ends: place (a named hex of the sheet), sea (a worker's `coast` end), junction (three or more
steps meet), otherwise unexplained."""
import re, sys, os
from collections import defaultdict
os.chdir(os.path.dirname(os.path.abspath(__file__)))
from reg import ALL

SHEET = "../../../map_graphics/xml/operation-olympic.xml"
PLACES = set(re.findall(r'<hex id="(\d{4})" name=', open(SHEET, encoding="utf-8").read()))
OWN = {"west": (0, 20.75), "middle": (20.75, 40.75), "east": (40.75, 99)}


# steps read only by a non-owning strip, confirmed by the coordinator on crops (2026-09-29)
SEAM = {frozenset(p.split("-")) for p in "2214-2215 2215-2116 1914-1915 1915-1916 1918-1919 "
        "4104-4203 3905-3906 4131-4031 3932-4031".split()}

# steps Ben removed on the physical map (2026-09-29): 2116 carries only 2016-2116-2215
REMOVE = {frozenset(p.split("-")) for p in "2214-2115 2115-2116".split()}


def col(h):
    return int(h[:2])


steps, coast, ends_said = set(), set(), {}
for strip, (lo, hi) in OWN.items():
    for line in open(f"out/roads-{strip}.txt", encoding="utf-8"):
        line = line.split("#")[0].strip()
        if not line.startswith("road:"):
            continue
        body, _, ends = line[5:].partition("|")
        hexes = body.split()
        tok = re.findall(r"[a-z]+", ends.replace("ends:", ""))
        if tok and tok[0] == "coast" and lo < col(hexes[0]) <= hi + 0.25:
            coast.add(hexes[0])
        if tok and tok[-1] == "coast" and lo < col(hexes[-1]) <= hi + 0.25:
            coast.add(hexes[-1])
        for a, b in zip(hexes, hexes[1:]):
            if lo < (col(a) + col(b)) / 2 <= hi or frozenset((a, b)) in SEAM:
                steps.add(frozenset((a, b)))

steps -= REMOVE
adj = defaultdict(set)
for s in steps:
    a, b = tuple(s)
    adj[a].add(b); adj[b].add(a)


def end_reason(h):
    if h in PLACES:
        return "place"
    if len(adj[h]) >= 3:
        return "junction"
    if h in coast:
        return "sea"
    return "unexplained"


used, links = set(), []
stops = [h for h in sorted(adj) if len(adj[h]) != 2 or h in PLACES]
for s in stops:
    for n in sorted(adj[s]):
        if frozenset((s, n)) in used:
            continue
        chain = [s, n]; used.add(frozenset((s, n)))
        while chain[-1] not in stops:
            nxt = [m for m in sorted(adj[chain[-1]]) if frozenset((chain[-1], m)) not in used]
            if not nxt:
                break
            used.add(frozenset((chain[-1], nxt[0]))); chain.append(nxt[0])
        links.append(chain)
for s in steps - used:                         # pure rings with no stop: split in two at opposite hexes
    a, b = tuple(s)
    chain = [a, b]; used.add(s)
    while True:
        nxt = [m for m in sorted(adj[chain[-1]]) if frozenset((chain[-1], m)) not in used]
        if not nxt:
            break
        used.add(frozenset((chain[-1], nxt[0]))); chain.append(nxt[0])
    k = len(chain) // 2
    links += [chain[:k + 1], chain[k:]]

# pieces (connected components)
seen, pieces = set(), []
for h in sorted(adj):
    if h in seen:
        continue
    comp, todo = set(), [h]
    while todo:
        x = todo.pop()
        if x not in comp:
            comp.add(x); todo += adj[x]
    seen |= comp; pieces.append(comp)

out = [f'  <link kind="road" line="road" hexes="{" ".join(c)}" ends="{end_reason(c[0])} {end_reason(c[-1])}"/>'
       for c in links]
open("out/road-links.xml", "w", encoding="utf-8").write("\n".join(out) + "\n")
print(len(steps), "steps,", len(links), "links,", len(pieces), "pieces:", sorted(len(p) for p in pieces))
print("unexplained ends:", sorted({h for c in links for h in (c[0], c[-1]) if end_reason(h) == "unexplained"}))
tri = sorted({tuple(sorted((a, b, c))) for a in adj for b in adj[a] for c in adj[b] if c != a and a in adj[c]})
print("triangles:", tri)
print("places not reached:", sorted(PLACES - set(adj)))
# Copyright Ben Paul Wise. All Rights Reserved.
