"""overlay.py -- draw a candidate hex grid over a scan crop to check calibration.

python overlay.py SRC OUT x0 y0 x1 y1 scale orientation offset size ox oy cols rows fmt cstart cstep rstart rstep
"""
import sys
sys.path.insert(0, r"C:\repos\ghub-per\titan-alloy\hexgames\map_graphics\xml")
from PIL import Image, ImageDraw
from lxml import etree
import hexsheet2svg as H
Image.MAX_IMAGE_PIXELS = None

a = sys.argv[1:]
src, out = a[0], a[1]
x0, y0, x1, y1 = map(int, a[2:6])
scale = float(a[6])
orient, offset, size, ox, oy, cols, rows, fmt, cs, cst, rs, rst = a[7:19]
el = etree.Element("grid", orientation=orient, offset=offset, size=size, ox=ox, oy=oy, cols=cols, rows=rows,
                   terrain="t")
el.set("id-format", fmt); el.set("col-start", cs); el.set("col-step", cst); el.set("row-start", rs); el.set("row-step", rst)
g = H.Grid(el)
im = Image.open(src).convert("RGB").crop((x0, y0, x1, y1))
im = im.resize((int(im.width*scale), int(im.height*scale)), Image.LANCZOS)
d = ImageDraw.Draw(im)
for (c, r), pid in g.cells.items():
    cx, cy = g.centre(c, r)
    if not (x0-40 < cx < x1+40 and y0-40 < cy < y1+40):
        continue
    pts = [((x-x0)*scale, (y-y0)*scale) for x, y in g.polygon(c, r)]
    d.polygon(pts, outline=(255, 0, 255))
    d.text(((cx-x0)*scale-10, (cy-y0)*scale-5), pid, fill=(200, 0, 0))
im.save(out)
print(out, im.size, "cells", len(g.cells))
