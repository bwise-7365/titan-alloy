# Copyright Ben Paul Wise. All Rights Reserved.
"""stage4_terrain.py -- one terrain value per hex from elevation statistics and hand rulings.

Classes (the design memorandum's list): clear, broken, mountain, marsh, steppe, sea.
  sea       less than half the hex is land (a place's hex is land whatever the fraction)
  mountain  high and rugged: median >= 1800 m, or relief (std of 19 ETOPO1 samples) >= 400 m,
            or range >= 1400 m with mean >= 500 m
  broken    std >= 130 m or mean >= 700 m
  steppe    inside the hand-drawn arid/steppe polygons (Inner Mongolia, Hulunbuir, Horqin, Ordos)
            unless mountain
  marsh     a lake covers a fifth of the hex, or inside the hand-drawn floodplain polygons (Sanjiang,
            Nen-Sungari lowland, the 1938-47 Yellow River flood zone), or a LAKES hex
  then cd_data.RANGES (named ranges, explicit hexes) and the place rulings override, in that order.
Writes terrain_hex.json {hex: {cls, mean, std, ...}} and diag_terrain.png.
"""
import json
import os
import statistics
import sys

from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(__file__))
import lattice as L  # noqa: E402
import cd_data as D  # noqa: E402

OUT = os.path.dirname(os.path.abspath(__file__))

# (lat, lon) polygons
STEPPE = [
    # Inner Mongolian plateau north of the Yinshan / Great Wall line, east to the Greater Khingan foot
    [(37.0, 95.0), (37.0, 105.5), (38.6, 105.5), (39.8, 107.0), (40.9, 108.2), (41.2, 111.5), (41.0, 114.3),
     (42.3, 115.6), (43.2, 118.0), (44.9, 119.6), (46.0, 119.8), (47.4, 118.5), (49.0, 117.4), (51.0, 118.3),
     (51.0, 95.0)],
    # Hulunbuir steppe west of the Khingan, Argun valley
    [(47.4, 118.5), (49.0, 117.4), (51.0, 118.3), (51.0, 121.5), (49.9, 121.0), (48.4, 120.0), (47.2, 119.8)],
    # Horqin sandy lands between the Khingan and the Liao / Sungari plain
    [(42.6, 119.8), (43.3, 119.6), (44.9, 120.8), (45.9, 122.0), (45.3, 123.2), (44.0, 123.3), (43.0, 122.3)],
    # Ordos
    [(37.6, 107.0), (38.6, 107.0), (40.3, 108.0), (40.5, 110.2), (39.2, 110.6), (37.9, 109.5), (37.3, 108.0)],
]
MARSH = [
    # Sanjiang plain: Amur - Ussuri - Sungari floodplain
    [(47.9, 130.5), (48.4, 134.4), (47.2, 134.9), (46.3, 133.9), (46.1, 132.3), (46.5, 130.8), (47.3, 130.2)],
    # Nen - Sungari lowland (Zhalong marshes, the Songnen plain's wet core)
    [(47.9, 123.4), (47.7, 125.3), (46.7, 125.9), (45.7, 125.3), (45.5, 124.0), (46.5, 123.2)],
    # Yellow River flood zone 1938-47 (eastern Henan, northern Anhui, to the Huai and Hongze Lake)
    [(34.75, 113.9), (34.65, 114.9), (34.1, 116.2), (33.4, 117.3), (33.0, 118.4), (33.3, 119.4), (33.9, 119.5),
     (33.4, 118.9), (32.9, 118.5), (32.6, 117.3), (32.6, 116.3), (33.1, 115.2), (33.9, 114.2), (34.5, 113.8)],
]


def point_in_poly(lat, lon, poly):
    inside = False
    n = len(poly)
    for i in range(n):
        y1, x1 = poly[i]
        y2, x2 = poly[(i + 1) % n]
        if (y1 > lat) != (y2 > lat):
            x = x1 + (lat - y1) * (x2 - x1) / (y2 - y1)
            if lon < x:
                inside = not inside
    return inside


def place_hexes():
    """place id -> hex id, after PLACE_HEX moves."""
    out = {}
    for pid, label, lat, lon, kind, flags, terr in D.PLACES:
        out[pid] = D.PLACE_HEX_BY_SCALE.get(L.HEX_KM, {}).get(pid) or L.hex_id(*L.hex_of_ll(lat, lon))
    return out


def classify(geo, elev):
    phex = place_hexes()
    place_terrain = {}
    place_country = {}
    for pid, label, lat, lon, kind, flags, terr in D.PLACES:
        if terr:
            place_terrain[phex[pid]] = terr
    land_places = set(phex.values())
    ruled_land = set(D.LAND_HEXES_BY_SCALE.get(L.HEX_KM, "").split())
    lake_hexes = {L.hex_id(*L.hex_of_ll(lat, lon)): name for name, lat, lon in D.LAKES}
    range_cls = {}
    for cls, name, hexes in D.RANGES:
        for h in L.rescale_ids(hexes).split():      # the ranges are ruled on the 100 km lattice
            range_cls[h] = cls
    out = {}
    for h, g in geo.items():
        c, r = L.parse_id(h)
        lat, lon = L.px_to_ll(*L.centre(c, r))
        rec = dict(land=g["land"], lake=g["lake"], country=g["country"], lat=round(lat, 2), lon=round(lon, 2))
        if g["land"] < 0.5 and h not in land_places and h not in ruled_land:
            rec["cls"] = "sea"
            out[h] = rec
            continue
        samples = [v for v in (elev.get(h) or []) if v is not None]
        land_vals = [v for v in samples if v > 0] or [max(v, 0) for v in samples] or [0]
        mean = statistics.fmean(land_vals)
        std = statistics.pstdev(land_vals) if len(land_vals) > 1 else 0.0
        med = statistics.median(land_vals)
        mx, mn = max(land_vals), min(land_vals)
        rec.update(mean=round(mean), std=round(std), med=round(med), max=mx, min=mn)
        if med >= 1800 or std >= 400 or (mx - mn >= 1400 and mean >= 500):
            cls = "mountain"
        elif std >= 130 or mean >= 700:
            cls = "broken"
        else:
            cls = "clear"
        if cls != "mountain" and any(point_in_poly(lat, lon, p) for p in STEPPE):
            cls = "steppe"
        if g["lake"] >= 0.2 or h in lake_hexes or (cls in ("clear", "broken") and any(point_in_poly(lat, lon, p) for p in MARSH)):
            cls = "marsh"
        if h in range_cls:
            cls = range_cls[h]
        if h in place_terrain:
            cls = place_terrain[h]
        rec["cls"] = cls
        if h in lake_hexes:
            rec["lake_name"] = lake_hexes[h]
        out[h] = rec
    # a ruled-land hex without a country takes its land neighbours' commonest country
    for h, rec in out.items():
        if rec["cls"] != "sea" and not rec["country"]:
            c, r = L.parse_id(h)
            votes = {}
            for d, n in L.neighbours(c, r):
                nb = out.get(L.hex_id(*n))
                if nb and nb["cls"] != "sea" and nb["country"]:
                    votes[nb["country"]] = votes.get(nb["country"], 0) + 1
            rec["country"] = max(votes, key=votes.get) if votes else 1
    return out


COLOURS = {"sea": (105, 185, 236), "clear": (234, 222, 190), "broken": (201, 168, 92), "mountain": (139, 104, 60),
           "steppe": (236, 213, 129), "marsh": (185, 197, 177)}


def diagnostic(terrain, path):
    img = Image.new("RGB", (L.SHEET_W, L.SHEET_H), (243, 239, 226))
    dr = ImageDraw.Draw(img)
    for h, t in terrain.items():
        c, r = L.parse_id(h)
        dr.polygon(L.polygon(c, r), fill=COLOURS[t["cls"]], outline=(120, 110, 90))
        cx, cy = L.centre(c, r)
        dr.text((cx - 12, cy - 16), h, fill=(70, 70, 70))
        if t["cls"] != "sea":
            dr.text((cx - 16, cy - 2), "%d/%d" % (t["mean"], t["std"]), fill=(20, 20, 20))
    rivers = json.load(open(os.path.join(OUT, L.cache("rivers.json"))))
    for edges in rivers.values():
        for e in edges:
            hx, d = e.split(":")
            c, r = L.parse_id(hx)
            a, b = L.FLAT["edge_corners"][d]
            dr.line([L.corner(c, r, a), L.corner(c, r, b)], fill=(30, 80, 220), width=5)
    phex = place_hexes()
    for pid, label, lat, lon, kind, flags, terr in D.PLACES:
        c, r = L.parse_id(phex[pid])
        cx, cy = L.centre(c, r)
        if kind != "wp":
            dr.ellipse([cx - 7, cy + 10, cx + 7, cy + 24], fill=(200, 30, 30))
            dr.text((cx + 9, cy + 10), label, fill=(120, 0, 0))
        else:
            dr.ellipse([cx - 3, cy + 14, cx + 3, cy + 20], fill=(200, 30, 30))
    img.save(path)


def main():
    geo = json.load(open(os.path.join(OUT, L.cache("geo_hex.json"))))
    elev = json.load(open(os.path.join(OUT, L.cache("elev_hex.json"))))
    terrain = classify(geo, elev)
    json.dump(terrain, open(os.path.join(OUT, L.cache("terrain_hex.json")), "w"), indent=0)
    counts = {}
    for t in terrain.values():
        counts[t["cls"]] = counts.get(t["cls"], 0) + 1
    print(counts)
    diagnostic(terrain, os.path.join(OUT, L.cache("diag_terrain.png")))
    print("wrote terrain_hex.json, diag_terrain.png")


if __name__ == "__main__":
    main()
# Copyright Ben Paul Wise. All Rights Reserved.
