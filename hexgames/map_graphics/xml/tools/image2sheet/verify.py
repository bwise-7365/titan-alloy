# Copyright Ben Paul Wise. All Rights Reserved.
"""verify.py -- stage 9: collect the render-versus-scan differences of one round; the verify gate.

    python verify.py MAP ROUND

A verify reader compares each scan tile (work/MAP/tiles/scan/TILE.jpg) with the render tile cut in the same
box (work/MAP/tiles/render/TILE.jpg, from `tile.py MAP render` after the sheet is rendered) and writes
work/MAP/verify/ROUND/TILE.json:
  {"tile": TILE, "reader": ..., "differences": [
     {"at": HEX or HEXSIDE, "feature": "terrain" | "place" | "rivers" | "rail" | "road" | "marker" | "other",
      "print": what the scan shows, "render": what the render shows}]}
This script checks every hex and hexside token, drops the differences named in
work/MAP/verify/exceptions.json (a list of {"at", "feature", "reason"}), merges duplicates read in two
overlapping tiles, and writes work/MAP/verify/ROUND/differences.json with the tiles that saw each one.
GATE: every tile has a record and no difference remains. Exit 1 otherwise. The differences go back to the
catalogue as resolutions (merge.py), never straight into the sheet.
"""
import collections
import os
import sys

import common as C


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    cfg = C.load_config(argv[1])
    grid = C.make_grid(cfg, C.load_fit(cfg))
    folder = C.work(cfg, "verify", argv[2])
    tiles = C.read_json(C.work(cfg, "tiles", "manifest.json"))["tiles"]
    exc_path = C.work(cfg, "verify", "exceptions.json")
    exceptions = C.read_json(exc_path) if os.path.exists(exc_path) else []

    def key(at, ctx):
        return at if at in grid.ids or "*" == at else C.canonical_side(grid, at, ctx)

    named = {(key(e["at"], "exceptions"), e["feature"]) for e in exceptions}
    # a difference whose place has a catalogue resolution is resolved; the focused round (contact.py) re-checks it
    res_path = C.work(cfg, "catalogue", "resolutions.json")
    for r in (C.read_json(res_path) if os.path.exists(res_path) else []):
        if " " in r["at"]:
            continue
        feature = "river" if "rivers" == r["feature"] else r["feature"]
        named.add((key(r["at"], "resolutions"), feature))
        named.add((key(r["at"], "resolutions"), r["feature"]))
    missing = [t["name"] for t in tiles if not os.path.exists(os.path.join(folder, t["name"] + ".json"))]
    found = collections.OrderedDict()
    excepted = 0
    for t in tiles:
        if t["name"] in missing:
            continue
        rec = C.read_json(os.path.join(folder, t["name"] + ".json"))
        for d in rec["differences"]:
            k = (key(d["at"], "verify %s %s" % (argv[2], t["name"])), d["feature"])
            if k in named or ("*", d["feature"]) in named:
                excepted += 1
                continue
            found.setdefault(k, dict(at=k[0], feature=k[1], print=d["print"], render=d["render"], tiles=[]))
            found[k]["tiles"].append(t["name"])
    out = list(found.values())
    C.write_json(os.path.join(folder, "differences.json"), out)
    counts = collections.Counter(d["feature"] for d in out)
    print("round %s: records %d of %d; differences %d %s; excepted readings %d" % (
        argv[2], len(tiles) - len(missing), len(tiles), len(out), dict(counts), excepted))
    if missing:
        print("missing records:", " ".join(missing))
    passed = not missing and not out
    print("GATE PASSED" if passed else "GATE FAILED (see %s)" % os.path.join(folder, "differences.json"))
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
