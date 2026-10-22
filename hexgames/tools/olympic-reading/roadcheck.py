# Copyright Ben Paul Wise. All Rights Reserved.
import heapq, math
from cells import *
from roadchains import ROADS
from classify import T


def segdist(p, a, b):
    (x, y), (x0, y0), (x1, y1) = xy(p), xy(a), xy(b)
    dx, dy = x1 - x0, y1 - y0
    t = max(0, min(1, ((x - x0) * dx + (y - y0) * dy) / max(dx * dx + dy * dy, 1e-9)))
    return math.hypot(x - x0 - t * dx, y - y0 - t * dy)


def join(a, b):
    dist = {a: (0, 0.0)}; prev = {}; pq = [(0, 0.0, a)]
    while pq:
        k, c, p = heapq.heappop(pq)
        if p == b:
            break
        for d in NB_SHIFT:
            n = nb(p, d)
            if not n:
                continue
            cand = (k + 1, c + segdist(n, a, b))
            if n not in dist or cand < dist[n]:
                dist[n] = cand; prev[n] = p; heapq.heappush(pq, (cand[0], cand[1], n))
    path = [b]
    while path[-1] != a:
        path.append(prev[path[-1]])
    return path[::-1]


def route(ch):
    h = ch.split(); out = [h[0]]
    for a, b in zip(h, h[1:]):
        out += join(a, b)[1:]
    return out


CHAINS = []
bad = []
for name, w in ROADS:
    for h in w.split():
        if h not in CELLS:
            bad.append((name, "no cell", h))
    if any(h not in CELLS for h in w.split()):
        continue
    hs = route(w)
    extra = len(hs) - len(w.split())
    if extra > 3:
        bad.append((name, "waypoints far apart: +%d hexes" % extra))
    CHAINS.append((name, hs))
if __name__ == "__main__":
    for b in bad:
        print(b)
    print(len(CHAINS), "roads,", sum(len(h) - 1 for _, h in CHAINS), "steps")
# Copyright Ben Paul Wise. All Rights Reserved.
