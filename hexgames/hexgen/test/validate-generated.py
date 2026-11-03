# Copyright Ben Paul Wise. All Rights Reserved.
"""validate-generated.py HEXGEN_CLI HEXRULES_DIR OUT_DIR -- generated sheets validate against hexsheet.xsd.

Runs hexgen_cli on HexRules' corpus settings and on three variants of them (pointy, even offset with
altitude, fractal with roads on every edge), writing each sheet and its SVG into OUT_DIR, then
validates every sheet with HexRules' validate.py. Exit status 0 only if every step succeeds.
"""
import os
import subprocess
import sys


def main(cli, hexrules, out):
    os.makedirs(out, exist_ok=True)
    base = os.path.join(hexrules, "corpus", "valid", "generator-hexkrieg.xml")
    text = open(base, encoding="utf-8").read()
    variants = {
        "hexkrieg": text,
        "pointy": text.replace('orientation="flat"', 'orientation="pointy"'),
        "even-alt": text.replace('offset="odd"', 'offset="even"').replace(
            "</generator>", '  <altitude max-land="2000" max-depth="200"/>\n</generator>'),
        "fractal-edges": text.replace('method="perlin"', 'method="fractal"').replace(
            '<side at="left"', '<side at="top" roads="3" cities="1"/>\n    <side at="bottom" roads="3" cities="1"/>\n    <side at="left"'),
    }
    sheets = []
    for name, settings in variants.items():
        if name != "hexkrieg" and settings == text:
            print("validate-generated: variant %s changed nothing" % name)
            return 1
        path = os.path.join(out, "settings-%s.xml" % name)
        open(path, "w", encoding="utf-8", newline="\n").write(settings)
        sheet = os.path.join(out, "sheet-%s.xml" % name)
        r = subprocess.run([cli, path, sheet, "--svg", os.path.join(out, "sheet-%s.svg" % name)])
        if r.returncode != 0:
            print("validate-generated: hexgen_cli failed on %s" % name)
            return 1
        sheets.append(sheet)
    validate = os.path.join(hexrules, "python", "validate.py")
    return subprocess.run([sys.executable, validate] + sheets).returncode


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:4]))
# Copyright Ben Paul Wise. All Rights Reserved.
