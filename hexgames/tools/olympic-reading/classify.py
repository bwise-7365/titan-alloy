# Copyright Ben Paul Wise. All Rights Reserved.
import json, math
from cells import *
M = json.load(open("measure.json"))
F = M["frac"]
SEA_T, MTN_T, ROUGH_T = 0.85, 0.75, 0.5


def terrain(h):
    f = F[h]
    if f["water"] >= SEA_T:
        return "sea"
    land = max(1e-6, 1 - f["water"])
    # Ben 2026-09-25: more than 25% rough makes the hex rough, unless at least 25% is mountain (then mountain)
    if f["mountain"] / land >= 0.25:
        return "mountain"
    if (f["rough"] + f["mountain"]) / land > 0.25:
        return "rough"
    return "clear"


T = {h: terrain(h) for h in CELLS}
if __name__ == "__main__":
    import collections
    print(collections.Counter(T.values()))
    COL = {"sea": (40, 170, 240), "mountain": (120, 100, 60), "rough": (185, 170, 120), "clear": (225, 225, 230)}
    base = Image.open(IMG).convert("RGB")
    out = Image.new("RGB", (base.width * 2, base.height), "white")
    out.paste(base, (0, 0))
    dr = ImageDraw.Draw(out)
    for h in CELLS:
        x, y = xy(h)
        pts = [(x + base.width + SIZE * math.cos(math.radians(a)), y + SIZE * math.sin(math.radians(a))) for a in range(0, 360, 60)]
        dr.polygon(pts, fill=COL[T[h]], outline=(150, 150, 150))
    out.resize((out.width // 2, out.height // 2)).save("classmap.jpg", quality=85)
# Copyright Ben Paul Wise. All Rights Reserved.
