# Copyright Ben Paul Wise. All Rights Reserved.
"""maneuvers.py -- the maneuver studies on the standard (75 km) sheet, drawn by illustrate.py.

Two maneuvers the 100 km sheet had no room for (Ben, 2026-10-06; in the memorandum since then):
  A. Xue Yue's defence of Changsha (the 1939-42 battles, the third of them as the model): screens on the
     river lines north of the city, the main body withdrawn into the hills off the axis, a garrison in the
     fortified city, then the flank groups striking into the lane behind the attacker's wing; and why it
     failed in 1944 against eight divisions on three columns.
  B. The envelopment of Hengyang, June to August 1944: the ring, the siege, the relief attempts, the fall.
The pieces are those of the preliminary counter set; group-army pieces stand in for the armies named in
the text, and "id:r" draws a piece reduced.  Positions are worked examples for the prototype, not a
scenario.  Drawing order: map, minor rivers, washes, counters, then moves and attacks on top (so a
one-hex move stays visible), move numbers, river letters, notes.  Notes are placed in crop-relative
pixels (both crops are 599 px wide) with leader lines.
"""
import math

# the minor rivers are printed on the sheet (the minor-river line); a letter tag on each, near the western
# end of its hexside across the railway, clear of the moves that cross the middle of that hexside
RIVER_TAGS = [("1932:s", "X"), ("1933:s", "M")]

CHANGSHA_CROP = ("1630", "2336")
HENGYANG_CROP = ("1533", "2239")
TRIM = 33.0          # px from a hex centre to the edge of the counter on it, along a move
BOW = 26.0           # px a one-hex move bows sideways, so it shows between two counters


def maneuvers(I):
    JP, KMT, US = I.JP, I.KMT, I.US
    CUT = "#c81e1e"
    DARK = "#7a1f12"
    xa, ya = I.crop_box(*CHANGSHA_CROP)[:2]
    xb, yb = I.crop_box(*HENGYANG_CROP)[:2]

    def A(x, y):
        return (xa + x, ya + y)

    def B(x, y):
        return (xb + x, yb + y)

    # ---------------------------------------------------------------------------------------- primitives
    def toward(p, q, d):
        dx, dy = q[0] - p[0], q[1] - p[1]
        n = math.hypot(dx, dy) or 1.0
        return (p[0] + dx / n * d, p[1] + dy / n * d)

    def disc(p, q, n, color, side=1):
        """The move number beside the start of a move from p toward q."""
        dx, dy = q[0] - p[0], q[1] - p[1]
        L_ = math.hypot(dx, dy) or 1.0
        ux, uy = dx / L_, dy / L_
        return I.step((p[0] + ux * 10 - uy * 17 * side, p[1] + uy * 10 + ux * 17 * side), n, color)

    def curve(p0, p1, color, dash, bow):
        mx, my = (p0[0] + p1[0]) / 2, (p0[1] + p1[1]) / 2
        dx, dy = p1[0] - p0[0], p1[1] - p0[1]
        L_ = math.hypot(dx, dy) or 1.0
        cx, cy = mx - dy / L_ * bow, my + dx / L_ * bow
        d = "M%.1f,%.1f Q%.1f,%.1f %.1f,%.1f" % (p0[0], p0[1], cx, cy, p1[0], p1[1])
        return ('<path d="%s" fill="none" stroke="#ffffff" stroke-width="10" stroke-opacity="0.8" stroke-linecap="round"/>'
                '<path d="%s" fill="none" stroke="%s" stroke-width="5" stroke-linecap="round"%s marker-end="url(#arrow-%s)"/>'
                % (d, d, color, ' stroke-dasharray="11,7"' if dash else "", color.lstrip("#")))

    def shift(pts, d):
        """Move a polyline sideways by d px (positive: to the left of the direction of travel)."""
        out = []
        for i, p in enumerate(pts):
            segs = [(pts[j], pts[j + 1]) for j in (i - 1, i) if 0 <= j < len(pts) - 1]
            nx = ny = 0.0
            for a, b in segs:
                L_ = math.hypot(b[0] - a[0], b[1] - a[1]) or 1.0
                nx += (b[1] - a[1]) / L_
                ny += -(b[0] - a[0]) / L_
            k = math.hypot(nx, ny) or 1.0
            out.append((p[0] + nx / k * d, p[1] + ny / k * d))
        return out

    def mv(path, color, n=None, dash=False, start=False, end=True, bow=1, side=1, offset=0.0):
        """A move along hex centres.  start/end: a counter stands there at the end of the figure, so the
        line stops at its edge.  A one-hex move is drawn as a curve bowing to the left (bow=1) or right."""
        pts = [I.P(p) for p in path]
        p0, p1 = pts[0], pts[-1]
        if start:
            pts[0] = toward(pts[0], pts[1], TRIM)
        if end:
            pts[-1] = toward(pts[-1], pts[-2], TRIM + 4)
        if offset:
            pts = shift(pts, offset)
        if len(pts) == 2 and start and end:
            # the straight line would be a stub between two counters; bow it out instead
            a = toward(p0, p1, TRIM * 0.7)
            b = toward(p1, p0, TRIM * 0.9)
            mxp, myp = (p0[0] + p1[0]) / 2, (p0[1] + p1[1]) / 2
            dx, dy = p1[0] - p0[0], p1[1] - p0[1]
            L_ = math.hypot(dx, dy) or 1.0
            off = (-dy / L_ * BOW * bow, dx / L_ * BOW * bow)
            a = (a[0] + off[0] * 0.9, a[1] + off[1] * 0.9)
            b = (b[0] + off[0] * 0.9, b[1] + off[1] * 0.9)
            o = [curve(a, b, color, dash, BOW * 0.6 * bow)]
            q0 = a
        else:
            o = [I.arrow(pts, color, width=5, dash=dash)]
            q0 = pts[0]
        if n is not None:
            o.append(disc(q0, pts[1] if len(pts) > 2 or not (start and end) else I.P(path[1]), n, color, side))
        return "".join(o)

    def atk(a, b, color, n=None, side=1):
        """An attack from hex a into hex b: a burst on the hexside and a short arrow across it, on top."""
        pa, pb = I.P(a), I.P(b)
        mid = ((pa[0] + pb[0]) / 2, (pa[1] + pb[1]) / 2)
        o = [I.burst(mid, None, r=15)]
        o.append(I.arrow([toward(mid, pa, 20), toward(mid, pb, 19)], color, width=5))
        if n is not None:
            o.append(disc(toward(mid, pa, 20), mid, n, color, side))
        return "".join(o)

    def rivers():
        return []

    def side_point(edge, t):
        """A point a fraction t along a hexside, from its first corner to its second (s: east to west)."""
        h, d = edge.split(":")
        c, r = I.L.parse_id(h)
        a, b = I.L.FLAT["edge_corners"][d]
        (ax, ay), (bx, by) = I.L.corner(c, r, a), I.L.corner(c, r, b)
        return (ax + (bx - ax) * t, ay + (by - ay) * t)

    def river_tags():
        return [I.tag(side_point(e, 0.82), n) for e, n in RIVER_TAGS]

    leg_a = [(JP, False, "Japanese move or attack"), (KMT, False, "KMT move or attack"), (KMT, True, "KMT withdrawal"),
             ("burst", False, "battle"), ("#2f74c0", False, "minor river: X Xinqiang, M Miluo")]
    leg_b = [(JP, False, "Japanese move or attack"), (KMT, False, "KMT move or attack"), (KMT, True, "KMT withdrawal"),
             (US, True, "air supply"), ("zone:" + CUT, False, "the ring: occupied or in ZOC"), ("burst", False, "battle")]

    # ------------------------------------------------------------------------------------------ A. Changsha
    def a1():
        return rivers() + [
            I.zone(["1933", "2033"], KMT, 0.10, KMT, 2.5, "4,5"),
            I.zone(["2133", "2134"], KMT, 0.16, KMT, 3, "8,6"),
            I.zone(["1833"], KMT, 0.10, KMT, 3, "8,6"),
            I.stack("1932", ["jp-air", "jp-d34", "jp-d3"]), I.stack("2032", ["jp-d40"]),
            I.stack("1933", ["kmt-ga30"]), I.stack("2033", ["kmt-ga26"]),
            I.stack("2133", ["kmt-ga24"]), I.stack("2134", ["kmt-hq9", "kmt-ga27"]),
            I.stack("1833", ["kmt-ga22"]), I.stack("1934", ["kmt-fort", "kmt-ga10"]),
        ] + river_tags() + [
            I.callout(A(165, 110), "1932", "Yueyang:|the railhead", JP),
            I.callout(A(10, 200), "1933", "screens on|the river lines", KMT, anchor="start"),
            I.callout(A(10, 290), "1833", "west group,|across the Xiang", KMT, anchor="start"),
            I.callout(A(150, 352), "1934", "garrison in the|fortified city", KMT),
            I.callout(A(590, 330), ["2133", "2134"], "main body:|off the axis,|in the hills", KMT, anchor="end"),
        ]

    def a2():
        return rivers() + [
            I.stack("1932", ["jp-air"]), I.stack("1933", ["jp-d34", "jp-d3"]), I.stack("2034", ["jp-d40"]),
            I.stack("1833", ["kmt-ga22", "kmt-ga30"]), I.stack("2133", ["kmt-ga24", "kmt-ga26"]),
            I.stack("2134", ["kmt-hq9", "kmt-ga27"]), I.stack("1934", ["kmt-fort", "kmt-ga10"]),
            I.burst(((I.P("1932")[0] + I.P("1933")[0]) / 2, (I.P("1932")[1] + I.P("1933")[1]) / 2), None, r=15),
            mv(["1932", "1933"], JP, 1, start=True, side=-1),
            mv(["1933", "1833"], KMT, 2, dash=True, start=True),
            I.burst(((I.P("2032")[0] + I.P("2033")[0]) / 2, (I.P("2032")[1] + I.P("2033")[1]) / 2), None, r=15),
            mv(["2032", "2033", "2034"], JP, 3, side=-1),
            mv(["2033", "2133"], KMT, 4, dash=True, start=False),
        ] + river_tags() + [
            I.callout(A(10, 205), "1833", "the screens step|aside, not back", KMT, anchor="start"),
            I.callout(A(590, 440), "2034", "the east wing swings|around toward the city", JP, anchor="end"),
        ]

    def a3():
        return rivers() + [
            I.zone(["2034"], CUT, 0.22, CUT, 4),
            I.stack("1932", ["jp-air"]), I.stack("1933", ["jp-d34", "jp-d3"]), I.stack("2034", ["jp-d40"]),
            I.stack("1833", ["kmt-ga22", "kmt-ga30"]), I.stack("2033", ["kmt-ga26"]), I.stack("2133", ["kmt-ga24"]),
            I.stack("2134", ["kmt-hq9", "kmt-ga27"]), I.stack("1934", ["kmt-fort", "kmt-ga10"]),
            atk("1933", "1934", JP, 1, side=-1),
            mv(["2133", "2033"], KMT, 2, start=True, side=-1),
            atk("2134", "2034", KMT, 3),
            atk("1833", "1933", KMT, 4),
        ] + river_tags() + [
            I.callout(A(360, 112), "2033", "into the lane|behind the east wing", KMT),
            I.callout(A(590, 440), "2034", "the east wing:|out of supply", DARK, anchor="end"),
            I.callout(A(10, 205), "1933", "attacked from|three sides", DARK, anchor="start"),
            I.callout(A(150, 352), "1934", "assaults on the|city repulsed", KMT),
        ]

    def a4():
        return rivers() + [
            I.stack("1932", ["jp-air", "jp-d34:r", "jp-d3"]), I.stack("2032", ["jp-d40:r"]),
            I.stack("1933", ["kmt-ga30"]), I.stack("1833", ["kmt-ga22"]), I.stack("2133", ["kmt-ga24", "kmt-ga26"]),
            I.stack("2134", ["kmt-hq9", "kmt-ga27"]), I.stack("1934", ["kmt-fort", "kmt-ga10"]),
            I.burst(((I.P("2034")[0] + I.P("2033")[0]) / 2, (I.P("2034")[1] + I.P("2033")[1]) / 2), None, r=15),
            mv(["2034", "2033", "2032"], JP, 1, side=-1),
            mv(["2033", "2133"], KMT, 2, dash=True),
            mv(["1933", "1932"], JP, 3, start=True, side=-1),
            mv(["1833", "1933"], KMT, 4, start=True),
        ] + river_tags() + [
            I.callout(A(360, 112), "2032", "breaks out through|the lane, a step lost", JP),
            I.callout(A(10, 205), "1932", "the center back|over the Xinqiang", JP, anchor="start"),
            I.callout(A(590, 330), "2134", "the position|restored, the|attacker weaker", KMT, anchor="end"),
        ]

    def a5():
        return rivers() + [
            I.stack("1934", ["jp-d58", "jp-d34"]), I.stack("1933", ["jp-d68", "jp-air"]),
            I.stack("2133", ["jp-d13"]), I.stack("2134", ["jp-d3"]), I.stack("1833", ["jp-d40"]), I.stack("1834", ["jp-d116"]),
            I.stack("2233", ["kmt-ga24:r"]), I.stack("2235", ["kmt-hq9", "kmt-ga27:r"]), I.stack("1733", ["kmt-ga22:r"]),
            atk("1933", "1934", JP, 1, side=-1),
            mv(["2132", "2133"], JP, 2, side=-1),
            mv(["2033", "2134"], JP, 3, side=-1),
            mv(["1832", "1833", "1834"], JP, 4, offset=-36),
            mv(["2133", "2233"], KMT, dash=True, start=True),
            mv(["2134", "2235"], KMT, dash=True, start=True),
            mv(["1833", "1733"], KMT, dash=True, start=True),
        ] + river_tags() + [
            I.callout(A(400, 530), "1934", "18 June:|the city falls", DARK),
            I.callout(A(590, 175), "2133", "the east column attacks the|flank groups in the hills", JP, anchor="end"),
            I.callout(A(10, 380), "1834", "the west column|takes the far bank", JP, anchor="start"),
            I.callout(A(590, 512), "2235", "nothing remains to close|in behind the center", KMT, anchor="end"),
        ]

    # ------------------------------------------------------------------------------------------ B. Hengyang
    RING = ["1835", "1936", "1937", "1837", "1737", "1736"]

    def b1():
        return [
            I.stack("1934", ["jp-air", "jp-d34", "jp-d58"]), I.stack("1935", ["jp-d68"]),
            I.stack("1834", ["jp-d40", "jp-d116"]), I.stack("2035", ["jp-d3", "jp-d13"]),
            I.stack("1836", ["us-14af", "kmt-fort", "kmt-ga10"]),
            I.stack("1635", ["kmt-hq9", "kmt-ga27"]), I.stack("1638", ["kmt-ga24"]), I.stack("2136", ["kmt-ga30"]),
            I.callout(B(265, 444), "1836", "Hengyang: fortified city,|airfield, the 10th Army", KMT),
            I.callout(B(127, 158), "1635", "relief from the|west (79 Army)", KMT),
            I.callout(B(10, 575), "1638", "relief from|Guangxi (62 Army)", KMT, anchor="start"),
            I.callout(B(590, 372), "2136", "eastern group", KMT, anchor="end"),
            I.callout(B(470, 112), "1934", "Changsha fallen,|18 June", JP),
        ]

    def ring_b2_b3(relief_at):
        return [
            I.stack("1934", ["jp-air", "jp-d34"]), I.stack("1935", ["jp-d58"]),
            I.stack("1736", ["jp-d116"]), I.stack("1936", ["jp-d68"]), I.stack("1937", ["jp-d13"]), I.stack("2036", ["jp-d3"]),
            I.stack("1636", ["jp-d40"]),
            I.stack("1635", ["kmt-hq9", "kmt-ga27"]), I.stack(relief_at, ["kmt-ga24"]), I.stack("2136", ["kmt-ga30"]),
        ]

    def b2():
        return [I.zone(RING, CUT, 0.16, CUT, 3, "8,6")] + ring_b2_b3("1638") + [
            I.stack("1836", ["us-14af", "kmt-fort", "kmt-ga10"]),
            mv(["1834", "1835", "1736"], JP, 1, side=-1),
            mv(["1935", "1936"], JP, 2, start=True),
            mv(["2035", "2036", "1937"], JP, 3, side=-1),
            mv(["2035", "2036"], JP, 4, offset=-30),
            mv(["1834", "1735", "1636"], JP, 5),
            mv(["1934", "1935"], JP, start=True, bow=-1),
        ] + [
            I.callout(B(480, 432), "1937", "13 Div cuts the|railway at Leiyang", JP),
            I.callout(B(10, 420), "1636", "40 Div screens|the west", JP, anchor="start"),
        ]

    def b3():
        return [I.zone(RING, CUT, 0.16, CUT, 3, "8,6")] + ring_b2_b3("1738") + [
            I.stack("1836", ["kmt-fort", "kmt-ga10:r"]),
            atk("1936", "1836", JP, 1), atk("1736", "1836", JP, 2, side=-1),
            atk("1635", "1636", KMT, 3),
            mv(["1638", "1738", "1737"], KMT, 4, end=False, side=-1),
            atk("1736", "1737", JP, 5, side=-1),
            mv(["1737", "1738"], KMT, 6, dash=True, end=True, start=False, side=-1, offset=-18),
            atk("2136", "2036", KMT, 7),
            mv([B(265, 625), "1838", "1837", "1836"], US, dash=True, offset=0),
        ] + [
            I.callout(B(127, 158), "1635", "79 Army:|blocked", KMT),
            I.callout(B(10, 560), "1738", "62 Army reaches the|ring, is thrown back", KMT, anchor="start"),
            I.callout(B(255, 600), None, "air drops: the|only supply", US, anchor="end"),
            I.callout(B(590, 372), "2136", "eastern|group blocked", KMT, anchor="end"),
        ]

    def b4():
        return [
            I.stack("1934", ["jp-air"]), I.stack("1835", ["jp-d58"]), I.stack("1836", ["jp-d116:r", "jp-d68:r"]),
            I.stack("1937", ["jp-d13"]), I.stack("2036", ["jp-d3"]), I.stack("1636", ["jp-d40"]),
            I.stack("1635", ["kmt-hq9", "kmt-ga27"]), I.stack("1738", ["kmt-ga24"]), I.stack("2136", ["kmt-ga30"]),
            mv(["1935", "1835"], JP, 1, end=True),
            atk("1835", "1836", JP, 2), atk("1937", "1836", JP, 2, side=-1),
            mv(["1936", "1836"], JP, 3, end=True), mv(["1736", "1836"], JP, 3, end=True, side=-1),
            mv(["1836", "1737", "1637", "1538"], JP, dash=True, start=True),
        ] + [
            I.callout(B(330, 525), "1836", "8 August: the 10th Army|surrenders after 47 days", DARK),
            I.callout(B(10, 560), "1538", "next: Lingling|and Guilin", JP, anchor="start"),
            I.callout(B(590, 120), None, "the fortified-city rule should|make the siege cost two or|three Japanese activations", "#111111", anchor="end"),
        ]

    A_ = dict(crop=CHANGSHA_CROP, size=56.0, text=0.62, legend=leg_a, legend_w=330, legend_at=("left", "bottom"))
    B_ = dict(crop=HENGYANG_CROP, size=56.0, text=0.62, legend=leg_b, legend_w=360, legend_at=("right", "bottom"))
    return [
        dict(A_, name="a1-changsha-position", title="Changsha 1941-42: the position", sub="Screens forward, the main body in the hills",
             title_counters=["kmt-hq9"], items=a1, legend=leg_a[4:] + [("zone:" + KMT, False, "KMT groups (outlined)")]),
        dict(A_, name="a2-changsha-advance", title="Changsha: the advance", sub="The screens step aside; the east wing swings around",
             items=a2),
        dict(A_, name="a3-changsha-furnace", title="Changsha: the counterattack", sub="The flank groups strike into the lane",
             items=a3, legend=leg_a[:2] + leg_a[3:] + [("zone:" + CUT, False, "out of supply")]),
        dict(A_, name="a4-changsha-withdrawal", title="Changsha: the withdrawal", sub="Back to the start line, weaker",
             items=a4),
        dict(A_, name="a5-changsha-1944", title="Changsha 1944: three columns", sub="Eight divisions fill the axis and both lanes",
             title_counters=["jp-dir-ichigo2"], items=a5),
        dict(B_, name="b1-hengyang-position", title="Hengyang 1944: the position", sub="22 June, after the fall of Changsha",
             title_counters=["jp-dir-ichigo2"], items=b1, legend=leg_b[:2]),
        dict(B_, name="b2-hengyang-ring", title="Hengyang: the ring closes", sub="23 June to 2 July",
             items=b2, legend=leg_b[:1] + leg_b[4:5]),
        dict(B_, name="b3-hengyang-siege", title="Hengyang: siege and relief", sub="July: the ring holds against relief",
             items=b3),
        dict(B_, name="b4-hengyang-fall", title="Hengyang: the fall", sub="4 to 8 August",
             items=b4, legend=leg_b[:1] + leg_b[5:]),
    ]
# Copyright Ben Paul Wise. All Rights Reserved.
