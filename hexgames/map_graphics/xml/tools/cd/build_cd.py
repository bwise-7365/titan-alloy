# Copyright Ben Paul Wise. All Rights Reserved.
"""build_cd.py -- assemble the Circling Dragons hexsheet from the staged data.

Inputs (same folder): terrain_hex.json (stage4), rivers.json (stage3), cd_data.py (hand data),
lattice.py (projection and grid).  Output: circling-dragons.xml, validated separately by
tools/validate-xml.py and rendered by map_graphics/xml/hexsheet2svg.py.
"""
import json
import os
import sys
from xml.sax.saxutils import escape, quoteattr

sys.path.insert(0, os.path.dirname(__file__))
import lattice as L  # noqa: E402
import cd_data as D  # noqa: E402

OUT = os.path.dirname(os.path.abspath(__file__))

# How the rivers are written. "edges": one edge element per hexside, as the PGG sheet (HexMapEd draws river edges
# with its waviness; Ben, 2026-10-06); the confluences go into a comment. "paths": path chains with ids and a
# junction element at every confluence, named in ABC; for when hexview draws river path chains as river edges.
RIVER_FORM = "paths"

# Map version (build_cd.py [1|2|3]; 3 is the standard sheet circling-dragons.xml, Ben 2026-10-06; 1 and 2 write
# circling-dragons-v1.xml and -v2.xml for comparison). 1: the base sheet. 2: the Mongolian
# operations: named passes and Gobi tracks, waterless desert with its water points, the Kwantung Army's fortified
# zones, Soviet airfields and the fuel airlift. 3: the political layer: Mengjiang and the Inner Mongolian autonomy
# marker, the Tonghua redoubt, the weather divide, the speculative Soviet liaison route.
VERSION = 3

COUNTRY_NAMES = {1: "China", 2: "Mongolia", 3: "U.S.S.R.", 4: "Korea", 6: "Indochina", 7: "Indochina", 8: "Burma",
                 9: "Formosa", 10: "Japan", 11: "India", 12: "Siam", 13: "Kazakhstan"}
REGION_TINT = {"Mongolia": "tint-mn", "U.S.S.R.": "tint-su", "Korea": "tint-ko", "Indochina": "tint-ic",
               "Burma": "tint-bu", "Formosa": "tint-jp", "Japan": "tint-jp", "India": "tint-bu", "Siam": "tint-ic",
               "Kazakhstan": "tint-su", "Manchukuo": "tint-mk"}
SLOT_DIRS = ("n", "ne", "se", "s", "sw", "nw")


def at_scale(x, y):
    """A pixel position on the 100 km sheet moved to the same geography at this scale; the panels keep their
    size, so they stay in the same quiet corners.  The identity at 100 km."""
    k = L.REF_KM / L.HEX_KM
    return int(round(L.MARGIN_X + (x - L.MARGIN_X) * k)), int(round(L.MARGIN_Y + (y - L.MARGIN_Y) * k))


def a(**kw):
    """Attribute string from keyword arguments; None values are omitted; keys with _ become -."""
    parts = []
    for k, v in kw.items():
        if v is None or v == "":
            continue
        parts.append("%s=%s" % (k.replace("_", "-"), quoteattr(str(v))))
    return " ".join(parts)


class Builder:
    def __init__(self):
        self.terrain = json.load(open(os.path.join(OUT, L.cache("terrain_hex.json"))))
        self.rivers = json.load(open(os.path.join(OUT, L.cache("rivers.json"))))
        self.places = {}
        rows = list(D.PLACES) + (list(D.PLACES_V2) if 2 <= VERSION else []) + (list(D.PLACES_V3) if 3 <= VERSION else [])
        extra = {}
        if 2 <= VERSION:
            extra.update(D.PLACE_FLAGS_V2)
        if 3 <= VERSION:
            for pid, fl in D.PLACE_FLAGS_V3.items():
                extra[pid] = (extra.get(pid, "") + " " + fl).strip()
        for pid, label, lat, lon, kind, flags, terr in rows:
            h = D.PLACE_HEX_BY_SCALE.get(L.HEX_KM, {}).get(pid) or L.hex_id(*L.hex_of_ll(lat, lon))
            fl = flags.split() + extra.get(pid, "").split()
            if "wp" == kind and fl:
                kind = "mark"
            self.places[pid] = dict(id=pid, label=label, lat=lat, lon=lon, kind=kind, flags=fl, hex=h)
        self.hex_place = {}
        for p in self.places.values():
            self.hex_place.setdefault(p["hex"], []).append(p)
        self.lines = []
        self.problems = []
        self.river_edges = set()
        for edges in self.rivers.values():
            self.river_edges.update(edges)
        self.vgraph, _ = L.build_vertex_graph()

    # ---- helpers -------------------------------------------------------------------------------
    def cls(self, h):
        return self.terrain[h]["cls"]

    def is_sea(self, h):
        return self.cls(h) == "sea"

    def unit(self, h):
        """Political unit of a land hex: country name, with Manchukuo split out of China."""
        t = self.terrain[h]
        if t["cls"] == "sea":
            return None
        name = COUNTRY_NAMES.get(t["country"], "China")
        if name == "China" and self.in_manchukuo(h):
            return "Manchukuo"
        return name

    def in_manchukuo(self, h):
        t = self.terrain[h]
        return point_in_poly(t["lat"], t["lon"], D.MANCHUKUO)

    def place_label_for_hex(self, h):
        ps = [p for p in self.hex_place.get(h, []) if p["kind"] != "wp"]
        return ps[0]["label"].title() if ps and ps[0]["label"].isupper() else (ps[0]["label"] if ps else None)

    def chain(self, place_ids):
        hexes = []
        for pid in place_ids:
            if pid not in self.places:
                raise KeyError("unknown place %s" % pid)
            h = L.parse_id(self.places[pid]["hex"])
            if not hexes:
                hexes.append(h)
                continue
            seg = L.hex_line(hexes[-1], h)
            hexes.extend(seg[1:])
        # drop immediate back-steps
        out = []
        for h in hexes:
            if len(out) >= 2 and out[-2] == h:
                out.pop()
            elif not out or out[-1] != h:
                out.append(h)
        return [L.hex_id(*h) for h in out]

    def exit_hex(self, h, direction):
        c, r = L.parse_id(h)
        if direction is None:
            return None
        if direction == "w":
            cands = ["nw", "sw"]
        elif direction == "e":
            cands = ["ne", "se"]
        else:
            cands = [direction]
        for d in cands:
            n = L.neighbour(c, r, d)
            if L.in_grid(*n):
                return L.hex_id(*n)
        return None

    def exit_path(self, h, direction):
        """The hexes from beside h to the map edge in a compass direction: one step at 100 km, more at a
        smaller scale.  West and east alternate nw/sw (ne/se) to hold the row; a step that would put a land
        line into the sea tries the two neighbouring directions first."""
        if direction is None:
            return []
        alts = {"w": ("nw", "sw"), "e": ("ne", "se")}
        side = {"n": ("nw", "ne"), "s": ("sw", "se"), "ne": ("n", "se"), "se": ("s", "ne"), "sw": ("s", "nw"), "nw": ("n", "sw")}
        path, cur = [], h
        for k in range(12):
            c, r = L.parse_id(cur)
            if direction in alts:
                order = [alts[direction][k % 2], alts[direction][(k + 1) % 2]]
            else:
                order = [direction] + list(side.get(direction, ()))
            nxt = None
            for i, d in enumerate(order):
                n = L.neighbour(c, r, d)
                if not L.in_grid(*n):
                    continue
                cand = L.hex_id(*n)
                if self.is_sea(cand) and i < len(order) - 1:
                    continue        # keep a land line on land while there is another way
                nxt = cand
                break
            if nxt is None:
                break
            path.append(nxt)
            cur = nxt
            if self.on_edge(cur):
                break
        return path

    def on_edge(self, h):
        c, r = L.parse_id(h)
        return c in (0, L.COLS - 1) or r in (0, L.ROWS - 1)

    # ---- networks ------------------------------------------------------------------------------
    def build_network(self, kind, specs, exits):
        links = []
        for lid, name, ids in specs:
            hexes = self.chain(ids)
            exit_dir = None
            for xid, pid, direction in exits:
                if xid == lid:
                    if self.places[pid]["hex"] != hexes[-1] and self.places[pid]["hex"] != hexes[0]:
                        self.problems.append("%s exit place %s is not at an end" % (lid, pid))
                    if direction is not None:
                        ex = self.exit_path(self.places[pid]["hex"], direction)
                        if not ex:
                            self.problems.append("%s: no exit hex %s of %s" % (lid, direction, pid))
                        elif self.places[pid]["hex"] == hexes[-1]:
                            hexes.extend(ex)
                        else:
                            hexes[:0] = ex[::-1]
                    exit_dir = pid
            links.append(dict(id="%s-%s" % (kind[:2], lid), kind=kind, name=name, hexes=hexes, exit=exit_dir))
        # junctions: hexes shared by two or more links
        through = {}
        for lk in links:
            for h in lk["hexes"]:
                through.setdefault(h, set()).add(lk["id"])
        junctions = {h: sorted(ids) for h, ids in through.items() if len(ids) >= 2}
        for lk in links:
            ends = []
            for end in (lk["hexes"][0], lk["hexes"][-1]):
                if lk["exit"] and (end == self.places[lk["exit"]]["hex"] and self.on_edge(end) or
                                   (end not in self.hex_place and self.on_edge(end))):
                    ends.append("edge")
                elif end in junctions and lk["id"] in junctions[end]:
                    ends.append("junction")
                elif any(p["kind"] != "wp" for p in self.hex_place.get(end, [])):
                    ends.append("place")
                else:
                    ends.append("unexplained")
                    self.problems.append("%s ends unexplained at %s" % (lk["id"], end))
            lk["ends"] = ends
            for h in lk["hexes"]:
                if self.is_sea(h):
                    self.problems.append("%s enters sea hex %s" % (lk["id"], h))
            for x, y in zip(lk["hexes"], lk["hexes"][1:]):
                if not L.adjacent(L.parse_id(x), L.parse_id(y)):
                    self.problems.append("%s step %s-%s not adjacent" % (lk["id"], x, y))
        # shared stretches (two links on the same hexside) are reported, not forbidden
        steps = {}
        for lk in links:
            for x, y in zip(lk["hexes"], lk["hexes"][1:]):
                key = tuple(sorted((x, y)))
                steps.setdefault(key, []).append(lk["id"])
        for key, ids in steps.items():
            if len(ids) > 1:
                self.problems.append("note: %s share the step %s-%s" % ("/".join(ids), key[0], key[1]))
        return links, junctions

    # ---- borders -------------------------------------------------------------------------------
    def border_edges(self):
        out = []
        seen = set()
        for h in self.terrain:
            if self.is_sea(h):
                continue
            c, r = L.parse_id(h)
            ua = self.unit(h)
            for d, n in L.neighbours(c, r):
                hn = L.hex_id(*n)
                if self.is_sea(hn):
                    continue
                ub = self.unit(hn)
                if ua == ub:
                    continue
                e = L.edge_name(c, r, d)
                if e in seen or e in self.river_edges:
                    continue
                seen.add(e)
                out.append((e, "border-mk" if "Manchukuo" in (ua, ub) and "China" in (ua, ub) else "border"))
        return out

    # ---- document ------------------------------------------------------------------------------
    def build(self):
        o = self.lines
        o.append('<?xml version="1.0" encoding="UTF-8"?>')
        o.append('<sheet xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="hexsheet.xsd"')
        suffix = {1: "-v1", 2: "-v2", 3: ""}[VERSION] + L.SUFFIX
        subtitle = {1: "version 1: the base sheet", 2: "version 2: the Mongolian operations",
                    3: "design prototype"}[VERSION]
        if L.SUFFIX:
            subtitle += ", %g km version" % L.HEX_KM
        o.append('       id="circling-dragons%s" title="Circling Dragons: China 1944-1945 (%s)"' % (suffix, subtitle))
        o.append('       source="Designed sheet, not a scan: built by map_graphics/xml/tools/cd/build_cd.py from Natural Earth 1:50M land, lakes and countries, 1:10M river centerlines and ETOPO1 elevations on an Albers equal-area conic (27 N, 45 N; 35 N 118 E); %g km between hex centers; named ranges, places and networks ruled by hand from the design memorandum (Circling Dragons/circling_dragons_map_and_rules_V3.tex)"' % L.HEX_KM)
        o.append('       width="%d" height="%d" background="paper" font="Georgia, Times New Roman, serif" urban="symbol" junctions="explicit">' % (L.SHEET_W, L.SHEET_H))
        o.append('  <grid id="main" orientation="flat" offset="odd" cols="%d" rows="%d" size="%g" ox="%g" oy="%g" id-format="{col:02}{row:02}" terrain="sea" id-side="n"/>'
                 % (L.COLS, L.ROWS, L.SIZE, L.OX, L.OY))
        o.append('  <palette>')
        for cid, val, name in [
            # paper, seablue and tan are Ben's values of 2026-10-06; gametrack keeps the old paper for the tracks
            ("paper", "#D76139", "sheet background"), ("gametrack", "#f3efe2", "track and box cells"),
            ("seablue", "#6FC6F3", "sea"), ("tan", "#F1F4E3", "clear"),
            ("ochre", "#c9a85c", "broken"), ("umber", "#8f6b3f", "mountain"), ("sand", "#ead98a", "arid / steppe"),
            ("greygreen", "#b9c9b4", "marsh / floodplain"), ("grid", "#8f8a78", "grid over land"),
            ("gridw", "#5a9ccc", "grid over sea"), ("ink", "#111111", None), ("white", "#ffffff", None),
            ("riverblue", "#2f74c0", "river"), ("lakeblue", "#6fa8dc", "lake"), ("railgrey", "#3c3a36", "railway"),
            ("roadbrown", "#a0522d", "strategic road"), ("borderred", "#b0281f", "international border"),
            ("mkpurple", "#7a4b8a", "Manchukuo boundary"), ("cityred", "#9b1c1c", "major city"),
            ("gold", "#e8b923", "city, capital"), ("cyan", "#29a8d8", "port"), ("navy", "#1f3a6e", "airfield"),
            ("label", "#6b6656", "area names"), ("panel", "#f7f4ea", None), ("panelink", "#2b2b2b", None),
            ("copyright", "#000000", "copyright notice on the margin"),
            ("tint-su", "#e9b8b0", "U.S.S.R."), ("tint-mn", "#d9c9a0", "Mongolia"), ("tint-ko", "#c8d8a8", "Korea"),
            ("tint-ic", "#e0d0a0", "Indochina / Siam"), ("tint-bu", "#c8c8e0", "Burma / India"),
            ("tint-jp", "#e8c0d8", "Japan / Formosa"), ("tint-mk", "#d8c8e0", "Manchukuo"),
            ("kmt", "#2b5f9e", "KMT"), ("ccp", "#c9231c", "CCP"), ("japan", "#e8842a", "Japan"),
            ("soviet", "#d0281f", "Soviet entry hex")] + (
            [("desertsand", "#ddd0a8", "desert (waterless)"), ("trackbrown", "#8a6a3c", "Gobi track"),
             ("fortink", "#2b2b2b", "fortified zone"), ("passwhite", "#fbfbf6", "mountain pass")] if 2 <= VERSION else []) + (
            [("tint-mj", "#d8c0a8", "Mengjiang"), ("redoubtbrown", "#5a3a2a", "Tonghua redoubt"),
             ("weatherblue", "#4a6fa5", "weather divide"), ("autonomygreen", "#3a8f4a", "Inner Mongolian autonomy"),
             ("courierred", "#c9231c", "Soviet liaison route")] if 3 <= VERSION else []):
            o.append('    <color %s/>' % a(id=cid, value=val, name=name))
        o.append('  </palette>')
        o.append('  <terrains>')
        o.append('    <terrain id="sea" name="Sea" fill="seablue" stroke="gridw"/>')
        o.append('    <terrain id="clear" name="Clear / plain" fill="tan" stroke="grid"/>')
        o.append('    <terrain id="broken" name="Broken / hills" fill="ochre" stroke="grid"/>')
        o.append('    <terrain id="mountain" name="Mountain" fill="umber" stroke="grid"/>')
        o.append('    <terrain id="marsh" name="Marsh / floodplain" fill="greygreen" stroke="grid" pattern="dots"/>')
        o.append('    <terrain id="steppe" name="Arid / steppe" fill="sand" stroke="grid"/>')
        if 2 <= VERSION:
            o.append('    <terrain id="desert" name="Desert (waterless)" fill="desertsand" stroke="grid"/>')
        o.append('  </terrains>')
        o.append('  <lines>')
        o.append('    <line id="river" stroke="riverblue" width="7"/>')
        if self.MINOR:
            o.append('    <line id="minor-river" stroke="riverblue" width="3.5"/>')
        o.append('    <line id="border" stroke="borderred" width="4" dash="12,4,2,4"/>')
        o.append('    <line id="border-mk" stroke="mkpurple" width="3" dash="8,5"/>')
        o.append('    <line id="rail" stroke="railgrey" width="3" ticks="true"/>')
        o.append('    <line id="road" stroke="roadbrown" width="4" dash="9,5"/>')
        if 2 <= VERSION:
            o.append('    <line id="track" stroke="trackbrown" width="2.5" dash="2,5"/>')
        if 3 <= VERSION:
            o.append('    <line id="courier" stroke="courierred" width="1.5" dash="1,6"/>')
            o.append('    <line id="redoubt" stroke="redoubtbrown" width="5" dash="14,6"/>')
            o.append('    <line id="weather" stroke="weatherblue" width="3" dash="3,9"/>')
        o.append('  </lines>')
        o.append('  <legend>')
        o.append('    <mark id="airfield" name="Major airfield (Ichi-Go / 1945 objective)" shape="triangle" color="navy" size="0.3 0.3"/>')
        o.append('    <mark id="riverport" name="River port (Yangtze / Sungari strategic transport)" shape="pictogram" pictogram="port" color="riverblue" size="0.26 0.26"/>')
        o.append('    <mark id="lake" name="Lake" shape="ellipse" color="lakeblue" size="0.5 0.34"/>')
        o.append('    <mark id="soviet-entry" name="Soviet entry hex (August 1945)" shape="star" color="soviet" size="0.34 0.34"/>')
        if 2 <= VERSION:
            o.append('    <mark id="pass" name="Mountain pass" shape="diamond" color="passwhite" size="0.3 0.3"/>')
            o.append('    <mark id="fort" name="Kwantung Army fortified zone" shape="rect" color="fortink" size="0.22 0.22"/>')
            o.append('    <mark id="well" name="Water point (Gobi)" shape="ellipse" color="lakeblue" size="0.22 0.22"/>')
        if 3 <= VERSION:
            o.append('    <mark id="autonomy" name="Inner Mongolian autonomy declaration (Sonid, September 1945)" shape="triangle" color="autonomygreen" size="0.3 0.3"/>')
        o.append('  </legend>')
        # terrain
        if 2 <= VERSION:
            for h, t in self.terrain.items():
                if "steppe" == t["cls"] and any(point_in_poly(t["lat"], t["lon"], p) for p in D.DESERT):
                    t["cls"] = "desert"
        by_cls = {}
        for h, t in self.terrain.items():
            by_cls.setdefault(t["cls"], []).append(h)
        o.append('  <!-- terrain: one value per hex; the grid default is sea -->')
        for cls in ("clear", "broken", "mountain", "steppe", "desert", "marsh"):
            ids = sorted(by_cls.get(cls, []))
            if ids:
                o.append('  <hexes terrain="%s" ids="%s"/>' % (cls, " ".join(ids)))
        # rivers: hexside edges, as the PGG sheet writes them (Ben, 2026-10-06: HexMapEd draws river edges with its
        # waviness, path chains it does not). The chains and confluences are still computed, so the hexsides are
        # the cleaned chains and the confluences are recorded here in ABC; a junction element needs path chains,
        # so none is written while the rivers are edges.
        paths, junctions = self.river_paths()
        if "paths" == RIVER_FORM:
            o.append('  <!-- rivers along hexsides: only the operational barriers of the design memorandum. Each river is one or')
            o.append('       more path chains; a confluence is a junction at a hex corner named in ABC (flat-top: A due east, B the')
            o.append('       north-west corner, C the south-west corner; hexcoord Grid.cpp basisOf). The Yalu and Tumen touch at')
            o.append('       their sources without joining: no junction there. -->')
            for p in paths:
                line = "minor-river" if p["river"] in self.MINOR else "river"
                o.append('  <path %s/>' % a(id=p["id"], kind="river", name=p["name"], line=line, edges=" ".join(p["edges"]),
                                            ends=" ".join(p["ends"])))
            for j in junctions:
                o.append('  <junction %s/>' % a(at=j["at"], paths=" ".join(j["paths"]), name=j["name"]))
        else:
            o.append('  <!-- rivers along hexsides: only the operational barriers of the design memorandum, written as edge')
            o.append('       elements like the PGG sheet so that HexMapEd draws them with its waviness. Confluences, at hex corners')
            o.append('       named in ABC (flat-top: A due east, B the north-west corner, C the south-west corner; hexcoord')
            o.append('       Grid.cpp basisOf):')
            for j in junctions:
                o.append('         %s at %s (%s)' % (j["name"], j["at"], ", ".join(j["paths"])))
            o.append('       The Yalu and Tumen touch at their sources without joining. -->')
            for p in paths:
                o.append('  <!-- %s: %s, %s to %s -->' % (p["id"], p["name"], p["ends"][0], p["ends"][1]))
                for e in p["edges"]:
                    o.append('  <edge at="%s" line="river"/>' % e)
        # borders
        o.append('  <!-- borders between political units where no river marks them -->')
        for e, line in self.border_edges():
            o.append('  <edge at="%s" line="%s"/>' % (e, line))
        # rails
        rails, rail_j = self.build_network("rail", D.RAILS, D.RAIL_EXITS)
        o.append('  <!-- railways: the trunks the memorandum names, Peiping to Hankow and Canton, Longhai, Jinpu, the Manchurian lines, Korea, the Soviet feeders -->')
        for lk in rails:
            o.append('  <link %s/>' % a(id=lk["id"], kind="rail", name=lk["name"], line="rail", hexes=" ".join(lk["hexes"]), ends=" ".join(lk["ends"])))
        for h in sorted(rail_j):
            o.append('  <junction %s/>' % a(at=h, links=" ".join(rail_j[h]), name=self.place_label_for_hex(h)))
        roads, road_j = self.build_network("road", D.ROADS, D.ROAD_EXITS)
        o.append('  <!-- strategic roads: only those whose removal would change a decision (Burma Road, the Guizhou roads, Sichuan - Shaanxi, Hengyang - Chihchiang - Guiyang, Nanning - Lang Son) -->')
        for lk in roads:
            o.append('  <link %s/>' % a(id=lk["id"], kind="road", name=lk["name"], line="road", hexes=" ".join(lk["hexes"]), ends=" ".join(lk["ends"])))
        for h in sorted(road_j):
            o.append('  <junction %s/>' % a(at=h, links=" ".join(road_j[h]), name=self.place_label_for_hex(h)))
        if 2 <= VERSION:
            tracks, track_j = self.build_network("track", D.TRACKS_V2, [])
            o.append('  <!-- Gobi tracks and the Soviet supply road: the axes of August 1945, no more -->')
            for lk in tracks:
                o.append('  <link %s/>' % a(id=lk["id"], kind="track", name=lk["name"], line="track", hexes=" ".join(lk["hexes"]), ends=" ".join(lk["ends"])))
            for h in sorted(track_j):
                o.append('  <junction %s/>' % a(at=h, links=" ".join(track_j[h]), name=self.place_label_for_hex(h)))
        if 3 <= VERSION:
            couriers, courier_j = self.build_network("courier", D.COURIER_V3, [])
            o.append('  <!-- the speculative Soviet liaison route from Yan an across the Ordos -->')
            for lk in couriers:
                o.append('  <link %s/>' % a(id=lk["id"], kind="courier", name=lk["name"], line="courier", hexes=" ".join(lk["hexes"]), ends=" ".join(lk["ends"])))
            for h in sorted(courier_j):
                o.append('  <junction %s/>' % a(at=h, links=" ".join(courier_j[h]), name=self.place_label_for_hex(h)))
        # places
        o.append('  <!-- operational nodes -->')
        for h in sorted(self.hex_place):
            ps = [p for p in self.hex_place[h] if p["kind"] != "wp"]
            if not ps:
                continue
            p = ps[0]
            glyphs = []
            flags = p["flags"]
            used = set()
            if "mark" != p["kind"]:
                if "capital" in flags:
                    glyphs.append('<glyph symbol="capital" color="gold" scale="1.3"/>')
                elif p["kind"] == "major":
                    glyphs.append('<glyph symbol="city-major" color="cityred"/>')
                elif p["kind"] == "city":
                    glyphs.append('<glyph symbol="city" color="gold"/>')
                else:
                    glyphs.append('<glyph symbol="town" color="white"/>')
                used.add("c")
            if "port" in flags:
                slot = self.sea_slot(h) or "se"
                used.add(slot)
                glyphs.append('<glyph symbol="port" slot="%s" color="cyan"/>' % slot)
            if "rport" in flags:
                slot = next(s for s in ("sw", "se", "nw", "ne") if s not in used)
                used.add(slot)
                glyphs.append('<glyph mark="riverport" slot="%s"/>' % slot)
            if "air" in flags:
                slot = next(s for s in ("ne", "nw", "se", "sw") if s not in used)
                used.add(slot)
                glyphs.append('<glyph mark="airfield" slot="%s"/>' % slot)
            if "sov" in flags:
                slot = next(s for s in ("n", "nw", "ne", "sw") if s not in used)
                used.add(slot)
                glyphs.append('<glyph mark="soviet-entry" slot="%s"/>' % slot)
            for flag, mark in (("pass", "pass"), ("fort", "fort"), ("well", "well"), ("autonomy", "autonomy")):
                if flag in flags:
                    slot = "c" if "c" not in used else next(s for s in ("s", "se", "sw", "n", "ne", "nw") if s not in used)
                    used.add(slot)
                    glyphs.append('<glyph mark="%s" slot="%s"/>' % (mark, slot))
            o.append('  <hex id="%s" name=%s>%s</hex>' % (h, quoteattr(p["label"]), "".join(glyphs)))
        for h, t in self.terrain.items():
            if t.get("lake_name"):
                o.append('  <hex id="%s" name=%s><glyph mark="lake"/></hex>' % (h, quoteattr(t["lake_name"])))
        # regions
        o.append('  <!-- political units: tints for the non-Chinese territories and Manchukuo; China itself untinted -->')
        units = {}
        for h in self.terrain:
            u = self.unit(h)
            if u and u != "China":
                units.setdefault(u, []).append(h)
        for u in sorted(units):
            o.append('  <region %s/>' % a(layer="country", name=u, hexes=" ".join(sorted(units[u])), tint=REGION_TINT.get(u, "tint-mn"), opacity="0.28"))
        if 3 <= VERSION:
            mj = sorted(h for h, t in self.terrain.items()
                        if "sea" != t["cls"] and "China" == self.unit(h) and point_in_poly(t["lat"], t["lon"], D.MENGJIANG))
            o.append('  <!-- the political layer: Mengjiang (Prince De, under Japan), the Kwantung Army redoubt, the weather divide.')
            o.append('       UNDER ACTIVE DEVELOPMENT (Ben, 2026-10-06): the elements are placed, the rules behind them are provisional. -->')
            o.append('  <region %s/>' % a(layer="political", name="Mengjiang", hexes=" ".join(mj), tint="tint-mj", opacity="0.3"))
            o.append('  <region %s/>' % a(layer="political", name="Tonghua redoubt", hexes=L.rescale_ids(D.REDOUBT_HEXES), outline="redoubt"))
            for e in self.weather_divide():
                o.append('  <edge at="%s" line="weather"/>' % e)
        # labels
        o.append('  <!-- place names -->')
        for h in sorted(self.hex_place):
            ps = [p for p in self.hex_place[h] if p["kind"] != "wp"]
            if not ps:
                continue
            p = ps[0]
            if not p["label"]:
                continue
            big = p["kind"] == "major" or "capital" in p["flags"]
            slot = LABEL_SLOT.get(p["id"], "s")
            o.append('  <label %s/>' % a(text=p["label"], at=h, slot=slot, size=18 if big else (11 if "mark" == p["kind"] else 13),
                                         weight="bold" if big else "normal", color="ink", halo="true"))
        o.append('  <!-- area names -->')
        area_labels = list(D.AREA_LABELS) + (list(D.LABELS_V2) if 2 <= VERSION else []) + (list(D.LABELS_V3) if 3 <= VERSION else [])
        area_labels += list(D.LABELS_BY_SCALE.get(L.HEX_KM, []))
        for text, lat, lon, size, angle, kind in area_labels:
            x, y = L.ll_to_px(lat, lon)
            colour = {"country": "label", "sea": "white", "range": "label", "river": "riverblue", "area": "label"}[kind]
            italic = kind in ("sea", "river", "area")
            weight = "bold" if kind in ("country", "range") else "normal"
            o.append('  <label %s/>' % a(text=text, x="%.0f" % x, y="%.0f" % y, size=size, angle=angle, color=colour,
                                         italic="true" if italic else None, weight=weight,
                                         spacing=3 if kind == "country" else (2 if kind == "range" else 0), halo="true"))
        # the copyright notice, black on the orange margin in the lower-left and upper-right corners (Ben, 2026-10-06):
        # each label is centred in its margin band, flush with the left or right edge of the hex area
        o.append('  <!-- copyright -->')
        notice = "Copyright Ben Paul Wise"
        o.append('  <label %s/>' % a(text=notice, x="%.0f" % L.MARGIN_X, y="%.0f" % (L.SHEET_H - L.MARGIN_Y / 2), size=26,
                                     color="copyright", anchor="start"))
        o.append('  <label %s/>' % a(text=notice, x="%.0f" % (L.SHEET_W - L.MARGIN_X), y="%.0f" % (L.MARGIN_Y / 2), size=26,
                                     color="copyright", anchor="end"))
        self.panels()
        o.append('</sheet>')
        return "\n".join(o) + "\n"

    # ---- rivers as path chains with ABC vertex junctions ------------------------------------------
    RIVER_NAMES = {"yangtze": "Yangtze", "yellow": "Yellow River (1938-47 course below Huayuankou)", "han": "Han",
                   "xiang": "Xiang", "amur": "Amur", "argun": "Argun", "ussuri": "Ussuri", "sungari": "Sungari",
                   "nen": "Nen", "liao": "Liao", "yalu": "Yalu", "tumen": "Tumen"}
    # minor rivers at this scale (cd_data): drawn with the thin minor-river line, kept whole by the spur cleaning
    MINOR = {k for k, _, _ in D.MINOR_RIVERS_BY_SCALE.get(L.HEX_KM, [])}
    RIVER_NAMES.update({k: n for k, n, _ in D.MINOR_RIVERS_BY_SCALE.get(L.HEX_KM, [])})
    # Flat-top corner name -> ABC vector from the hex centre (hexcoord Grid.cpp basisOf: A due east, B to the
    # north-west corner, C to the south-west corner; the opposite corners are the negatives).
    CORNER_ABC = {"e": "A", "w": "-A", "nw": "B", "se": "-B", "sw": "C", "ne": "-C"}
    # River pairs that share a vertex without joining (sources on the same mountain): no junction.
    NO_JUNCTION = {frozenset(("yalu", "tumen"))}

    @staticmethod
    def edge_parts(e):
        h, d = e.split(":")
        c, r = L.parse_id(h)
        return c, r, d

    def hexes_at(self, v):
        """The in-grid hexes around a vertex key."""
        out = set()
        for e in self.vgraph[v].values():
            c, r, d = e
            out.add((c, r))
            n = L.neighbour(c, r, d)
            if L.in_grid(*n):
                out.add(n)
        return out

    def vertex_ref(self, v):
        """HEX:CORNER in ABC for a vertex, named from the first in-grid hex around it."""
        for (c, r) in sorted(self.hexes_at(v)):
            for name in L.FLAT["corners"]:
                if L.vkey(L.corner(c, r, name)) == v:
                    return "%s:%s" % (L.hex_id(c, r), self.CORNER_ABC[name])
        raise ValueError("vertex %r is on no hex" % (v,))

    # tributary -> the river it joins; a tributary's end on the main river's chain is a confluence
    TRIBUTARY = {"han": "yangtze", "xiang": "yangtze", "nen": "sungari", "sungari": "amur", "ussuri": "amur",
                 "argun": "amur"}
    # chain ends the snapping leaves short: extend along hexsides to the sea (a mouth the centreline data
    # stops before) or to the map edge (a river entering from off the map)
    # A free river end is extended to the sea, the map edge, or (a tributary that stops a hexside or two short of
    # its main river, the Ussuri at Khabarovsk on the 75 km sheet) to another river, toward the given point.
    EXTEND_TO = {"yalu": ("sea", (39.85, 124.30)), "yellow": ("edge", (35.80, 102.00)), "ussuri": ("amur", (48.48, 135.08))}

    @staticmethod
    def adjacency(edges):
        """vertex -> [hexside names] and hexside -> (vertex, vertex)."""
        adj = {}
        ends_of = {}
        for e in edges:
            va, vb = L.edge_vertices(*Builder.edge_parts(e))
            ends_of[e] = (va, vb)
            adj.setdefault(va, []).append(e)
            adj.setdefault(vb, []).append(e)
        return adj, ends_of

    @staticmethod
    def chains_of(edges, river_of):
        """Simple chains over the union of every river's hexsides: a chain is cut at every vertex of degree
        other than two and wherever the river changes, so each chain belongs to one river and a tributary
        meeting a main river mid-chain cuts the main river there.  Returns [(edges, vertices)] with one more
        vertex than edges."""
        adj, ends_of = Builder.adjacency(edges)
        used = set()
        chains = []

        def other(e, v):
            return ends_of[e][1] if ends_of[e][0] == v else ends_of[e][0]

        def walk(v0, e0):
            chain = [e0]
            verts = [v0, other(e0, v0)]
            used.add(e0)
            while len(adj[verts[-1]]) == 2:
                nxt = [x for x in adj[verts[-1]] if x not in used]
                if not nxt or river_of[nxt[0]] != river_of[chain[-1]]:
                    break
                e = nxt[0]
                used.add(e)
                chain.append(e)
                verts.append(other(e, verts[-1]))
            return chain, verts

        starts = [v for v, es in adj.items() if len(es) != 2]
        # a vertex where the river changes at degree two is also a chain end
        for v, es in adj.items():
            if len(es) == 2 and river_of[es[0]] != river_of[es[1]]:
                starts.append(v)
        for v in starts:
            for e in adj[v]:
                if e not in used:
                    chains.append(walk(v, e))
        for e in edges:               # pure cycles, should not happen
            if e not in used:
                chains.append(walk(ends_of[e][0], e))
        return chains

    def cheapest_vertex_path(self, v0, goal, toward, limit=4):
        """Shortest hexside path from v0 to a goal vertex, over land hexsides only; among the goal vertices at
        the shortest distance, the one nearest the pixel point `toward` (a river mouth, a map edge)."""
        seen = {v0: None}
        frontier = [v0]
        for _ in range(limit):
            nxt = []
            hits = []
            for v in frontier:
                for w, (c, r, d) in self.vgraph[v].items():
                    if w in seen or self.touches_sea(L.edge_name(c, r, d)):
                        continue
                    seen[w] = v
                    if goal(w):
                        hits.append(w)
                    nxt.append(w)
            if hits:
                px, py = toward
                w = min(hits, key=lambda k: (L.vpos(k)[0] - px) ** 2 + (L.vpos(k)[1] - py) ** 2)
                path = [w]
                while seen[path[-1]] is not None:
                    path.append(seen[path[-1]])
                return path[::-1]
            frontier = nxt
        return None

    def river_paths(self):
        river_of = {}
        for key, edges in self.rivers.items():       # main rivers come before their tributaries in RIVERS,
            for e in edges:                          # so a shared hexside stays with the main river
                if not self.touches_sea(e) and e not in river_of:
                    river_of[e] = key

        def chains():
            return self.chains_of(list(river_of), river_of)

        # 1. drop snapping spurs: a chain of one or two hexsides hanging off a fork, ending free inland
        while True:
            adj, _ = self.adjacency(list(river_of))
            spurs = []
            for chain, verts in chains():
                if len(chain) > 2 or river_of[chain[0]] in self.MINOR:
                    continue
                for free, fork in ((verts[0], verts[-1]), (verts[-1], verts[0])):
                    if len(adj[free]) == 1 and len(adj[fork]) >= 3 and len(self.hexes_at(free)) == 3 \
                            and not any(self.is_sea(L.hex_id(*h)) for h in self.hexes_at(free)):
                        spurs.append(chain)
                        break
            if not spurs:
                break
            for chain in spurs:
                self.problems.append("note: river %s spur %s dropped" % (river_of[chain[0]], " ".join(chain)))
                for e in chain:
                    del river_of[e]
        # 2. a piece of a main river that begins where a tributary ends (degree two) and runs to a fork of the
        #    main river is the tributary's lower course under the main river's name in the data (the Xiang's
        #    last hexside, the Nen below Qiqihar): relabel it
        while True:
            adj, _ = self.adjacency(list(river_of))
            at_end = {}
            cl = chains()
            for i, (chain, verts) in enumerate(cl):
                at_end.setdefault(verts[0], []).append(i)
                at_end.setdefault(verts[-1], []).append(i)
            relabel = None
            for i, (chain, verts) in enumerate(cl):
                main = river_of[chain[0]]
                for meet, fork in ((verts[0], verts[-1]), (verts[-1], verts[0])):
                    if len(adj[meet]) == 2 and len(adj[fork]) >= 3:
                        others = [cl[j] for j in at_end[meet] if j != i]
                        if len(others) == 1 and self.TRIBUTARY.get(river_of[others[0][0][0]]) == main:
                            relabel = (chain, river_of[others[0][0][0]])
                            break
                if relabel:
                    break
            if not relabel:
                break
            chain, trib = relabel
            self.problems.append("note: river %s piece %s relabelled %s (its mouth)" % (river_of[chain[0]], " ".join(chain), trib))
            for e in chain:
                river_of[e] = trib
        # 3. extend ends the data leaves short: to the sea (a mouth) or to the map edge (a river entering)
        for key, (target, latlon) in self.EXTEND_TO.items():
            adj, _ = self.adjacency(list(river_of))
            toward = L.ll_to_px(*latlon)
            for chain, verts in chains():
                if river_of[chain[0]] != key:
                    continue
                # the end nearer the target point is the one to extend
                v = min((verts[0], verts[-1]),
                        key=lambda k: (L.vpos(k)[0] - toward[0]) ** 2 + (L.vpos(k)[1] - toward[1]) ** 2)
                if len(adj[v]) == 1:
                    hexes = self.hexes_at(v)
                    if len(hexes) < 3 or any(self.is_sea(L.hex_id(*h)) for h in hexes):
                        continue
                    if target == "sea":
                        goal = lambda w: any(self.is_sea(L.hex_id(*h)) for h in self.hexes_at(w))
                    elif target == "edge":
                        goal = lambda w: len(self.hexes_at(w)) < 3
                    else:
                        goal = lambda w, t=target, a=adj: any(river_of.get(e) == t for e in a.get(w, ()))   # adj: vertex -> its river hexsides
                    path = self.cheapest_vertex_path(v, goal, toward)
                    if path:
                        added = [L.edge_name(*self.vgraph[a][b]) for a, b in zip(path, path[1:])]
                        for e in added:
                            river_of[e] = key
                        self.problems.append("note: river %s extended to the %s by %s" % (key, target, " ".join(added)))
        # 4. the chains, then the junctions where two or more chains end at one vertex
        paths = []
        at_vertex = {}
        counter = {}
        for chain, verts in chains():
            key = river_of[chain[0]]
            counter[key] = counter.get(key, 0) + 1
            pid = "rv-%s-%d" % (key, counter[key])
            paths.append(dict(id=pid, river=key, name=self.RIVER_NAMES[key], edges=chain, v=(verts[0], verts[-1])))
            at_vertex.setdefault(verts[0], []).append(pid)
            at_vertex.setdefault(verts[-1], []).append(pid)
        by_id = {p["id"]: p for p in paths}
        junctions = []
        joined = set()
        for v, pids in at_vertex.items():
            if len(set(pids)) < 2:
                continue
            rivers = {by_id[p]["river"] for p in pids}
            if len(rivers) == 2 and frozenset(rivers) in self.NO_JUNCTION:
                continue
            names = sorted({self.RIVER_NAMES[r].split(" (")[0] for r in rivers})
            name = (" - ".join(names) + " confluence") if len(names) > 1 else (names[0] + " fork")
            junctions.append(dict(at=self.vertex_ref(v), paths=sorted(set(pids)), name=name))
            joined.add(v)
        for p in paths:
            reasons = []
            for v in p["v"]:
                hexes = self.hexes_at(v)
                if v in joined:
                    reasons.append("junction")
                elif len(hexes) < 3:
                    reasons.append("edge")
                elif any(self.is_sea(L.hex_id(*h)) for h in hexes):
                    reasons.append("sea")
                else:
                    reasons.append("source")
            p["ends"] = reasons
        junctions.sort(key=lambda j: j["at"])
        return paths, junctions

    def touches_sea(self, edge):
        """A river hexside with a sea hex on either side is the coast, not a river (the chain still drains
        there); a hexside on the map edge is kept (the Amur along the north edge)."""
        h, d = edge.split(":")
        c, r = L.parse_id(h)
        n = L.neighbour(c, r, d)
        if self.is_sea(h):
            return True
        return L.in_grid(*n) and self.is_sea(L.hex_id(*n))

    def weather_divide(self):
        """Hexsides between land hexes north and south of the weather divide, canonical names."""
        out = []
        seen = set()
        for h, t in self.terrain.items():
            if "sea" == t["cls"]:
                continue
            c, r = L.parse_id(h)
            north = D.WEATHER_DIVIDE_LAT <= t["lat"]
            for d, n in L.neighbours(c, r):
                hn = L.hex_id(*n)
                tn = self.terrain[hn]
                if "sea" == tn["cls"] or north == (D.WEATHER_DIVIDE_LAT <= tn["lat"]):
                    continue
                e = L.edge_name(c, r, d)
                if e not in seen:
                    seen.add(e)
                    out.append(e)
        return out

    def sea_slot(self, h):
        c, r = L.parse_id(h)
        for d, n in L.neighbours(c, r):
            if self.is_sea(L.hex_id(*n)):
                return d
        return None

    def panels(self):
        o = self.lines
        o.append('  <!-- furniture: the tracks and boxes the current rules concept calls for; placeholders for the prototype.')
        o.append('       Laid in the corners the war left quiet, as Downfall lays its tracks: Soviet staging and the Pacific War')
        o.append('       track over Mongolia, the Far Eastern staging over the Sea of Japan, the Hump over Laos, the rest at sea.')
        o.append('       Panel children are grouped by kind (texts, boxes, tracks, tables): the reference renderer draws them in')
        o.append('       document order and hexview kind by kind, so grouping keeps the two drawings equal. -->')
        # ---- north-west: Mongolia (the Soviet entry side) --------------------------------------------------
        NX, NY = at_scale(70, 70)
        o.append('  <panel id="pacific" x="%d" y="%d" w="590" h="100" fill="panel" stroke="ink">' % (NX, NY))
        o.append('    <text x="20" y="24" size="15" weight="bold" color="panelink">Pacific War track (exogenous): Soviet entry and surrender change the rules</text>')
        o.append('    <track x="20" y="36" cell-w="54" cell-h="44" cells="1 2 3 4 5 6 7 Soviet_entry Surrender Endgame" fill="gametrack"/>')
        o.append('  </panel>')
        o.append('  <panel id="soviet-west" x="%d" y="%d" w="590" h="190" fill="panel" stroke="ink">' % (NX, NY + 115))
        o.append('    <text x="20" y="24" size="15" weight="bold" color="panelink">Soviet staging: Trans-Baikal Front and Mongolia</text>')
        o.append('    <text x="20" y="44" size="12" color="panelink">Off the map until Soviet entry; then enters at the starred hexes: Manzhouli, Tamsag Bulag,</text>')
        o.append('    <text x="20" y="60" size="12" color="panelink">and Sain Shand (Pliyev&apos;s Soviet-Mongolian group, 450 km across the Gobi to Dolonnor and Kalgan).</text>')
        o.append('    <text x="20" y="76" size="12" color="panelink">Choibalsan is the railhead from Borzya (1939); everything beyond it moved by truck.</text>')
        if 2 <= VERSION:
            o.append('    <box label="Trans-Baikal Front" x="20" y="90" w="175" h="80" fill="gametrack"/>')
            o.append('    <box label="Soviet-Mongolian group" x="207" y="90" w="175" h="80" fill="gametrack"/>')
            o.append('    <box label="Fuel airlift (Tamsag, Matad, Choibalsan)" x="394" y="90" w="175" h="80" fill="gametrack"/>')
        else:
            o.append('    <box label="Trans-Baikal Front" x="20" y="90" w="250" h="80" fill="gametrack"/>')
            o.append('    <box label="Soviet-Mongolian group" x="290" y="90" w="250" h="80" fill="gametrack"/>')
        o.append('  </panel>')
        if 3 <= VERSION:
            o.append('  <panel id="weather-table" x="%d" y="%d" w="590" h="150" fill="panel" stroke="ink">' % (NX, NY + 320))
            o.append('    <text x="20" y="24" size="15" weight="bold" color="panelink">Weather: the divide runs along the Qinling and the Huai (dashed blue line)</text>')
            o.append('    <text x="20" y="44" size="12" color="panelink">Cold: steppe supply distance doubles again, movement -1. Mud: Manchurian plain, movement -1. Monsoon: air support -1.</text>')
            o.append('    <table x="20" y="56" cell-w="110" cell-h="26" size="11">')
            o.append('      <row><cell>Zone</cell><cell>Nov - Mar</cell><cell>Apr - May</cell><cell>Jun - Sep</cell><cell>Oct</cell></row>')
            o.append('      <row><cell>North</cell><cell>Cold</cell><cell>Mud</cell><cell>Clear</cell><cell>Clear</cell></row>')
            o.append('      <row><cell>South</cell><cell>Clear</cell><cell>Clear</cell><cell>Monsoon</cell><cell>Clear</cell></row>')
            o.append('    </table>')
            o.append('  </panel>')
        # ---- east: the Sea of Japan (the Far Eastern Fronts' side) -----------------------------------------
        o.append('  <panel id="soviet-east" x="%d" y="%d" w="260" h="260" fill="panel" stroke="ink">' % at_scale(2240, 790))
        o.append('    <text x="16" y="24" size="15" weight="bold" color="panelink">Soviet staging: Far East</text>')
        o.append('    <text x="16" y="44" size="12" color="panelink">Enters at the starred hexes:</text>')
        o.append('    <text x="16" y="60" size="12" color="panelink">Heihe, Tongjiang, Khabarovsk,</text>')
        o.append('    <text x="16" y="76" size="12" color="panelink">Iman, Suifenhe, Tumen.</text>')
        o.append('    <box label="1st Far Eastern Front" x="16" y="90" w="228" h="70" fill="gametrack"/>')
        o.append('    <box label="2nd Far Eastern Front" x="16" y="172" w="228" h="70" fill="gametrack"/>')
        o.append('  </panel>')
        # ---- south-west: Laos (the Burma side) ------------------------------------------------------------
        o.append('  <panel id="hump" x="%d" y="%d" w="250" h="150" fill="panel" stroke="ink">' % at_scale(70, 2660))
        o.append('    <text x="16" y="24" size="15" weight="bold" color="panelink">India - Burma</text>')
        o.append('    <text x="16" y="44" size="12" color="panelink">Hump airlift and Ledo Road; enters by</text>')
        o.append('    <text x="16" y="60" size="12" color="panelink">the Burma Road at the west edge.</text>')
        o.append('    <box label="Hump / Ledo Road" x="16" y="74" w="218" h="60" fill="gametrack"/>')
        o.append('  </panel>')
        # ---- south-east: the East China Sea; the column sits at the foot of the sheet (Ben, 2026-10-06) ----------
        X, Y = at_scale(1900, 1895)
        W = 590
        o.append('  <panel id="title" x="%d" y="%d" w="%d" h="130" fill="panel" stroke="ink">' % (X, Y, W))
        o.append('    <text x="20" y="48" size="40" weight="bold" color="ccp">CIRCLING</text>')
        o.append('    <text x="258" y="48" size="40" weight="bold" color="kmt">DRAGONS</text>')
        o.append('    <text x="20" y="80" size="18" color="panelink">China 1944-1945  -  Defeat Japan  -  Control China</text>')
        o.append('    <text x="20" y="104" size="12" color="panelink">Design prototype sheet. One hex = %g km between centers; equal-area projection.</text>' % L.HEX_KM)
        o.append('    <text x="20" y="121" size="12" color="panelink">Terrain: one value per hex. Rivers: operational barriers only. Networks: decisive lines only.</text>')
        o.append('  </panel>')
        Y += 145
        o.append('  <panel id="key" x="%d" y="%d" w="%d" h="235" fill="panel" stroke="ink">' % (X, Y, W))
        o.append('    <text x="20" y="28" size="18" weight="bold" color="panelink">Terrain and features</text>')
        rows = [("tan", "Clear / plain"), ("ochre", "Broken / hills"), ("umber", "Mountain"), ("sand", "Arid / steppe"),
                ("greygreen", "Marsh / floodplain"), ("seablue", "Sea")]
        for i, (fill, name) in enumerate(rows):
            x = 20 + (i % 3) * 190
            y = 44 + (i // 3) * 38
            o.append('    <text x="%d" y="%d" size="13" color="panelink">%s</text>' % (x + 38, y + 17, name))
        lines = ["River hexside (blue): Yangtze, Yellow River (1938-47 course), Han, Xiang; Amur, Argun,",
                 "Ussuri, Sungari, Nen, Liao, Yalu, Tumen. Railway: black, ticked. Strategic road: brown, dashed.",
                 "Border: red, dashed. Manchukuo boundary: purple, dashed.",]
        if self.MINOR:
            lines[1:3] = ["Ussuri, Sungari, Nen, Liao, Yalu, Tumen. Minor river (thin blue): %s." % ", ".join(
                              self.RIVER_NAMES[k] for k, _, _ in D.MINOR_RIVERS_BY_SCALE.get(L.HEX_KM, [])),
                          "Railway: black, ticked. Strategic road: brown, dashed. Border: red; Manchukuo: purple."]
        lines += [
                 "Red square: major city. Gold star: capital. Gold disc: city. White square: town.",
                 "Anchor: port (blue: sea; dark blue: river port). Triangle: major airfield. Ellipse: lake.",
                 "Red star: Soviet entry hex. Rail segments carry a state in play: Open, Interdicted, Broken."]
        for i, t in enumerate(lines):
            o.append('    <text x="20" y="%d" size="12" color="panelink">%s</text>' % (138 + i * 17, escape(t)))
        for i, (fill, name) in enumerate(rows):
            x = 20 + (i % 3) * 190
            y = 44 + (i // 3) * 38
            o.append('    <box x="%d" y="%d" w="30" h="24" fill="%s"/>' % (x, y, fill))
        o.append('  </panel>')
        Y += 250
        months = ["Apr 44", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "Jan 45", "Feb", "Mar",
                  "Apr 45", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "Jan 46", "Feb", "Mar"]
        o.append('  <panel id="time" x="%d" y="%d" w="%d" h="130" fill="panel" stroke="ink">' % (X, Y, W))
        o.append('    <text x="20" y="24" size="15" weight="bold" color="panelink">Initiative / time track: KMT, CCP, J-KMT, J-CCP; one cell = one month</text>')
        o.append('    <track x="20" y="36" cell-w="45" cell-h="40" cells="%s" wrap="12" fill="gametrack"/>' % " ".join(m.replace(" ", "_") for m in months))
        o.append('  </panel>')
        Y += 145
        o.append('  <panel id="legitimacy" x="%d" y="%d" w="%d" h="135" fill="panel" stroke="ink">' % (X, Y, W))
        o.append('    <text x="20" y="24" size="15" weight="bold" color="panelink">United Front / Legitimacy: the cost of Chinese-on-Chinese combat</text>')
        o.append('    <text x="20" y="56" size="13" weight="bold" color="kmt">KMT</text>')
        o.append('    <text x="20" y="104" size="13" weight="bold" color="ccp">CCP</text>')
        o.append('    <track x="70" y="36" cell-w="45" cell-h="32" cells="-4 -3 -2 -1 0 +1 +2 +3 +4" fill="gametrack"/>')
        o.append('    <track x="70" y="84" cell-w="45" cell-h="32" cells="-4 -3 -2 -1 0 +1 +2 +3 +4" fill="gametrack"/>')
        o.append('  </panel>')
        Y += 150
        o.append('  <panel id="pools" x="%d" y="%d" w="%d" h="125" fill="panel" stroke="ink">' % (X, Y, W))
        o.append('    <text x="20" y="24" size="15" weight="bold" color="japan">Shared Japanese pools, drawn on by both Japanese factions</text>')
        for i, name in enumerate(["Replacements", "Support", "Rail capacity", "Logistics"]):
            o.append('    <box label="%s" x="%d" y="36" w="130" h="75" fill="gametrack"/>' % (name, 20 + i * 140))
        o.append('  </panel>')
        Y += 140
        o.append('  <panel id="offmap" x="%d" y="%d" w="%d" h="125" fill="panel" stroke="ink">' % (X, Y, W))
        o.append('    <text x="20" y="24" size="15" weight="bold" color="panelink">Off-map boxes</text>')
        for i, name in enumerate(["U.S. Strategic Lift", "Japan: home islands, sea transport"]):
            o.append('    <box label="%s" x="%d" y="36" w="270" h="75" fill="gametrack"/>' % (name, 20 + i * 280))
        o.append('  </panel>')
        return


# label slot overrides where the default (below) collides with a neighbour or a line
LABEL_SLOT = {
    "tientsin": "se", "baoding": "sw", "shijiazhuang": "sw", "anyang": "sw", "xinxiang": "sw", "kaifeng": "se",
    "zhengzhou": "sw", "luoyang": "sw", "xuzhou": "se", "jinan": "se", "yueyang": "sw", "changsha": "sw",
    "zhuzhou": "se", "hengyang": "sw", "lingling": "sw", "guilin": "se", "liuzhou": "sw", "jiujiang": "ne",
    "nanchang": "se", "anqing": "n", "wuhu": "n", "nanking": "n", "shanghai": "s", "hangzhou": "sw",
    "ningbo": "se", "mukden": "se", "siping": "se", "hsinking": "se", "kirin": "se", "harbin": "nw",
    "qiqihar": "nw", "jinzhou": "sw", "yingkou": "sw", "dalian": "s", "antung": "s", "shanhaiguan": "se",
    "chengde": "ne", "peiping": "nw", "kalgan": "nw", "datong": "nw", "taiyuan": "sw", "yanan": "nw", "xian": "s",
    "hanzhong": "s", "chongqing": "s", "chengdu": "s", "guiyang": "s", "kunming": "s", "canton": "s", "hongkong": "se",
    "hankow": "ne", "yichang": "n", "changde": "n", "baoqing": "s", "zhijiang": "n", "laohekou": "n", "nanyang": "ne",
    "xinyang": "se", "wanxian": "n", "mudanjiang": "se", "suifenhe": "se", "jiamusi": "ne", "beian": "ne",
    "heihe": "s", "hailar": "s", "manzhouli": "s", "tumen": "se", "pyongyang": "se", "seoul": "se", "pusan": "se",
    "wonsan": "se", "hamhung": "se", "chongjin": "se", "vladivostok": "se", "khabarovsk": "s", "dushan": "nw",
    "tongguan": "s", "baoji": "s", "linfen": "sw", "lanzhou": "s", "hanoi": "nw", "haiphong": "se", "langson": "ne",
    "foochow": "se", "amoy": "se", "swatow": "se", "shaoguan": "se", "ganzhou": "ne", "wuzhou": "s", "haichow": "ne",
    "qingdao": "se", "chefoo": "ne", "bengbu": "se", "nanning": "s", "tongliao": "ne", "solun": "nw", "chifeng": "ne",
    "dolonnor": "nw", "guisui": "s", "baotou": "s", "jinhua": "se", "nagasaki": "sw", "taipei": "se",
    "choibalsan": "s", "sainshand": "s", "tamsag": "s", "tongjiang": "ne", "iman": "se",
}


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


def main():
    global VERSION
    if 1 < len(sys.argv) and sys.argv[1] in ("1", "2", "3"):
        VERSION = int(sys.argv[1])
    b = Builder()
    xml = b.build()
    path = os.path.join(OUT, "circling-dragons%s%s.xml" % ({1: "-v1", 2: "-v2", 3: ""}[VERSION], L.SUFFIX))
    open(path, "w", encoding="utf-8").write(xml)
    print("wrote", path, len(xml), "bytes")
    for p in b.problems:
        print("  ", p)


if __name__ == "__main__":
    main()
# Copyright Ben Paul Wise. All Rights Reserved.
