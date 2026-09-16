# Copyright Ben Paul Wise. All Rights Reserved.
"""merge.py -- stage 6: combine the tile records into one catalogue; the agreement gate.

    python merge.py MAP [--partial]

Reads every reader record work/MAP/catalogue/read/TILE.json (one per tile of the manifest; --partial
merges what exists and lists the missing tiles) and the hand-written resolutions
work/MAP/catalogue/resolutions.json. Checks each record (every hex of the tile's area has a terrain from
the vocabulary; every hexside token names neighbouring hexes, both inside the area). Then, for every hex
and every hexside read in more than one tile, compares the readings:
  terrain and place per hex; each hexside line kind (rivers ...) and link kind (rail, road ...) per hexside,
  where a tile whose area holds both hexes and does not list the hexside reads "absent".
Writes work/MAP/catalogue/catalogue.json (agreed features, resolutions applied) and
work/MAP/catalogue/disagreements.json (every disagreement and every reader "unclear" still open).

resolutions.json is a list of {"at": HEX or HEXSIDE, "feature": "terrain" | "place" | a line or link kind,
"value": the decided value (terrain id; place object or null; true or false for a hexside), "reason": what
the focused look showed, "look": the crop it came from, "exception": true for a named exception}.
GATE: no open disagreement and no open unclear. Exit 1 otherwise.
"""
import collections
import os
import sys

import common as C


def kinds(cfg):
    return ["rivers"] + [k for k in cfg["vocabulary"]["hexside_lines"] if "river" != k] + list(cfg["vocabulary"]["links"])


def feature_name(feature):
    """Readers and resolutions may say "river" (the line id) for the catalogue kind "rivers"."""
    return "rivers" if "river" == feature else feature


def check_record(cfg, grid, tile, rec):
    ctx = "tile %s" % tile["name"]
    area = set(tile["core"]) | set(tile["margin"])
    terrains = set(cfg["vocabulary"]["terrain"])
    glyphs = set(cfg["vocabulary"]["places"])
    missing = area - set(rec["hexes"])
    if missing:
        raise ValueError("%s: no terrain for %s" % (ctx, " ".join(sorted(missing))))
    for pid, h in rec["hexes"].items():
        if pid not in area:
            raise ValueError("%s: hex %s is not in the tile's area" % (ctx, pid))
        if h["terrain"] not in terrains:
            raise ValueError("%s: hex %s terrain '%s' is not in the vocabulary" % (ctx, pid, h["terrain"]))
        if h.get("place") and h["place"]["glyph"] not in glyphs:
            raise ValueError("%s: hex %s place glyph '%s' is not in the vocabulary" % (ctx, pid, h["place"]["glyph"]))
    sides = {}
    for kind in kinds(cfg):
        names = set()
        for token in rec.get(kind, []):
            name = C.canonical_side(grid, token, ctx)
            if not set(name.split(":")[0].split("-")) <= area:
                raise ValueError("%s: %s %s leaves the tile's area" % (ctx, kind, token))
            names.add(name)
        sides[kind] = names
    return area, sides


def status(cfg, grid, tiles):
    """--status: which tiles have a complete reader record, which records fail the check, which are missing.
    Run it before launching readers and after every batch, a drop or a usage limit."""
    folder = C.work(cfg, "catalogue", "read")
    complete, bad, missing = [], [], []
    for t in tiles:
        path = os.path.join(folder, t["name"] + ".json")
        if not os.path.exists(path):
            missing.append(t["name"])
            continue
        try:
            check_record(cfg, grid, t, C.read_json(path))
            complete.append(t["name"])
        except (ValueError, KeyError, TypeError) as e:
            bad.append("%s (%s)" % (t["name"], str(e)[:120]))
    print("complete %d of %d" % (len(complete), len(tiles)))
    print("bad %d: %s" % (len(bad), "; ".join(bad)))
    print("missing %d: %s" % (len(missing), " ".join(missing)))
    return 0 if not bad and not missing else 1


def place_key(h):
    p = h.get("place")
    return None if not p else (p["glyph"], p.get("name"), p.get("vp"))


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    cfg = C.load_config(argv[1])
    grid = C.make_grid(cfg, C.load_fit(cfg))
    tiles = C.read_json(C.work(cfg, "tiles", "manifest.json"))["tiles"]
    if "--status" in argv:
        return status(cfg, grid, tiles)
    # --records DIR reads other records (e.g. the drafts, for a dry run); --out DIR writes the results there
    folder = argv[argv.index("--records") + 1] if "--records" in argv else C.work(cfg, "catalogue", "read")
    out_dir = argv[argv.index("--out") + 1] if "--out" in argv else C.work(cfg, "catalogue")
    missing = [t["name"] for t in tiles if not os.path.exists(os.path.join(folder, t["name"] + ".json"))]
    if missing and "--partial" not in argv:
        print("no reader record for %d tiles: %s" % (len(missing), " ".join(missing)))
        return 1
    res_path = C.work(cfg, "catalogue", "resolutions.json")
    resolutions = {}
    for r in (C.read_json(res_path) if os.path.exists(res_path) else []):
        key = (r["at"] if ":" in r["at"] or " " in r["at"] or r["at"] in grid.ids
               else C.canonical_side(grid, r["at"], "resolutions"), feature_name(r["feature"]))
        if key in resolutions:
            raise ValueError("%s: two resolutions for %s %s; keep one" % (res_path, key[1], key[0]))
        resolutions[key] = r
    terrain = collections.defaultdict(dict)
    place = collections.defaultdict(dict)
    side_votes = {k: collections.defaultdict(dict) for k in kinds(cfg)}
    unclear, markers, notes = [], [], []
    for t in tiles:
        if t["name"] in missing:
            continue
        rec = C.read_json(os.path.join(folder, t["name"] + ".json"))
        area, sides = check_record(cfg, grid, t, rec)
        for pid, h in rec["hexes"].items():
            terrain[pid][t["name"]] = h["terrain"]
            place[pid][t["name"]] = place_key(h)
        for kind, names in sides.items():
            for name in C.all_sides_within(grid, area):
                side_votes[kind][name][t["name"]] = name in names
        for u in rec.get("unclear", []):
            # an unclear entry is a question: one naming several options ("3901:nw / 3901:n") stays verbatim
            try:
                at = u["at"] if u["at"] in grid.ids else C.canonical_side(grid, u["at"], "tile %s unclear" % t["name"])
            except ValueError:
                at = u["at"]
            unclear.append(dict(u, at=at, feature=feature_name(u["feature"]), tile=t["name"]))
        markers += [dict(m, tile=t["name"]) for m in rec.get("markers", [])]
        notes += ["%s: %s" % (t["name"], n) for n in rec.get("notes", [])]
    open_items = []
    cat = dict(hexes={}, markers=markers)
    for pid in sorted(terrain, key=lambda p: grid.ids[p]):
        entry = {}
        for feature, votes, show in (("terrain", terrain[pid], str), ("place", place[pid], str)):
            values = set(votes.values())
            if (pid, feature) in resolutions:
                value = resolutions[(pid, feature)]["value"]
                value = tuple(value.values()) if isinstance(value, dict) else value
            elif 1 == len(values):
                value = values.pop()
            else:
                open_items.append(dict(at=pid, feature=feature, readings={k: show(v) for k, v in votes.items()}))
                continue
            if "terrain" == feature:
                entry["terrain"] = value
            elif value is not None:
                entry["place"] = dict(zip(("glyph", "name", "vp"), value))
        cat["hexes"][pid] = entry
    for kind, votes in side_votes.items():
        chosen = []
        for name, by_tile in votes.items():
            if (name, kind) in resolutions:
                if resolutions[(name, kind)]["value"]:
                    chosen.append(name)
                continue
            values = set(by_tile.values())
            if 1 == len(values):
                if values.pop():
                    chosen.append(name)
            else:
                open_items.append(dict(at=name, feature=kind, readings=by_tile))
        cat[kind] = sorted(chosen, key=lambda n: [grid.ids[h] for h in n.split(":")[0].split("-")])
    for u in unclear:
        if (u["at"], u["feature"]) not in resolutions:
            open_items.append(dict(at=u["at"], feature=u["feature"], unclear=u["note"], tile=u["tile"]))
    uncovered = [pid for pid in grid.ids if pid not in cat["hexes"] and pid not in {o["at"] for o in open_items}]
    os.makedirs(out_dir, exist_ok=True)
    C.write_json(os.path.join(out_dir, "catalogue.json"), cat)
    C.write_json(os.path.join(out_dir, "disagreements.json"), open_items)
    C.write_json(os.path.join(out_dir, "notes.json"), notes)
    counts = collections.Counter(o["feature"] for o in open_items)
    print("records %d of %d tiles; hexes %d (uncovered %d); %s" % (
        len(tiles) - len(missing), len(tiles), len(cat["hexes"]), len(uncovered),
        "  ".join("%s %d" % (k, len(cat[k])) for k in kinds(cfg))))
    print("resolutions %d; open: %d %s" % (len(resolutions), len(open_items), dict(counts)))
    passed = not open_items and not missing and not uncovered
    print("GATE PASSED" if passed else "GATE FAILED (see disagreements.json)")
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
