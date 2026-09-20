# Copyright Ben Paul Wise. All Rights Reserved.
"""chain2catalogue.py -- stage S/A of the chain reading process: the chain files <-> the catalogue schema.

    python chain2catalogue.py MAP
    python chain2catalogue.py MAP --from-catalogue

Default: reads work/MAP/chain/terrain.json ({"hex": "terrain"}), work/MAP/chain/places.json
({"places": {"hex": {"glyph", "name", "vp"}}, "markers": [...]}, merge.py's catalogue shapes for places and
markers, copied exactly) and one work/MAP/chain/<kind>.json per chain kind (rail: chains of "hexes"; river
and border: chains of "sides"), and writes work/MAP/chain/catalogue.json in EXACTLY merge.py's catalogue
schema, so that
    python assemble.py MAP --catalogue work/MAP/chain/catalogue.json --out work/MAP/chain/sheet.xml
runs unchanged. A step or hexside that leaves the map ("HEX:DIR") is kept as a HEX:DIR entry, as
assemble.py expects: for rail (a "hexes" chain) it comes from an end whose "at" is a "HEX:DIR" token,
whatever the end's "reason"; for river and border (a "sides" chain) it is already an ordinary member of
the chain's "sides" list.

--from-catalogue: the reverse, and the seed source for the chain-reading stages. Reads
work/MAP/catalogue/catalogue.json (the tile process's reading) and writes chain/terrain.json,
chain/places.json, and one chain file per kind (rail, river, border) whose chains are the connected
pieces of the catalogue's hexsides or steps -- each chain split at every junction, exactly the way
assemble.py's own chains() splits a link's steps -- every end given reason "unexplained" and look null
(this is a mechanical reversal, not a reading; the human reasons come from stage C). Also writes
chain/<kind>-seeds.json: every piece end, which is also every junction hex or hexside once a piece has
been split there.
"""
import collections
import os
import sys

import common as C

sys.path.insert(0, os.path.join(C.XML_DIR, "tools"))
import network_check as N  # noqa: E402

KINDS_HEX = ("rail",)
KINDS_SIDE = ("river", "border")


def catalogue_key(line):
    """merge.py calls the catalogue's river hexsides "rivers"; every other kind keeps its own name."""
    return "rivers" if "river" == line else line


def side_index(grid):
    """Every hexside of the grid (inner and map-edge), three ways: side-key -> (vertex-key, vertex-key);
    side-key -> common.py's canonical name; and that name -> the side-key. Built the way
    network_check.Sheet.build_sides does, for this one grid."""
    corners = N.Corners(grid.size)
    side_ends, side_ref, name_key = {}, {}, {}
    for pid, (c, r) in sorted(grid.ids.items(), key=lambda kv: kv[1]):
        for d in C.directions(grid):
            a, b = grid.edge_ends(c, r, d)
            va, vb = corners.key(a), corners.key(b)
            key = tuple(sorted((va, vb)))
            name = C.side_name(grid, pid, d)
            side_ends[key] = (va, vb)
            side_ref.setdefault(key, name)
            name_key[name] = key
    return side_ends, side_ref, name_key


class GridAdapter:
    """The attributes network_check.py's piece-joining functions need (side_ends, side_ref for
    side_pieces; order for link_pieces), built from common.py's grid instead of a parsed sheet."""

    def __init__(self, grid):
        self.grid = grid
        self.side_ends, self.side_ref, self.name_key = side_index(grid)

    def order(self, pid):
        return self.grid.ids[pid]


def graph_chains(edges):
    """Nodes joined by edges [(u, v), ...], as ordered walks split at every end and junction (any node whose
    degree is not 2); mirrors assemble.py's chains(), generalised to any hashable, orderable node -- a hex id,
    a hexside-vertex key, or (for a rail piece's off-map spur) an edge token."""
    adj = collections.defaultdict(list)
    for a, b in edges:
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
        for n in sorted(adj):
            if pass_ends and 2 == len(adj[n]):
                continue
            for m in sorted(adj[n]):
                if frozenset((n, m)) not in used:
                    out.append(walk(n, m))
    return out


# ---------------------------------------------------------------- default mode: chain files -> catalogue
def flatten_sides(cfg, grid, kind):
    data = C.read_json(C.work(cfg, "chain", kind + ".json"))
    if kind != data.get("kind"):
        raise ValueError("chain/%s.json: kind field is %r, not %r" % (kind, data.get("kind"), kind))
    names = set()
    for ch in data["chains"]:
        ctx = "chain/%s.json chain %s" % (kind, ch.get("id"))
        for tok in ch["sides"]:
            names.add(C.canonical_side(grid, tok, ctx))
    return sorted(names, key=lambda n: [grid.ids[h] for h in n.split(":")[0].split("-")])


def flatten_hexes(cfg, grid, kind):
    data = C.read_json(C.work(cfg, "chain", kind + ".json"))
    if kind != data.get("kind"):
        raise ValueError("chain/%s.json: kind field is %r, not %r" % (kind, data.get("kind"), kind))
    names = set()
    for ch in data["chains"]:
        ctx = "chain/%s.json chain %s" % (kind, ch.get("id"))
        hexes = ch["hexes"]
        for a, b in zip(hexes, hexes[1:]):
            names.add(C.canonical_side(grid, "%s-%s" % (a, b), ctx))
        for e in ch["ends"]:
            if ":" in e["at"]:
                names.add(C.canonical_side(grid, e["at"], ctx))
    return sorted(names, key=lambda n: [grid.ids[h] for h in n.split(":")[0].split("-")])


def build_catalogue(cfg, grid):
    terrain = C.read_json(C.work(cfg, "chain", "terrain.json"))
    voc_terrain = set(cfg["vocabulary"]["terrain"])
    missing = sorted(pid for pid in grid.ids if pid not in terrain)
    if missing:
        raise ValueError("chain/terrain.json: no terrain for %d hexes: %s" % (len(missing), " ".join(missing[:20])))
    bad = sorted({t for t in terrain.values() if t not in voc_terrain})
    if bad:
        raise ValueError("chain/terrain.json: terrain %s is not in the vocabulary" % bad[0])
    places_data = C.read_json(C.work(cfg, "chain", "places.json"))
    places, markers = places_data["places"], places_data["markers"]
    cat = {"hexes": {}, "markers": markers}
    for pid in sorted(grid.ids, key=lambda p: grid.ids[p]):
        entry = {"terrain": terrain[pid]}
        if pid in places:
            entry["place"] = places[pid]
        cat["hexes"][pid] = entry
    for line in cfg["vocabulary"]["hexside_lines"]:
        cat[catalogue_key(line)] = flatten_sides(cfg, grid, line) if line in KINDS_SIDE else []
    for kind in cfg["vocabulary"]["links"]:
        cat[kind] = flatten_hexes(cfg, grid, kind) if kind in KINDS_HEX else []
    return cat


# ---------------------------------------------------------------- --from-catalogue mode: catalogue -> chain files
def rail_pieces_from_catalogue(tokens):
    """Catalogue rail tokens ("A-B" steps, "HEX:DIR" edge crossings) as ordered chains of {"hexes", "ends"}. An
    edge token is a one-step spur off whichever hex it names, a leaf in the step graph by construction."""
    edges = []
    for tok in tokens:
        if ":" in tok:
            hexid = tok.split(":")[0]
            edges.append((hexid, tok))
        else:
            a, b = tok.split("-")
            edges.append((a, b))
    out = []
    for raw in graph_chains(edges):
        hexes = [n for n in raw if ":" not in n]
        if not hexes:
            raise ValueError("chain2catalogue --from-catalogue: rail piece %s has no real hex" % " ".join(raw))
        out.append({"hexes": hexes,
                    "ends": [{"at": raw[0], "reason": "unexplained", "look": None},
                             {"at": raw[-1], "reason": "unexplained", "look": None}]})
    return out


def side_pieces_from_catalogue(adapter, tokens):
    """Catalogue hexside tokens (inner "A-B" or map-edge "HEX:DIR", both real hexsides with real corner
    vertices) as ordered chains of {"sides", "ends"}, split at every vertex where more than two of the given
    hexsides meet."""
    edges = []
    edge_name = {}
    for name in tokens:
        if name not in adapter.name_key:
            raise ValueError("chain2catalogue --from-catalogue: %r is not a hexside of this grid" % name)
        va, vb = adapter.side_ends[adapter.name_key[name]]
        edges.append((va, vb))
        edge_name[frozenset((va, vb))] = name
    out = []
    for raw in graph_chains(edges):
        sides = [edge_name[frozenset((a, b))] for a, b in zip(raw, raw[1:])]
        out.append({"sides": sides,
                    "ends": [{"at": sides[0], "reason": "unexplained", "look": None},
                             {"at": sides[-1], "reason": "unexplained", "look": None}]})
    return out


def write_chain_file(cfg, kind, chains_list, key):
    ordered = [dict(id=i, **ch) for i, ch in enumerate(chains_list, 1)]
    done = sorted({h for ch in ordered for h in ch[key]})
    C.write_json(C.work(cfg, "chain", kind + ".json"),
                 {"kind": kind, "chains": ordered, "todo": [], "done": done, "notes": []})
    seeds = sorted({e["at"] for ch in ordered for e in ch["ends"]})
    C.write_json(C.work(cfg, "chain", kind + "-seeds.json"), seeds)
    return ordered


def reverse(cfg, grid):
    cat = C.read_json(C.work(cfg, "catalogue", "catalogue.json"))
    terrain = {pid: h["terrain"] for pid, h in cat["hexes"].items()}
    C.write_json(C.work(cfg, "chain", "terrain.json"), terrain)
    places = {pid: h["place"] for pid, h in cat["hexes"].items() if h.get("place")}
    C.write_json(C.work(cfg, "chain", "places.json"), {"places": places, "markers": cat.get("markers", [])})
    adapter = GridAdapter(grid)
    counts = {}
    for kind in KINDS_HEX:
        chains_list = rail_pieces_from_catalogue(cat.get(kind, []))
        counts[kind] = len(write_chain_file(cfg, kind, chains_list, "hexes"))
    for line in KINDS_SIDE:
        chains_list = side_pieces_from_catalogue(adapter, cat.get(catalogue_key(line), []))
        counts[line] = len(write_chain_file(cfg, line, chains_list, "sides"))
    print("wrote chain/terrain.json (%d hexes), chain/places.json (%d places, %d markers); chains %s" % (
        len(terrain), len(places), len(cat.get("markers", [])),
        "  ".join("%s %d" % (k, v) for k, v in counts.items())))
    return 0


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    cfg = C.load_config(argv[1])
    grid = C.make_grid(cfg, C.load_fit(cfg))
    if "--from-catalogue" in argv:
        return reverse(cfg, grid)
    cat = build_catalogue(cfg, grid)
    C.write_json(C.work(cfg, "chain", "catalogue.json"), cat)
    counts = {k: len(v) for k, v in cat.items() if k not in ("hexes", "markers")}
    print("wrote chain/catalogue.json: hexes %d; markers %d; %s" % (
        len(cat["hexes"]), len(cat["markers"]), "  ".join("%s %d" % (k, v) for k, v in counts.items())))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
