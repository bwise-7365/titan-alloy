# Copyright Ben Paul Wise. All Rights Reserved.
"""validate-xml.py DIR [DIR...] -- validate every *.xml in the directories against the schema it names.

Each instance names its schema with xsi:noNamespaceSchemaLocation, resolved relative to the instance
file.  Uses lxml (libxml2), the same engine as XML Copy Editor.  Also applies the checks XSD 1.0
cannot express: hexrules table rows have one cell per column; hexsheet junctions (explicit sheets
give every link and path an id; a junction names two or more chains of one kind that pass through
its hex; a chain is in one junction per hex; ends="junction" has its junction; implicit sheets have
no junction elements).  Exit status 1 on any failure.
"""
import glob
import os
import sys

from lxml import etree

XSI = "{http://www.w3.org/2001/XMLSchema-instance}noNamespaceSchemaLocation"


def sheet_checks(root):
    """hexsheet.xsd Junction rules (doc/2026-09-23-network-junctions-proposal.md, section 3)."""
    problems = []
    mode = root.get("junctions")
    links = {l.get("id"): l for l in root.findall("link") if l.get("id")}
    paths = {p.get("id"): p for p in root.findall("path") if p.get("id")}
    junctions = root.findall("junction")
    if mode == "implicit":
        if junctions:
            problems.append("line %d: junctions=\"implicit\" but the sheet has junction elements" % junctions[0].sourceline)
        return problems
    for tag, table in (("link", links), ("path", paths)):
        for el in root.findall(tag):
            if not el.get("id"):
                problems.append("line %d: %s without id on an explicit sheet" % (el.sourceline, tag))
    seen = {}  # (at, chain id) -> line of the junction holding it
    joined = set()  # (at, chain id) for the ends check
    for j in junctions:
        at = j.get("at")
        members = (j.get("links") or "").split()
        table = links
        if j.get("paths"):
            if members:
                problems.append("line %d: junction at %s names both links and paths" % (j.sourceline, at))
            members = j.get("paths").split()
            table = paths
        if len(set(members)) < 2:
            problems.append("line %d: junction at %s needs two distinct members" % (j.sourceline, at))
        kinds = set()
        for m in members:
            el = table.get(m)
            if el is None:
                problems.append("line %d: junction at %s names %r, not a %s id" % (j.sourceline, at, m, "path" if table is paths else "link"))
                continue
            kinds.add(el.get("kind"))
            if table is links and at not in el.get("hexes").split():
                problems.append("line %d: link %s does not pass through junction hex %s" % (j.sourceline, m, at))
            # a path junction sits at a vertex HEX:CORNER; whether the path's hexsides meet there needs the
            # lattice (hexcoord); the loader will check it, this script does not.
            if (at, m) in seen:
                problems.append("line %d: %s is in two junctions at %s (also line %d)" % (j.sourceline, m, at, seen[(at, m)]))
            seen[(at, m)] = j.sourceline
            joined.add((at, m))
        if 1 < len(kinds):
            problems.append("line %d: junction at %s mixes kinds %s" % (j.sourceline, at, sorted(kinds)))
    for lid, l in links.items():
        ends = (l.get("ends") or "").split()
        hexes = l.get("hexes").split()
        for reason, hx in zip(ends, (hexes[0], hexes[-1])):
            if reason == "junction" and (hx, lid) not in joined:
                problems.append("line %d: link %s ends \"junction\" at %s but no junction there holds it" % (l.sourceline, lid, hx))
    return problems


def extra_checks(doc):
    problems = []
    if doc.getroot().tag == "sheet":
        problems.extend(sheet_checks(doc.getroot()))
    for t in doc.iter("table"):
        ncol = len(t.findall("col"))
        for r in t.findall("row"):
            n = len(r.findall("cell"))
            if 0 < ncol and n != ncol:
                problems.append("table %s row %r: %d cells, %d columns" % (t.get("id"), r.get("label"), n, ncol))
    return problems


def main(dirs):
    # ctest takes our output through a pipe, and Windows then encodes it as the console code page: a
    # validation message quoting a sheet's Cyrillic place name would throw UnicodeEncodeError.
    sys.stdout.reconfigure(encoding="utf-8")
    schemas = {}
    ok = True
    total = 0
    for d in dirs:
        for f in sorted(glob.glob(os.path.join(d, "*.xml"))):
            total += 1
            doc = etree.parse(f)
            loc = doc.getroot().get(XSI)
            if loc is None:
                print("FAIL", f, "no xsi:noNamespaceSchemaLocation")
                ok = False
                continue
            xsd = os.path.normpath(os.path.join(os.path.dirname(f), loc))
            if xsd not in schemas:
                schemas[xsd] = etree.XMLSchema(etree.parse(xsd))
            problems = []
            if not schemas[xsd].validate(doc):
                problems.extend("line %d: %s" % (e.line, e.message) for e in schemas[xsd].error_log)
            problems.extend(extra_checks(doc))
            if problems:
                ok = False
                print("FAIL", os.path.relpath(f))
                for p in problems:
                    print("    ", p)
            else:
                print("OK  ", os.path.relpath(f))
    print("validate-xml: %d files, %s" % (total, "all valid" if ok else "FAILURES"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:] or ["."]))
# Copyright Ben Paul Wise. All Rights Reserved.
