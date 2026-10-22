# Copyright Ben Paul Wise. All Rights Reserved.
"""build_trc.py -- The Russian Campaign, Compass Games 2020 remake: the complete counter sheet, both faces.

Transcribed from trc_units_sep14_page_1_front.jpg and page_2_back.  The back sheet is
the front mirrored left to right, which is how each back was matched to its front.
"""
import re

OUT = r"C:\repos\ghub-per\titan-alloy\hexgames\unit_graphics\xml\the-russian-campaign.xml"

# ---- German block, 10 columns x 12 rows.  Each entry: (designation, icon, echelon, value, style, back)
# back: a date "Nov-Dec 1941" [+ " Moscow"], a region text, a diagonal city, "Special", or a band
I, A, M, C, MT, PA, HQ = "infantry", "armour", "mechanized", "cavalry", "infantry+mountain", "infantry+airborne", "hq"
X3, X4, X5 = "corps", "army", "army-group"
G = "german"
german = [
    [("N", HQ, X5, "1-7", G, "AGN"), ("41", A, X3, "8-7", G, "AGN"), ("56", A, X3, "7-7", G, "AGN"), ("1", I, X3, "4-4", G, "AGN"), ("2", I, X3, "4-4", G, "AGN"),
     ("10", I, X3, "3-4", G, "AGN"), ("26", I, X3, "3-4", G, "AGN"), ("28", I, X3, "3-4", G, "AGN"), ("38", I, X3, "3-4", G, "AGN"), None],
    [("C", HQ, X5, "1-7", G, "AGC"), ("24", A, X3, "8-7", G, "AGC"), ("39", A, X3, "8-7", G, "AGC"), ("46", A, X3, "6-7", G, "AGC"), ("47", A, X3, "7-7", G, "AGC"),
     ("57", A, X3, "6-7", G, "AGC"), ("5", I, X3, "4-4", G, "AGC"), ("6", I, X3, "4-4", G, "AGC"), ("7", I, X3, "5-4", G, "AGC"), ("8", I, X3, "4-4", G, "AGC")],
    [("9", I, X3, "5-4", G, "AGC"), ("12", I, X3, "4-4", G, "AGC"), ("13", I, X3, "3-4", G, "AGC"), ("20", I, X3, "3-4", G, "AGC"), ("42", I, X3, "3-4", G, "AGC"),
     ("43", I, X3, "5-4", G, "AGC"), ("53", I, X3, "4-4", G, "AGC"), ("S", HQ, X5, "1-7", G, "AGS"), ("3", A, X3, "7-7", G, "AGS"), ("48", A, X3, "6-7", G, "AGS")],
    [("14", A, X3, "7-7", G, "AGS"), ("52", M, X3, "3-6", G, "AGS"), ("49", MT, X3, "5-5", G, "AGS"), ("4", I, X3, "5-4", G, "AGS"), ("17", I, X3, "3-4", G, "AGS"),
     ("29", I, X3, "3-4", G, "AGS"), ("44", I, X3, "5-4", G, "AGS"), ("55", I, X3, "3-4", G, "AGS"), ("11", I, X3, "4-4", G, "Rumania"), ("30", I, X3, "4-4", G, "Rumania")],
    [("54", I, X3, "3-4", G, "Rumania"), ("40", A, X3, "8-7", G, "Jul-Aug 1941"), ("27", I, X3, "4-4", G, "Jul-Aug 1941"), ("23", I, X3, "4-4", G, "Jul-Aug 1941"), ("34", I, X3, "4-4", G, "Sep-Oct 1941"),
     ("35", I, X3, "4-4", G, "Sep-Oct 1941"), ("51", I, X3, "4-4", G, "Nov-Dec 1941"), ("50", M, X3, "4-6", G, "Nov-Dec 1941"), ("59", I, X3, "4-4", G, "May-Jun 1942"), ("72", I, X3, "3-4", G, "Jul-Aug 1942")],
    [("3", MT, X3, "2-3", G, "Jul-Aug 1942"), ("1", C, X3, "4-7", G, "Jan-Feb 1943"), ("22", MT, X3, "3-5", G, "May-Jun 1943"), ("11", M, X3, "4-6", G, "Jul-Aug 1943"), ("26", M, X3, "4-6", G, "Jul-Aug 1943"),
     ("29", M, X3, "4-6", G, "Nov-Dec 1943"), ("43", M, X3, "5-6", G, "Nov-Dec 1943"), ("90", I, X3, "3-4", G, "May-Jun 1944"), ("GD", A, X3, "10-8", G, "Nov-Dec 1944"), ("15", MT, X3, "2-5", G, "Special")],
    [("Res", I, X3, "2-7", "ss", "Jul-Aug 1941"), ("1", A, X3, "10-8", "ss", "May-Jun 1943"), ("2", A, X3, "9-8", "ss", "Jan-Feb 1943"), ("3", A, X3, "7-7", "ss", "Mar-Apr 1943"),
     ("HG", M, X3, "7-8", "luftwaffe", "Special"), ("Stuka", "stuka", None, "", "luftwaffe", None), None, ("91", MT, X3, "2-5", G, "Special"), ("97", MT, X3, "2-5", G, "Special"), None],
    [("4", A, X3, "6-7", "ss", "Special"), ("5", M, X3, "5-5", "ss", "Special"), ("6", M, X3, "4-4", "ss", "Sep-Oct 1943"), ("15", C, X3, "4-7", "ss", "Jul-Aug 1944"),
     ("Stuka", "stuka", None, "", "luftwaffe", None), ("Stuka", "stuka", None, "", "luftwaffe", None), None, ("21", MT, X3, "4-5", G, "Special"), ("36", MT, X3, "3-5", G, "Special"), None],
    [("1", I, X3, "3-5", "rumanian", "Rumania"), ("2", I, X3, "4-6", "rumanian", "Rumania"), ("4", I, X3, "3-4", "rumanian", "Rumania"), ("Cv", C, X3, "2-7", "rumanian", "Rumania"), ("5", I, X3, "2-4", "rumanian", "Jul-Aug 1941"),
     ("2", I, X3, "3-4", "finnish", "Finland"), ("4", I, X3, "4-3", "finnish", "Finland"), None, ("Dtl", MT, X3, "3-5", G, "Special"), None],
    [("6", I, X3, "2-4", "rumanian", "May-Jun 1942"), ("7", I, X3, "2-4", "rumanian", "May-Jun 1942"), ("Pz", A, X3, "2-6", "rumanian", "May-Jun 1942"), ("3", I, X3, "2-2", "rumanian", "Sep-Oct 1942"), None,
     ("6", I, X3, "4-3", "finnish", "Finland"), ("7", I, X3, "2-3", "finnish", "Finland"), None, ("Hitler", "hitler", None, "1-8", G, "~Berlin"), None],
    [("1", I, X3, "4-6", "hungarian", "Jul-Aug 1941"), ("2", I, X3, "3-3", "hungarian", "May-Jun 1942"), ("3", I, X3, "2-4", "italian", "AGC"), ("4", I, X3, "2-3", "italian", "Sep-Oct 1941"),
     ("Weather", "marker-weather", None, "", "marker", None), ("Year", "marker-word", None, "", "marker", None), ("Loco", "loco", None, "", "marker", None), ("Loco", "loco", None, "", "marker", None), ("Loco", "loco", None, "", "marker", None), ("Loco", "loco", None, "", "marker", None)],
    [("3", I, X3, "2-2", "hungarian", "Jul-Aug 1942"), None, None, ("5", I, X3, "2-3", "italian", "May-Jun 1942"),
     ("Turn", "marker-word", None, "", "marker", None), ("Impulse", "marker-word", None, "", "marker", None), ("Loco", "loco", None, "", "marker", None), ("Loco", "loco", None, "", "marker", None), ("Loco", "loco", None, "", "marker", None), ("Loco", "loco", None, "", "marker", None)],
]
R, GU = "russian", "guards"
russian = [
    [("1G", A, X4, "8-4", GU, "Nov-Dec 1941 Moscow"), ("2G", A, X4, "7-4", GU, "Nov-Dec 1941"), ("3G", A, X4, "7-4", GU, "Nov-Dec 1941"), ("4G", A, X4, "7-4", GU, "Nov-Dec 1941"), ("5G", A, X4, "7-4", GU, "Mar-Apr 1942"),
     ("6G", A, X4, "7-4", GU, "May-Jun 1942"), ("7G", A, X4, "7-4", GU, "Sep-Oct 1942"), ("8G", A, X4, "7-4", GU, "Nov-Dec 1942"), ("9G", A, X4, "7-4", GU, "Mar-Apr 1943"), ("Stalin", "stalin", None, "1-8", R, "~Moscow")],
    [("11", I, X4, "6-3", R, "Baltic MD"), ("12", I, X4, "6-3", R, "Kiev MD"), ("16", I, X4, "6-3", R, "Jul-Aug 1941"), ("39", I, X4, "6-3", R, "Nov-Dec 1941"), ("59", I, X4, "6-3", R, "Nov-Dec 1941"),
     ("60", I, X4, "6-3", R, "Nov-Dec 1941"), ("3", I, X4, "5-3", R, "Western MD"), ("5", I, X4, "5-3", R, "Kiev MD"), ("6", I, X4, "5-3", R, "Kiev MD"), ("8", I, X4, "5-3", R, "Baltic MD")],
    [("9", I, X4, "5-3", R, "Odessa MD"), ("10", I, X4, "5-3", R, "Western MD"), ("20", I, X4, "5-3", R, "Jul-Aug 1941 Moscow"), ("22", I, X4, "5-3", R, "~Kalinin"), ("23", I, X4, "5-3", R, "Finnish Border"),
     ("26", I, X4, "5-3", R, "Kiev MD"), ("29", I, X4, "5-3", R, "Sep-Oct 1941 Moscow"), ("34", I, X4, "5-3", R, "Sep-Oct 1941 Moscow"), ("37", I, X4, "5-3", R, "Sep-Oct 1941"), ("40", I, X4, "5-3", R, "Jul-Aug 1941 Kursk")],
    [("43", I, X4, "5-3", R, "Sep-Oct 1941"), ("61", I, X4, "5-3", R, "Nov-Dec 1941"), ("7", I, X4, "4-3", R, "Finnish Border"), ("13", I, X4, "4-3", R, "~Minsk"), ("18", I, X4, "4-3", R, "~Kiev"),
     ("19", I, X4, "4-3", R, "~Moscow"), ("21", I, X4, "4-3", R, "~Tula"), ("24", I, X4, "4-3", R, "Jul-Aug 1941 Kursk"), ("27", I, X4, "4-3", R, "~Riga"), ("30", I, X4, "4-3", R, "Sep-Oct 1941 Moscow")],
    [("31", I, X4, "4-3", R, "Sep-Oct 1941 Moscow"), ("32", I, X4, "4-3", R, "Sep-Oct 1941 Moscow"), ("33", I, X4, "4-3", R, "Sep-Oct 1941 Moscow"), ("49", I, X4, "4-3", R, "Sep-Oct 1941"), ("51", I, X4, "4-3", R, "Sep-Oct 1941"),
     ("3", I, X4, "3-3", R, "Western MD"), ("5", I, X4, "3-3", R, "Sep-Oct 1941"), ("52", I, X4, "3-3", R, "Sep-Oct 1941"), ("55", I, X4, "3-3", R, "Sep-Oct 1941"), ("14", I, X4, "3-3", R, "~Special Rail")],
    [("1", C, X3, "2-7", R, "~Kiev"), ("2", C, X3, "2-7", R, "~Odessa"), ("3", C, X3, "2-7", R, "Western MD"), ("5", C, X3, "3-7", R, "Odessa MD"), ("6", C, X3, "3-7", R, "Kiev MD"),
     ("7", C, X3, "3-7", R, "Western MD"), ("1G", C, X3, "5-7", GU, "Sep-Oct 1941 Moscow"), ("2G", C, X3, "4-7", GU, "Jan-Feb 1942"), ("3G", C, X3, "4-7", GU, "Jan-Feb 1943"), ("4G", C, X3, "4-7", GU, "Jul-Aug 1943")],
    [("1", A, X3, "2-5", R, "Baltic MD"), ("2", A, X3, "2-5", R, "~Moscow"), ("3", A, X3, "2-5", R, "Kiev MD"), ("4", A, X3, "2-5", R, "Kiev MD"), ("5", A, X3, "2-5", R, "Western MD"),
     ("6", A, X3, "2-5", R, "~Minsk"), ("7", A, X3, "3-5", R, "Baltic MD"), ("8", A, X3, "3-5", R, "Western MD"), ("9", A, X3, "3-5", R, "~Leningrad"), ("10", A, X3, "3-5", R, "~Riga")],
    [("11", A, X3, "3-5", R, "Kiev MD"), ("12", A, X3, "3-5", R, "Odessa MD"), ("1", PA, X3, "1-2", R, "Nov-Dec 1941 Moscow"), ("5", PA, X3, "1-2", R, "Sep-Oct 1941 Moscow"), ("8", PA, X3, "1-2", R, "Nov-Dec 1941 Moscow"),
     None, ("1", A, X4, "6-5", R, "Mar-Apr 1942"), ("2", A, X4, "6-5", R, "Mar-Apr 1942"), ("3", A, X4, "6-5", R, "Jul-Aug 1942"), ("4", A, X4, "6-5", R, "Jul-Aug 1942")],
    [("1G", A, X4, "10-7", GU, "Mar-Apr 1942"), ("2G", A, X4, "8-6", GU, "Sep-Oct 1942"), ("3G", A, X4, "8-6", GU, "Nov-Dec 1942"), ("4G", A, X4, "8-6", GU, "Jan-Feb 1943"), ("STAVKA", HQ, None, "1-7", R, "~Moscow"),
     None, ("2", "worker", None, "2", "worker", "Jan-Feb 1942|any City"), ("1", "worker", None, "1", "worker", "Nov-Dec 1941|any City"), ("1", "worker", None, "1", "worker", "Mar-Apr 1943|any City"), ("1", "worker", None, "1", "worker", "Jul-Aug 1942|any City")],
    [("5G", A, X4, "8-6", GU, "May-Jun 1943"), ("6G", A, X4, "8-6", GU, "Jul-Aug 1943"), ("7G", A, X4, "8-6", GU, "Sep-Oct 1943"), ("8G", A, X4, "8-6", GU, "Nov-Dec 1943"), None,
     None, ("2", "worker", None, "2", "worker", "May-Jun 1942|any City"), ("1", "worker", None, "1", "worker", "Jan-Feb 1942|any City"), ("1", "worker", None, "1", "worker", "Nov-Dec 1942|any City"), ("1", "worker", None, "1", "worker", "Mar-Apr 1942|any City")],
    [None, None, None, None, None, ("Partisans", "partisan", None, "", "worker", "Nov-Dec 1941|available"), ("Stalino", "worker", None, "2", "worker", "~Stalino"), ("", "worker", None, "1", "worker", "Jan-Feb 1942|any City"),
     ("Moscow", "worker", None, "3", "worker", "~Moscow"), ("Kharkov", "worker", None, "2", "worker", "~Kharkov")],
    [None, None, None, None, ("Partisans", "partisan", None, "", "worker", "Nov-Dec 1941|available"), ("Partisans", "partisan", None, "", "worker", "Sep-Oct 1941|available"), ("Leningrad", "worker", None, "2", "worker", "~Leningrad"),
     ("Kiev", "worker", None, "2", "worker", "~Kiev"), ("Archangel", "worker", None, "(2)", "worker", "~Archangel"), None],
]

YEAR_FILL = {"1941": "y41", "1942": "y42", "1943": "y43", "1944": "y44"}
NAT_LETTER = {"rumanian": "R", "finnish": "F", "hungarian": "H", "italian": "I"}


def slug(s):
    return re.sub(r"[^A-Za-z0-9]+", "-", s).strip("-").lower() or "x"


def front(desig, icon, ech, value, style):
    o = ['    <front style="%s">' % style]
    if icon == "stuka":
        o.append('      <silhouette kind="aircraft"/>')
        o.append('      <text slot="BOTTOM" size="medium">Stuka</text>')
    elif icon == "loco":
        o.append('      <silhouette kind="locomotive" color="ink"/>')
    elif icon in ("hitler", "stalin"):
        o.append('      <emblem kind="%s" color="%s"/>' % ("balkenkreuz" if icon == "hitler" else "star", "ink" if icon == "hitler" else "red"))
        o.append('      <text slot="TC" size="medium">%s</text>' % desig)
        o.append('      <value>%s</value>' % value)
    elif icon == "worker":
        o.append('      <symbol icon="worker" fill="red" stroke="white"/>')
        if desig:
            o.append('      <text slot="TC">%s</text>' % desig)
        o.append('      <value>%s</value>' % value)
    elif icon == "partisan":
        o.append('      <silhouette kind="rifle" color="ink"/>')
        o.append('      <text slot="BOTTOM" size="medium" color="yellow">Partisans</text>')
    elif icon == "marker-weather":
        o.append('      <emblem kind="cloud" color="white"/>')
        o.append('      <text slot="BOTTOM" size="medium">Weather</text>')
    elif icon == "marker-word":
        o.append('      <text slot="CENTRE" size="large" weight="bold">%s</text>' % desig)
    else:
        base, _, mod = icon.partition("+")
        mods = ' modifiers="%s"' % mod if mod else ""
        if style == "guards":
            o.append('      <symbol icon="%s"%s fill="red" stroke="white"/>' % (base, mods))
        else:
            o.append('      <symbol icon="%s"%s/>' % (base, mods))
        if ech:
            o.append('      <echelon level="%s"/>' % ech)
        if base == "hq":
            o.append('      <text slot="MR" size="medium">%s</text>' % desig)
            if desig == "STAVKA":
                o.pop()
                o.append('      <text slot="TC">STAVKA</text>')
        else:
            o.append('      <text slot="MR" size="small">%s</text>' % desig)
        if style in NAT_LETTER:
            o.append('      <text slot="ML" size="small" weight="bold">%s</text>' % NAT_LETTER[style])
        if value:
            o.append('      <value>%s</value>' % value)
    o.append('    </front>')
    return "\n".join(o)


def back(spec, style):
    if spec is None:
        return "    <back derived=\"same\"/>"
    o = ['    <back style="%s">' % style]
    if spec.startswith("~"):                                   # diagonal city or word
        colour = ' color="red"' if "Special" in spec else ""
        o.append('      <text slot="free" x="50" y="52" rotate="-40" size="medium" weight="bold"%s>%s</text>' % (colour, spec[1:]))
    elif spec == "Special":
        o.append('      <tile fill="white" color="red">Special</tile>')
    elif re.match(r"^[A-Z][a-z]{2}-[A-Z][a-z]{2} 19\d\d", spec):
        date, _, rest = spec.partition("|")
        m = re.match(r"^(\S+) (19\d\d)(?: (.*))?$", date)
        months, year, place = m.group(1), m.group(2), m.group(3)
        lines = months + "|" + year + ("|" + place if place else "")
        o.append('      <tile fill="%s">%s</tile>' % (YEAR_FILL[year], lines))
        if rest:
            o.append('      <band color="ink" text-color="white">%s</band>' % rest)
    else:                                                       # region text: AGN, Kiev MD, Finland ...
        o.append('      <text slot="CENTRE" size="medium">%s</text>' % spec.replace(" MD", "|MD").replace("Finnish Border", "Finnish|Border"))
    o.append('    </back>')
    return "\n".join(o)


x = ['<?xml version="1.0" encoding="UTF-8"?>',
     '<counters xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="hexcounters.xsd"',
     '          id="trc-2020" title="The Russian Campaign, Compass Games 2020 remake" size="12.7" corner="0.06"',
     '          source="trc_units_sep14_page_1_front.jpg, page_2_back_1200px.jpg; both faces, complete">',
     '''  <palette>
    <color id="ink" value="#222222"/>
    <color id="white" value="#ffffff"/>
    <color id="red" value="#e0272a"/>
    <color id="yellow" value="#f2d24a"/>
    <color id="g-ground" value="#a9bda9" name="German"/>
    <color id="g-box" value="#8a9a78"/>
    <color id="ss-ground" value="#3a3a3a" name="SS"/>
    <color id="lw-ground" value="#4a6a8a" name="Luftwaffe"/>
    <color id="lw-box" value="#bcd0e0"/>
    <color id="ru-ground" value="#4f7a55" name="Rumanian"/>
    <color id="fi-ground" value="#e8e8ec" name="Finnish"/>
    <color id="fi-text" value="#2c3fa0"/>
    <color id="hu-ground" value="#9a9a9a" name="Hungarian"/>
    <color id="it-ground" value="#8fd0a0" name="Italian"/>
    <color id="it-box" value="#d8e060"/>
    <color id="r-ground" value="#c9a463" name="Russian"/>
    <color id="r-box" value="#a8874a"/>
    <color id="w-ground" value="#8b4a1e" name="Workers"/>
    <color id="m-ground" value="#f2f2f2" name="Markers"/>
    <color id="y41" value="#ffffff" name="arrives 1941"/>
    <color id="y42" value="#f7c9a0" name="arrives 1942"/>
    <color id="y43" value="#bfe0f5" name="arrives 1943"/>
    <color id="y44" value="#f5c0cf" name="arrives 1944"/>
  </palette>
  <styles>
    <style id="german" ground="g-ground" text="ink" box-stroke="ink" box-fill="g-box"/>
    <style id="ss" ground="ss-ground" text="white" box-stroke="white"/>
    <style id="luftwaffe" ground="lw-ground" text="white" box-stroke="ink" box-fill="lw-box"/>
    <style id="rumanian" ground="ru-ground" text="white" box-stroke="white"/>
    <style id="finnish" ground="fi-ground" text="fi-text" box-stroke="fi-text"/>
    <style id="hungarian" ground="hu-ground" text="ink" box-stroke="ink" box-fill="white"/>
    <style id="italian" ground="it-ground" text="ink" box-stroke="ink" box-fill="it-box"/>
    <style id="russian" ground="r-ground" text="ink" box-stroke="ink" box-fill="r-box"/>
    <style id="guards" ground="r-ground" text="ink" box-stroke="white" box-fill="red"/>
    <style id="worker" ground="w-ground" text="white" box-stroke="white" box-fill="red"/>
    <style id="marker" ground="m-ground" text="ink" box-stroke="ink"/>
  </styles>''']
ids = []
seen = set()
for prefix, block in (("g", german), ("r", russian)):
    for row in block:
        for cell in row:
            if cell is None:
                ids.append(None)
                continue
            desig, icon, ech, value, style, bk = cell
            cid = "%s-%s-%s" % (prefix, style[:2], slug((desig or icon) + "-" + (icon.split("+")[0])))
            base = cid
            n = 2
            while cid in seen:
                cid = "%s-%d" % (base, n); n += 1
            seen.add(cid)
            ids.append(cid)
            fam = "leader" if icon in ("hitler", "stalin") else "marker" if icon.startswith("marker") or icon == "loco" else "support" if icon == "stuka" else "unit"
            x.append('  <counter id="%s" family="%s" name="%s">' % (cid, fam, (desig or icon) + " " + style))
            x.append(front(desig, icon, ech, value, style))
            x.append(back(bk, style))
            x.append('  </counter>')
# one sheet: German block, a blank column, Russian block -- 21 columns x 12 rows, as printed
x.append('  <sheet id="sheet" title="The Russian Campaign 2020" cols="21" rows="12" gutter="0.6" margin="8" mirror="horizontal">')
gi = iter(ids[:sum(len(r) for r in german)])
ri = iter(ids[sum(len(r) for r in german):])
for r in range(12):
    for c in range(10):
        cid = next(gi)
        x.append('    <place counter="%s"/>' % cid if cid else '    <place blank="true"/>')
    x.append('    <place blank="true"/>')
    for c in range(10):
        cid = next(ri)
        x.append('    <place counter="%s"/>' % cid if cid else '    <place blank="true"/>')
x.append('  </sheet>')
x.append('</counters>')
open(OUT, "w", encoding="utf-8").write("\n".join(x))
print("wrote", OUT, len(seen), "counters")
# Copyright Ben Paul Wise. All Rights Reserved.
