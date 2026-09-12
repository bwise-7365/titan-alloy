"""build_pgg.py -- Panzergruppe Guderian: the two 'new' counter sections (PGG_Countersheet_1-2_new.pdf, 2-2_new.pdf).

Each PDF page prints the front on the left and the back, mirrored, on the right.  Backs:
German units flip to a reduced value; Soviet units flip to an untried 'U' face on a paler
ground; support markers flip to their second state.
"""
import re

OUT = r"C:\repos\ghub-per\titan-alloy\hexgames\unit_graphics\xml\panzergruppe-guderian.xml"

# (designation, corps/army code, icon, echelon, front value, back value)   -- None = empty cell
# icons: inf, arm, mot, cav; special: hq (Soviet army HQ), air, cross, dis, gt, boom, oos, rail, star
S1 = [  # sheet 1/2, Soviet, 10 columns
    [("Lukin|16th Army", "2216", "hq", None, "(3)\u260510", "\u2605"), ("Remezov|13th Army", "0123-26", "hq", None, "(4)\u260510", "\u2605"), ("Kachalov|28th Army", "4X", "hq", None, "(2)\u260510", "\u2605"),
     ("Khomenko|30th Army", "2W", "hq", None, "(3)\u260510", "\u2605"), ("Maslikev|29th Army", "7X", "hq", None, "(3)\u260510", "\u2605"), ("Rakutin|24th Army", "4015", "hq", None, "(2)\u260510", "\u2605"),
     ("Yrshakov|22nd Army", "1V", "hq", None, "(3)\u260510", "\u2605"), ("Gramenko|21st Army", "3X", "hq", None, "(2)\u260510", "\u2605"), ("Kurchkin|20th Army", "0108-15", "hq", None, "(4)\u260510", "\u2605"),
     ("Konev|19th Army", "1414", "hq", None, "(5)\u260510", "\u2605")],
    [("Dolmatov|31st Army", "8X", "hq", None, "(2)\u260510", "\u2605"), ("Vshnvsky|32nd Army", "10X", "hq", None, "(2)\u260510", "\u2605"), ("Onprenko|33rd Army", "11X", "hq", None, "(2)\u260510", "\u2605"),
     ("Zkhatkin|49th Army", "8X", "hq", None, "(3)\u260510", "\u2605"), ("Rokovski|Reserve", "1X", "hq", None, "(5)\u260510", "\u2605"),
     ("64", "", "inf", "division", "2-4-6", "U-6"), ("F", "", "inf", "division", "2-4-6", "U-6"), ("275", "", "inf", "division", "2-4-6", "U-6"), ("8Mos", "", "inf", "division", "2-4-6", "U-6"), ("13Mos", "", "inf", "division", "2-4-6", "U-6")],
    [("48", "", "inf", "division", "1-4-6", "U-6"), ("61", "", "inf", "division", "6-5-6", "U-6"), ("89", "", "inf", "division", "5-5-6", "U-6"), ("127", "", "inf", "division", "5-5-6", "U-6"), ("143", "", "inf", "division", "5-5-6", "U-6"),
     ("153", "", "inf", "division", "4-5-6", "U-6"), ("180", "", "inf", "division", "4-5-6", "U-6"), ("181", "", "inf", "division", "4-5-6", "U-6"), ("232", "", "inf", "division", "4-5-6", "U-6"), ("260", "", "inf", "division", "4-5-6", "U-6")],
    [None] * 10,
    [("18", "", "inf", "division", "3-5-6", "U-6"), ("138", "", "inf", "division", "3-5-6", "U-6"), ("167", "", "inf", "division", "3-5-6", "U-6"), ("162", "", "inf", "division", "2-5-6", "U-6"), ("178", "", "inf", "division", "2-5-6", "U-6"),
     ("300", "", "inf", "division", "2-5-6", "U-6"), ("76", "", "inf", "division", "1-5-6", "U-6"), ("111", "", "inf", "division", "7-6-6", "U-6"), ("100", "", "inf", "division", "6-6-6", "U-6"), ("112", "", "inf", "division", "4-6-6", "U-6")],
    [("65", "", "inf", "division", "2-6-6", "U-6"), ("126", "", "inf", "division", "7-7-6", "U-6"), ("174", "", "inf", "division", "6-7-6", "U-6"), ("154", "", "inf", "division", "4-7-6", "U-6"), ("117", "", "inf", "division", "3-7-6", "U-6"),
     ("50", "", "inf", "division", "9-8-6", "U-6"), ("123", "", "inf", "division", "8-8-6", "U-6"), ("161", "", "inf", "division", "8-8-6", "U-6"), ("258", "", "inf", "division", "6-8-6", "U-6"), ("49", "", "inf", "division", "5-8-6", "U-6")],
    [("137", "", "inf", "division", "5-8-6", "U-6"), ("1Mos", "", "inf", "division", "5-8-6", "U-6"), ("171", "", "inf", "division", "3-8-6", "U-6"), ("57", "", "inf", "division", "3-4-6", "U-6"), ("42", "", "inf", "division", "3-4-6", "U-6"),
     ("133", "", "inf", "division", "3-4-6", "U-6"), ("8Mos", "", "inf", "division", "3-4-6", "U-6"), ("17Mos", "", "inf", "division", "3-4-6", "U-6"), ("172", "", "inf", "division", "1-3-6", "U-6"), ("86", "", "inf", "division", "5-4-6", "U-6")],
    [None] * 10,
    [("17", "", "inf", "division", "4-4-6", "U-6"), ("118", "", "inf", "division", "4-4-6", "U-6"), ("25", "", "inf", "division", "3-4-6", "U-6"), ("102", "", "inf", "division", "2-3-6", "U-6"), ("19", "", "inf", "division", "1-3-6", "U-6"),
     ("121", "", "inf", "division", "1-3-6", "U-6"), ("143", "", "inf", "division", "1-3-6", "U-6"), ("152", "", "inf", "division", "1-3-6", "U-6"), ("232", "", "inf", "division", "3-3-6", "U-6"), ("134", "", "inf", "division", "3-3-6", "U-6")],
    [("172", "", "inf", "division", "2-3-6", "U-6"), ("154", "", "inf", "division", "2-3-6", "U-6"), ("158", "", "inf", "division", "2-3-6", "U-6"), ("38", "", "inf", "division", "2-2-6", "U-6"), ("91", "", "inf", "division", "2-2-6", "U-6"),
     ("45", "", "inf", "division", "1-2-6", "U-6"), ("55", "", "inf", "division", "4-3-6", "U-6"), ("56", "", "inf", "division", "3-3-6", "U-6"), ("108", "", "inf", "division", "1-1-6", "U-6"), ("110", "", "inf", "division", "1-1-6", "U-6")],
    [("46", "", "inf", "division", "2-1-6", "U-6"), ("5Mos", "", "inf", "division", "2-1-6", "U-6"), ("187", "", "inf", "division", "2-2-6", "U-6"), ("2", "", "inf", "division", "0-0-6", "U-6"), ("27", "", "inf", "division", "0-0-6", "U-6"),
     ("60", "", "inf", "division", "0-0-6", "U-6"), ("19", "", "inf", "division", "0-1-6", "U-6"), ("13", "", "inf", "division", "1-1-6", "U-6"), ("53", "", "inf", "division", "0-0-6", "U-6"), ("145", "", "inf", "division", "0-0-6", "U-6")],
    [None] * 10,
    [("210", "", "mot", "division", "4-10", "U-10"), ("208", "", "mot", "division", "4-10", "U-10"), ("205", "", "mot", "division", "4-10", "U-10"), ("4", "", "mot", "division", "2-10", "U-10"), ("29", "", "mot", "division", "1-10", "U-10"),
     ("0", "", "arm", "division", "0-10", "U-10"), ("22", "", "arm", "division", "0-10", "U-10"), ("23", "", "arm", "division", "0-10", "U-10"), ("7", "", "arm", "division", "3-10", "U-10"), ("19", "", "arm", "division", "3-10", "U-10")],
    [("204", "", "mot", "division", "5-10", "U-10"), ("22", "", "mot", "division", "6-10", "U-10"), ("57", "", "mot", "division", "8-10", "U-10"), ("103", "", "mot", "division", "8-10", "U-10"), ("1PrGd", "", "mot", "division", "8-10", "U-10"),
     ("8", "", "arm", "division", "7-10", "U-10"), ("31", "", "arm", "division", "5-10", "U-10"), ("26", "", "arm", "division", "5-10", "U-10"), ("27", "", "arm", "division", "4-10", "U-10"), ("10", "", "arm", "division", "3-10", "U-10")],
]
S2 = [  # sheet 2/2, German, 10 columns
    [(d, "", "inf", "division", "2-7", "1-7") for d in ("5", "6", "7", "15", "17", "23", "26", "31", "34", "35")],
    [(d, "", "inf", "division", "2-7", "1-7") for d in ("78", "137", "161", "252", "258", "263", "268", "292")] + [("1", "3H", "cav", "division", "4-5", "2-5"), ("Lw", "", "air", None, "", "Air|Interdiction")],
    [(d, c, "mot", "division", "9-7", "4-7") for d, c in (("5", "3D"), ("6", "2B"), ("7", "7E"), ("15", "7E"), ("17", "7E"), ("23", "5E"), ("26", "2B"), ("31", "9G"), ("34", "9G"), ("35", "3D"))],
    [None] * 10,
    [(d, c, "mot", "division", "9-7", "4-7") for d, c in (("78", "8F"), ("137", "5E"), ("161", "3D"), ("252", "8F"), ("258", "6E"), ("263", "5E"), ("268", "7E"), ("292", "6E"))] + [("GD", "3E", "mot", "division", "4-10", "2-10"), ("Lehr", "2A", "mot", "division", "3-10", "2-10")],
    [(d, c, "inf", "regiment", "2-10", "1-10") for d, c in (("3/3", "3H"), ("12/4", "3H"), ("6/7", "1C"), ("69/10", "3E"), ("5/12", "1C"), ("40/17", "3E"), ("52/18", "3E"), ("73/19", "2A"), ("112/20", "1C"))] + [("Lw", "", "air", None, "", "Air|Interdiction")],
    [(d, c, "inf", "regiment", "2-10", "1-10") for d, c in (("394/3", "3H"), ("33/4", "3H"), ("7/7", "1C"), ("86/10", "3E"), ("25/12", "1C"), ("63/17", "3E"), ("101/18", "3E"), ("74/19", "2A"), ("59/20", "1C"))] + [("Lw", "", "air", None, "", "Air|Interdiction")],
    [None] * 10,
    [(d, c, "arm", "regiment", "4-10", "2-10") for d, c in (("6/3", "3H"), ("35/4", "3H"), ("25/7", "1C"), ("7/10", "3E"), ("29/12", "1C"), ("39/17", "3E"), ("18/18", "3E"))] + [("", "", "cross", None, "", "DIS"), ("", "", "cross", None, "", "DIS"), ("4(DE)", "3E", "ss", "regiment", "3-10", "2-10")],
    [(d, c, "arm", "regiment", "4-10", "2-10") for d, c in (("27/19", "2A"), ("21/20", "1C"))] + [(d, c, "mot", "regiment", "3-10", "2-10") for d, c in (("69/10", "3H"), ("86/10", "3H"), ("11/14", "1C"), ("53/14", "1C"), ("30/18", "2A"))] + [("", "", "cross", None, "", "DIS"), ("", "", "cross", None, "", "DIS"), ("3(DR)", "3E", "ss", "regiment", "3-10", "2-10")],
    [("Game|Turn", "", "gt", None, "", "Game|Turn")] + [(d, c, "mot", "regiment", "3-10", "2-10") for d, c in (("51/18", "2A"), ("76/20", "1C"), ("90/20", "1C"), ("15/29", "3E"), ("71/29", "3E"))] + [("", "", "cross", None, "", "DIS"), ("", "", "cross", None, "", "DIS"), ("", "", "cross", None, "", "DIS"), ("2(GR)", "3E", "ss", "regiment", "3-10", "2-10")],
    [("", "", "boom", None, "", "Out of|Supply")] * 3 + [("DIS", "", "dis", None, "", "Out of|Supply")] * 2 + [("DIS", "", "dis", None, "", "RAIL|CUT")] * 3 + [("2Mos", "", "sinf", "division", "0-0-6", "U-6"), ("7Mos", "", "sinf", "division", "0-0-6", "U-6")],
    [("", "", "boom", None, "", "Out of|Supply")] * 3 + [("DIS", "", "dis", None, "", "Out of|Supply")] * 2 + [("DIS", "", "dis", None, "", "RAIL|CUT")] * 3 + [("", "", "sair", None, "", "Air|Interdiction"), ("296", "", "sinf", "division", "0-0-6", "U-6")],
]

ICON = {"inf": "infantry", "arm": "armour", "mot": "mechanized", "cav": "cavalry", "sinf": "infantry"}


def slug(s):
    return re.sub(r"[^A-Za-z0-9]+", "-", s).strip("-").lower() or "x"


def counter(cid, cell, side):
    desig, code, icon, ech, val, bval = cell
    style = {"1": "soviet", "2": "german"}[side]
    fam = "unit"
    f = []
    b = []
    if icon == "hq":
        f.append('      <text slot="TC" size="small" weight="bold">%s</text>' % desig)
        f.append('      <text slot="UR" size="small">%s</text>' % code)
        f.append('      <value size="medium">%s</value>' % val)
        b.append('      <emblem kind="star" color="ink"/>')
    elif icon in ("air", "sair"):
        fam = "support"
        st = "soviet" if icon == "sair" else "german"
        f.append('      <silhouette kind="aircraft" color="ink"/>')
        return cid, fam, st, f, ['      <text slot="CENTRE" size="small" color="white">%s</text>' % bval], st + "-back"
    elif icon == "cross":
        fam = "marker"
        f.append('      <emblem kind="balkenkreuz" color="ink" color2="white"/>')
        return cid, fam, "german", f, ['      <text slot="CENTRE" size="medium" weight="bold">%s</text>' % bval], "plain"
    elif icon == "gt":
        fam = "marker"
        f.append('      <text slot="CENTRE" size="medium" weight="bold" color="white">%s</text>' % desig)
        return cid, fam, "redmarker", f, ['      <text slot="CENTRE" size="medium" weight="bold">%s</text>' % bval], "plain"
    elif icon == "boom":
        fam = "marker"
        f.append('      <emblem kind="cloud" color="ink"/>')
        return cid, fam, "plain", f, ['      <text slot="CENTRE" size="small" weight="bold">%s</text>' % bval], "plain"
    elif icon == "dis":
        fam = "marker"
        f.append('      <text slot="CENTRE" size="medium" weight="bold">DIS</text>')
        return cid, fam, "plain", f, ['      <text slot="CENTRE" size="small" weight="bold">%s</text>' % bval], "plain"
    else:
        st = "ss" if icon == "ss" else "soviet" if icon == "sinf" else style
        ic = ICON.get(icon, "infantry")
        f.append('      <symbol icon="%s"/>' % ic)
        if ech:
            f.append('      <echelon level="%s"/>' % ech)
        if desig:
            f.append('      <text slot="ML" rotate="-90" size="small">%s</text>' % desig)
        if code:
            f.append('      <text slot="UR" size="small">%s</text>' % code)
        f.append('      <value>%s</value>' % val)
        if bval.startswith("U"):
            b.append('      <symbol icon="%s"/>' % ic)
            b.append('      <echelon level="%s"/>' % ech)
            b.append('      <value>%s</value>' % bval)
            return cid, fam, st, f, b, "soviet-back"
        b.append('      <symbol icon="%s"/>' % ic)
        if ech:
            b.append('      <echelon level="%s"/>' % ech)
        if desig:
            b.append('      <text slot="ML" rotate="-90" size="small">%s</text>' % desig)
        if code:
            b.append('      <text slot="UR" size="small">%s</text>' % code)
        b.append('      <value>%s</value>' % bval)
        return cid, fam, st, f, b, st
    return cid, fam, style, f, b, "soviet-back"


x = ['<?xml version="1.0" encoding="UTF-8"?>',
     '<counters xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="hexcounters.xsd"',
     '          id="pgg" title="Panzergruppe Guderian counter sections 1/2 and 2/2" size="12.7" corner="0.04"',
     '          source="PGG_Countersheet_1-2_new.pdf, PGG_Countersheet_2-2_new.pdf; front left, mirrored back right">',
     '''  <palette>
    <color id="ink" value="#111111"/>
    <color id="white" value="#ffffff"/>
    <color id="sov" value="#e21414" name="Soviet"/>
    <color id="sov-back" value="#f28c8c" name="Soviet untried"/>
    <color id="ger" value="#9aa88c" name="German"/>
    <color id="ger-back" value="#8a9880"/>
    <color id="ss-ground" value="#000000" name="Waffen-SS"/>
    <color id="mk-red" value="#e21414"/>
  </palette>
  <styles>
    <style id="soviet" ground="sov" text="ink" box-stroke="ink"/>
    <style id="soviet-back" ground="sov-back" text="ink" box-stroke="ink"/>
    <style id="german" ground="ger" text="ink" box-stroke="ink"/>
    <style id="german-back" ground="ger-back" text="white" box-stroke="ink"/>
    <style id="ss" ground="ss-ground" text="white" box-stroke="white"/>
    <style id="plain" ground="white" text="ink" box-stroke="ink"/>
    <style id="redmarker" ground="mk-red" text="white" box-stroke="white"/>
  </styles>''']
sheets = []
seen = set()
for side, block in (("1", S1), ("2", S2)):
    order = []
    for row in block:
        for cell in row:
            if cell is None:
                order.append(None)
                continue
            cid = "s%s-%s" % (side, slug((cell[0] or cell[2]) + "-" + cell[2] + "-" + cell[4]))
            base, n = cid, 2
            while cid in seen:
                cid = "%s-%d" % (base, n); n += 1
            seen.add(cid)
            order.append(cid)
            cid, fam, fst, f, b, bst = counter(cid, cell, side)
            x.append('  <counter id="%s" family="%s" name="%s">' % (cid, fam, (cell[0] or cell[2]).replace("|", " ")))
            x.append('    <front style="%s">' % fst)
            x.extend(f)
            x.append('    </front>')
            x.append('    <back style="%s">' % bst)
            x.extend(b)
            x.append('    </back>')
            x.append('  </counter>')
    sheets.append((side, order, len(block)))
for side, order, rows in sheets:
    x.append('  <sheet id="section-%s" title="Panzer Gruppe Guderian Counter Section - Nr. %s/2" cols="10" rows="%d" gutter="0.5" margin="8" mirror="horizontal">' % (side, side, rows))
    for cid in order:
        x.append('    <place counter="%s"/>' % cid if cid else '    <place blank="true"/>')
    x.append('  </sheet>')
x.append('</counters>')
open(OUT, "w", encoding="utf-8").write("\n".join(x))
print("wrote", OUT, len(seen), "counters")
