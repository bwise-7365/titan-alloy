# Copyright Ben Paul Wise. All Rights Reserved.
"""compare.py -- the same place in several images side by side: before/after crops for the report.

    python compare.py MAP OUT.jpg (--hex ID [--radius R] | --box X0 Y0 X1 Y1) [--scale S] [--grid]
                      IMAGE [IMAGE ...]

Each IMAGE is a source name from maps/MAP.json, "render" (the sheet PNG), or a path to a PNG in the same
pixel frame as the primary source (for example an older render of the sheet). Each panel is the same box,
labelled with the image's name, and --grid draws the primary calibration's overlay on every panel. The
result is one JPEG under 300 KB (panels shrink to fit). Default scale 1.
"""
import os
import sys

from PIL import Image, ImageDraw

import common as C
import crop
import overlay


def panel(cfg, name, box, scale, grid):
    if name in cfg["sources"] or "render" == name:
        img = crop.cut(cfg, name, box, scale, grid)
        label = name
    else:
        with Image.open(name) as full:
            img = full.convert("RGB").crop(tuple(int(round(v)) for v in box))
        img = img.resize((int(img.width * scale), int(img.height * scale)), Image.LANCZOS)
        if grid is not None:
            overlay.draw_grid(img, grid, tuple(int(round(v)) for v in box), scale)
        label = os.path.basename(name)
    d = ImageDraw.Draw(img, "RGBA")
    d.rectangle((0, 0, img.width, 26), fill=(255, 255, 255, 215))
    d.text((6, 4), label, font=overlay.font(18), fill=(200, 0, 0, 255))
    return img


def main(argv):
    if len(argv) < 5:
        print(__doc__)
        return 2
    cfg = C.load_config(argv[1])
    out = argv[2]
    grid = C.make_grid(cfg, C.load_fit(cfg))
    scale = float(crop.option(argv, "--scale", default="1"))
    if "--hex" in argv:
        pid = C.known(grid, crop.option(argv, "--hex"), "compare")
        half = float(crop.option(argv, "--radius", default="2")) * C.spacing(grid)
        cx, cy = grid.centre(*grid.ids[pid])
        box = (cx - half, cy - half, cx + half, cy + half)
    elif "--box" in argv:
        box = tuple(float(v) for v in crop.option(argv, "--box", 4))
    else:
        print(__doc__)
        return 2
    skip = {"--hex", "--radius", "--box", "--scale"}
    images, i = [], 3
    while i < len(argv):
        if argv[i] in skip:
            i += 5 if "--box" == argv[i] else 2
            continue
        if "--grid" != argv[i]:
            images.append(argv[i])
        i += 1
    panels = [panel(cfg, name, box, scale, grid if "--grid" in argv else None) for name in images]
    gap = 8
    sheet = Image.new("RGB", (sum(p.width for p in panels) + gap * (len(panels) - 1), max(p.height for p in panels)),
                      (255, 255, 255))
    x = 0
    for p in panels:
        sheet.paste(p, (x, 0))
        x += p.width + gap
    print("wrote", C.save_jpeg(sheet, out))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
