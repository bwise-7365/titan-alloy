# Copyright Ben Paul Wise. All Rights Reserved.
"""rectify.py -- warp a photographed map into a flat raster the other stages can use.

    python rectify.py SOURCE OUT.png --quad x0 y0 x1 y1 x2 y2 x3 y3 --size W H [--margin N]
                      [--check ID x y ...]

--margin keeps N output pixels of whatever lies OUTSIDE the four corners (the printed border, the table
top, a hand). Without it the rectangle fills the output exactly and everything beyond it is cut, so a
corner read even slightly inside the true one loses that edge. Read the corners generously and pass a
margin; trimming afterwards is free, recovering a cut edge is not.

A photograph of a flat sheet is a projective view of it, so ONE homography, fitted to four points whose
true shape is known, removes the keystone. The four --quad points are the corners of a printed rectangle
in the photograph, in the order top-left, top-right, bottom-right, bottom-left, read off a ruler crop
(crop.py --ruler --scale 1). The map's own printed border is the obvious rectangle; a track or chart box
works too. --size is the rectangle's true shape in output pixels: take the aspect ratio from a flatbed
scan of the same map, or from the sheet's real inches, and the size from how large you want the result.

The output frame IS that rectangle, so the stages that follow treat the result as an ordinary source:
calibrate it as usual (its control points are new, the grid facts are not). Paper curl and lens
distortion are left as a small residual, which the per-point calibration then absorbs; --check takes
extra printed points and prints how far each lands from where the homography says it should, which is
the honest measure of how flat the sheet was.
"""
import sys

import numpy as np
from PIL import Image

Image.MAX_IMAGE_PIXELS = None


def homography(src, dst):
    """The 3x3 H with dst ~ H src, from four or more point pairs, by least squares (DLT)."""
    rows = []
    for (sx, sy), (dx, dy) in zip(src, dst):
        rows.append([sx, sy, 1, 0, 0, 0, -dx * sx, -dx * sy, -dx])
        rows.append([0, 0, 0, sx, sy, 1, -dy * sx, -dy * sy, -dy])
    _, _, vt = np.linalg.svd(np.array(rows, dtype=float))
    return vt[-1].reshape(3, 3) / vt[-1][-1]


def apply_h(h, pt):
    v = h @ np.array([pt[0], pt[1], 1.0])
    return (v[0] / v[2], v[1] / v[2])


def pairs(argv, flag, per):
    """The numbers after flag, in groups of per."""
    if flag not in argv:
        return []
    out, i = [], argv.index(flag) + 1
    while i < len(argv) and not argv[i].startswith("--"):
        out.append(argv[i])
        i += 1
    if len(out) % per:
        raise ValueError("%s takes groups of %d values, got %d" % (flag, per, len(out)))
    return [out[k:k + per] for k in range(0, len(out), per)]


def main(argv):
    if len(argv) < 3 or "--quad" not in argv or "--size" not in argv:
        print(__doc__)
        return 2
    source, out = argv[1], argv[2]
    quad = [(float(x), float(y)) for x, y in pairs(argv, "--quad", 2)]
    if 4 != len(quad):
        raise ValueError("--quad takes exactly four corners: top-left, top-right, bottom-right, bottom-left")
    w, h = (int(v) for v in pairs(argv, "--size", 2)[0])
    m = float(pairs(argv, "--margin", 1)[0][0]) if "--margin" in argv else 0.0
    corners = [(m, m), (w - m, m), (w - m, h - m), (m, h - m)]

    forward = homography(quad, corners)          # photograph -> flat
    backward = homography(corners, quad)         # flat -> photograph, what Image.transform wants
    for (px, py), want in zip(quad, corners):
        got = apply_h(forward, (px, py))
        print("corner (%.0f, %.0f) -> (%.1f, %.1f), wanted (%.0f, %.0f), off %.2f px" % (
            px, py, got[0], got[1], want[0], want[1], np.hypot(got[0] - want[0], got[1] - want[1])))

    img = Image.open(source).convert("RGB")
    coeffs = (backward / backward[2, 2]).reshape(9)[:8]
    flat = img.transform((w, h), Image.PERSPECTIVE, tuple(coeffs), Image.BICUBIC)
    flat.save(out)
    print("%s: %d x %d -> %s: %d x %d" % (source, img.width, img.height, out, w, h))

    checks = pairs(argv, "--check", 3)
    if checks:
        print("check points (how flat the sheet was, in output pixels):")
        for name, sx, sy in checks:
            x, y = apply_h(forward, (float(sx), float(sy)))
            print("  %s: photograph (%s, %s) lands at (%.1f, %.1f)" % (name, sx, sy, x, y))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
