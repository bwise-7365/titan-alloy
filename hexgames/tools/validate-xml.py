# Copyright Ben Paul Wise. All Rights Reserved.
"""validate-xml.py DIR [DIR...] -- validate every *.xml in the directories against the schema it names.

Each instance names its schema with xsi:noNamespaceSchemaLocation, resolved relative to the instance
file.  Uses lxml (libxml2), the same engine as XML Copy Editor.  Also applies the checks XSD 1.0
cannot express: hexrules table rows have one cell per column.  Exit status 1 on any failure.
"""
import glob
import os
import sys

from lxml import etree

XSI = "{http://www.w3.org/2001/XMLSchema-instance}noNamespaceSchemaLocation"


def extra_checks(doc):
    problems = []
    for t in doc.iter("table"):
        ncol = len(t.findall("col"))
        for r in t.findall("row"):
            n = len(r.findall("cell"))
            if 0 < ncol and n != ncol:
                problems.append("table %s row %r: %d cells, %d columns" % (t.get("id"), r.get("label"), n, ncol))
    return problems


def main(dirs):
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
