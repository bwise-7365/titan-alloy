import glob
import os
import sys

from lxml import etree

d = r"C:\repos\ghub-per\titan-alloy\hexgames\game_rules\xml"
schema = etree.XMLSchema(etree.parse(os.path.join(d, "hexrules.xsd")))
print("schema OK (libxml2 %s)" % ".".join(map(str, etree.LIBXML_VERSION)))

ok = True
for f in sorted(glob.glob(os.path.join(d, "*.xml"))):
    doc = etree.parse(f)
    name = os.path.basename(f)
    if schema.validate(doc):
        print("OK  ", name)
    else:
        ok = False
        print("FAIL", name)
        for e in schema.error_log:
            print("    line %d: %s" % (e.line, e.message))
    # XSD 1.0 cannot count cells per row; check it here.
    for t in doc.iter("table"):
        ncol = len(t.findall("col"))
        for r in t.findall("row"):
            n = len(r.findall("cell"))
            if n != ncol:
                ok = False
                print("    table %s row %r: %d cells, %d columns" % (t.get("id"), r.get("label"), n, ncol))
    # Summary counts.
    print("      rules %d, notes %d, lists %d, tables %d, phases %d" % (
        len(list(doc.iter("rule"))), len(list(doc.iter("note"))),
        len(list(doc.iter("list"))), len(list(doc.iter("table"))),
        len(list(doc.iter("phase")))))

sys.exit(0 if ok else 1)
