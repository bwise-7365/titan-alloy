# Copyright Ben Paul Wise. All Rights Reserved.
"""lattice.py -- find the hex lattice of a scanned map from its own periodicity (reader tool B1).

    python lattice.py IMAGE --out DIR [--overlay] [--svg] [--anchor ID C R ID C R ...] [--spacing PX] [--max-side N]

Writes DIR/lattice.json (contract: tools/reader/README.md) and, with --overlay, DIR/overlay.jpg for a
person's eye: printed hexes in magenta, unprinted lattice cells in orange.

Method, in the order the numbers are found:
  period      the autocorrelation of an edge map of the scan peaks at every lattice translation; the
              nearest ring of six peaks gives the centre-to-centre spacing and the lattice angle
  orientation pointy lattices have a neighbour due east (angle 0 mod 60); flat ones at 30 mod 60
  rotation    the residue of that angle; reported, never corrected in the pixels
  phase       cross-correlation with a seven-hex outline template locates one hex centre
  extent      a cell is printed when at least four of its six edges carry edge energy above the
              Otsu threshold pooled over every cell; the complement inside the index box is clip_index
Numbering (printed ids) is not read here; anchors are added by legend.py or a person.
"""
import json
import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

Image.MAX_IMAGE_PIXELS = None
SQRT3 = math.sqrt(3)
GATE_PHASE = 1.0       # outline-minus-interior ink of the seven-hex template at its best phase, relative to the image mean
GATE_RING = 4          # of the six nearest autocorrelation peaks that must be present


def option(argv, key, default=None):
    return argv[argv.index(key) + 1] if key in argv else default


# ---------------------------------------------------------------- edge map


def gradient(im):
    g = np.asarray(im.convert("L"), dtype=np.float32)
    e = np.hypot(ndimage.sobel(g, axis=1), ndimage.sobel(g, axis=0))
    top = np.percentile(e, 99.0)
    return np.clip(e / max(top, 1e-6), 0.0, 1.0)


def edge_map(path, max_side):
    """Gradient magnitude of the grey scan at full resolution (for the outline tests) and downscaled so its
    longer side is max_side (for the autocorrelation); returns (E_small, f, rgb_full, E_full, grey)."""
    im = Image.open(path).convert("RGB")
    W, H = im.size
    f = min(1.0, max_side / max(W, H))
    small = im.resize((int(W * f), int(H * f)), Image.LANCZOS) if f < 1.0 else im
    grey = np.asarray(im.convert("L"), dtype=np.float32) / 255.0
    return gradient(small), f, im, gradient(im), grey


def candidates_at(rgb, max_side):
    """Ranked, scored candidates from the autocorrelation at one working resolution, spacings in source
    pixels."""
    W, H = rgb.size
    f = min(1.0, max_side / max(W, H))
    small = rgb.resize((int(W * f), int(H * f)), Image.LANCZOS) if f < 1.0 else rgb
    e = gradient(small)
    h, w = e.shape
    a = autocorrelation(e)
    cands, _ = find_period(e, a, rmin=max(8, int(0.015 * min(h, w))), rmax=min(h, w) // 4)
    for c in cands:
        c["s_px"] = c["s"] / f
        c["f"] = f
        c["cx_px"], c["cy_px"] = c["cx"] / f, c["cy"] / f
    return cands


def consistent_candidates(rgb, scales=(1600, 2400)):
    """Candidates from every working resolution, merged by spacing (2 percent) and orientation. A candidate
    is scored at the FINEST scale where it appears: a fine printed grid can be invisible at the coarse
    scale (Downfall larger) and a coarse texture flatters itself there."""
    merged = []
    for m in sorted(scales):
        for c in candidates_at(rgb, m):
            same = [d for d in merged if abs(d["s_px"] - c["s_px"]) < 0.02 * c["s_px"] and orientation_of(d["ang"])[0] == orientation_of(c["ang"])[0]]
            if same:
                same[0].update(c)          # the finer scale's numbers replace the coarser
            else:
                merged.append(c)
    return merged


# ---------------------------------------------------------------- period and angle


def autocorrelation(e):
    z = e - e.mean()
    F = np.fft.fft2(z)
    a = np.fft.ifft2(F * np.conj(F)).real
    a = np.fft.fftshift(a)
    return a / a.max()


def local_maxima(a, rmin, rmax):
    h, w = a.shape
    cy, cx = h // 2, w // 2
    mx = ndimage.maximum_filter(a, size=5)
    ys, xs = np.nonzero((a == mx) & (a > 0.02))
    out = []
    for y, x in zip(ys, xs):
        dx, dy = x - cx, y - cy
        r = math.hypot(dx, dy)
        if rmin <= r <= rmax:
            out.append((float(a[y, x]), float(dx), float(dy)))
    out.sort(reverse=True)
    return out


def subpixel(a, x, y):
    """Parabolic refinement of a peak at integer offsets (x, y) from the centre of a."""
    h, w = a.shape
    cy, cx = h // 2, w // 2
    j, i = int(round(cy + y)), int(round(cx + x))
    if not (0 < i < w - 1 and 0 < j < h - 1):
        return x, y
    def vertex(l, m, r):
        den = l - 2 * m + r
        return 0.0 if 0 == den else 0.5 * (l - r) / den
    return x + vertex(a[j, i - 1], a[j, i], a[j, i + 1]), y + vertex(a[j - 1, i], a[j, i], a[j + 1, i])


def ring_values(peaks, s, ang):
    """The strongest peak found at each of the six lattice-ring positions round the candidate (0 if none)."""
    out = []
    for m in range(6):
        tx, ty = s * math.cos(math.radians(ang + 60 * m)), s * math.sin(math.radians(ang + 60 * m))
        out.append(max([pv for pv, px, py in peaks if math.hypot(px - tx, py - ty) < 0.12 * s] + [0.0]))
    return out


def ring_count(peaks, s, ang):
    return sum(v > 0 for v in ring_values(peaks, s, ang))


def harmonics(peaks, s, ang):
    """Collinear peaks at k * s for k = 2.., as (k, x, y)."""
    found = []
    for pv, px, py in peaks:
        pr = math.hypot(px, py)
        k = int(round(pr / s))
        if k < 2 or abs(pr - k * s) > 0.08 * s:
            continue
        pang = math.degrees(math.atan2(py, px))
        d = abs((pang - ang + 180) % 360 - 180)
        if min(d, abs(d - 180)) < 2.0:
            found.append((k, px, py))
    return found


def candidates(a, rmin, rmax, limit=12):
    """Ring-qualified peaks, one per distinct (spacing, orientation), strongest first. rmin encodes a
    plausibility bound: no wargame map has more than about sixty hex rows across its short side, so a
    period under 1.5 percent of the short side is a screen or a texture, never the lattice."""
    peaks = local_maxima(a, rmin, rmax)
    if not peaks:
        raise ValueError("no autocorrelation peak between %d and %d px" % (rmin, rmax))
    out, seen = [], set()
    for v, x, y in peaks[:80]:
        s = math.hypot(x, y)
        ang = math.degrees(math.atan2(y, x))
        rv = ring_values(peaks, s, ang)
        if sum(t > 0 for t in rv) < 6:          # a lattice has all six; a screen or ruling has fewer
            continue
        key = (int(round(s)), orientation_of(ang)[0])
        if key in seen:
            continue
        seen.add(key)
        x, y = subpixel(a, x, y)
        s = math.hypot(x, y)
        ang = math.degrees(math.atan2(y, x))
        hs = harmonics(peaks, s, ang)
        k_best = 1
        for k, px, py in sorted(hs, reverse=True)[:1]:
            px, py = subpixel(a, px, py)
            s = math.hypot(px, py) / k
            k_best = k
        out.append(dict(v=v, s=s, ang=ang, ring=6, harmonic=k_best))
        if len(out) >= limit:
            break
    if not out:
        raise ValueError("no candidate with all six ring peaks; the scan has no clear lattice")
    return out, peaks[:12]


def hinted_candidate(a, peaks, hint_small):
    """The strongest autocorrelation peak within 8 percent of the hinted spacing (in the downscaled
    image), whatever its ring, as a candidate; None if there is none."""
    best = None
    for v, x, y in peaks:
        s = math.hypot(x, y)
        if abs(s - hint_small) <= 0.08 * hint_small and (best is None or v > best[0]):
            best = (v, x, y)
    if best is None:
        return None
    v, x, y = best
    x, y = subpixel(a, x, y)
    s = math.hypot(x, y)
    ang = math.degrees(math.atan2(y, x))
    return dict(v=v, s=s, ang=ang, ring=ring_count(peaks, s, ang), harmonic=1, hinted=True)


def find_period(e, a, rmin, rmax, hint_small=None):
    """Every ring-qualified candidate is scored against the edge map with the seven-hex template: ink on
    the outline minus ink inside, relative to the image mean. Candidates that are lattice vectors of a
    finer candidate (twice or three times its spacing at the same angle, root-three times it at thirty
    degrees) are derived, never the fundamental. Returns the candidates, best first, and the peaks."""
    cands, peaks = candidates(a, rmin, rmax)
    if hint_small:
        hinted = hinted_candidate(a, local_maxima(a, max(4, int(0.5 * hint_small)), rmax), hint_small)
        if hinted is None:
            raise ValueError("no autocorrelation peak within 8 percent of the hinted spacing")
        cands = [hinted]
    for c in cands:
        orientation, _ = orientation_of(c["ang"])
        size = c["s"] / SQRT3
        cx, cy, score = phase(e, size, c["s"], c["ang"], orientation)
        c.update(cx=cx, cy=cy, score=score)
    def derived(c):
        for d in cands:
            if d is c or d["s"] >= c["s"]:
                continue
            ratio = c["s"] / d["s"]
            dang = abs((c["ang"] - d["ang"] + 180) % 60 - 30)     # 30 = same direction mod 60, 0 = rotated by 30
            for k, want in ((2.0, 30), (3.0, 30), (SQRT3, 0), (2 * SQRT3, 0)):
                if abs(ratio - k) < 0.05 * k and abs(dang - want) < 4:
                    return True
        return False
    cands = [c for c in cands if not derived(c)] or cands
    for c in cands:
        # the same lattice is found along any of its six neighbour directions; report the canonical one
        # so that the residue is the scan's rotation and not a multiple of sixty degrees
        orientation, rot = orientation_of(c["ang"])
        c["ang"] = rot if "pointy" == orientation else 30.0 + rot
    cands.sort(key=lambda c: -c["score"])
    return cands, peaks


def orientation_of(ang):
    m = (ang + 15.0) % 60.0 - 15.0          # residue in [-15, 45)
    if m < 15.0:
        return "pointy", m
    return "flat", m - 30.0


# ---------------------------------------------------------------- phase


def hex_outline_points(size, rot_deg, orientation, n=24):
    start = 30.0 if "pointy" == orientation else 0.0
    pts = []
    for k in range(6):
        a0 = math.radians(start + 60 * k + rot_deg)
        a1 = math.radians(start + 60 * (k + 1) + rot_deg)
        for t in np.linspace(0.0, 1.0, n, endpoint=False):
            pts.append((size * ((1 - t) * math.cos(a0) + t * math.cos(a1)),
                        size * ((1 - t) * math.sin(a0) + t * math.sin(a1))))
    return pts


def basis(spacing, ang, orientation):
    """Lattice basis in pixels: u along the measured neighbour angle, w at +60 degrees."""
    a = math.radians(ang)
    u = (spacing * math.cos(a), spacing * math.sin(a))
    b = math.radians(ang + 60.0)
    w = (spacing * math.cos(b), spacing * math.sin(b))
    return u, w


def phase(e, size, spacing, ang, orientation):
    """Correlate a seven-hex outline template with the edge map; the peak is a hex centre. The score is
    ink on the outline minus ink inside the seven hexes, relative to the image mean: a lattice has ink
    on its outline and paper inside; a halftone rosette, a doubled lattice and the root-three ring all
    have ink inside as well."""
    h, w = e.shape
    rot = orientation_of(ang)[1]
    u, v = basis(spacing, ang, orientation)
    T = np.zeros_like(e)
    centres = [(0, 0)] + [(round(u[0] * i + v[0] * j), round(u[1] * i + v[1] * j))
                          for i, j in ((1, 0), (-1, 0), (0, 1), (0, -1), (1, -1), (-1, 1))]
    pts = hex_outline_points(size, rot, orientation)
    for cx, cy in centres:
        for px, py in pts:
            x, y = int(round(cx + px)) % w, int(round(cy + py)) % h
            T[y, x] = 1.0
    T = np.clip(ndimage.gaussian_filter(T, 0.7) * 2.0, 0.0, 1.0)
    yy, xx = np.mgrid[0:h, 0:w]
    I = np.zeros_like(e)
    for cx, cy in centres:
        dx = (xx - cx + w // 2) % w - w // 2
        dy = (yy - cy + h // 2) % h - h // 2
        I[(dx * dx + dy * dy) < (0.55 * size) ** 2] = 1.0
    Fe = np.fft.fft2(e)
    C = np.fft.ifft2(Fe * np.conj(np.fft.fft2(T))).real
    Ci = np.fft.ifft2(Fe * np.conj(np.fft.fft2(I))).real
    j, i = np.unravel_index(int(np.argmax(C)), C.shape)
    outline = C[j, i] / T.sum()
    inside = Ci[j, i] / I.sum()
    score = float((outline - inside) / (e.mean() + 1e-9))
    return float(i), float(j), score


# ---------------------------------------------------------------- refinement


def refine_lattice(e, cx, cy, spacing, ang, size, orientation, reach=3):
    """Ben's rule: the perfect lattice that minimises the RMS deviation from the printed outlines.
    Each cell's centre is moved to the best local match of one hex outline within +-reach px; the
    lattice (origin, spacing, angle) is then fitted to every well-matched centre by least squares."""
    h, w = e.shape
    rot = orientation_of(ang)[1]
    pts = np.array(hex_outline_points(size, rot, orientation, n=12))
    offs = np.arange(-reach, reach + 1)
    dx, dy = np.meshgrid(offs, offs)
    cells, u, v = index_frame(e, cx, cy, spacing, ang, size, orientation)
    obs = []
    for (c, r), (x, y, a, b) in cells.items():
        if not (size < x < w - size and size < y < h - size):
            continue
        xs = np.clip(np.rint(x + dx[..., None] + pts[:, 0]).astype(int), 0, w - 1)
        ys = np.clip(np.rint(y + dy[..., None] + pts[:, 1]).astype(int), 0, h - 1)
        score = e[ys, xs].mean(axis=-1)
        j, i = np.unravel_index(int(np.argmax(score)), score.shape)
        if score[j, i] < 0.15:
            continue
        obs.append((a, b, x + offs[i], y + offs[j]))
    if len(obs) < 12:
        return cx, cy, spacing, ang, len(obs)
    A = np.array([[1, 0, a, b] for a, b, _, _ in obs] + [[0, 1, a, b] for a, b, _, _ in obs], dtype=float)
    # unknowns: ox, oy, and the basis u=(ux,uy), w=(wx,wy) as a full 2x2 (rotation and scale free)
    Ax = np.array([[1, 0, a, 0, b, 0] for a, b, _, _ in obs] + [[0, 1, 0, a, 0, b] for a, b, _, _ in obs], dtype=float)
    bx = np.array([x for _, _, x, _ in obs] + [y for _, _, _, y in obs], dtype=float)
    sol = np.linalg.lstsq(Ax, bx, rcond=None)[0]
    ox, oy, ux, uy, wx, wy = sol
    s1, s2 = math.hypot(ux, uy), math.hypot(wx, wy)
    new_spacing = 0.5 * (s1 + s2)
    new_ang = math.degrees(math.atan2(uy, ux))
    return float(ox), float(oy), float(new_spacing), float(new_ang), len(obs)


def local_match(e, x, y, pts, offs, dx, dy):
    """Best outline match near (x, y): returns (x', y', score) within the +-reach window."""
    h, w = e.shape
    xs = np.clip(np.rint(x + dx[..., None] + pts[:, 0]).astype(int), 0, w - 1)
    ys = np.clip(np.rint(y + dy[..., None] + pts[:, 1]).astype(int), 0, h - 1)
    score = e[ys, xs].mean(axis=-1)
    j, i = np.unravel_index(int(np.argmax(score)), score.shape)
    return x + offs[i], y + offs[j], float(score[j, i])


def neighbours_of(key, orientation):
    """The six (dc, dr) index steps of a cell in the renderer's offset convention (odd rows or columns
    shifted), which is what index_frame produces."""
    c, r = key
    if "pointy" == orientation:
        shifted = (r % 2 == 1)
        steps = ((1, 0), (-1, 0), (1, -1), (1, 1), (0, -1), (0, 1)) if shifted else ((1, 0), (-1, 0), (0, -1), (0, 1), (-1, -1), (-1, 1))
    else:
        shifted = (c % 2 == 1)
        steps = ((0, -1), (0, 1), (1, 0), (1, 1), (-1, 0), (-1, 1)) if shifted else ((0, -1), (0, 1), (1, -1), (1, 0), (-1, -1), (-1, 0))
    return [(c + dc, r + dr) for dc, dr in steps]


def grow(e, cells, size, rot, orientation, spacing, reach):
    """Ben's propagation (2026-09-19): the global structure is the hex topology, the image is used only
    locally. Seeds are the cells whose outline matches sharply at the rigid lattice's prediction; from the
    best seed the lattice grows best-first: a frontier cell's position is predicted from an already placed
    neighbour (its position plus the ideal step between the two cells), refined within +-reach px against
    the outline, and accepted with the match score as its confidence. Nothing accumulates, because every
    placement is anchored to the ink; a region with no outline (sea, furniture) is crossed on prediction
    alone with confidence 0. Returns {key: (x, y, score)} and the largest move from the rigid lattice."""
    import heapq
    pts = np.array(hex_outline_points(size, rot, orientation, n=12))
    offs = np.arange(-reach, reach + 1)
    dx, dy = np.meshgrid(offs, offs)
    h, w = e.shape
    ideal = {k: (v[0], v[1]) for k, v in cells.items()}
    inside = lambda x, y: size < x < w - size and size < y < h - size
    scored = []
    for k, (x, y) in ideal.items():
        if inside(x, y):
            bx, by, sc = local_match(e, x, y, pts, offs, dx, dy)
            scored.append((sc, k, bx, by))
    scored.sort(reverse=True)
    placed = {}
    heap = []

    def push_neighbours(k):
        x, y, _ = placed[k]
        for n in neighbours_of(k, orientation):
            if n in placed or n not in ideal:
                continue
            px = x + (ideal[n][0] - ideal[k][0])
            py = y + (ideal[n][1] - ideal[k][1])
            if inside(px, py):
                bx, by, sc = local_match(e, px, py, pts, offs, dx, dy)
            else:
                bx, by, sc = px, py, 0.0
            heapq.heappush(heap, (-sc, n, bx, by, px, py))

    if scored:
        sc, k, bx, by = scored[0]
        placed[k] = (bx, by, sc)
        push_neighbours(k)
    while heap or len(placed) < len(ideal):
        if not heap:
            rest = [t for t in scored if t[1] not in placed]
            if not rest:
                break
            sc, k, bx, by = rest[0]
            placed[k] = (bx, by, sc)
            push_neighbours(k)
            continue
        negsc, n, bx, by, px, py = heapq.heappop(heap)
        if n in placed:
            continue
        sc = -negsc
        placed[n] = (bx, by, sc) if sc >= 0.12 else (px, py, 0.0)
        push_neighbours(n)
    for k, (x, y) in ideal.items():
        if k not in placed:
            placed[k] = (x, y, 0.0)
    shifts = [math.hypot(placed[k][0] - ideal[k][0], placed[k][1] - ideal[k][1]) for k in ideal]
    return placed, float(max(shifts))


def shared_vertices(cells, ideal, size, rot, orientation):
    """The perfect grid, deformed smoothly: a vertex is identified by its position on the IDEAL lattice
    (three cells meet there exactly), and drawn at the mean of those cells' estimates of it, so
    neighbouring cells tile without gaps or overlaps however far the growth moved them (Ben, A.2)."""
    corner = hex_outline_points(size, rot, orientation, n=1)
    q = 0.5 * size
    est = {}
    for key, val in cells.items():
        ix, iy = ideal[key]
        for px, py in corner:
            vk = (int(round((ix + px) / q)), int(round((iy + py) / q)))
            est.setdefault(vk, []).append((val[0] + px, val[1] + py))
    mean = {vk: (sum(t[0] for t in v) / len(v), sum(t[1] for t in v) / len(v)) for vk, v in est.items()}
    out = {}
    for key, val in cells.items():
        ix, iy = ideal[key]
        out[key] = [mean[(int(round((ix + px) / q)), int(round((iy + py) / q)))] for px, py in corner]
    return out


def refit(e, placed, ideal_ab, size, rot, orientation, spacing, reach, printed, rounds=2):
    """Ben's experiment (2026-09-19): fit the perfect lattice to the RECORDED cell positions, weighted by
    confidence, not to the image. The residual of each cell against that fit is the scan's deformation
    where it is smooth and a SLIP where it jumps against its neighbours (a cell locked onto the wrong
    printed hex). Slipped cells are re-placed at the fit plus the median residual of their neighbours,
    refined locally, and the lattice is refitted. Only PRINTED cells take part (Borodino, 2026-09-19:
    cells grown over chart panels and set-up maps lock onto furniture and made the report read 10.9 px
    where the map itself fits to 3.8); every other cell is placed at the fit plus its printed
    neighbours' median residual, confidence 0. Returns placed, the fit and a report."""
    pts = np.array(hex_outline_points(size, rot, orientation, n=12))
    offs = np.arange(-reach, reach + 1)
    dx, dy = np.meshgrid(offs, offs)
    h, w = e.shape
    keys = sorted(k for k in placed if k in printed)
    others = [k for k in placed if k not in printed]
    cols = 1 + max(c for c, _ in placed)
    rows = 1 + max(r for _, r in placed)
    report = []
    fit = None
    for rnd in range(rounds + 1):
        A, bx, by, wt = [], [], [], []
        for k in keys:
            x, y, sc = placed[k]
            ab = ideal_ab[k]
            A.append([1.0, ab[0], ab[1]])
            bx.append(x)
            by.append(y)
            wt.append(0.05 + sc)
        A, bx, by, wt = np.array(A), np.array(bx), np.array(by), np.sqrt(np.array(wt))
        sx = np.linalg.lstsq(A * wt[:, None], bx * wt, rcond=None)[0]
        sy = np.linalg.lstsq(A * wt[:, None], by * wt, rcond=None)[0]
        fit = dict(ox=float(sx[0]), oy=float(sy[0]), u=(float(sx[1]), float(sy[1])), w=(float(sx[2]), float(sy[2])))
        fitted = {k: (fit["ox"] + ideal_ab[k][0] * fit["u"][0] + ideal_ab[k][1] * fit["w"][0],
                      fit["oy"] + ideal_ab[k][0] * fit["u"][1] + ideal_ab[k][1] * fit["w"][1]) for k in keys}
        res = {k: (placed[k][0] - fitted[k][0], placed[k][1] - fitted[k][1]) for k in keys}
        rms = math.sqrt(sum(r[0] ** 2 + r[1] ** 2 for r in res.values()) / len(res))
        # smooth residual: median over the index neighbourhood, confident cells only
        fx = np.full((rows, cols), np.nan)
        fy = np.full((rows, cols), np.nan)
        for (c, r), (rx, ry) in res.items():
            if placed[(c, r)][2] >= 0.12:
                fx[r, c], fy[r, c] = rx, ry
        def smooth_at(c, r):
            win_x = fx[max(0, r - 2):r + 3, max(0, c - 2):c + 3]
            win_y = fy[max(0, r - 2):r + 3, max(0, c - 2):c + 3]
            vx, vy = win_x[~np.isnan(win_x)], win_y[~np.isnan(win_y)]
            if len(vx) < 4:
                return 0.0, 0.0
            return float(np.median(vx)), float(np.median(vy))
        slips = []
        for k in keys:
            smx, smy = smooth_at(*k)
            jump = math.hypot(res[k][0] - smx, res[k][1] - smy)
            if jump > 0.3 * spacing:
                slips.append(k)
        report.append(dict(round=rnd, rms_px=round(rms, 2), slips=len(slips),
                           spacing=round(0.5 * (math.hypot(*fit["u"]) + math.hypot(*fit["w"])), 3)))
        if rnd == rounds or not slips:
            break
        for k in slips:
            smx, smy = smooth_at(*k)
            px, py = fitted[k][0] + smx, fitted[k][1] + smy
            if size < px < w - size and size < py < h - size:
                nx, ny, sc = local_match(e, px, py, pts, offs, dx, dy)
                placed[k] = (nx, ny, sc) if sc >= 0.12 else (px, py, 0.0)
            else:
                placed[k] = (px, py, 0.0)
    for k in others:
        smx, smy = smooth_at(*k)
        placed[k] = (fit["ox"] + ideal_ab[k][0] * fit["u"][0] + ideal_ab[k][1] * fit["w"][0] + smx,
                     fit["oy"] + ideal_ab[k][0] * fit["u"][1] + ideal_ab[k][1] * fit["w"][1] + smy, 0.0)
    return placed, fit, report


# ---------------------------------------------------------------- index frame and extent


def index_frame(e, cx, cy, spacing, ang, size, orientation):
    """Every lattice cell whose centre lies inside the image, as (c, r) in the renderer's convention."""
    h, w = e.shape
    u, v = basis(spacing, ang, orientation)
    det = u[0] * v[1] - u[1] * v[0]
    def to_ab(x, y):
        dx, dy = x - cx, y - cy
        return (dx * v[1] - dy * v[0]) / det, (u[0] * dy - u[1] * dx) / det
    corners = [to_ab(0, 0), to_ab(w, 0), to_ab(0, h), to_ab(w, h)]
    amin, amax = int(math.floor(min(p[0] for p in corners))) - 1, int(math.ceil(max(p[0] for p in corners))) + 1
    bmin, bmax = int(math.floor(min(p[1] for p in corners))) - 1, int(math.ceil(max(p[1] for p in corners))) + 1
    cells = {}
    for a in range(amin, amax + 1):
        for b in range(bmin, bmax + 1):
            x, y = cx + a * u[0] + b * v[0], cy + a * u[1] + b * v[1]
            if -size <= x <= w + size and -size <= y <= h + size:
                if "pointy" == orientation:
                    c, r = a + (b // 2), b
                else:
                    c, r = a, b + (a // 2)
                cells[(c, r)] = (x, y, a, b)
    c0 = min(c for c, _ in cells)
    r0 = min(r for _, r in cells)
    return {(c - c0, r - r0): val for (c, r), val in cells.items()}, u, v


def edge_profile_points(size, rot, orientation, offsets, n=16):
    """For each of the six edges and each normal offset d, n points along the edge moved d px along the
    outward normal: the intensity profile across the hexside, averaged along it."""
    start = 30.0 if "pointy" == orientation else 0.0
    out = []
    for k in range(6):
        a0 = math.radians(start + 60 * k + rot)
        a1 = math.radians(start + 60 * (k + 1) + rot)
        mid = math.radians(start + 60 * k + 30 + rot)          # the edge's outward normal
        nx, ny = math.cos(mid), math.sin(mid)
        edge = []
        for d in offsets:
            pts = []
            for t in np.linspace(0.08, 0.92, n):
                x = size * ((1 - t) * math.cos(a0) + t * math.cos(a1)) + d * nx
                y = size * ((1 - t) * math.sin(a0) + t * math.sin(a1)) + d * ny
                pts.append((x, y))
            edge.append(pts)
        out.append(edge)
    return out


def printed_cells(g, cells, size, rot, orientation, gap=4):
    """A cell is printed when a thin line lies on its outline. On the GREY image the intensity profile
    across a hexside, averaged along the hexside, shows a line as a dip or peak at the outline that is
    absent gap px to either side; paper texture, mottle and halftone average out along the edge. The
    score is the median over the six edges of |p(0) - mean(p(-gap), p(+gap))|, allowing the line to sit
    one pixel off; Otsu over all cells separates printed cells from margin."""
    h, w = g.shape
    offsets = [-gap, -1, 0, 1, gap]
    prof = edge_profile_points(size, rot, orientation, offsets)
    def mean_at(x, y, pts):
        xs = np.clip(np.rint([x + px for px, _ in pts]).astype(int), 0, w - 1)
        ys = np.clip(np.rint([y + py for _, py in pts]).astype(int), 0, h - 1)
        return float(g[ys, xs].mean())
    scores = {}
    for key, (x, y, _, _) in cells.items():
        per_edge = []
        for edge in prof:
            p = [mean_at(x, y, pts) for pts in edge]
            base = 0.5 * (p[0] + p[4])
            per_edge.append(max(abs(p[1] - base), abs(p[2] - base), abs(p[3] - base)))
        scores[key] = float(np.median(per_edge))
    pool = np.array(list(scores.values()))
    t = otsu(pool)
    printed = {key for key, v in scores.items() if v > t}
    return printed, t


def otsu(values, bins=128):
    hist, edges = np.histogram(values, bins=bins)
    mids = 0.5 * (edges[:-1] + edges[1:])
    total = hist.sum()
    best, thr = -1.0, float(mids[0])
    w0 = 0.0
    s0 = 0.0
    s_all = float((hist * mids).sum())
    for i in range(bins):
        w0 += hist[i]
        if 0 == w0 or total == w0:
            continue
        s0 += hist[i] * mids[i]
        m0 = s0 / w0
        m1 = (s_all - s0) / (total - w0)
        v = w0 * (total - w0) * (m0 - m1) ** 2
        if v > best:
            best, thr = v, float(edges[i + 1])
    return thr


def ranges(keys, cols, rows):
    """Index-box clip tokens 'cCrR-cCrR' along columns, for the unprinted cells."""
    out = []
    for c in range(cols):
        run = None
        for r in range(rows + 1):
            here = (c, r) in keys and r < rows
            if here and run is None:
                run = r
            if not here and run is not None:
                out.append("c%dr%d-c%dr%d" % (c, run, c, r - 1) if r - 1 > run else "c%dr%d" % (c, run))
                run = None
    return out


# ---------------------------------------------------------------- numbering


def printed_index(key, orientation, offset, parity):
    """The printed grid's (column, row) index of a lattice cell. The lattice index frame pairs cells into
    zigzag columns (pointy) or rows (flat) one way ("odd", the renderer's convention); a sheet may print
    them paired the other way ("even"), which shifts every other row's column (pointy) or every other
    column's row (flat) by one. parity says which rows or columns are shifted, since the index frame's
    origin is arbitrary."""
    c, r = key
    if "odd" == offset:
        return c, r
    if "pointy" == orientation:
        return c + ((r + parity) % 2), r
    return c, r + ((c + parity) % 2)


def number_from_anchors(argv, cells, orientation):
    """--anchor ID C R [ID C R ...]: printed id ID is the cell with index (C, R). Two anchors far apart fix
    the numbering: the id is split into two equal halves of digits; both orders (row-col, col-row), both
    grid offsets and both parities are tried; the one whose steps are +-1 in both axes and fits every
    anchor wins. Anchors must include both parities of the cross axis (rows for pointy, columns for
    flat), or the offset cannot be told (Tarawa 2026-09-19: four odd-row anchors fitted, and every even
    row was one column out). Returns the numbering block or None when no anchors were given."""
    if "--anchor" not in argv:
        return None
    i = argv.index("--anchor") + 1
    anchors = []
    while i + 2 < len(argv) and not argv[i].startswith("--"):
        anchors.append((argv[i], int(argv[i + 1]), int(argv[i + 2])))
        i += 3
    if len(anchors) < 2:
        raise ValueError("--anchor needs at least two ID C R triples")
    digits = len(anchors[0][0])
    if digits % 2 or any(len(t[0]) != digits or not t[0].isdigit() for t in anchors):
        raise ValueError("anchors must be all-digit ids of equal even length: %s" % [t[0] for t in anchors])
    cross = [t[2] % 2 for t in anchors] if "pointy" == orientation else [t[1] % 2 for t in anchors]
    if len(set(cross)) < 2:
        raise ValueError("anchors must include both %s parities to fix the grid offset: %s"
                         % ("row" if "pointy" == orientation else "column", [t[0] for t in anchors]))
    half = digits // 2
    for order in ("row-col", "col-row"):
        def split(t):
            first, second = int(t[0][:half]), int(t[0][half:])
            return (first, second) if "row-col" == order else (second, first)
        for offset, parity in (("odd", 0), ("even", 0), ("even", 1)):
            idx = [printed_index((t[1], t[2]), orientation, offset, parity) for t in anchors]
            rows = [split(t)[0] for t in anchors]
            colsn = [split(t)[1] for t in anchors]
            # the steps come from the first pair of anchors that differ in both axes
            pair = next(((a, b) for a in range(len(idx)) for b in range(a + 1, len(idx))
                         if idx[a][0] != idx[b][0] and idx[a][1] != idx[b][1]), None)
            if pair is None:
                continue
            a, b = pair
            (c1, r1), (c2, r2) = idx[a], idx[b]
            cstep = (colsn[b] - colsn[a]) / (c2 - c1)
            rstep = (rows[b] - rows[a]) / (r2 - r1)
            if abs(abs(cstep) - 1) > 1e-9 or abs(abs(rstep) - 1) > 1e-9:
                continue
            cstep, rstep = int(round(cstep)), int(round(rstep))
            cstart, rstart = colsn[a] - cstep * c1, rows[a] - rstep * r1
            if not all(split(t) == (rstart + rstep * ic[1], cstart + cstep * ic[0]) for t, ic in zip(anchors, idx)):
                continue
            fmt = "{row:0%d}{col:0%d}" % (half, half) if "row-col" == order else "{col:0%d}{row:0%d}" % (half, half)
            return {"id-format": fmt, "col-start": cstart, "row-start": rstart, "col-step": cstep, "row-step": rstep,
                    "half": half, "order": order, "offset": offset, "parity": parity,
                    "anchors": [{"id": t[0], "col": t[1], "row": t[2]} for t in anchors]}
    raise ValueError("no numbering with unit steps fits the anchors %s (two of them must differ in both axes)" % [t[0] for t in anchors])


def cell_label(key, numbering, cols, rows):
    """The printed id when the numbering is known, else the zero-padded index (Ben, A.3): 'c17 r1' is
    written 1701, with as many digits per axis as the larger extent needs."""
    c, r = key
    if numbering:
        pc, pr = printed_index(key, numbering["orientation"], numbering["offset"], numbering["parity"])
        col = numbering["col-start"] + numbering["col-step"] * pc
        row = numbering["row-start"] + numbering["row-step"] * pr
        h = numbering["half"]
        if col < 0 or row < 0:
            return "?"
        return "%0*d%0*d" % ((h, row, h, col) if "row-col" == numbering["order"] else (h, col, h, row))
    d = max(2, len(str(max(cols, rows))))
    return "%0*d%0*d" % (d, c, d, r)


# ---------------------------------------------------------------- main


def main(argv):
    if len(argv) < 2 or "--out" not in argv:
        print(__doc__)
        return 2
    path = argv[1]
    out = option(argv, "--out")
    max_side = int(option(argv, "--max-side", 2400))   # 1600 blurred Downfall's fine grid into its sea texture
    os.makedirs(out, exist_ok=True)
    e, f, rgb, ef, grey = edge_map(path, max_side)
    h, w = e.shape
    a = autocorrelation(e)
    hint = float(option(argv, "--spacing", 0)) or None
    if hint:
        cands, peaks = find_period(e, a, rmin=max(8, int(0.015 * min(h, w))), rmax=min(h, w) // 4, hint_small=hint * f)
        for c in cands:
            c["s_px"], c["cx_px"], c["cy_px"] = c["s"] / f, c["cx"] / f, c["cy"] / f
    else:
        cands = consistent_candidates(rgb)
    for c in cands:
        if not math.isfinite(c["score"]):
            c["score"] = -1.0
    top = max(c["score"] for c in cands)
    # a decorative texture can tie the printed grid on contrast; it is finer than the grid it decorates,
    # so among candidates within a tenth of the best the coarsest wins
    best = max((c for c in cands if c["score"] >= 0.9 * top) if top > 0 else cands, key=lambda c: c["s_px"])
    spacing, ang, ring, harmonic = best["s_px"], best["ang"], best["ring"], best["harmonic"]
    cx, cy, pscore = best["cx_px"], best["cy_px"], best["score"]
    orientation, rot = orientation_of(ang)
    size = spacing / SQRT3
    # from here on everything is in source pixels, on the full-resolution edge map
    f0, f = f, 1.0
    e = ef
    period = spacing
    drift_note = None
    for _ in range(3):
        cx2, cy2, spacing2, ang2, nfit = refine_lattice(e, cx, cy, spacing, ang, size, orientation, reach=max(3, int(0.06 * spacing)))
        if abs(spacing2 - period) > 0.03 * period:
            drift_note = "refinement drifted to %.2f px from the autocorrelation period %.2f: a texture; period kept" % (spacing2, period)
            break
        cx, cy, spacing, ang = cx2, cy2, spacing2, ang2
        size = spacing / SQRT3
    orientation, rot = orientation_of(ang)
    size = spacing / SQRT3
    cells, u, v = index_frame(e, cx, cy, spacing, ang, size, orientation)
    ideal_xy = {k: (v[0], v[1]) for k, v in cells.items()}
    ideal_ab = {k: (v[2], v[3]) for k, v in cells.items()}
    placed, worst_shift = grow(e, cells, size, rot, orientation, spacing, reach=max(4, int(0.1 * spacing)))
    # the printed test runs on the grown positions first, so that only the map's own cells shape the
    # refit and its report; it runs again on the final positions, which is what lattice.json records
    grown_cells = {k: (placed[k][0], placed[k][1], a, b) for k, (x, y, a, b) in cells.items()}
    printed, thr = printed_cells(grey, grown_cells, size, rot, orientation)
    placed, lattice_fit, refit_report = refit(e, placed, ideal_ab, size, rot, orientation, spacing, reach=max(4, int(0.1 * spacing)),
                                              printed=printed)
    confidence = {k: v[2] for k, v in placed.items()}
    cells = {k: (placed[k][0], placed[k][1], a, b) for k, (x, y, a, b) in cells.items()}
    printed, thr = printed_cells(grey, cells, size, rot, orientation)
    numbering = number_from_anchors(argv, cells, orientation)
    if numbering:
        numbering["orientation"] = orientation
    cols = 1 + max(c for c, _ in cells)
    rows = 1 + max(r for _, r in cells)
    unprinted = {k for k in cells if k not in printed}
    origin = cells[(0, 0)][:2] if (0, 0) in cells else (cx, cy)
    notes = []
    if drift_note:
        notes.append(drift_note)
    if best.get("hinted"):
        notes.append("lattice taken from the --spacing hint")
    if ring < GATE_RING:
        notes.append("only %d of the six nearest autocorrelation peaks found" % ring)
    if pscore < GATE_PHASE:
        notes.append("phase score %.3f under the gate %.2f" % (pscore, GATE_PHASE))
    if worst_shift > 0.1 * spacing:
        notes.append("growth moved cells up to %.1f px from the rigid lattice: a folded or warped scan; cells use the grown positions" % worst_shift)
    if abs(rot) > 0.5:
        notes.append("lattice rotated %.2f degrees: the scan is rotated; cells must use the basis, not the grid element" % rot)
    result = {
        "source": os.path.abspath(path), "size_px": [int(e.shape[1]), int(e.shape[0])],
        "orientation": orientation, "size": size / f, "ox": origin[0] / f, "oy": origin[1] / f,
        "rotation_deg": rot, "spacing": spacing / f,
        "basis": {"u": [u[0] / f, u[1] / f], "w": [v[0] / f, v[1] / f]},
        "index": {"cols": [0, cols - 1], "rows": [0, rows - 1], "offset": "odd"},
        "printed": {"count": len(printed), "threshold": thr,
                    "cells": sorted("c%dr%d" % k for k in printed)},
        "clip_index": ranges(unprinted, cols, rows),
        "refit": {"lattice": lattice_fit, "rounds": refit_report, "cells": "printed"},
        "grown": {"worst_px": worst_shift,
                  "cells": {"c%dr%d" % k: [round(v[0], 1), round(v[1], 1), round(confidence[k], 2)] for k, v in cells.items()}},
        "numbering": numbering,
        "method": {"candidates": [{"spacing_px": round(c["s_px"], 2), "angle": round(c["ang"], 2), "phase_score": round(c["score"], 2),
                                   "orientation": orientation_of(c["ang"])[0]} for c in cands],
                   "ring": ring, "harmonic": harmonic, "phase_score": pscore, "fitted_cells": nfit, "piecewise": False},
        "passed": ring >= GATE_RING and pscore >= GATE_PHASE,
        "notes": notes,
    }
    with open(os.path.join(out, "lattice.json"), "w", encoding="utf-8") as fh:
        json.dump(result, fh, indent=1)
    print("%s: %s spacing %.3f px size %.3f rot %.2f deg ring %d/6 harmonic %d cover %.2f fit %d grown %.1fpx refit(printed) %s cells %d printed %d  %s" % (
        os.path.basename(path), orientation, spacing / f, size / f, rot, ring, harmonic, pscore, nfit, worst_shift,
        " ".join("r%d:rms%.1f/slips%d" % (t["round"], t["rms_px"], t["slips"]) for t in refit_report), len(cells), len(printed),
        ("PASSED" if result["passed"] else "FAILED " + "; ".join(notes))
        + ("  [period from --spacing %.1f, not found by the search]" % hint if hint else "")))
    if "--overlay" in argv:
        overlay(rgb, cells, ideal_xy, printed, size, rot, orientation, os.path.join(out, "overlay.jpg"))
    if "--svg" in argv:
        overlay_svg(path, rgb.size, cells, ideal_xy, printed, size, rot, orientation, os.path.join(out, "overlay.svg"), numbering,
                    lattice_fit, ideal_ab)
    return 0


def overlay_svg(image_path, wh, cells, ideal, printed, size, rot, orientation, path, numbering=None, fit=None, ideal_ab=None):
    """A zoomable overlay: the scan embedded at full resolution, the grown lattice drawn as a perfect grid
    deformed smoothly (shared vertices, so cells tile), each cell labelled with its printed id when the
    numbering is known and otherwise its zero-padded index."""
    import base64
    W, H = wh
    import io
    buf = io.BytesIO()
    Image.open(image_path).convert("RGB").save(buf, format="JPEG", quality=85)   # a PNG scan would make the SVG several times larger
    data = base64.b64encode(buf.getvalue()).decode("ascii")
    mime = "image/jpeg"
    polys = shared_vertices(cells, ideal, size, rot, orientation)
    cols = 1 + max(c for c, _ in cells)
    rows = 1 + max(r for _, r in cells)
    font = max(6, int(size * 0.28))
    parts = ['<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="%d" height="%d" viewBox="0 0 %d %d">' % (W, H, W, H),
             '<image href="data:%s;base64,%s" width="%d" height="%d"/>' % (mime, data, W, H)]
    for key, val in cells.items():
        x, y = val[0], val[1]
        poly = " ".join("%.1f,%.1f" % pt for pt in polys[key])
        colour = "#eb00aa" if key in printed else "#ff7800"
        parts.append('<polygon points="%s" fill="none" stroke="%s" stroke-width="1" stroke-opacity="0.8"/>' % (poly, colour))
        parts.append('<text x="%.1f" y="%.1f" font-size="%d" font-family="Arial" fill="%s" text-anchor="middle" opacity="0.85">%s</text>' % (
            x, y + 0.45 * size, font, colour, cell_label(key, numbering, cols, rows)))
    if fit and ideal_ab:
        corner = hex_outline_points(size, rot, orientation, n=1)
        for key in cells:
            a_, b_ = ideal_ab[key]
            fx = fit["ox"] + a_ * fit["u"][0] + b_ * fit["w"][0]
            fy = fit["oy"] + a_ * fit["u"][1] + b_ * fit["w"][1]
            poly = " ".join("%.1f,%.1f" % (fx + px, fy + py) for px, py in corner)
            parts.append('<polygon points="%s" fill="none" stroke="#1f6fd8" stroke-width="0.6" stroke-opacity="0.6"/>' % poly)
    parts.append("</svg>")
    with open(path, "w", encoding="utf-8") as fh:
        fh.write(chr(10).join(parts))
    print("wrote", path)


def overlay(rgb, cells, ideal, printed, size, rot, orientation, path):
    """The reduced JPEG overlay for a quick look: the grown grid with shared vertices, printed cells in
    magenta, others in orange, under 300 KB."""
    img = rgb.copy()
    d = ImageDraw.Draw(img, "RGBA")
    polys = shared_vertices(cells, ideal, size, rot, orientation)
    for key in cells:
        poly = polys[key]
        colour = (235, 0, 170, 200) if key in printed else (255, 120, 0, 200)
        d.line(poly + [poly[0]], fill=colour, width=1)
    W, H = img.size
    sc = min(1.0, 1400 / max(W, H))
    if sc < 1.0:
        img = img.resize((int(W * sc), int(H * sc)), Image.LANCZOS)
    q = 70
    while True:
        img.save(path, quality=q)
        if os.path.getsize(path) < 300_000 or q <= 30:
            break
        q -= 10
    print("wrote", path)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
