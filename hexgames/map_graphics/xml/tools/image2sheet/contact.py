# Copyright Ben Paul Wise. All Rights Reserved.
"""contact.py -- a focused verify round: scan and render side by side for every place the catalogue changed.

    python contact.py MAP OUTDIR [--per-sheet N] [--radius R] [--scale S] [AT ...]

For each place (a hex "0420", a hexside "0419-0420" or "HEX:DIR"; default: every resolution in
work/MAP/catalogue/resolutions.json), cut the same box from the primary scan and from the rendered sheet with the
grid overlay, put the two side by side under a caption (the place, the feature and the decided value), and pack N
such pairs (default 4) onto each contact sheet, OUTDIR/sheet-NN.jpg, each under 300 KB. A person or model reads the
sheets and lists any pair where the render still disagrees with the scan: a cheap second verify round after the
first round's resolutions, instead of re-reading whole tiles.
"""
import json
import os
import sys

from PIL import Image, ImageDraw

import common as C
import crop
import overlay


def place_centre(grid, at):
    if at in grid.ids:
        return grid.centre(*grid.ids[at])
    (ax, ay), (bx, by) = C.side_segment(grid, C.canonical_side(grid, at, "contact"))
    return (ax + bx) / 2, (ay + by) / 2


def pair(cfg, grid, at, caption, radius, scale):
    cx, cy = place_centre(grid, at)
    half = radius * C.spacing(grid)
    box = (cx - half, cy - half, cx + half, cy + half)
    left = crop.cut(cfg, "primary", box, scale, grid)
    right = crop.cut(cfg, "render", box, scale, grid)
    out = Image.new("RGB", (left.width + right.width + 6, max(left.height, right.height) + 24), (255, 255, 255))
    out.paste(left, (0, 24))
    out.paste(right, (left.width + 6, 24))
    d = ImageDraw.Draw(out)
    d.text((4, 3), caption[:90], font=overlay.font(15), fill=(200, 0, 0))
    return out


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    cfg = C.load_config(argv[1])
    outdir = argv[2]
    os.makedirs(outdir, exist_ok=True)
    grid = C.make_grid(cfg, C.load_fit(cfg))
    per = int(crop.option(argv, "--per-sheet", default="4"))
    radius = float(crop.option(argv, "--radius", default="1.1"))
    scale = float(crop.option(argv, "--scale", default="0.9"))
    skip = {"--per-sheet", "--radius", "--scale"}
    list_file = crop.option(argv, "--list")
    skip |= {"--list", "--terrain"}
    ats = [a for i, a in enumerate(argv[3:], 3) if a not in skip and argv[i - 1] not in skip]
    if list_file:
        ats += C.read_json(list_file)
    if ats and "--terrain" in argv:
        # captions for a terrain audit: the catalogue's terrain and the measured cover of each hex
        cat = C.read_json(C.work(cfg, "catalogue", "catalogue.json"))["hexes"]
        fr = C.read_json(C.work(cfg, "candidates.json"))["hexes"]
        places = [(a, "%s %s: woods %.2f blob %.2f/%.2f lake %.2f swamp %.2f rail %.2f road %.2f" % (
            a, cat[a]["terrain"], fr[a]["woods"], fr[a].get("woods_blob", 0), fr[a].get("woods_blob_cover", 0),
            fr[a]["lake"], fr[a]["swamp"], fr[a]["rail"], fr[a]["road"])) for a in ats]
    elif ats:
        places = [(a, a) for a in ats]
    else:
        places = []
        for r in C.read_json(C.work(cfg, "catalogue", "resolutions.json")):
            at = r["at"]
            if " " in at or r.get("value") is None:
                continue  # a closed reader question, decided by another resolution
            places.append((at, "%s %s = %s" % (at, r["feature"], r["value"])))
    pairs = [pair(cfg, grid, at, caption, radius, scale) for at, caption in places]
    index = []
    for n in range(0, len(pairs), per):
        group = pairs[n:n + per]
        cols = 2
        rows = (len(group) + cols - 1) // cols
        w = max(p.width for p in group)
        h = max(p.height for p in group)
        sheet = Image.new("RGB", (cols * w + 10, rows * h + 10 * rows), (230, 230, 230))
        for k, p in enumerate(group):
            sheet.paste(p, ((k % cols) * (w + 10), (k // cols) * (h + 10)))
        path = os.path.join(outdir, "sheet-%02d.jpg" % (n // per + 1))
        C.save_jpeg(sheet, path)
        index.append({"sheet": os.path.basename(path), "places": [c for _, c in places[n:n + per]]})
    with open(os.path.join(outdir, "index.json"), "w", encoding="utf-8") as f:
        json.dump(index, f, ensure_ascii=False, indent=1)
    print("wrote %d contact sheets for %d places to %s" % (len(index), len(places), outdir))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
