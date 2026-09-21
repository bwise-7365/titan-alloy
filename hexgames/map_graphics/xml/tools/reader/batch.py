# Copyright Ben Paul Wise. All Rights Reserved.
"""batch.py -- run lattice.py over every scan in a folder and write the batch table.

    python batch.py "example maps" [--work work] [--hint "Dai Senso.png=77.2" ...] [--only NAME ...]
                    [--anchors anchors.json]

For each image (png, jpg, jpeg) it runs lattice.py with --overlay --svg into work/<map>/ (the map's
name with spaces and punctuation as underscores), passing --spacing for any image named in a --hint and --anchor for any image with anchors in the
--anchors file (default anchors.json beside this script: {"maps": [{"file", "anchors": [[id, c, r], ...], "printed": [id, ...] (hexes the printed test missed, by eye),
"note", "skip"}]}); an entry with "skip": true is not a map scan (a detail photograph) and is left out.
It appends the tool's one report line to work/batch-report.txt, prefixed with the run time, and
writes work/batch-table.json: one row per map, [name, orientation, spacing, r0 rms, r0 slips, last rms,
last slips, printed, cells, PASS|FAIL, hinted]. --only limits the run to images whose file name
contains one of the given strings.
"""

import json
import os
import re
import subprocess
import sys
import time

IMAGE_EXT = (".png", ".jpg", ".jpeg")


def work_name(filename):
    stem = os.path.splitext(filename)[0]
    return re.sub(r"[^A-Za-z0-9]+", "_", stem).strip("_")


def parse_hints(argv):
    hints = {}
    for i, a in enumerate(argv):
        if "--hint" == a:
            name, px = argv[i + 1].rsplit("=", 1)
            hints[name] = float(px)
    return hints


def parse_only(argv):
    only = []
    take = False
    for a in argv[2:]:
        if a.startswith("--"):
            take = "--only" == a
            continue
        if take:
            only.append(a)
    return only


def table_row(name, line, hinted):
    m = re.search(r": (\w+) spacing ([\d.]+) px .* refit\(printed\) (.*?) cells (\d+) printed (\d+)  (PASSED|FAILED)", line)
    if not m:
        return [name, "?", 0.0, 0.0, 0, 0.0, 0, 0, 0, "ERROR", hinted]
    rounds = re.findall(r"r\d+:rms([\d.]+)/slips(\d+)", m.group(3))
    first, last = rounds[0], rounds[-1]
    return [name, m.group(1), round(float(m.group(2)), 1), float(first[0]), int(first[1]), float(last[0]), int(last[1]),
            int(m.group(5)), int(m.group(4)), "PASS" if "PASSED" == m.group(6) else "FAIL", hinted]


def main(argv):
    folder = argv[1]
    work = os.path.join(os.path.dirname(os.path.abspath(__file__)), "work")
    if "--work" in argv:
        work = argv[argv.index("--work") + 1]
    hints = parse_hints(argv)
    anchors_path = argv[argv.index("--anchors") + 1] if "--anchors" in argv else os.path.join(os.path.dirname(os.path.abspath(__file__)), "anchors.json")
    anchors, skip = {}, set()
    if os.path.exists(anchors_path):
        with open(anchors_path, encoding="utf-8") as fh:
            maps = json.load(fh)["maps"]
            anchors = {m["file"]: m["anchors"] for m in maps if m["anchors"]}
            printed_extra = {m["file"]: m["printed"] for m in maps if m.get("printed")}
            skip = {m["file"] for m in maps if m.get("skip")}
    only = parse_only(argv)
    here = os.path.dirname(os.path.abspath(__file__))
    os.makedirs(work, exist_ok=True)
    images = sorted(f for f in os.listdir(folder) if f.lower().endswith(IMAGE_EXT) and f not in skip)
    if only:
        images = [f for f in images if any(s in f for s in only)]
    rows = []
    with open(os.path.join(work, "batch-report.txt"), "a", encoding="utf-8") as report:
        report.write("=== %s  %d maps\n" % (time.strftime("%Y-%m-%d %H:%M"), len(images)))
        for f in images:
            name = work_name(f)
            cmd = [sys.executable, os.path.join(here, "lattice.py"), os.path.join(folder, f),
                   "--out", os.path.join(work, name), "--overlay", "--svg"]
            hinted = f in hints
            if hinted:
                cmd += ["--spacing", str(hints[f])]
            if f in anchors:
                cmd += ["--anchor"] + [str(v) for a in anchors[f] for v in a]
            if f in printed_extra:
                cmd += ["--printed"] + list(printed_extra[f])
            t0 = time.time()
            proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
            line = (proc.stdout.strip().splitlines() or [proc.stderr.strip().splitlines()[-1] if proc.stderr.strip() else "no output"])
            line = next((l for l in line if " spacing " in l), line[-1])
            entry = "%3ds %s" % (time.time() - t0, line)
            print(entry, flush=True)
            report.write(entry + "\n")
            report.flush()
            rows.append(table_row(name, line, hinted))
    table_path = os.path.join(work, "batch-table.json")
    if only and os.path.exists(table_path):
        # a partial run replaces its own rows and keeps the rest of the last full table
        done = {r[0] for r in rows}
        with open(table_path, encoding="utf-8") as fh:
            rows = [r for r in json.load(fh) if r[0] not in done] + rows
        rows.sort(key=lambda r: r[0].lower())
    with open(table_path, "w", encoding="utf-8") as fh:
        json.dump(rows, fh, indent=0)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
