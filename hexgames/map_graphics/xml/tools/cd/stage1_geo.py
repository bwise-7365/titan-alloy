# Copyright Ben Paul Wise. All Rights Reserved.
"""stage1_geo.py -- rasterise Natural Earth land, lakes and countries onto the lattice.

Writes geo_hex.json: per hex id {land: fraction, lake: fraction, country: code or null}.
Also writes owner.npy (pixel -> hex index) for later stages and a diagnostic PNG.
"""
import json
import os
import sys

import numpy as np
from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(__file__))
import lattice as L  # noqa: E402

NE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ne")   # Natural Earth GeoJSON, see README.md
OUT = os.path.dirname(os.path.abspath(__file__))

COUNTRY_CODES = {"China": 1, "Hong Kong S.A.R.": 1, "Macao S.A.R": 1, "Macau S.A.R": 1, "Mongolia": 2,
                 "Russia": 3, "North Korea": 4, "South Korea": 4, "Vietnam": 6, "Laos": 7, "Myanmar": 8,
                 "Taiwan": 9, "Japan": 10, "India": 11, "Bhutan": 11, "Nepal": 11, "Thailand": 12,
                 "Kazakhstan": 13, "Bangladesh": 11}
BBOX = (90.0, 15.0, 145.0, 60.0)  # lon0 lat0 lon1 lat1 filter


def rings_of(geom):
    if geom is None:
        return
    if geom["type"] == "Polygon":
        for ring in geom["coordinates"]:
            yield ring
    elif geom["type"] == "MultiPolygon":
        for poly in geom["coordinates"]:
            for ring in poly:
                yield ring


def polys_of(geom):
    """Outer ring and holes of each polygon."""
    if geom is None:
        return
    if geom["type"] == "Polygon":
        yield geom["coordinates"]
    elif geom["type"] == "MultiPolygon":
        for poly in geom["coordinates"]:
            yield poly


def in_bbox(ring):
    lons = [p[0] for p in ring]
    lats = [p[1] for p in ring]
    return not (max(lons) < BBOX[0] or min(lons) > BBOX[2] or max(lats) < BBOX[1] or min(lats) > BBOX[3])


def draw_feature_mask(features, size, keyfn=None):
    """Draw polygons (outer rings filled, holes cleared) into an L image; keyfn gives the fill value."""
    img = Image.new("L", size, 0)
    dr = ImageDraw.Draw(img)
    for f in features:
        val = keyfn(f) if keyfn else 255
        if not val:
            continue
        for poly in polys_of(f["geometry"]):
            outer = poly[0]
            if not in_bbox(outer):
                continue
            pts = [L.ll_to_px(p[1], p[0]) for p in outer]
            dr.polygon(pts, fill=val)
            for hole in poly[1:]:
                if in_bbox(hole):
                    dr.polygon([L.ll_to_px(p[1], p[0]) for p in hole], fill=0)
    return img


def owner_raster(size):
    """pixel -> hex index (c * ROWS + r), or -1 outside the lattice."""
    w, h = size
    xs = np.arange(w, dtype=np.float64) + 0.5
    ys = np.arange(h, dtype=np.float64) + 0.5
    X, Y = np.meshgrid(xs, ys)
    best = np.full((h, w), -1, dtype=np.int32)
    bestd = np.full((h, w), np.inf)
    c0 = np.round((X - L.OX) / (1.5 * L.SIZE)).astype(np.int32)
    for dc in (-1, 0, 1):
        c = c0 + dc
        okc = (c >= 0) & (c < L.COLS)
        y0 = L.OY + np.where(c % 2 == 1, L.SQRT3 / 2 * L.SIZE, 0.0)
        r0 = np.round((Y - y0) / (L.SQRT3 * L.SIZE)).astype(np.int32)
        for drr in (-1, 0, 1):
            r = r0 + drr
            ok = okc & (r >= 0) & (r < L.ROWS)
            cx = L.OX + c * 1.5 * L.SIZE
            cy = y0 + r * L.SQRT3 * L.SIZE
            d = (cx - X) ** 2 + (cy - Y) ** 2
            # a point farther than the circumradius from the nearest centre is outside the map area
            better = ok & (d < bestd) & (d <= L.SIZE ** 2 * 1.0001)
            bestd = np.where(better, d, bestd)
            best = np.where(better, c * L.ROWS + r, best)
    return best


def main():
    size = (L.SHEET_W, L.SHEET_H)
    land = json.load(open(os.path.join(NE, "ne_50m_land.geojson"), encoding="utf-8"))["features"]
    lakes = json.load(open(os.path.join(NE, "ne_50m_lakes.geojson"), encoding="utf-8"))["features"]
    countries = json.load(open(os.path.join(NE, "ne_50m_admin_0_countries.geojson"), encoding="utf-8"))["features"]
    land_img = draw_feature_mask(land, size)
    lake_img = draw_feature_mask(lakes, size)
    country_img = draw_feature_mask(countries, size, lambda f: COUNTRY_CODES.get(f["properties"].get("NAME"), 0))
    land_a = np.array(land_img) > 0
    lake_a = np.array(lake_img) > 0
    country_a = np.array(country_img).astype(np.int32)
    owner = owner_raster(size)
    nhex = L.COLS * L.ROWS
    inside = owner >= 0
    idx = owner[inside]
    tot = np.bincount(idx, minlength=nhex).astype(np.float64)
    landc = np.bincount(idx, weights=land_a[inside], minlength=nhex)
    lakec = np.bincount(idx, weights=lake_a[inside], minlength=nhex)
    ncode = 16
    cc = np.bincount(idx * ncode + np.where(land_a[inside], country_a[inside], 0), minlength=nhex * ncode).reshape(nhex, ncode)
    out = {}
    for c in range(L.COLS):
        for r in range(L.ROWS):
            i = c * L.ROWS + r
            if tot[i] == 0:
                continue
            counts = cc[i].copy()
            counts[0] = 0
            country = int(np.argmax(counts)) if counts.sum() > 0 else None
            out[L.hex_id(c, r)] = dict(land=round(float(landc[i] / tot[i]), 3), lake=round(float(lakec[i] / tot[i]), 3),
                                       country=country, cfrac=round(float(counts.max() / max(1, landc[i])), 2) if country else 0)
    json.dump(out, open(os.path.join(OUT, L.cache("geo_hex.json")), "w"), indent=0)
    nland = sum(1 for v in out.values() if v["land"] >= 0.5)
    print("hexes %d, land(>=0.5) %d, sea %d" % (len(out), nland, len(out) - nland))
    # diagnostic: land/sea/lake colour plus country code tint, with the lattice drawn over it
    diag = Image.new("RGB", size, (105, 185, 236))
    px = diag.load()
    arr = np.array(diag)
    arr[land_a] = (234, 222, 190)
    arr[lake_a] = (120, 170, 220)
    tint = {2: (220, 200, 160), 3: (230, 190, 190), 4: (200, 220, 200), 6: (210, 210, 170), 7: (210, 210, 170),
            8: (200, 200, 220), 9: (220, 220, 200), 10: (230, 210, 230)}
    for code, col in tint.items():
        arr[land_a & (country_a == code)] = col
    diag = Image.fromarray(arr)
    dr = ImageDraw.Draw(diag)
    for c in range(L.COLS):
        for r in range(L.ROWS):
            dr.polygon(L.polygon(c, r), outline=(120, 110, 90))
            h = L.hex_id(c, r)
            v = out.get(h)
            if v and v["land"] >= 0.5:
                cx, cy = L.centre(c, r)
                dr.text((cx - 12, cy - 6), h, fill=(60, 60, 60))
    diag.save(os.path.join(OUT, L.cache("diag_geo.png")))
    print("wrote geo_hex.json, owner.npy, diag_geo.png")


if __name__ == "__main__":
    main()
# Copyright Ben Paul Wise. All Rights Reserved.
