# Copyright Ben Paul Wise. All Rights Reserved.
"""crop of a quarter-scale photo with its own pixel coordinates drawn every 50 px: ruler.py NAME X0 Y0 W H OUT"""
import sys
from PIL import Image, ImageDraw, ImageFont
n, x0, y0, w, h, out = sys.argv[1], *map(int, sys.argv[2:6]), sys.argv[6]
im = Image.open(f"{n}_d4.png").convert("RGB").crop((x0, y0, x0 + w, y0 + h))
d = ImageDraw.Draw(im, "RGBA"); f = ImageFont.truetype("arialbd.ttf", 12)
for x in range((x0 // 50 + 1) * 50, x0 + w, 50):
    d.line([(x - x0, 0), (x - x0, h)], fill=(0, 200, 0, 90))
    if x % 100 == 0: d.text((x - x0 + 2, 2), str(x), fill=(0, 120, 0), font=f)
for y in range((y0 // 50 + 1) * 50, y0 + h, 50):
    d.line([(0, y - y0), (w, y - y0)], fill=(0, 200, 0, 90))
    if y % 100 == 0: d.text((2, y - y0 + 2), str(y), fill=(0, 120, 0), font=f)
im.save(out, quality=88)
# Copyright Ben Paul Wise. All Rights Reserved.
