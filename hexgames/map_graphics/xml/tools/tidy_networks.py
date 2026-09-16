# Copyright Ben Paul Wise. All Rights Reserved.
"""tidy_networks.py -- rewrite a hexsheet's rivers and rail/road links into connected networks.

    python tidy_networks.py SHEET.xml [--dry-run]

Deterministic, no randomness. A network kind (river, or one link kind) is rewritten only when
network_check.py finds it broken, so running the tool on its own output changes nothing.

Rivers live on the hexside-vertex graph. Each authored guide below (a source-to-mouth list of hex
ids) is routed along hexsides that stay close to the guide's centre line, avoid curling round a hex
and do not touch an earlier river except at the mouth. On sheets whose traced hexsides are usable
hints, long traced pieces are kept too and routes prefer traced hexsides. Then: cycles are broken,
curls straightened and hairs pruned; short pieces within reach of another river are joined;
dangling pieces are run down to sea, a lake, the map edge or another river; what is still short
is dropped.

Links (rail, road) live on the hex graph. Terminals are the named places (rail: cities; road:
every named place) and the map-edge ends of long traced pieces. A shortest-path Steiner heuristic
(Takahashi-Matsuyama) connects them one at a time to the growing network, with sea, lake and
blocked hexsides impassable, woods, swamp and mountain dear, traced steps cheap, sharp turns and
river crossings penalised; then a few trunk lines are added between major cities whose route over
the network is much longer than their direct route. The result is written as maximal chains.
"""
import collections
import heapq
import math
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import network_check as N  # noqa: E402

TERRAIN_COST = {"clear": 1.0, "woods": 1.8, "swamp": 2.2, "mountain": 2.8}
RAIL_PLACES = ("city-major", "capital", "city", "city-minor")
ROAD_PLACES = N.PLACES

# ---------------------------------------------------------------- per-sheet authoring
SHEETS = {
    "trc": dict(
        river_hints=True, keep_traced=10,
        link_exit_min=8, trunks=6, trunk_ratio=1.3, trunk_reach=14,
        rivers=[
            ("Dnieper", "M12 O13 P14 R15 T17 V18 W19 Y19 AA19 CC19 DD18 FF18 GG19 GG20"),
            ("Desna", "S13 T14 U16 V17 W18"),
            ("Pripet", "O24 Q22 S21 U20 V19"),
            ("Don", "V9 X9 Z9 AA9 CC9 EE8 GG9 HH10 HH12 II13 II14 II15"),
            ("Donets", "X12 Z13 BB13 DD13 FF13 GG12 HH12"),
            ("Volga", "P10 O8 N6 M5"),
            ("Volga", "M5 O4 R3 U2 W1"),
            ("Volga", "Y1 AA3 CC4 EE5 GG6 II7 KK6 MM5 OO5 QQ5 QQ4"),
            ("Oka", "V13 U11 U9 T7 T5 T3 U2"),
            ("Moskva", "Q11 R9 S8 T7"),
            ("Western Dvina", "N13 L15 J16 H17 G17"),
            ("Neman", "Q19 O19 M20 K21 I22 G22"),
            ("Vistula", "S27 Q25 O24 L24 I25 F25"),
            ("Oder", "Q30 O28 L28 I28 F28 D27"),
            ("Dniester", "U27 W26 Y25 AA24 CC24 DD25 EE25"),
            ("Southern Bug", "X22 Z21 BB21 DD21 EE23"),
            ("Danube", "CC33 DD31 EE30 FF29 GG28 HH28"),
            ("Prut", "W29 Y29 AA29 CC29 EE29"),
            ("Volkhov", "I11 G10 E9"),
            ("Northern Dvina", "I4 G3 E2 B2"),
            ("Kuban", "OO10 OO12 NN14 MM16 KK18"),
        ],
        # Country borders as printed on "TRC v5 deluxe/TRC5 map adjusted.png" (the sheet's own 1224 x 1483
        # frame): dark hexsides traced from the scan, gaps closed along the darkest hexsides, label strokes
        # dropped. Each entry is one chain of hexsides joined end to end; chains meet at shared corners.
        borders=[
            ("German frontier, Baltic to Hungary",
             "R28:ne Q28:e P28:sw P29:e P29:ne O29:e N29:se N29:e N29:ne M29:e L29:sw L30:e K29:se K29:e J29:sw "
             "J30:e J30:ne I30:e H30:sw H31:e G30:se G30:e F30:se F29:sw F29:se F28:sw F28:se F27:sw F27:se "
             "F26:sw F26:se"),
            ("Warsaw sector, east side", "Q27:se R27:ne Q26:se R26:ne Q25:se Q25:e P25:se P25:e O24:se O24:e "
                                         "N24:se N24:e M23:se M23:e L23:sw L24:e K23:se K23:e"),
            ("Warsaw sector, south corner", "R28:se R28:e"),
            ("East Prussia", "G25:e G25:se H26:e H25:sw I25:e J25:ne J25:e J24:sw J24:se J23:sw"),
            ("Lithuania", "J23:se J23:e I22:se J22:ne I21:se I21:e H21:se H21:e G20:se G20:e F20:sw"),
            ("Hungary, north", "P33:sw P33:se P32:sw P32:se P31:sw P31:se P30:sw Q30:e R30:ne Q29:se R29:ne R29:e "
                               "R28:sw"),
            ("Hungary, Carpathians", "S28:e T28:ne T28:e T28:se U28:e V28:ne V28:e"),
            ("Rumania, north-west", "V33:sw V33:se V33:e U32:se V32:ne U31:se V31:ne V31:e V30:sw W30:e X30:ne X30:e "
                                    "X29:sw Y29:e Z29:ne Y28:se Y28:e X28:se X28:e X28:ne W28:e V28:se"),
            ("Rumania, east", "V27:sw V27:se V26:sw W26:e X26:ne X26:e X26:se Y26:e Z26:ne Z26:e Z26:se AA26:e "
                              "BB26:ne BB26:e BB26:se CC26:e CC26:se DD27:e DD26:sw EE26:e"),
            ("Bulgaria, north", "CC33:se DD33:ne DD33:e DD32:sw DD32:se DD31:sw DD31:se DD30:sw EE30:e FF30:ne "
                                "FF30:e FF29:sw GG29:e"),
            ("Bulgaria, south", "HH33:sw HH33:se HH32:sw II32:e JJ32:ne II31:se"),
        ]),
    "pgg": dict(
        river_hints=False, keep_traced=0,     # the traced hexsides are the scan's hex grid, not rivers
        link_exit_min=8, trunks=4, trunk_ratio=1.2, trunk_reach=16,
        rivers=[   # read off "PGG - low res.png" through a 13-city affine fit, so about a hex out
            ("Днепр", "3604 3606 3508 3510 3411 3313 3315 3116 2917 2716 2616 2417 2318 2217 2117 1916 1716 "
                      "1516 1317 1217 1017 0817 0618 0520 0421 0523 0424 0425 0327 0128"),
            ("Западная Двина", "1702 1803 1805 1707 1508 1409 1209 1110 0911 0712 0513 0313 0113"),
            ("Межа", "3703 3503 3304 3104 3003 2803 2704 2504 2304 2104 1904 1804"),
            ("Вопь", "3409 3210 3112 3013 2915 2816"),
            ("Каспля", "1515 1513 1411 1210 1110"),
            ("Сож", "1920 1821 1622 1524 1425 1327 1228 1129 1029 0930 0830 0831"),
            ("Остёр", "2226 2128 1929 1830 1831"),
            ("Десна", "3020 3022 2924 2925 2926 2827 2729 2530 2531"),
            ("Угра", "3322 3421 3521 3721 3920 4120 4118 4216 4315 4415 4616 4817 5018 5219 5419 5619"),
            ("Ока", "5230 5329 5528 5626 5624 5622 5620 5618 5617"),
            ("Волга", "3901 4102 4301 4501 4701 4901"),
            ("Гжать", "4507 4505 4304 4302"),
            ("Яуза", "4906 5006 5107 5307 5406 5606 5605"),
            ("Болва", "3826 3828 3930 3931"),
        ]),
    # Dai Senso: the sheet's own border, zone, region, mountain, river and lake hexsides (traced from the game-map
    # scan) are repaired in place; links and places are authored, read off pic4573603.png with the game-map scan
    # and map-detail.png where counters hide the print (tasks/09-ds-map.md). A link chain lists hexes in order;
    # consecutive hexes that are not neighbours are joined over land by land_path.
    "dai-senso": dict(
        repair_lines=("border", "zone", "region", "mountain", "river", "lake"),
        # Printed stretches the scan trace missed, read off photo crops (tasks/09 repair log).
        boundary_add={"zone": "w3108:se w3007:e w3007:se w2907:e",
                      # the Bangladesh region border's dots running south past Dacca to the Bay of Bengal
                      "region": "w4313:e w4213:ne w4213:e"},
        # Traced hexsides the print does not show: the pale forest edge round w4309/w4310, taken for region dots.
        boundary_remove={"region": "w4207:ne w4208:ne w4307:se w4308:se"},
        link_lines={"rail": "rail", "road": "road"},
        links={
            "rail": [
                ("Trans-Siberian", "w6011 w6012 w6013 w6014 w6015 w6016 w6017 w6018 w6019 w6020 w5921 w5821 w5722 "
                                   "w5723 w5724 w5824 w5825 w5926 w5927 w5827 e5701 e5702 e5703 e5603 e5503 e5402 "
                                   "e5302"),
                ("Omsk-Semipalatinsk", "w6012 w5913 w5813 w5714"),
                ("Novosibirsk-Semipalatinsk", "w6015 w5915 w5814 w5714"),
                ("Turksib", "w5714 w5613 w5513 w5412 w5411"),
                ("Volochaevsk-Sovetskaya Gavan", "e5703 e5803 e5704 e5604"),
                ("Chinese Eastern", "w5824 w5725 w5625 w5626 w5527 w5427 e5401 e5302"),
                ("South Manchurian", "w5427 w5327 w5226 w5126"),
                ("Hsinking-Vladivostok", "w5327 e5301 e5302"),
                ("Mukden-Fusan", "w5226 w5127 w5027 e4901"),
                ("Mukden-Peiping", "w5226 w5225 w5124 w5223"),
                ("Peiping-Kweisui", "w5223 w5222"),
                ("Peiping-Hankow-Canton", "w5223 w5022 w4923 w4723 w4622 w4521 w4322 w4222"),
                ("Hengyang-Kweilin", "w4521 w4420"),
                ("Kweiyang-Nanning", "w4520 w4320"),
                ("Lunghai", "w4921 w4922 w4923 w4924 w4925"),
                ("Tientsin-Pukow-Shanghai", "w5124 w5024 w4925 w4825 w4725 w4726"),
                ("Tsinan-Tsingtao", "w5024 w5025"),
                ("Yunnan", "w4418 w4318 w4219"),
                ("Hanoi-Nanning", "w4219 w4320"),
                ("Indochina coast", "w4219 w3920 w3619"),
                ("Saigon-Bangkok", "w3619 w3618 w3817"),
                ("Bangkok-Chiengmai", "w3817 w4016"),
                ("Bangkok-Singapore", "w3817 w3716 w3616 w3517 w3417 w3317 w3217 w3218"),
                ("Rangoon-Moulmein", "w4015 w3916"),
                ("Burma", "w4416 w4316 w4215 w4115 w4015"),
                ("Assam", "w4415 w4516"),
                ("Peshawar-New Delhi", "w4908 w4808 w4709 w4609 w4510"),
                ("New Delhi-Karachi", "w4510 w4509 w4508 w4507 w4406 w4405"),
                ("Jaipur-Ahmadabad", "w4509 w4408 w4308"),
                ("Ganges", "w4510 w4410 w4411 w4412 w4413 w4414 w4415"),
                ("Ahmadabad-Nagpur", "w4308 w4208 w4209"),
                ("Bombay-Nagpur", "w4107 w4108 w4109 w4209"),
                ("Nagpur-Hyderabad", "w4209 w4110 w4009"),
                ("Hyderabad-Kakinada", "w4009 w4010"),
                ("Hyderabad-Bangalore", "w4009 w3909 w3808"),
                ("Bombay-Mangalore", "w4107 w4007 w3908 w3807"),
                ("Mangalore-Madras", "w3807 w3808 w3809"),
                ("Calcutta-Dacca", "w4213 w4314 w4414"),
                ("Calcutta-Nagpur", "w4213 w4212 w4211 w4210 w4209"),
                ("Calcutta coast", "w4213 w4212 w4112 w4011 w4010"),
                ("Java", "w2819 w2720 w2721 w2722"),
                ("Sumatra", "w2919 w2818"),
                ("Formosa", "w4325 w4224"),
                ("Luzon", "w4025 w3824"),
                ("Hokkaido", "e5405 e5306 e5305 e5205"),
                ("Asahigawa-Kushiro", "e5306 e5307"),
                ("Tohoku", "e5105 e5005 e4905"),
                ("Aomori-Niigata", "e5105 e5004"),
                ("Tokaido", "e4902 e4903 e4904 e4905"),
                ("Kyoto-Osaka", "e4903 e4803"),
                ("Nagoya-Osaka", "e4904 e4803"),
                ("Hokuriku", "e4904 e5003 e5004 e4905"),
                ("Geraldton-Perth", "w1622 w1523 w1423"),
                ("Trans-Australian", "w1423 w1424 w1525 w1526 w1527 e1501 e1502 e1503 e1504 e1404 e1304"),
                ("Adelaide-Canberra", "e1304 e1305 e1306 e1307 e1308"),
                ("Canberra-Sydney", "e1308 e1309"),
                ("North Coast", "e1309 e1408 e1509 e1609 e1709 e1808"),
                ("Canberra-Melbourne", "e1308 e1207 e1107"),
                ("Adelaide-Melbourne", "e1304 e1204 e1105 e1106 e1107"),
                ("Queensland coast", "e1808 e1908 e2007 e2107 e2206"),
                ("Cloncurry-Townsville", "e2004 e2005 e1906 e2006 e2107"),
                ("Rockhampton inland", "e1808 e1807 e1806"),
                ("North Island", "e1517 e1417 e1318"),
                ("South Island", "e1217 e1317"),
            ],
            "road": [
                ("Semipalatinsk-Urumchi", "w5613 w5514 w5415"),
                ("Urumchi-Lanchow", "w5415 w5416 w5317 w5217 w5118 w5119 w5019"),
                ("Lanchow-Sian", "w5019 w4920 w4921"),
                ("Lanchow-Chungking", "w5019 w4919 w4819 w4719 w4620"),
                ("Chungking-Kunming", "w4620 w4520 w4419 w4418"),
                ("Burma Road", "w4418 w4317 w4216 w4215"),
                ("Ledo Road", "w4516 w4416"),
                ("Kweilin-Nanning", "w4420 w4320"),
                ("Ulan Ude-Kweisui", "w5722 w5621 w5522 w5422 w5323 w5222"),
                ("Komsomolsk-Nikolaevsk", "e5803 e5804 e5805"),
                ("Sakhalin", "e5806 e5706 e5606"),
            ],
        },
        places=dict(
            capital="w5415 w5019 w4715 w5821 w5621 w5818 w5824 w5427 w5223 w5027 w4725 w4620 w4418 w3817 w4015 w4314 "
                    "w4510 w4405 w3509 w3218 w2819 w3824 e5603 e5810 e4905 e1308 e1318",
            city="w6012 w6015 w5411 w5118 w4719 w5226 w5126 w5022 w5021 w4921 w4923 w4723 w4622 w4726 w4520 w4420 "
                 "w4219 w3920 w4215 w4213 w4410 w4509 w4709 w4308 w4107 w4009 w3808 w2919 w2722 w3220 w3322 w3022 "
                 "w3423 w3223 w3426 w3126 w2925 w2824 w1423 w4322 w4222 "
                 "e5701 e5302 e5305 e5005 e4904 e4903 e4803 e4902 e4801 e4226 e3004 e2906 e2707 e2808 e3001 e2606 "
                 "e2107 e1304 e1309 e1609 e1107 e1517",
            town="w6018 w6020 w5714 w5210 w5212 w5012 w5722 w5625 w5527 w5327 w5222 w5024 w4924 w4925 w5127 w4625 "
                 "w4525 w4224 w4320 w3618 w4016 w3916 w4018 w4216 w4416 w4516 w4415 w4115 w4412 w4513 w4908 w4406 "
                 "w4209 w3608 w3609 w3217 w3016 w2720 w2922 w3825 w2626 w1622 w4521 "
                 "e5703 e5803 e5706 e5606 e5306 e5105 e5004 e5003 e4802 e3504 e3003 e2806 e2605 e2607 e2508 e2004 "
                 "e1802",
            port="w4405 w4107 w4010 w4314 w4112 w3807 w3809 w3509 w3510 w3406 w3714 w4015 w3517 w3417 w3218 w2818 "
                 "w2722 w3220 w3223 w3423 w2925 w2824 w2927 w3126 w3824 w3625 w3626 w3426 w4025 w4219 w3920 w3619 "
                 "w4121 w4222 w4726 w5025 w5124 w2625 w2519 w3206 w2806 w2225 w1423 "
                 "e5101 e4901 e4902 e4904 e4803 e4801 e4905 e5005 e5106 e5205 e5307 e5407 e5302 e5604 e5805 e5806 "
                 "e5810 e5813 e5616 e5618 e5723 e4309 e3806 e3604 e3411 e3514 e3516 e3014 e3117 e3923 e4323 e4114 "
                 "e4520 e4128 e3425 e2922 e3004 e2906 e2907 e2808 e2611 e2618 e2314 e1914 e2118 e2420 e1920 e2606 "
                 "e2206 e2401 e1808 e1408 e1309 e1304 e1318 e1217",
        ),
        # Hexes the sheet's sampled terrain calls sea but the print shows as land: each holds a printed capital,
        # city, town or port, or a printed rail line runs through it (the print draws many pale blue, part land).
        land="w4825 w3716 w3317 w4011 w1523 w3925 e5405 e1509 e1105 e1106 e1417 e1317 "
             "w4405 w3406 w3206 w2806 w4107 w3807 w3609 w3509 w3510 w4112 w4213 w3714 w3916 w3016 w3217 w3618 w3218 "
             "w3619 w2919 w2819 w2519 w3220 w4322 w4222 w3322 w2922 w2722 w1622 w3423 w3223 w4224 w2824 w4925 w4625 "
             "w4525 w4025 w3825 w3625 w2925 w2625 w2225 w3626 w3426 w3126 w2626 w5127 w2927 "
             "e4901 e4801 e4902 e4802 e5003 e4803 e3003 e3604 e3504 e3004 e1304 e5305 e5205 e5105 e2605 e5806 e5606 "
             "e5106 e3806 e2906 e2606 e2206 e5407 e5307 e2907 e2607 e2107 e1107 e2808 e2508 e4309 e1609 e1309 e3411 "
             "e2611 e5813 e3514 e3014 e2314 e1914 e5616 e3117 e1517 e1217 e5618 e2618 e2118 e1318 e2420 e1920 e2922 "
             "e5723 e4323 e3923 e3425 e4128"),
}

# ---------------------------------------------------------------- tuning
CORRIDOR = 1.2          # river cost grows with (distance from the guide / hex size) squared
CURL = 1.0              # two turns the same way in a row (hugging one hex)
HINT_SIDE = 0.45        # a traced river hexside
JOIN_REACH = 3.0        # a short river piece joins another river within this cost
HAIR = 2                # leaf branches of this many hexsides or fewer, off a junction, are pruned
NET_STEP = 0.05         # a link step already on the network being built
HINT_STEP = 0.3         # a traced link step, times terrain
HINT_HEX = 0.6          # a hex on a traced link piece, times terrain
TURN = (0.0, 0.15, 1.5)  # link direction change of 0, 60 and 120 degrees; 180 is forbidden
BRIDGE = 0.7            # a link step across a river hexside
BESIDE = 0.4            # entering a hex next to the network but not on it


# ---------------------------------------------------------------- rivers
class Rivers:
    def __init__(self, sheet, cfg):
        self.s = sheet
        self.cfg = cfg
        traced = set(sheet.lines.get("river", {}))
        self.hints = {x for x in traced if sheet.river_sideP(x)} if cfg["river_hints"] else set()
        self.blocked = sheet.blocked_sides()

    def allowed(self, side):
        return self.s.river_sideP(side) and side not in self.blocked

    def verts(self, sides):
        return {v for x in sides for v in self.s.side_ends[x]}

    def adjacency(self, sides):
        adj = collections.defaultdict(set)
        for x in sides:
            a, b = self.s.side_ends[x]
            adj[a].add(x)
            adj[b].add(x)
        return adj

    def route_back(self, sources, targets, cost, forbidden, limit=math.inf):
        """Cheapest hexside path from a source vertex to a target vertex, or None. A search state
        is (vertex, previous vertex, last turn) so a curl round one hex can be charged."""
        s = self.s
        heap = [(0.0, k, v, None, 0, None, None) for k, v in enumerate(sorted(sources))]
        heapq.heapify(heap)
        seq = len(heap)
        done = {}
        while heap:
            c, _, v, prev, sign, parent, side_in = heapq.heappop(heap)
            state = (v, prev, sign)
            if state in done:
                continue
            done[state] = (parent, side_in)
            if c > limit:
                return None
            if v in targets and prev is not None:
                path = []
                while done[state][0] is not None:
                    state, side = done[state]
                    path.append(side)
                return path[::-1]
            for side in sorted(s.vertex_sides[v]):
                if not self.allowed(side):
                    continue
                a, b = s.side_ends[side]
                w = b if a == v else a
                if w == prev or (w in forbidden and w not in targets):
                    continue
                turn = 0
                if prev is not None:
                    cross = (v[0] - prev[0]) * (w[1] - v[1]) - (v[1] - prev[1]) * (w[0] - v[0])
                    turn = 1 if cross > 0 else -1
                if (w, v, turn) in done:
                    continue
                step = cost(side) + (CURL if turn != 0 and turn == sign else 0.0)
                heapq.heappush(heap, (c + step, seq, w, v, turn, state, side))
                seq += 1
        return None

    # ---- guides
    def guide(self, name, ids, rivers):
        s = self.s
        hexes = [s.known(p) for p in ids.split()]
        pts = [s.centre(p) for p in hexes]
        size = s.grids[0].size
        memo = {}

        def cost(side):
            if side not in memo:
                a, b = s.side_ends[side]
                mid = ((a[0] + b[0]) / 4.0, (a[1] + b[1]) / 4.0)
                d = min(seg_dist(mid, p, q) for p, q in zip(pts, pts[1:])) / size
                memo[side] = (HINT_SIDE if side in self.hints else 1.0) * (1.0 + CORRIDOR * d * d)
            return memo[side]

        usable = {v for v in s.vertex_sides if any(self.allowed(x) for x in s.vertex_sides[v])}
        existing = self.verts(rivers)
        sources = self.end_vertices(hexes[0], pts[0], usable, size)
        targets = self.end_vertices(hexes[-1], pts[-1], usable, size)
        targets |= {v for v in existing if math.dist(unkey(v), pts[-1]) <= 1.2 * size}
        path = self.route_back(sources, targets, cost, existing - sources)
        if path is None:
            path = self.route_back(sources, targets, cost, set())
        if path is None:
            raise ValueError("%s: river guide %s (%s) cannot be routed" % (s.path, name, ids))
        return set(path)

    def end_vertices(self, pid, pt, usable, size):
        """A guide end: in water, every usable shore corner within two hexes (a water hex's own
        corners often have no land-land hexside); on land, the usable corner nearest the hex centre."""
        s = self.s
        if s.waterP(pid):
            shore = {v for v in usable if s.outletP(v) and math.dist(unkey(v), pt) <= 2.0 * size}
            if not shore:
                raise ValueError("%s: river guide end %s has no shore within two hexes" % (s.path, pid))
            return shore
        return {min(usable, key=lambda v: (math.dist(unkey(v), pt), v))}

    # ---- tidying
    def untangle(self, sides):
        sides = self.break_cycles(sides)
        changed = True
        while changed:
            changed = False
            for fix in (self.straighten, self.prune_hairs):
                new = fix(sides)
                if new != sides:
                    sides, changed = new, True
        return sides

    def break_cycles(self, sides):
        adj = self.adjacency(sides)
        keep, seen = set(), set()
        for root in sorted(adj, key=lambda v: (not self.s.outletP(v), v)):
            if root in seen:
                continue
            seen.add(root)
            queue = collections.deque([root])
            while queue:
                v = queue.popleft()
                for x in sorted(adj[v]):
                    a, b = self.s.side_ends[x]
                    w = b if a == v else a
                    if w not in seen:
                        seen.add(w)
                        keep.add(x)
                        queue.append(w)
        return keep

    def straighten(self, sides):
        """Replace a run of 4 or 5 hexsides round one hex by the 2 or 1 on its other side."""
        s = self.s
        adj = self.adjacency(sides)
        for pid in sorted(s.hex_sides, key=s.order):
            ring = s.hex_sides[pid]
            on = [x in sides for x in ring]
            k = sum(on)
            if k not in (4, 5) or all(on):
                continue
            start = next(i for i in range(6) if on[i] and not on[i - 1])
            run = [ring[(start + j) % 6] for j in range(k)]
            if not all(on[(start + j) % 6] for j in range(k)):
                continue
            other = [ring[(start + k + j) % 6] for j in range(6 - k)]
            inner = self.verts(run) - set(self.run_ends(run))
            if any(len(adj[v]) != 2 for v in inner):
                continue
            if not all(self.allowed(x) for x in other):
                continue
            other_inner = self.verts(other) - set(self.run_ends(run))
            if any(adj[v] for v in other_inner):
                continue
            return (sides - set(run)) | set(other)
        return sides

    def run_ends(self, run):
        count = collections.Counter(v for x in run for v in self.s.side_ends[x])
        return [v for v, n in count.items() if n == 1]

    def prune_hairs(self, sides):
        adj = self.adjacency(sides)
        for leaf in sorted(v for v in adj if len(adj[v]) == 1 and not self.s.outletP(v)):
            branch, v, prev = [], leaf, None
            while len(adj[v]) <= 2 and len(branch) <= HAIR:
                nxt = [x for x in adj[v] if x not in branch]
                if not nxt:
                    break
                x = nxt[0]
                branch.append(x)
                a, b = self.s.side_ends[x]
                prev, v = v, (b if a == v else a)
            if len(adj[v]) == 3 and len(branch) <= HAIR:
                return sides - set(branch)
        return sides

    def drain(self, sides):
        """Join short pieces within reach; run pieces with no outlet down to one."""
        s = self.s
        cost = lambda x: HINT_SIDE if x in self.hints else 1.0  # noqa: E731
        outlets = {v for v in s.vertex_sides if s.outletP(v)}
        for piece, ends in N.side_pieces(s, sides):
            if not piece <= sides:
                continue          # already merged into another piece this pass
            own = self.verts(piece)
            if own & outlets:
                continue
            others = self.verts(sides - piece)
            limit = JOIN_REACH if len(piece) <= N.RIVER_MAX_SHORT else math.inf
            path = self.route_back(set(ends) or own, outlets | others, cost, own - set(ends), limit)
            if path is not None:
                sides = sides | set(path)
        return sides

    def drop(self, sides):
        s = self.s
        keep = set()
        for piece, _ in N.side_pieces(s, sides):
            if len(piece) > N.RIVER_MAX_SHORT and any(s.outletP(v) for v in self.verts(piece)):
                keep |= piece
        return keep

    def build(self):
        s = self.s
        rivers = set()
        for name, ids in self.cfg["rivers"]:
            rivers |= self.guide(name, ids, rivers)
        if self.cfg["keep_traced"]:
            near = self.verts(rivers)
            for piece, _ in N.side_pieces(s, self.hints):
                if len(piece) >= self.cfg["keep_traced"] and not self.verts(piece) & near:
                    rivers |= piece
        rivers = self.untangle(rivers)
        rivers = self.drain(rivers)
        rivers = self.untangle(rivers)
        rivers = self.drain(rivers)
        return self.drop(self.untangle(rivers))


def seg_dist(p, a, b):
    ax, ay = a
    bx, by = b
    dx, dy = bx - ax, by - ay
    L = dx * dx + dy * dy
    t = 0.0 if L == 0 else max(0.0, min(1.0, ((p[0] - ax) * dx + (p[1] - ay) * dy) / L))
    return math.hypot(p[0] - ax - t * dx, p[1] - ay - t * dy)


def unkey(v):
    return (v[0] / 2.0, v[1] / 2.0)


# ---------------------------------------------------------------- links
class Links:
    def __init__(self, sheet, cfg, kind, rivers):
        self.s = sheet
        self.cfg = cfg
        self.kind = kind
        self.rivers = rivers
        self.blocked = sheet.blocked_sides()
        self.hint_adj = N.link_graph(sheet.links[kind])
        self.hint_steps = {(a, b) for a in self.hint_adj for b in self.hint_adj[a]}

    def terminals(self):
        s = self.s
        wanted = RAIL_PLACES if self.kind == "rail" else ROAD_PLACES
        places = [p for p in sorted(s.places, key=s.order) if s.places[p] in wanted]
        majors = [p for p in places if s.places[p] in N.MAJOR]
        exits = []
        for hexes, _ in N.link_pieces(s, self.hint_adj):
            if len(hexes) < self.cfg["link_exit_min"]:
                continue
            edge = sorted((h for h in hexes if s.boundary_hexP(h) and not s.waterP(h)), key=s.order)
            if edge and all(self.hops(edge[0], t) > 3 for t in places + exits):
                exits.append(edge[0])
        first = (majors or places)[0]
        reach = self.reachable(first)
        for p in places:
            if p not in reach:
                raise ValueError("%s: %s cannot reach named place %s" % (s.path, self.kind, p))
        for p in exits:
            if p not in reach:
                print("%s: %s map-edge end %s is cut off (blocked hexsides or water); not a terminal"
                      % (s.path, self.kind, p))
        rest = [p for p in places + exits if p != first and p in reach]
        return first, rest, majors

    def reachable(self, start):
        """Hexes a link could reach from start: through land, across no blocked hexside; water
        hexes that hold a named place are reachable but not passed through."""
        s = self.s
        seen, stack = {start}, [start]
        while stack:
            h = stack.pop()
            if s.waterP(h) and h != start:
                continue
            for i, n in s.neighbours(h):
                if n in seen or s.hex_sides[h][i] in self.blocked:
                    continue
                if s.waterP(n) and n not in s.places:
                    continue
                seen.add(n)
                stack.append(n)
        return seen

    def hops(self, a, b):
        (ax, ay), (bx, by) = self.s.centre(a), self.s.centre(b)
        return math.hypot(ax - bx, ay - by) / (math.sqrt(3) * self.s.grids[0].size)

    def enter_cost(self, a, b, i, side, net):
        s = self.s
        if (a, b) in net:
            return NET_STEP
        terrain = s.terrain[b]
        if s.waterP(b):
            base = 1.0
        elif terrain in TERRAIN_COST:
            base = TERRAIN_COST[terrain]
        else:
            raise ValueError("%s: no link cost for terrain %s at %s" % (s.path, terrain, b))
        if (a, b) in self.hint_steps:
            base *= HINT_STEP
        elif b in self.hint_adj:
            base *= HINT_HEX
        if side in self.rivers:
            base += BRIDGE
        return base

    def route(self, sources, targets, net_hexes, net):
        """Cheapest hex path from the network (or a source) to a target: [hex ...], or None."""
        s = self.s
        beside = {n for h in net_hexes for _, n in s.neighbours(h)} - net_hexes
        heap = [(0.0, k, h, -1, None) for k, h in enumerate(sorted(sources, key=s.order))]
        heapq.heapify(heap)
        seq = len(heap)
        done = {}
        while heap:
            c, _, h, d, parent = heapq.heappop(heap)
            if (h, d) in done:
                continue
            done[(h, d)] = parent
            if h in targets and d >= 0:
                path, state = [], (h, d)
                while state is not None:
                    path.append(state[0])
                    state = done[state]
                return path[::-1]
            if s.waterP(h) and d >= 0:
                continue            # a port city in water ends a route; nothing passes through it
            for i, n in s.neighbours(h):
                side = s.hex_sides[h][i]
                if side in self.blocked or (s.waterP(n) and n not in targets):
                    continue
                turn = 0 if d < 0 else min((i - d) % 6, (d - i) % 6)
                if turn == 3 or (n, i) in done:
                    continue
                step = self.enter_cost(h, n, i, side, net) + TURN[turn]
                if n in beside and n not in targets:
                    step += BESIDE
                heapq.heappush(heap, (c + step, seq, n, i, (h, d)))
                seq += 1
        return None

    def build(self):
        s = self.s
        first, rest, majors = self.terminals()
        net_hexes, net = {first}, set()
        remaining = list(rest)
        while remaining:
            path = self.route(net_hexes, set(remaining), net_hexes, net)
            if path is None:
                raise ValueError("%s: %s cannot reach %s" % (s.path, self.kind, " ".join(remaining)))
            self.add(path, net_hexes, net)
            remaining = [t for t in remaining if t not in net_hexes]
        self.trunks(majors, net_hexes, net)
        return chains(s, net)

    def add(self, path, net_hexes, net):
        for a, b in zip(path, path[1:]):
            net.add((a, b))
            net.add((b, a))
        net_hexes.update(path)

    def trunks(self, majors, net_hexes, net):
        s = self.s
        for _ in range(self.cfg["trunks"]):
            best = None
            for i, a in enumerate(majors):
                over = bfs_hops(net, a)
                for b in majors[i + 1:]:
                    if self.hops(a, b) > self.cfg["trunk_reach"]:
                        continue
                    direct = self.route({a}, {b}, set(), set())
                    if direct is None or b not in over:
                        continue
                    ratio = over[b] / (len(direct) - 1)
                    if ratio > self.cfg["trunk_ratio"] and (best is None or ratio > best[0]):
                        best = (ratio, a, b)
            if best is None:
                return
            _, a, b = best
            self.add(self.route({a}, {b}, set(), set()), net_hexes, net)  # direct: no discount for the network
        return


def bfs_hops(net, start):
    adj = collections.defaultdict(list)
    for a, b in net:
        adj[a].append(b)
    dist = {start: 0}
    queue = collections.deque([start])
    while queue:
        h = queue.popleft()
        for n in sorted(adj[h]):
            if n not in dist:
                dist[n] = dist[h] + 1
                queue.append(n)
    return dist


def chains(sheet, net):
    """The network as maximal chains between hexes whose degree is not two."""
    adj = collections.defaultdict(set)
    for a, b in net:
        adj[a].add(b)
    used = set()
    out = []
    breaks = [h for h in sorted(adj, key=sheet.order) if len(adj[h]) != 2]
    for start in breaks + sorted(adj, key=sheet.order):
        for n in sorted(adj[start], key=sheet.order):
            if (start, n) in used:
                continue
            chain, prev, h = [start], start, n
            used.update({(start, n), (n, start)})
            chain.append(h)
            while len(adj[h]) == 2 and h != start:
                nxt = next(x for x in sorted(adj[h], key=sheet.order) if x != prev)
                if (h, nxt) in used:
                    break
                used.update({(h, nxt), (nxt, h)})
                prev, h = h, nxt
                chain.append(h)
            out.append(chain)
    return out


# ---------------------------------------------------------------- writing
EDGE_RE = re.compile(r'^(\s*)<edge at="([^"]+)" line="([^"]+)"/>\s*$')
LINK_RE = re.compile(r'^(\s*)<link kind="([^"]+)" line="([^"]+)" hexes="[^"]+"/>\s*$')


def edge_lines(sheet, line_id, sides, indent):
    old = sheet.lines.get(line_id, {})
    refs = [old.get(x, sheet.side_ref[x]) for x in sides]

    def key(ref):
        pid, d = ref.split(":")
        g, _, _ = sheet.grid_of(pid)
        return (sheet.order(pid), list(g.geom["edges"]).index(d))
    return ['%s<edge at="%s" line="%s"/>' % (indent, r, line_id) for r in sorted(refs, key=key)]


GLYPH_RE = re.compile(r'^(\s*)<hex id="([^"]+)"><glyph symbol="(capital|city|town|port)"[^>]*/></hex>\s*$')
PLACE_COLOUR = {"capital": "red", "city": "gold", "town": "white", "port": "cyan"}
LAND_NOTE = "<!-- place hexes the print draws pale blue (part land): land in play; tools/tidy_networks.py -->"


def rewrite(sheet, lines, links, link_lines=None, places=None, land=None):
    """lines: {edge line id: hexsides} replacing every <edge> of that line; links: {kind: chains}. A line id or
    link kind the sheet does not have yet goes after its last <edge> or <link>; a new link kind takes its line
    style from link_lines. places: {pid: [symbol ...]} replacing every capital, city, town and port glyph;
    land: [pid] written as one clear-terrain <hexes> after the sheet's terrain."""
    text = open(sheet.path, encoding="utf-8").read()
    out, slots, indent, line_of = [], {}, {}, {}
    edge_end = link_end = hexes_end = None
    skip_next = False
    for line in text.split("\n"):
        if skip_next:
            skip_next = False
            continue
        if land is not None and LAND_NOTE in line:
            skip_next = True                   # the note and the <hexes> it heads are rewritten
            continue
        em = EDGE_RE.match(line) if "<edge " in line else None
        if em and em.group(3) in lines:
            slots.setdefault(em.group(3), len(out))
            indent.setdefault("edge", em.group(1))
            edge_end = len(out)
            continue
        if "<edge " in line and any('line="%s"' % k in line for k in lines):
            raise ValueError("%s: unexpected edge form: %s" % (sheet.path, line))
        lm = LINK_RE.match(line) if "<link " in line else None
        if "<link " in line and not lm:
            raise ValueError("%s: unexpected link form: %s" % (sheet.path, line))
        if lm and lm.group(2) in links:
            slots.setdefault(lm.group(2), len(out))
            indent.setdefault("link", lm.group(1))
            line_of.setdefault(lm.group(2), lm.group(3))
            link_end = len(out)
            continue
        gm = GLYPH_RE.match(line) if places is not None and "<glyph " in line else None
        if gm:
            slots.setdefault("places", len(out))
            indent.setdefault("places", gm.group(1))
            continue
        if places is not None and re.search(r"<!-- (\d+ centres|port anchors[^>]*) -->", line):
            continue
        out.append(line)
        if em:
            indent.setdefault("edge", em.group(1))
            edge_end = len(out)
        if lm:
            indent.setdefault("link", lm.group(1))
            link_end = len(out)
        if "<hexes " in line:
            hexes_end = len(out)
    blocks = {}
    for line_id, sides in lines.items():
        if line_id not in slots:
            if edge_end is None:
                raise ValueError("%s: no <edge> to place new line %s after" % (sheet.path, line_id))
            slots[line_id] = edge_end
        blocks[line_id] = edge_lines(sheet, line_id, sides, indent.get("edge", ""))
    for kind, kchains in links.items():
        if kind not in slots:
            if link_end is None or kind not in (link_lines or {}):
                raise ValueError("%s: new link kind %s needs an existing <link> and a line style" % (sheet.path, kind))
            slots[kind] = link_end
            line_of[kind] = link_lines[kind]
        blocks[kind] = ['%s<link kind="%s" line="%s" hexes="%s"/>' % (indent.get("link", ""), kind, line_of[kind],
                                                                       " ".join(c)) for c in kchains]
    if places is not None:
        if "places" not in slots:
            raise ValueError("%s: no place glyphs to replace" % sheet.path)
        blocks["places"] = place_lines(sheet, places, indent["places"])
    if land is not None:
        if hexes_end is None:
            raise ValueError("%s: no <hexes> to place the land hexes after" % sheet.path)
        slots["land"] = hexes_end
        blocks["land"] = ["  " + LAND_NOTE, '<hexes terrain="clear" ids="%s"/>' % " ".join(sorted(land, key=sheet.order))]
    for name in sorted(blocks, key=lambda k: -slots[k]):
        out[slots[name]:slots[name]] = blocks[name]
    body = "\n".join(out)
    body = one_count(body, r"<!-- \d+ edges -->", "<!-- %d edges -->" % body.count("<edge "))
    body = one_count(body, r"<!-- \d+ links -->", "<!-- %d links -->" % body.count("<link "))
    body = body.replace("traced from the scan -->", "traced from the scan, then tidied by tools/tidy_networks.py -->")
    if places is not None:
        body = body.replace("<!-- cities and capitals -->", "<!-- capitals, cities, towns and ports -->")
    with open(sheet.path, "w", encoding="utf-8", newline="\n") as f:
        f.write(body)
    return


def one_count(body, pattern, text):
    """The first count comment matching pattern becomes text; any later ones go, and a line left empty by
    that goes with them (a sheet built from two halves had one comment per half)."""
    out, seen = [], 0
    for line in body.split("\n"):
        if re.search(pattern, line):
            seen += 1
            line = re.sub(pattern, text if 1 == seen else "", line)
            if seen > 1 and not line.strip():
                continue
        out.append(line)
    return "\n".join(out)


def place_lines(sheet, places, indent):
    """One <hex> per glyph; a port's slot faces its first sea neighbour, or the listed slot of a delta port."""
    out = []
    order = ("capital", "city", "town", "port")
    delta = dict(tok.split(":") for tok in N.DS_DELTA_PORTS.split())
    for pid in sorted(places, key=sheet.order):
        for sym in sorted(places[pid], key=order.index):
            if "port" == sym:
                g, _, _ = sheet.grid_of(pid)
                water = [i for i, n in sheet.neighbours(pid) if sheet.waterP(n)]
                if not water and pid not in delta:
                    raise ValueError("%s: port %s has no sea neighbour and is not a listed delta port" % (sheet.path, pid))
                slot = list(g.geom["edges"])[water[0]] if water else delta[pid]
                out.append('%s<hex id="%s"><glyph symbol="port" slot="%s" color="cyan"/></hex>' % (indent, pid, slot))
            else:
                out.append('%s<hex id="%s"><glyph symbol="%s" color="%s"/></hex>' % (indent, pid, sym, PLACE_COLOUR[sym]))
    return out


# ---------------------------------------------------------------- main
PLACE_HEADS = ("capital", "city", "town", "port")
GAP = 4                 # a boundary's loose end is joined to a place it may end within this many hexsides
LINE_GAP = {"river": 2}  # rivers reach less far: longer joins ran round forest edges the print does not show


def broken_kinds(sheet):
    """The kinds the broken rules name: a line id or link kind; "places" for a capital, city, town or port
    rule; both link kinds for a network rule."""
    sink = []
    out = set()
    for problem in N.check(sheet, quiet=True, emit=sink.append):
        head = problem.split()[0]
        if head in PLACE_HEADS:
            out.add("places")
        elif "network" == head:
            out.update(("rail", "road"))
        else:
            out.add(head)
    return out


class Boundaries:
    """Systematic repair of a sheet's own boundary, mountain, river and lake hexsides (Dai Senso): drop the
    hexsides the rules do not allow, join each loose end that may not end where it does to the nearest place it
    may end (within GAP hexsides), then drop pieces still shorter than the rules allow."""

    def __init__(self, sheet):
        self.s = sheet
        self.specks = set(N.DS_SPECKS.split())

    def allowed(self, line, side):
        return self.s.inner_sideP(side) if line in N.BOUNDARIES else self.s.land_sideP(side)

    def end_okP(self, line, v, sides):
        s = self.s
        if line in N.BOUNDARIES:
            others = {w for kind in N.BOUNDARIES if kind != line for x in s.lines.get(kind, {}) for w in s.side_ends[x]}
            return s.map_edge_vertexP(v) or s.coast_vertexP(v) or v in others
        if "river" == line:
            lakes = {w for x in s.lines.get("lake", {}) for w in s.side_ends[x]}
            return s.map_edge_vertexP(v) or s.coast_vertexP(v) or v in lakes
        return True

    def branch(self, v, sides):
        """The vertices from loose end v back to the first junction of its piece (v included)."""
        s = self.s
        adj = collections.defaultdict(set)
        for x in sides:
            for w in s.side_ends[x]:
                adj[w].add(x)
        out, used, u = {v}, set(), v
        while len(adj[u]) <= 2:
            nxt = [x for x in adj[u] if x not in used]
            if not nxt:
                break
            used.add(nxt[0])
            a, b = s.side_ends[nxt[0]]
            u = b if a == u else a
            out.add(u)
        return out

    def join(self, line, v, sides, piece, ends):
        """Shortest run of allowed hexsides from v to a vertex where the line may end or to any vertex of the line
        outside v's own branch: another piece, or another part of its own piece (a border meeting a border, or a
        broken ring closing)."""
        s = self.s
        own_branch = self.branch(v, sides)
        targets = {w for x in sides for w in s.side_ends[x]} - own_branch
        queue, seen = collections.deque([(v, [])]), {v}
        while queue:
            u, path = queue.popleft()
            if path and (u in targets or self.end_okP(line, u, sides)):
                return path
            if len(path) == LINE_GAP.get(line, GAP):
                continue
            for x in sorted(s.vertex_sides[u]):
                a, b = s.side_ends[x]
                w = b if a == u else a
                if w in seen or w in own_branch or x in sides or not self.allowed(line, x):
                    continue
                seen.add(w)
                queue.append((w, path + [x]))
        return None

    def prune_hairs(self, line, sides):
        """Drop branches of HAIR hexsides or fewer that leave a junction and end where the line may not end: trace
        spurs (a dash picked up beside a ridge, a label stroke), not printed ends."""
        s = self.s
        while True:
            adj = collections.defaultdict(set)
            for x in sides:
                for v in s.side_ends[x]:
                    adj[v].add(x)
            hair = None
            for leaf in sorted(v for v in adj if len(adj[v]) == 1 and not self.end_okP(line, v, sides)):
                branch, v = [], leaf
                while len(adj[v]) <= 2 and len(branch) <= HAIR:
                    nxt = [x for x in adj[v] if x not in branch]
                    if not nxt:
                        break
                    branch.append(nxt[0])
                    a, b = s.side_ends[nxt[0]]
                    v = b if a == v else a
                if len(adj[v]) >= 3 and len(branch) <= HAIR:
                    hair = branch
                    break
            if hair is None:
                return sides
            sides = sides - set(hair)

    def repair(self, line):
        """The line's hexsides after the repairs above. The sheet's authoring lists come first: SHEETS boundary_add
        (printed stretches the scan trace missed) is added, boundary_remove (traced hexsides the print does not
        show) is taken away."""
        s = self.s
        authored = SHEETS[s.id]
        added = {s.side_of(*ref.split(":")) for ref in authored.get("boundary_add", {}).get(line, "").split()}
        removed = {s.side_of(*ref.split(":")) for ref in authored.get("boundary_remove", {}).get(line, "").split()}
        sides = {x for x in (set(s.lines.get(line, {})) | added) - removed if self.allowed(line, x)}
        joined = True
        while joined:
            joined = False
            for piece, ends in N.side_pieces(s, sides):
                path = next((p for p in (self.join(line, v, sides, piece, ends) for v in ends
                                         if not self.end_okP(line, v, sides)) if p), None)
                if path:
                    sides |= set(path)
                    joined = True
                    break
        sides = self.prune_hairs(line, sides)
        least = {"mountain": N.RANGE_MIN}.get(line, N.BORDER_MIN if line in N.BOUNDARIES else 1)
        keep = set()
        for piece, _ in N.side_pieces(s, sides):
            verts = {v for x in piece for v in s.side_ends[x]}
            drains = "river" != line or any(self.end_okP(line, v, sides) for v in verts)
            if drains and (len(piece) >= least or {s.side_ref[x] for x in piece} <= self.specks):
                keep |= piece
        return keep


def land_path(sheet, a, b):
    """Cheapest path of neighbouring land hexes from a to b: one per step, plus a little for straying from the
    straight a-b line, so a chain need list only its turning points."""
    pa, pb = sheet.centre(a), sheet.centre(b)
    size = sheet.grids[0].size
    heap, done = [(0.0, a, None)], {}
    while heap:
        c, h, parent = heapq.heappop(heap)
        if h in done:
            continue
        done[h] = parent
        if h == b:
            path = [h]
            while done[path[-1]] is not None:
                path.append(done[path[-1]])
            return path[::-1]
        for _, n in sheet.neighbours(h):
            if n not in done and sheet.landP(n):
                heapq.heappush(heap, (c + 1.0 + 0.2 * seg_dist(sheet.centre(n), pa, pb) / size, n, h))
    return None


def authored_links(sheet, cfg, kind):
    """The hand-authored chains of one link kind, read off the print; consecutive listed hexes are joined by
    land_path. Written as maximal chains."""
    net = set()
    for name, ids in cfg["links"][kind]:
        hexes = [sheet.known(p) for p in ids.split()]
        for a, b in zip(hexes, hexes[1:]):
            path = land_path(sheet, a, b)
            if path is None:
                raise ValueError("%s: %s %s: no land path from %s to %s" % (sheet.path, kind, name, a, b))
            for x, y in zip(path, path[1:]):
                net.update({(x, y), (y, x)})
    return chains(sheet, net)


def authored_places(sheet, cfg):
    out = collections.defaultdict(list)
    for sym in PLACE_HEADS:
        for pid in cfg["places"][sym].split():
            out[sheet.known(pid)].append(sym)
    return out


def authored_borders(sheet, cfg):
    """The hand-authored border chains: each a list of hexside refs that must join end to end."""
    if "borders" not in cfg:
        raise ValueError("%s: border lines are broken and the sheet has no authored borders" % sheet.path)
    sides = set()
    for name, refs in cfg["borders"]:
        chain = [sheet.side_of(*ref.split(":")) for ref in refs.split()]
        for a, b in zip(chain, chain[1:]):
            if not set(sheet.side_ends[a]) & set(sheet.side_ends[b]):
                raise ValueError("%s: border %s: %s does not touch %s" % (
                    sheet.path, name, sheet.side_ref[a], sheet.side_ref[b]))
        sides |= set(chain)
    return sides


def main(argv):
    N.utf8_output()
    paths = [a for a in argv[1:] if not a.startswith("--")]
    if len(paths) != 1:
        print(__doc__)
        return 2
    sheet = N.Sheet(paths[0])
    if sheet.id not in SHEETS:
        raise ValueError("%s: no tidy settings for sheet id %s" % (sheet.path, sheet.id))
    cfg = SHEETS[sheet.id]
    kinds = broken_kinds(sheet)
    if not kinds:
        print("%s: networks already pass network_check; nothing to do" % sheet.path)
        return 0
    land = [sheet.known(p) for p in cfg.get("land", "").split() if sheet.waterP(sheet.known(p))]
    for pid in land:
        sheet.terrain[pid] = "clear"          # the links, ports and repairs below see the corrected terrain
    lines = {}
    if "river" in kinds and "rivers" in cfg:
        lines["river"] = Rivers(sheet, cfg).build()
    if "border" in kinds and "borders" in cfg:
        lines["border"] = authored_borders(sheet, cfg)
    for line in cfg.get("repair_lines", ()):
        if line in kinds:
            lines[line] = Boundaries(sheet).repair(line)
    unrepaired = {k for k in kinds if k in sheet.lines} - set(lines)
    if unrepaired:
        raise ValueError("%s: no repair for broken lines %s" % (sheet.path, " ".join(sorted(unrepaired))))
    river_sides = lines.get("river", set(sheet.lines.get("river", {})))
    authored = cfg.get("links", {})
    links = {}
    for kind in sorted((set(sheet.links) | set(authored)) & kinds):
        links[kind] = authored_links(sheet, cfg, kind) if kind in authored else Links(sheet, cfg, kind, river_sides).build()
    places = None
    if "places" in kinds:
        if "places" not in cfg:
            raise ValueError("%s: place rules are broken and the sheet has no authored places" % sheet.path)
        places = authored_places(sheet, cfg)
    print("%s: rewriting %s" % (sheet.path, " ".join(sorted(kinds | ({"land"} if land else set())))))
    if "--dry-run" in argv:
        return 0
    rewrite(sheet, lines, links, cfg.get("link_lines"), places, land or None)
    problems = N.check(N.Sheet(paths[0]), quiet=True)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
