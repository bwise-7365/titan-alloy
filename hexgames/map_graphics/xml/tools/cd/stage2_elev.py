# Copyright Ben Paul Wise. All Rights Reserved.
"""stage2_elev.py -- sample ETOPO1 elevations for every land hex (opentopodata.org, 100 points a call,
one call a second).  Resumable: elev_hex.json holds, per hex id, the list of 19 sample elevations
(centre, a ring of 6 at 0.45 R, a ring of 12 at 0.8 R).  Sea samples in a coastal hex are kept as
they are (ETOPO1 gives depths as negative numbers)."""
import json
import math
import os
import sys
import time
import urllib.parse
import urllib.request

sys.path.insert(0, os.path.dirname(__file__))
import lattice as L  # noqa: E402

OUT = os.path.dirname(os.path.abspath(__file__))
URL = "https://api.opentopodata.org/v1/etopo1?locations="


def sample_points(c, r):
    cx, cy = L.centre(c, r)
    pts = [(cx, cy)]
    for k in range(6):
        a = math.radians(60 * k)
        pts.append((cx + 0.45 * L.SIZE * math.cos(a), cy + 0.45 * L.SIZE * math.sin(a)))
    for k in range(12):
        a = math.radians(30 * k)
        pts.append((cx + 0.8 * L.SIZE * math.cos(a), cy + 0.8 * L.SIZE * math.sin(a)))
    return pts


def fetch(locs):
    q = "|".join("%.4f,%.4f" % (lat, lon) for lat, lon in locs)
    for attempt in range(6):
        try:
            with urllib.request.urlopen(URL + urllib.parse.quote(q, safe=",|"), timeout=60) as resp:
                data = json.loads(resp.read().decode("utf-8"))
            if data.get("status") == "OK":
                return [res["elevation"] for res in data["results"]]
            print("API status", data.get("status"), data.get("error"))
        except Exception as e:  # noqa: BLE001
            print("retry", attempt, e)
        time.sleep(3 + 3 * attempt)
    raise RuntimeError("elevation API failed")


def main():
    geo = json.load(open(os.path.join(OUT, L.cache("geo_hex.json"))))
    path = os.path.join(OUT, L.cache("elev_hex.json"))
    elev = json.load(open(path)) if os.path.exists(path) else {}
    todo = [h for h, v in geo.items() if v["land"] >= 0.25 and h not in elev]
    print("hexes to sample:", len(todo))
    batch, batch_hex = [], []
    for i, h in enumerate(todo):
        c, r = L.parse_id(h)
        pts = [L.px_to_ll(*p) for p in sample_points(c, r)]
        batch.extend(pts)
        batch_hex.append(h)
        if len(batch) + 19 > 100 or i == len(todo) - 1:
            vals = fetch(batch)
            for k, hh in enumerate(batch_hex):
                elev[hh] = [None if v is None else round(v) for v in vals[19 * k:19 * k + 19]]
            json.dump(elev, open(path, "w"))
            print("sampled %d / %d" % (len(elev), len(geo)), flush=True)
            batch, batch_hex = [], []
            time.sleep(1.1)
    print("done")


if __name__ == "__main__":
    main()
# Copyright Ben Paul Wise. All Rights Reserved.
