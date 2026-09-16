# Copyright Ben Paul Wise. All Rights Reserved.
"""check_config.py -- read maps/MAP.json and report what a later stage would throw on.

    python check_config.py MAP

Run it before stage 1, and again after stage 4's tuning. It reports ERRORs (a stage will fail on them)
and NOTEs (worth a look, not wrong), and exits 1 if any ERROR is left. It checks the keys the stages
read, that masks and terrains name each other, that the source image opens, and that the thresholds do
not still hold the untested "at least half the hex" guess. maps/template.json documents every key.
"""
import os
import sys

import common as C

TOP = ["map", "title", "sources", "grid", "tiles", "vocabulary", "candidates", "sheet"]
GRID = ["orientation", "offset", "cols", "rows", "id-format", "id-side", "terrain", "extent"]
DIRS = {"flat": {"n", "ne", "se", "s", "sw", "nw"}, "pointy": {"ne", "e", "se", "sw", "w", "nw"}}


def keys(block):
    """The real keys of a configuration block: '_note' documentation is not one."""
    return [k for k in block if not k.startswith("_")]


def check_grid(cfg, err, note):
    g = cfg["grid"]
    for k in GRID:
        if k not in g:
            err("grid: no '%s'" % k)
    if g.get("orientation") not in DIRS:
        err("grid: orientation '%s' is neither 'flat' nor 'pointy'" % g.get("orientation"))
    if g.get("offset") not in ("odd", "even"):
        err("grid: offset '%s' is neither 'odd' nor 'even'" % g.get("offset"))
    if g.get("orientation") in DIRS and g.get("id-side") not in DIRS[g["orientation"]]:
        err("grid: id-side '%s' is not a %s hexside (%s)" % (
            g.get("id-side"), g["orientation"], " ".join(sorted(DIRS[g["orientation"]]))))
    for k in ("cols", "rows"):
        if not isinstance(g.get(k), int) or 1 > g.get(k, 0):
            err("grid: %s must be a whole number of hexes" % k)
    if "id-format" in g and "cols" in g and "rows" in g:
        first = g["id-format"].format(col=g.get("col-start", 1), row=g.get("row-start", 1))
        last = g["id-format"].format(col=g.get("col-start", 1) + g["cols"] - 1,
                                     row=g.get("row-start", 1) + g["rows"] - 1)
        for want, got, which in ((g.get("extent", {}).get("first"), first, "first"),
                                 (g.get("extent", {}).get("last"), last, "last")):
            if want and want != got:
                err("grid: extent %s is '%s', but the grid's %s printed id is '%s'" % (which, want, which, got))
    if g.get("terrain") and g["terrain"] not in cfg.get("vocabulary", {}).get("terrain", {}):
        err("grid: default terrain '%s' is not in vocabulary.terrain" % g["terrain"])
    if 12 > len(g.get("control_points", [])) and 12 > len(cfg["sources"].get("primary", {}).get("control_points", [])):
        note("stage 1 has not run yet: fewer than 12 control points on the primary source")
    return


def check_sources(cfg, err, note):
    if "primary" not in cfg.get("sources", {}):
        err("sources: no 'primary'")
        return
    for name in keys(cfg["sources"]):
        src = cfg["sources"][name]
        for k in ("path", "size_hint", "line_dark"):
            if k not in src:
                err("sources.%s: no '%s'" % (name, k))
        path = src.get("path", "")
        if not os.path.exists(path):
            err("sources.%s: no image at %s" % (name, path))
        else:
            img = C.open_image(path)
            note("sources.%s: %s, %d x %d px" % (name, os.path.basename(path), img.width, img.height))
    return


def check_candidates(cfg, err, note):
    c = cfg["candidates"]
    masks = keys(c.get("colour_masks", {}))
    vocab = cfg.get("vocabulary", {})
    for k in ("colour_masks", "hex_masks", "terrain", "default_terrain", "links", "hexside_lines"):
        if k not in c:
            err("candidates: no '%s'" % k)
    for m in c.get("hex_masks", []):
        if m not in masks:
            err("candidates: hex_masks names '%s', which has no colour mask" % m)
    for kind in ("links", "hexside_lines"):
        for m in c.get(kind, []):
            if m not in masks:
                err("candidates: %s names '%s', which has no colour mask" % (kind, m))
            if m not in vocab.get(kind, {}):
                err("candidates: %s names '%s', which vocabulary.%s does not describe" % (kind, m, kind))
    for m in keys(c.get("presence", {})):
        if m not in c.get("links", []):
            err("candidates: presence names '%s', which is not a link" % m)
    order = []
    for name in keys(c.get("terrain", {})):
        rule = c["terrain"][name]
        if name not in vocab.get("terrain", {}):
            err("candidates.terrain: '%s' is not in vocabulary.terrain" % name)
        if rule.get("mask") not in masks:
            err("candidates.terrain.%s: mask '%s' has no colour mask" % (name, rule.get("mask")))
        low, at, sure = rule.get("maybe", 0), rule.get("at_least"), rule.get("sure", 1)
        if at is None:
            err("candidates.terrain.%s: no 'at_least'" % name)
        elif not low <= at <= sure:
            err("candidates.terrain.%s: thresholds are out of order (maybe %s, at_least %s, sure %s)" % (
                name, low, at, sure))
        elif 0.5 == at:
            err("candidates.terrain.%s: at_least is exactly 0.5, the untested 'at least half the hex' guess."
                " Set it from the person's labelled hexes (stage 0 step 5); on PGG the right value was 0.24" % name)
        order.append((name, at if at is not None else 0))
    for i, (name, at) in enumerate(order):
        mask = c["terrain"][name].get("mask")
        blobP = any(k in c.get("colour_masks", {}).get(mask, {}) for k in ("green", "close"))
        for later, later_at in order[i + 1:]:
            if blobP and 0.1 > later_at:
                note("candidates.terrain: '%s' is tested before '%s' (at_least %s), and its mask is a blob or"
                     " hue rule that can swallow a hatch. A hatch or symbol terrain must come FIRST" % (
                         name, later, later_at))
    if not c.get("default_terrain") in vocab.get("terrain", {}):
        err("candidates: default_terrain '%s' is not in vocabulary.terrain" % c.get("default_terrain"))
    return


def check_sheet(cfg, err, note):
    s = cfg["sheet"]
    for k in ("path", "png", "urban", "keep"):
        if k not in s:
            err("sheet: no '%s'" % k)
    if s.get("urban") not in ("buildings", "symbol"):
        err("sheet: urban is '%s'; hexsheet.xsd requires 'buildings' or 'symbol'" % s.get("urban"))
    for k in ("path", "png"):
        if k in s and not os.path.isdir(os.path.dirname(os.path.join(C.XML_DIR, s[k])) or C.XML_DIR):
            err("sheet: %s '%s' is not inside map_graphics/xml" % (k, s[k]))
    if "path" in s and not os.path.exists(os.path.join(C.XML_DIR, s["path"])):
        note("sheet: %s does not exist yet; assemble.py will write it, and 'keep' will copy nothing" % s["path"])
    return


def main(argv):
    if 2 > len(argv):
        print(__doc__)
        return 2
    cfg = C.load_config(argv[1])
    problems, notes = [], []
    err, note = problems.append, notes.append
    for k in TOP:
        if k not in cfg:
            err("no '%s' block" % k)
    if cfg.get("map") and not cfg["_path"].endswith(cfg["map"] + ".json"):
        err("map is '%s', but the file is %s: work/ folders are named after 'map'" % (
            cfg["map"], os.path.basename(cfg["_path"])))
    for block, checker in (("sources", check_sources), ("grid", check_grid),
                           ("candidates", check_candidates), ("sheet", check_sheet)):
        if block in cfg:
            checker(cfg, err, note)
    for n in notes:
        print("NOTE  " + n)
    for p in problems:
        print("ERROR " + p)
    print("check_config: %d errors, %d notes" % (len(problems), len(notes)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
