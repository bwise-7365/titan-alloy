# Copyright Ben Paul Wise. All Rights Reserved.
"""stage3_rivers.py -- snap the operational rivers to hexside chains.

Only the rivers the design memorandum names: Yangtze, Yellow River (on its 1938-47 flood course below
Huayuankou), Han, Xiang; Amur, Argun, Ussuri, Sungari, Nen, Liao; and the Yalu and Tumen as Korea's
border.  Every other river disappears into terrain.

Method: densify the Natural Earth centreline to ~3 px, snap each point to the nearest lattice vertex,
join consecutive vertices by the cheapest path along hexsides (cost: distance of each vertex from the
line), cut loops, and emit canonical hexsides.  Writes rivers.json: {river: [[hexside, ...] chains]}
and a diagnostic PNG with the centrelines over the snapped hexsides.
"""
import heapq
import json
import math
import os
import sys

from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(__file__))
import lattice as L  # noqa: E402
import cd_data as D  # noqa: E402

NE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ne")   # Natural Earth GeoJSON, see README.md
OUT = os.path.dirname(os.path.abspath(__file__))

# river -> (NE name matches [(name, name_en)], lat/lon bbox filter (lon0, lat0, lon1, lat1) or None)
RIVERS = {
    "yangtze": ([("Chang Jiang", None), ("Yangtze", None), ("Jinsha", None)], (100.0, 24.0, 123.0, 33.0)),
    "yellow": ([("Huang", None)], (100.0, 33.0, 113.75, 42.0)),   # upper and middle course only
    "han": ([("Han", "Han")], (105.0, 29.5, 115.0, 34.5)),
    "xiang": ([("Xiang", None)], (109.0, 24.5, 114.0, 30.0)),
    "amur": ([("Amur", None), ("Heilong Jiang", None)], (115.0, 44.0, 141.0, 56.0)),
    "argun": ([("Argun’", None), ("Argun'", None), ("Argun", None)], (115.0, 47.0, 125.0, 56.0)),
    "ussuri": ([("Ussuri", None)], (130.0, 43.0, 137.0, 49.0)),
    "sungari": ([("Songhua", None), ("Di’er Songhua", None), ("Di'er Songhua", None)], (122.0, 42.0, 133.0, 48.5)),
    "nen": ([("Nen", None)], (120.0, 44.0, 127.0, 52.0)),
    "liao": ([("Liao", None), ("Xiliao", None)], (119.5, 40.0, 126.0, 44.5)),
    "yalu": ([("Yalu", None)], (123.0, 39.0, 129.0, 43.0)),
    "tumen": ([("Tumen", None)], (127.0, 41.0, 131.5, 43.5)),
}

# The Yellow River's course of 1938-47, from the Huayuankou breach south-east through the Huai to
# Hongze Lake and the sea by the old (pre-1855) Yellow River bed.  (lat, lon) waypoints.
YELLOW_1938 = [(34.92, 113.65), (34.70, 113.95), (34.40, 114.20), (34.05, 114.40), (33.75, 114.55), (33.62, 114.63),
               (33.40, 114.90), (33.25, 115.20), (33.05, 115.50), (32.93, 115.80), (32.65, 116.25), (32.52, 116.55),
               (32.65, 116.95), (32.92, 117.35), (33.10, 117.80), (33.25, 118.30), (33.35, 118.75), (33.55, 119.10),
               (33.85, 119.55), (34.10, 120.00), (34.30, 120.40)]

# The estuary below Zhenjiang, which the centreline data leaves out.
YANGTZE_ESTUARY = [(32.20, 119.61), (32.05, 120.05), (31.95, 120.55), (31.85, 121.05), (31.60, 121.55), (31.40, 121.95)]

# Rivers the data draws beyond the point where the design wants them cut.
CUTS = {
    "liao": lambda lat, lon: lon < 122.0 and lat > 43.0,   # drop the West Liao above Tongliao (steppe river)
}


def features():
    data = json.load(open(os.path.join(NE, "ne_10m_rivers_lake_centerlines.geojson"), encoding="utf-8"))
    for f in data["features"]:
        yield f["properties"].get("name") or "", f["properties"].get("name_en") or "", f["geometry"]


def parts_of(geom):
    if geom is None:
        return []
    if geom["type"] == "LineString":
        return [geom["coordinates"]]
    if geom["type"] == "MultiLineString":
        return list(geom["coordinates"])
    return []


def densify(pts_px, step=3.0):
    out = []
    for (x0, y0), (x1, y1) in zip(pts_px, pts_px[1:]):
        d = math.hypot(x1 - x0, y1 - y0)
        n = max(1, int(d / step))
        for i in range(n):
            t = i / n
            out.append((x0 + (x1 - x0) * t, y0 + (y1 - y0) * t))
    out.append(pts_px[-1])
    return out


def hex_at_strict(px, py):
    h = L.hex_at(px, py)
    if h is None:
        return None
    cx, cy = L.centre(*h)
    if math.hypot(cx - px, cy - py) > L.SIZE * 1.001:
        return None
    return h


def nearest_vertex(px, py):
    h = hex_at_strict(px, py)
    if h is None:
        return None
    best, bd = None, 1e18
    for name in L.FLAT["corners"]:
        p = L.corner(h[0], h[1], name)
        d = (p[0] - px) ** 2 + (p[1] - py) ** 2
        if d < bd:
            best, bd = L.vkey(p), d
    return best


def seg_dist(p, a, b):
    ax, ay = a
    bx, by = b
    px, py = p
    dx, dy = bx - ax, by - ay
    if dx == 0 and dy == 0:
        return math.hypot(px - ax, py - ay)
    t = max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy)))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


def cheapest_path(graph, u, v, a, b):
    """Vertex path from u to v along hexsides, minimising the summed distance from the line a-b."""
    dist = {u: 0.0}
    prev = {}
    heap = [(0.0, u)]
    while heap:
        d, x = heapq.heappop(heap)
        if x == v:
            break
        if d > dist.get(x, 1e18):
            continue
        for y in graph[x]:
            nd = d + 1.0 + seg_dist(L.vpos(y), a, b) / L.SIZE
            if nd < dist.get(y, 1e18):
                dist[y] = nd
                prev[y] = x
                heapq.heappush(heap, (nd, y))
    if v not in prev:
        return None
    path = [v]
    while path[-1] != u:
        path.append(prev[path[-1]])
    return path[::-1]


def cut_loops(walk):
    """Remove every loop from a vertex walk (keep the first visit, drop what lies between repeats)."""
    out = []
    pos = {}
    for v in walk:
        if v in pos:
            k = pos[v]
            for w in out[k + 1:]:
                del pos[w]
            del out[k + 1:]
        else:
            pos[v] = len(out)
            out.append(v)
    return out


def snap_polyline(graph, pts_ll, cut=None):
    pts = [(lat, lon) for lon, lat in pts_ll]
    if cut:
        pts = [p for p in pts if not cut(*p)]
    if len(pts) < 2:
        return []
    px = densify([L.ll_to_px(lat, lon) for lat, lon in pts])
    walk = []
    last_pt = None
    for p in px:
        v = nearest_vertex(*p)
        if v is None:
            last_pt = None
            if walk and walk[-1] is not None:
                walk.append(None)   # off the map: break the chain
            continue
        if walk and walk[-1] == v:
            last_pt = p
            continue
        if walk and walk[-1] is not None and v not in graph[walk[-1]]:
            path = cheapest_path(graph, walk[-1], v, last_pt or p, p)
            if path is None:
                walk.append(None)
                walk.append(v)
            else:
                walk.extend(path[1:])
        else:
            walk.append(v)
        last_pt = p
    chains = []
    cur = []
    for v in walk:
        if v is None:
            if len(cur) > 1:
                chains.append(cut_loops(cur))
            cur = []
        else:
            cur.append(v)
    if len(cur) > 1:
        chains.append(cut_loops(cur))
    return chains


def canonical(e):
    h, d = e.split(":")
    c, r = L.parse_id(h)
    return L.edge_name(c, r, d)


def apply_rulings(rivers):
    """The hand rulings at this scale (cd_data): course corrections, then the minor rivers as explicit hexsides."""
    for key, ed in D.RIVER_EDITS_BY_SCALE.get(L.HEX_KM, {}).items():
        drop = {canonical(e) for e in ed.get("drop", "").split()}
        missing = drop - set(rivers[key])
        if missing:
            raise ValueError("river %s: ruling drops hexsides it does not have: %s" % (key, " ".join(sorted(missing))))
        kept = [e for e in rivers[key] if e not in drop]
        added = [canonical(e) for e in ed.get("add", "").split()]
        rivers[key] = kept + [e for e in added if e not in kept]
        print("%-8s ruling: dropped %d, added %d hexsides" % (key, len(drop), len(added)))
    for key, name, edges in D.MINOR_RIVERS_BY_SCALE.get(L.HEX_KM, []):
        rivers[key] = [canonical(e) for e in edges.split()]
        print("%-8s minor river, %d hexsides" % (key, len(rivers[key])))
    return rivers


def chains_to_edges(graph, chains):
    edges = []
    for ch in chains:
        for u, v in zip(ch, ch[1:]):
            c, r, d = graph[u][v]
            edges.append("%s:%s" % (L.hex_id(c, r), d))
    return edges


def main():
    graph, _ = L.build_vertex_graph()
    rivers = {}
    centrelines = {}
    for key, (matches, bbox) in RIVERS.items():
        polylines = []
        for name, name_en, geom in features():
            for want, want_en in matches:
                if name == want and (want_en is None or name_en == want_en):
                    for part in parts_of(geom):
                        pts = [p for p in part if bbox is None or (bbox[0] <= p[0] <= bbox[2] and bbox[1] <= p[1] <= bbox[3])]
                        if len(pts) >= 2:
                            polylines.append(pts)
        if key == "yellow":
            polylines.append([(lon, lat) for lat, lon in YELLOW_1938])
        if key == "yangtze":
            polylines.append([(lon, lat) for lat, lon in YANGTZE_ESTUARY])
        centrelines[key] = polylines
        all_edges = []
        for pl in polylines:
            chains = snap_polyline(graph, pl, CUTS.get(key))
            all_edges.extend(chains_to_edges(graph, chains))
        # one hexside once
        seen = set()
        uniq = [e for e in all_edges if not (e in seen or seen.add(e))]
        rivers[key] = uniq
        print("%-8s polylines %d  hexsides %d" % (key, len(polylines), len(uniq)))
    rivers = apply_rulings(rivers)
    json.dump(rivers, open(os.path.join(OUT, L.cache("rivers.json")), "w"), indent=0)
    # diagnostic
    img = Image.open(os.path.join(OUT, L.cache("diag_geo.png"))).convert("RGB")
    dr = ImageDraw.Draw(img)
    for key, pls in centrelines.items():
        for pl in pls:
            pts = [L.ll_to_px(p[1], p[0]) for p in pl]
            dr.line(pts, fill=(200, 40, 40), width=2)
    for key, edges in rivers.items():
        for e in edges:
            h, d = e.split(":")
            c, r = L.parse_id(h)
            a, b = L.FLAT["edge_corners"][d]
            dr.line([L.corner(c, r, a), L.corner(c, r, b)], fill=(20, 60, 220), width=5)
    img.save(os.path.join(OUT, L.cache("diag_rivers.png")))
    print("wrote rivers.json, diag_rivers.png")


if __name__ == "__main__":
    main()
# Copyright Ben Paul Wise. All Rights Reserved.
