# Copyright Ben Paul Wise. All Rights Reserved.
"""chain_check.py -- reports on the chain-reading process's state files; never gates (exit 0 always).

    python chain_check.py MAP [--kind rail|river|border] [--diff]

For each kind (or the one named), reads work/MAP/chain/<kind>.json and reports:
  (a) every consecutive pair in a chain that is not adjacent (rail: the two hexes are grid neighbours;
      river and border: the two hexsides share a vertex) -- a recording error, fix before anything else;
  (b) every chain end with its recorded reason, "unexplained" first;
  (c) the pieces after joining the kind's chains wherever they share a hex or hexside, largest first, and
      every pair of piece ends at most one hex apart (the signature of a reading gap);
  (d) with --diff: every hexside or step present in work/MAP/catalogue/catalogue.json (the tile process)
      and absent from work/MAP/chain/catalogue.json (the chain process), and the reverse -- run
      chain2catalogue.py first, since this reads its output rather than re-deriving it. Written to
      work/MAP/chain/questions.json as [{"at", "kind", "in": "tile" | "chain"}].
"""
import os
import sys

import common as C
import chain2catalogue as G

sys.path.insert(0, os.path.join(C.XML_DIR, "tools"))
import network_check as N  # noqa: E402

KINDS = {"rail": "hexes", "river": "sides", "border": "sides"}


def load_chain(cfg, kind):
    path = C.work(cfg, "chain", kind + ".json")
    if not os.path.exists(path):
        raise ValueError("%s: no chain file for kind '%s'" % (path, kind))
    data = C.read_json(path)
    if kind != data.get("kind"):
        raise ValueError("%s: kind field is %r, not %r" % (path, data.get("kind"), kind))
    return data


def bad_pairs(grid, adapter, kind, key, data, emit):
    n = 0
    for ch in data["chains"]:
        items = ch[key]
        for a, b in zip(items, items[1:]):
            ok = C.step_dir(grid, a, b) is not None if "hexes" == key else adjacent_sides(adapter, grid, a, b)
            if not ok:
                emit("RECORDING ERROR: %s chain %s: %s and %s are not adjacent" % (kind, ch["id"], a, b))
                n += 1
    return n


def adjacent_sides(adapter, grid, a, b):
    ctx = "chain_check"
    ka = C.canonical_side(grid, a, ctx)
    kb = C.canonical_side(grid, b, ctx)
    va = set(adapter.side_ends[adapter.name_key[ka]])
    vb = set(adapter.side_ends[adapter.name_key[kb]])
    return bool(va & vb)


def report_ends(kind, data, emit):
    ends = [(ch["id"], e) for ch in data["chains"] for e in ch["ends"]]
    ends.sort(key=lambda ie: (0 if "unexplained" == ie[1]["reason"] else 1, ie[1]["reason"], ie[1]["at"]))
    for cid, e in ends:
        emit("%s chain %s end %-16s reason %s" % (kind, cid, e["at"], e["reason"]))
    return [e["at"] for _, e in ends]


def hex_pieces(adapter, data):
    """(hexes, ends) per piece, ends being hex ids -- network_check.link_pieces's own shape."""
    adj = {}
    for ch in data["chains"]:
        hexes = ch["hexes"]
        for a, b in zip(hexes, hexes[1:]):
            adj.setdefault(a, set()).add(b)
            adj.setdefault(b, set()).add(a)
    return N.link_pieces(adapter, adj)


def side_pieces(grid, adapter, data):
    """(side-keys, ends) per piece, ends being VERTEX keys -- network_check.side_pieces's own shape."""
    names = {tok for ch in data["chains"] for tok in ch["sides"]}
    keys = {adapter.name_key[C.canonical_side(grid, name, "chain_check")] for name in names}
    return N.side_pieces(adapter, keys)


def report_pieces(grid, adapter, kind, key, data, emit):
    """Reports the pieces, largest first, and every pair of ends at most one hex apart. An end is a hex id
    (rail) or a vertex key (river, border); a vertex is named by one of its piece's own hexside names
    (network_check.vertex_name), since no hexside name belongs to it alone."""
    is_side = "sides" == key
    pieces = side_pieces(grid, adapter, data) if is_side else hex_pieces(adapter, data)
    pieces = sorted(pieces, key=lambda pe: -len(pe[0]))
    label, point = {}, {}
    for piece, ends in pieces:
        for e in ends:
            if is_side:
                label[e] = N.vertex_name(adapter, e, piece)
                point[e] = (e[0] / 2.0, e[1] / 2.0)
            else:
                label[e] = e
                point[e] = grid.centre(*grid.ids[e])
        emit("%s piece %4d %-6s ends %s" % (kind, len(piece), key, " ".join(label[e] for e in ends)))
    span = C.spacing(grid)
    all_ends = [e for _, ends in pieces for e in ends]
    for i, a in enumerate(all_ends):
        for b in all_ends[i + 1:]:
            if a == b:
                continue
            pa, pb = point[a], point[b]
            d = ((pa[0] - pb[0]) ** 2 + (pa[1] - pb[1]) ** 2) ** 0.5
            if d <= span * 1.05:
                emit("%s ends within one hex: %s / %s" % (kind, label[a], label[b]))
    return len(pieces)


def diff_report(cfg, kinds, emit):
    tile = C.read_json(C.work(cfg, "catalogue", "catalogue.json"))
    # Built afresh from the chain files every time, so the diff can never read a stale catalogue.
    chain = G.build_catalogue(cfg, C.make_grid(cfg, C.load_fit(cfg)))
    questions = []
    for kind in kinds:
        cat_key = G.catalogue_key(kind) if kind in G.KINDS_SIDE else kind
        tile_set, chain_set = set(tile.get(cat_key, [])), set(chain.get(cat_key, []))
        for at in sorted(tile_set - chain_set):
            emit("diff %s: %s in the tile catalogue only" % (kind, at))
            questions.append({"at": at, "kind": kind, "in": "tile"})
        for at in sorted(chain_set - tile_set):
            emit("diff %s: %s in the chain catalogue only" % (kind, at))
            questions.append({"at": at, "kind": kind, "in": "chain"})
    C.write_json(C.work(cfg, "chain", "questions.json"), questions)
    emit("questions: %d" % len(questions))
    return questions


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    cfg = C.load_config(argv[1])
    grid = C.make_grid(cfg, C.load_fit(cfg))
    adapter = G.GridAdapter(grid)
    kinds = [argv[argv.index("--kind") + 1]] if "--kind" in argv else list(KINDS)
    for kind in kinds:
        if kind not in KINDS:
            raise ValueError("chain_check: unknown kind %r (have %s)" % (kind, ", ".join(KINDS)))
    total_bad, total_pieces = 0, 0
    for kind in kinds:
        data = load_chain(cfg, kind)
        key = KINDS[kind]
        total_bad += bad_pairs(grid, adapter, kind, key, data, print)
        report_ends(kind, data, print)
        total_pieces += report_pieces(grid, adapter, kind, key, data, print)
    if "--diff" in argv:
        diff_report(cfg, kinds, print)
    print("chain_check: %d non-adjacent pairs, %d pieces" % (total_bad, total_pieces))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
