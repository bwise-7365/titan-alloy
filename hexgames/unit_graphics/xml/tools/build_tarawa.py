# Copyright Ben Paul Wise. All Rights Reserved.
"""build_tarawa.py -- D-Day at Tarawa: every US unit from the USMC OOB chart, Japanese unit types from the
rules samples, depth markers and the marker set.  Backs: US units flip to a reduced face (fewer pips,
lower strength, the weapons still held); Japanese units flip to a concealed face; depth markers to a type label."""
import re

OUT = r"C:\repos\ghub-per\titan-alloy\hexgames\unit_graphics\xml\d-day-at-tarawa.xml"
SHAPE = {"d": "diamond", "t": "triangle", "c": "circle"}

# (designation, type, turn, beach, target shape)   type: inf, hvy, eng (DE FT), sp (shore party), tank, art, hq
US = [
    ("I/3/2", "inf", "1", "R1", "d"), ("K/3/2", "inf", "1", "R1", "t"), ("3/A/1/18", "eng", "1", "R1", "c"), ("3/D/2/18", "sp", "1", "R1", "d"),
    ("E/2/2", "inf", "1", "R2", "d"), ("F/2/2", "inf", "1", "R2", "c"), ("2/A/1/18", "eng", "1", "R2", "t"), ("2/D/2/18", "sp", "1", "R2", "c"),
    ("E/2/8", "inf", "1", "R3", "d"), ("F/2/8", "inf", "1", "R3", "c"), ("2/C/1/18", "eng", "1", "R3", "d"), ("2/F/2/18", "sp", "1", "R3", "t"),
    ("L/3/2", "inf", "2", "R1", "c"), ("M/3/2", "hvy", "2", "R1", "t"), ("G/2/2", "inf", "2", "R2", "t"), ("H/2/2", "hvy", "2", "R2", "d"),
    ("G/2/8", "inf", "2", "R3", "t"), ("H/2/8", "hvy", "2", "R3", "d"),
    ("Hall 8/2", "hq", "3", "R2", None), ("Shoup 2/2", "hq", "3", "R2", None),
    ("3/A/2T", "tank", "4", "R1", "c"), ("2/A/2T", "tank", "4", "R2", "t"), ("2/C/2T", "tank", "4", "R3", "d"),
    ("A/1/2", "inf", "6", "R1", "d"), ("B/1/2", "inf", "6", "R1", "t"), ("C/1/2", "inf", "6", "R1", "c"), ("D/1/2", "hvy", "6", "R1", "c"),
    ("I/3/8", "inf", "6", "R3", "d"), ("K/3/8", "inf", "6", "R3", "t"), ("L/3/8", "inf", "6", "R3", "c"), ("M/3/8", "hvy", "6", "R3", "t"),
    ("1/A/2T", "tank", "6", "R1", "d"), ("1/A/1/18", "eng", "6", "R1", "d"), ("1/D/2/18", "sp", "6", "R1", "c"),
    ("3/C/2T", "tank", "6", "R3", "t"), ("3/C/1/18", "eng", "6", "R3", "t"), ("3/F/2/18", "sp", "6", "R3", "c"),
    ("A/1/8", "inf", "17", "R2", "d"), ("B/1/8", "inf", "17", "R2", "t"), ("C/1/8", "inf", "17", "R2", "c"), ("D/1/8", "hvy", "17", "R2", "c"),
    ("1/C/2T", "tank", "17", "R2", "c"), ("1/C/1/18", "eng", "17", "R2", "t"), ("1/F/2/18", "sp", "17", "R2", "d"),
    ("A/1/6", "inf", "28", "G", "d"), ("B/1/6", "inf", "28", "G", "t"), ("C/1/6", "inf", "28", "G", "c"), ("D/1/6", "hvy", "28", "G", "c"),
    ("1/B/2T", "tank", "28", "G", "d"), ("Holmes 6/2", "hq", "28", "G", None),
    ("A/1/10", "art", "E", "R2", "d"), ("B/1/10", "art", "E", "R2", "t"), ("C/1/10", "art", "E", "R2", "c"),
]
# type -> (icon, modifiers, steps, front value, front weapons, back steps, back value, back weapons)
TYPE = {
    "inf": ("infantry", "", 4, "7-2", "", 3, "5-2", "BR|RD"),
    "hvy": ("heavy-infantry", "", 4, "8-3", "MG", 3, "6-3", "MG|MO"),
    "eng": ("engineer", "", 1, "2", "DE|FT", 1, "2", "DE"),
    "sp": ("engineer", "", 1, "2", "SP", 1, "2", "SP"),
    "tank": ("tank", "", 2, "4-7", "", 1, "2-7", ""),
    "art": ("artillery", "", 2, "4-U", "", 1, "2-U", ""),
    "hq": ("hq", "", 0, "Hero RD", "", 0, "RD", ""),
}
# Japanese samples: (designation, icon, elite, strength, requirements, disc)
JP = [
    ("A/1/7SSNL", "infantry", True, "2", "FL|RD", None), ("B/1/7SSNL", "infantry", True, "2", "MG|FT", None), ("3/111", "infantry", False, "2", "MG", None),
    ("2/111", "infantry", False, "1", "BZ", None), ("Eng 4", "engineer", False, "1", "FT", None), ("HQ 3SB", "hq", True, "2", "RD", None),
    ("AT 1", "anti-tank", False, "1", "AR", None), ("MG 2", "machine-gun", True, "2", "MG|FT", None),
    ("Tank", "tank", False, "1", "AR", "red"), ("Tank", "tank", False, "1", "AR", "blue"), ("Tank", "tank", False, "1", "AR", "green"),
]
DEPTH = [("Coastal|Depth", "CC", "1"), ("Coastal|Depth", "MO", "1"), ("Inland|Depth", "FL", "1"), ("Inland|Depth", "CC", "1"), ("Armor|Depth", "AR", "1")]


def slug(s):
    return re.sub(r"[^A-Za-z0-9]+", "-", s).strip("-").lower()


x = ['<?xml version="1.0" encoding="UTF-8"?>',
     '<counters xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="hexcounters.xsd"',
     '          id="tarawa" title="D-Day at Tarawa" size="15.875" corner="0.05"',
     '          source="Copy_of_D-Day_at_Tarawa_USMC_OOB_Chart_V1.1.pdf; 4-step_Companies.pdf; DDAT-Rules_V13.pdf 2.2-2.26">',
     '''  <palette>
    <color id="ink" value="#111111"/>
    <color id="white" value="#ffffff"/>
    <color id="usmc" value="#8a9a3a" name="US Marines"/>
    <color id="cream" value="#f3ead2" name="Japanese"/>
    <color id="jpred" value="#c8202a"/>
    <color id="jpdark" value="#3a3a3a"/>
    <color id="blue" value="#3a78c0"/>
    <color id="orange" value="#f0a020"/>
    <color id="green" value="#5a9a3a"/>
    <color id="grey" value="#8a8a8a"/>
    <color id="teal" value="#2aa198"/>
    <color id="pos-red" value="#d23a3a" name="position colour"/>
    <color id="pos-blue" value="#3b8ed0" name="position colour"/>
    <color id="pos-green" value="#78b23c" name="position colour"/>
    <color id="band-green" value="#2f5f1f" name="replacement counter band"/>
  </palette>
  <styles>
    <style id="us" ground="usmc" text="white" box-stroke="ink"/>
    <style id="jp-elite" ground="cream" text="jpred" box-stroke="jpred"/>
    <style id="jp" ground="cream" text="jpred" box-stroke="white" box-fill="white"/>
    <style id="mk-blue" ground="blue" text="white" box-stroke="white"/>
    <style id="mk-orange" ground="orange" text="ink" box-stroke="ink"/>
    <style id="mk-green" ground="green" text="white" box-stroke="white"/>
    <style id="mk-grey" ground="grey" text="white" box-stroke="white"/>
  </styles>''']
ids = []
for desig, typ, turn, beach, shape in US:
    icon, mods, steps, val, wp, bsteps, bval, bwp = TYPE[typ]
    cid = "us-" + slug(desig)
    ids.append(cid)
    x.append('  <counter id="%s" family="%s" name="%s">' % (cid, "leader" if typ == "hq" else "unit", desig))
    x.append('    <front style="us">')
    x.append('      <text slot="TC" size="medium" weight="bold">%s</text>' % desig)
    if icon == "tank":
        x.append('      <silhouette kind="tank" color="ink"/>')
    else:
        x.append('      <symbol icon="%s"/>' % icon)
    if steps:
        x.append('      <steps count="%d" max="%d" slot="LCOL" color="white"/>' % (steps, steps))
    x.append('      <text slot="MR" size="small">%s|%s</text>' % (turn, beach))
    if shape:
        x.append('      <glyph kind="%s" slot="LL" color="ink"/>' % SHAPE[shape])
    x.append('      <value>%s</value>' % val)
    if wp:
        x.append('      <text slot="LR" size="small">%s</text>' % wp)
    x.append('    </front>')
    if typ in ("eng", "sp"):
        x.append('    <back derived="same"/>')
    else:
        x.append('    <back derived="reduced">')
        if bsteps:
            x.append('      <steps count="%d" max="%d" slot="LCOL" color="white"/>' % (bsteps, steps))
        x.append('      <value>%s</value>' % bval)
        if bwp:
            x.append('      <text slot="LR" size="small">%s</text>' % bwp)
        x.append('    </back>')
    x.append('  </counter>')
x.append('  <!-- the second, replacement counter of a four-step company carries a dark band (rules 2.21) -->')
x.append('''  <counter id="us-a-1-2-rdc" family="unit" name="A/1/2 replacement counter">
    <front style="us">
      <text slot="TC" size="medium" weight="bold">A/1/2</text>
      <symbol icon="infantry"/>
      <steps count="2" max="4" slot="LCOL" color="white"/>
      <text slot="MR" size="small">Rdc</text>
      <glyph kind="diamond" slot="LL" color="ink"/>
      <value>3-2</value>
      <text slot="LR" size="small">BR</text>
      <band color="band-green" height="6"/>
    </front>
    <back derived="reduced">
      <steps count="1" max="4" slot="LCOL" color="white"/>
      <value>1-2</value>
      <text slot="LR" size="small">RD</text>
    </back>
  </counter>''')
ids.append("us-a-1-2-rdc")
x.append('''  <counter id="us-lvt" family="marker" name="LVT 1/A/2AT">
    <front style="us">
      <text slot="TC" size="medium" weight="bold">1/A/2AT</text>
      <silhouette kind="lvt" color="ink"/>
      <steps count="2" max="2" slot="LCOL" color="white"/>
    </front>
    <back derived="reduced"><steps count="1" max="2" slot="LCOL" color="white"/></back>
  </counter>''')
ids.append("us-lvt")
for i, (desig, icon, elite, val, req, disc) in enumerate(JP):
    cid = "jp-%d-%s" % (i + 1, slug(desig))
    ids.append(cid)
    st = "jp-elite" if elite else "jp"
    x.append('  <counter id="%s" family="unit" name="Japanese %s">' % (cid, desig))
    x.append('    <front style="%s">' % st)
    x.append('      <text slot="TC" size="small" color="jpdark">%s</text>' % desig)
    if icon == "tank":
        x.append('      <silhouette kind="tank" color="jpdark"/>')
        x.append('      <glyph kind="disc" slot="UL" color="pos-%s"/>' % disc)
    else:
        x.append('      <symbol icon="%s"/>' % icon)
    x.append('      <text slot="LL" size="small" weight="bold">%s</text>' % req)
    x.append('      <value anchor="end">%s</value>' % val)
    x.append('    </front>')
    x.append('    <back derived="concealed"/>')
    x.append('  </counter>')
for i, (label, req, val) in enumerate(DEPTH):
    cid = "jp-depth-%d" % (i + 1)
    ids.append(cid)
    x.append('  <counter id="%s" family="marker" name="%s marker">' % (cid, label.replace("|", " ")))
    x.append('    <front style="jp">')
    x.append('      <text slot="TC" size="small" color="jpdark">%s</text>' % label)
    x.append('      <text slot="LL" size="small" weight="bold">%s</text>' % req)
    x.append('      <value anchor="end">%s</value>' % val)
    x.append('    </front>')
    x.append('    <back style="jp"><text slot="CENTRE" size="medium" weight="bold" color="jpdark">%s</text></back>' % label)
    x.append('  </counter>')
markers = [
    ("turn", "mk-blue", '<text slot="CENTRE" size="large" weight="bold">Turn</text>', '<text slot="CENTRE" size="large" weight="bold">Turn</text>'),
    ("phase", "mk-blue", '<text slot="CENTRE" size="large" weight="bold">Phase</text>', None),
] + [("action-%s" % L.lower(), "mk-orange", '<text slot="CENTRE" size="large" weight="bold">%s</text>' % L, '<text slot="CENTRE" size="medium">Japanese|Action</text>') for L in "AIMPR"] + [
    ("action-taken", "mk-green", '<text slot="CENTRE" size="medium" weight="bold">US|Action|Taken</text>', None),
    ("command-range", "mk-green", '<symbol icon="hq" stroke="white"/><text slot="TC" size="small">2/2</text><text slot="BOTTOM" size="medium" weight="bold">COMMAND RANGE</text>', None),
    ("support", "mk-green", '<emblem kind="star" color="white" scale="0.8"/><text slot="BOTTOM" size="medium" weight="bold">SUPPORT</text>', None),
    ("naval-gunfire", "mk-blue", '<text slot="TC" size="medium" weight="bold">Naval|Gunfire</text><value>9-U</value>', None),
    ("no-lvt-comm", "mk-green", '<text slot="CENTRE" size="medium" weight="bold">No|LVT|Comm</text>', None),
    ("hero", "mk-grey", '<emblem kind="star" color="white" scale="0.7"/><text slot="BOTTOM" size="medium" weight="bold">HERO</text>', '<text slot="CENTRE" size="medium" weight="bold">Inspired</text>'),
    ("garrison", "mk-grey", '<text slot="CENTRE" size="medium" weight="bold">Garrison</text>', None),
    ("disrupted-us", "mk-grey", '<text slot="CENTRE" size="medium" weight="bold">Disrupted</text>', '<text slot="CENTRE" size="medium" weight="bold">Disrupted|(prior)</text>'),
    ("disrupted-jp", "jp", '<text slot="CENTRE" size="medium" weight="bold" color="jpdark">Disrupted</text>', None),
    ("smoke", "mk-grey", '<emblem kind="cloud" color="white" scale="0.8"/><text slot="BOTTOM" size="medium" weight="bold">Smoke</text>', None),
    ("artillery-destroyed", "jp", '<symbol icon="artillery" stroke="jpdark"/><text slot="BOTTOM" size="small" weight="bold" color="jpdark">DESTROYED</text>', None),
    ("shibasaki", "jp", '<text slot="TC" size="small" color="jpdark">Adm. Shibasaki</text><emblem kind="roundel" color="jpred" color2="white" scale="0.7"/><text slot="BOTTOM" size="medium" weight="bold" color="jpdark">In Command</text>',
     '<text slot="TC" size="small" color="jpdark">Adm. Shibasaki</text><text slot="CENTRE" size="large" weight="bold">Killed</text>'),
]
for mid, st, f, b in markers:
    cid = "mk-" + mid
    ids.append(cid)
    x.append('  <counter id="%s" family="marker" name="%s">' % (cid, mid))
    x.append('    <front style="%s">%s</front>' % (st, f))
    x.append('    <back style="%s">%s</back>' % (st, b) if b else '    <back derived="same"/>')
    x.append('  </counter>')
cols = 10
rows = -(-len(ids) // cols)
x.append('  <sheet id="sheet" title="D-Day at Tarawa counters" cols="%d" rows="%d" gutter="0.5" margin="8" mirror="horizontal">' % (cols, rows))
for cid in ids:
    x.append('    <place counter="%s"/>' % cid)
x.append('  </sheet>')
x.append('</counters>')
open(OUT, "w", encoding="utf-8").write("\n".join(x))
print("wrote", OUT, len(ids), "counters")
# Copyright Ben Paul Wise. All Rights Reserved.
