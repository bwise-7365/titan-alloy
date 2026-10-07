# Copyright Ben Paul Wise. All Rights Reserved.
"""build_cd.py -- Circling Dragons (China 1944-45): the preliminary counter set.

Writes ../circling-dragons.xml in the hexcounters language.  The counts follow the subsection "Piece
scale and counts (preliminary)" of Circling Dragons/circling_dragons_map_and_rules_V3.tex, sized for the
75 km sheet (Ben, 2026-10-06: about 250 counters in all): about 126 unit pieces (Japan 51, the KMT 35, the
CCP 23, the Soviets 11, the puppets 6) and about 124 markers.  Two-state markers are double-sided, so one
counter carries both states: presence / base area, KMT / CCP control, rail interdicted / broken, port
denied / open, airfield captured / destroyed, and the front a Japanese formation faces.  Every value line
is a placeholder (strength-movement) for the prototype to revise.

Two sheets of 12 x 11 at 5/8 inch: sheet-1 Japan, the Soviets, the puppets and the US; sheet-2 the KMT,
the CCP and the neutral markers.  Render with ../counters2svg.py circling-dragons.xml --png.
"""
import os
from xml.sax.saxutils import escape

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "circling-dragons.xml")

x = ['<?xml version="1.0" encoding="UTF-8"?>',
     '<!-- Copyright Ben Paul Wise. All Rights Reserved. -->',
     '<counters xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="hexcounters.xsd"',
     '          id="circling-dragons" title="Circling Dragons: China 1944-1945 -- preliminary counter set" size="15.875" corner="0.05"',
     '          source="designed set, no original; counts from the design memorandum (preliminary)">',
     '''  <palette>
    <color id="ink" value="#111111"/>
    <color id="white" value="#ffffff"/>
    <color id="jp" value="#e8542a" name="China Expeditionary Army"/>
    <color id="kwa" value="#c98a2b" name="Kwantung Army"/>
    <color id="kmtblue" value="#2f5fa8" name="Kuomintang"/>
    <color id="ccpred" value="#c81e1e" name="Chinese Communist Party"/>
    <color id="pup" value="#f2c9a0" name="Nanjing regime"/>
    <color id="man" value="#7a8a40" name="Manchukuo"/>
    <color id="mj" value="#d9c48a" name="Mengjiang"/>
    <color id="sov" value="#8a5a2a" name="Soviet Union"/>
    <color id="usg" value="#9aae7a" name="United States"/>
    <color id="gold" value="#f0c020" name="US-equipped"/>
    <color id="grey" value="#c8c8c8" name="regional troops"/>
    <color id="orange" value="#f08020"/>
    <color id="black" value="#000000"/>
    <color id="lightblue" value="#7fa7cf"/>
  </palette>
  <styles>
    <style id="japan" ground="jp" text="ink" box-stroke="ink"/>
    <style id="kwantung" ground="kwa" text="ink" box-stroke="ink"/>
    <style id="nationalist" ground="kmtblue" text="white" box-stroke="ink" box-fill="white"/>
    <style id="communist" ground="ccpred" text="white" box-stroke="ink" box-fill="white"/>
    <style id="puppet" ground="pup" text="ink" box-stroke="ink"/>
    <style id="manchukuo" ground="man" text="ink" box-stroke="ink"/>
    <style id="mengjiang" ground="mj" text="ink" box-stroke="ink"/>
    <style id="soviet" ground="sov" text="white" box-stroke="white"/>
    <style id="usa" ground="usg" text="ink" box-stroke="ink"/>
    <style id="plain" ground="white" text="ink" box-stroke="ink"/>
  </styles>''']
sheets = {"sheet-1": [], "sheet-2": []}
seen = set()


def add(sheet, cid, fam, name, front, back=None, style="plain", count=1, front_attrs=""):
    """One counter record; the sheet remembers it with its copy count."""
    assert cid not in seen, cid
    seen.add(cid)
    x.append('  <counter id="%s" family="%s" name="%s"%s>' % (cid, fam, escape(name), ' count="%d"' % count if 1 < count else ""))
    x.append('    <front style="%s"%s>' % (style, front_attrs))
    x.append(front)
    x.append('    </front>')
    x.append(back if back else '    <back derived="same"/>')
    x.append('  </counter>')
    sheets[sheet].append((cid, count))


def unit(sheet, cid, style, name, icon, ech, steps, val, ul=None, tc=None, mr=None, lr=None, fill=None, mods=None,
         box_text=None, reduced=None, count=1):
    """A ground piece: symbol box, echelon, step pips down the left, labels, the value line."""
    sym = '      <symbol icon="%s"' % icon
    if mods:
        sym += ' modifiers="%s"' % mods
    if fill:
        sym += ' fill="%s"' % fill
    if box_text:
        sym += ' text="%s"' % escape(box_text)
    f = [sym + "/>"]
    if ech:
        f.append('      <echelon level="%s"/>' % ech)
    if steps:
        f.append('      <steps count="%d" mark="dot"/>' % steps)
    if ul:
        f.append('      <text slot="UL" size="small" weight="bold">%s</text>' % escape(ul))
    if tc:
        f.append('      <text slot="TC" size="small">%s</text>' % escape(tc))
    if mr:
        f.append('      <text slot="MR" rotate="90" size="small">%s</text>' % escape(mr))
    if lr:
        f.append('      <text slot="LR" size="small">%s</text>' % escape(lr))
    f.append('      <value>%s</value>' % val)
    back = None
    if reduced:
        bsteps, bval = reduced
        back = '    <back derived="reduced"><steps count="%d" mark="dot"/><value>%s</value></back>' % (bsteps, bval)
    add(sheet, cid, "unit", name, "\n".join(f), back, style, count)


def marker(sheet, cid, style, name, body, count=1, attrs="", fam="marker"):
    add(sheet, cid, fam, name, "      " + body, None, style, count, attrs)


def centre_text(s, size="medium"):
    return '<text slot="CENTRE" size="%s" weight="bold">%s</text>' % (size, escape(s))


def bottom(s):
    return '<text slot="BOTTOM" size="small" weight="bold">%s</text>' % escape(s)


def band(s, color="black", text_color="white"):
    return '<band color="%s" text-color="%s">%s</band>' % (color, text_color, escape(s))


def flip(sheet, cid, name, front_style, front_body, back_style, back_body, count=1, front_attrs="", back_attrs=""):
    """A two-state marker: the state changes by turning the counter over."""
    back = '    <back style="%s"%s>\n      %s\n    </back>' % (back_style, back_attrs, back_body)
    add(sheet, cid, "marker", name, "      " + front_body, back, front_style, count, front_attrs)


# ============================================================================ sheet 1: Japan
S1 = "sheet-1"
# China Expeditionary Army: one piece per division (the 1944 order of battle, armies as of spring 1944)
CEA = [("12A", (110, 62, 37)), ("1A", (69, 114)), ("NCAA", (63, 59, 35)), ("MGA", (26,)),
       ("11A", (3, 13, 27, 34, 39, 40, 58, 68, 116)), ("13A", (60, 65, 70)), ("23A", (104, 22))]
for army, divs in CEA:
    for d in divs:
        unit(S1, "jp-d%d" % d, "japan", "Japanese %d Division (%s)" % (d, army), "infantry", "division", 3, "3-4",
             ul=str(d), mr=army, reduced=(1, "1-4"))
unit(S1, "jp-tk3", "japan", "Japanese 3 Tank Division (12A)", "armour", "division", 3, "4-6", ul="3", mr="12A", reduced=(1, "2-6"))
# independent mixed brigades in the field, two brigades to a piece
for cid, army in (("jp-imb-nc1", "NCAA"), ("jp-imb-nc2", "NCAA"), ("jp-imb-11a", "11A"), ("jp-imb-13a", "13A")):
    unit(S1, cid, "japan", "Japanese independent mixed brigades (%s)" % army, "infantry", "brigade", 1, "1-3", ul="IMB", mr=army)
# one-step garrison pieces, the independent mixed brigades on the railway zones
for zone in ("Pinghan", "Jinpu", "Longhai", "Jingsui", "Tongpu", "Yangtze", "Canton"):
    unit(S1, "jp-g-%s" % zone.lower(), "japan", "Japanese railway garrison, %s" % zone, "garrison", "brigade", 1, "1-2", ul="IMB", tc=zone)
# headquarters: the area army a Japanese formation answers to decides the front it faces
for cid, ul, tc, name in (("jp-hq-cea", "CEA", "Nanjing", "China Expeditionary Army"), ("jp-hq-ncaa", "NCAA", "Peiping", "North China Area Army"),
                          ("jp-hq-6aa", "6 AA", "Hankow", "6 Area Army")):
    unit(S1, cid, "japan", "Japanese headquarters, " + name, "hq", "army-group", 0, "0-4", ul=ul, tc=tc)
# Kwantung Army, 1945: five armies, two pieces each
for num, where, where2 in ((3, "Dongning", "Hunchun"), (5, "Mudanjiang", "Linkou"), (4, "Sunwu", "Hailar"),
                           (30, "Changchun", "Meihekou"), (44, "Solun", "Liaoyuan")):
    unit(S1, "kw-%da" % num, "kwantung", "Kwantung Army, %d Army (%s)" % (num, where), "infantry", "corps", 2, "2-4",
         ul="%d A" % num, tc=where, reduced=(1, "1-4"))
    unit(S1, "kw-%da-2" % num, "kwantung", "Kwantung Army, %d Army (%s)" % (num, where2), "infantry", "corps", 2, "2-4",
         ul="%d A" % num, tc=where2, reduced=(1, "1-4"))
# Korea: 17 Area Army
unit(S1, "kr-34a", "japan", "Korea, 34 Army (north)", "infantry", "army", 2, "2-3", ul="34 A", tc="Korea N", reduced=(1, "1-3"))
unit(S1, "kr-58a", "japan", "Korea, 58 Army (south)", "infantry", "army", 2, "2-3", ul="58 A", tc="Korea S", reduced=(1, "1-3"))
unit(S1, "kr-17aa", "japan", "Korea, 17 Area Army reserve (Seoul)", "infantry", "army", 2, "2-3", ul="17 AA", tc="Seoul", reduced=(1, "1-3"))
# air support from the shared pool
marker(S1, "jp-air", "japan", "Japanese 5 Air Army support",
       '<text slot="UL" size="small" weight="bold">5 AA</text><glyph kind="range" slot="UR" text="3"/><silhouette kind="aircraft" color="ink"/>'
       + band("Air support"), count=2, fam="support")
# Japanese markers
marker(S1, "jp-init-kmt", "japan", "Initiative, Japanese forces facing the KMT",
       '<emblem kind="hexagon" color="ink" scale="0.8"/>' + bottom("Initiative|J facing KMT"), attrs=' split="kmtblue"')
marker(S1, "jp-init-ccp", "japan", "Initiative, Japanese forces facing the CCP",
       '<emblem kind="hexagon" color="ink" scale="0.8"/>' + bottom("Initiative|J facing CCP"), attrs=' split="ccpred"')
for cid, s in (("jp-dir-ichigo1", "Ichi-Go I|Henan"), ("jp-dir-ichigo2", "Ichi-Go II|Hunan"), ("jp-dir-ichigo3", "Ichi-Go III|Guangxi"),
               ("jp-dir-airfields", "Airfield|denial"), ("jp-dir-coast", "Coastal|defense"), ("jp-dir-hold", "Hold for|the KMT")):
    marker(S1, cid, "japan", "Japanese strategic directive: " + s.replace("|", " "), centre_text(s, "small") + band("Directive"))
flip(S1, "jp-front", "Japanese formation's front: facing the KMT (CCP player controls) / facing the CCP (KMT player controls)",
     "japan", centre_text("facing|KMT", "small") + band("Control"), "japan", centre_text("facing|CCP", "small") + band("Control"),
     count=3, front_attrs=' split="kmtblue"', back_attrs=' split="ccpred"')
for cid, s in (("jp-pool-repl", "Replace-|ments"), ("jp-pool-ops", "Operational|support"), ("jp-pool-rail", "Rail|capacity"), ("jp-pool-log", "Logistics")):
    marker(S1, cid, "japan", "Japanese shared pool: " + s.replace("|", "").replace("-", ""), '<emblem kind="pennant" color="ink" color2="white" scale="0.8"/>' + bottom(s))
marker(S1, "jp-security", "japan", "Japanese security zone (blockhouse line)", '<symbol icon="garrison"/>' + band("Security zone"), count=3)
marker(S1, "jp-surrendered", "japan", "Surrendered Japanese garrison", '<symbol icon="garrison"/>' + band("Surrendered", "white", "ink"), count=3)
for name in ("Hailar", "Aihui", "Sunwu", "Arshaan", "Hutou", "Dongning"):
    marker(S1, "kw-fz-%s" % name.lower(), "kwantung", "Kwantung fortified zone, " + name,
           '<symbol icon="text" text="Fort"/><text slot="TC" size="small" weight="bold">%s</text>' % name + bottom("fortified zone"))
marker(S1, "jp-river", "japan", "Japanese river transport (Yangtze, Sungari)", '<silhouette kind="ship" color="ink" scale="0.8"/>' + bottom("River|transport"), count=2)
for name in ("Kalgan", "Mukden", "Changchun", "Harbin"):
    marker(S1, "jp-depot-%s" % name.lower(), "japan", "Japanese depot and arms dump, " + name,
           '<emblem kind="pennant" color="ink" color2="gold" scale="0.8"/>' + bottom("Depot|" + name))
flip(S1, "mk-airfield", "Airfield captured / destroyed",
     "plain", '<silhouette kind="aircraft" color="ink" scale="0.7"/>' + band("Captured", "orange", "ink"),
     "plain", '<silhouette kind="aircraft" color="ink" scale="0.7"/>' + band("Destroyed"), count=3)

# ============================================================================ sheet 1: Soviets
SOV = [("sov-6gta", "6 Gds Tank Army", "armour", 4, "6-8", (2, "3-8")), ("sov-39a", "39 Army", "infantry", 4, "4-5", (2, "2-5")),
       ("sov-36a", "36 Army", "infantry", 3, "3-5", (1, "1-5")), ("sov-17a", "17 Army", "infantry", 3, "3-5", (1, "1-5")),
       ("sov-pliyev", "Pliyev CMG", "cav-mech", 3, "3-8", (1, "1-8")),
       ("sov-1rb", "1 Red Banner", "infantry", 4, "4-5", (2, "2-5")), ("sov-5a", "5 Army", "infantry", 4, "4-5", (2, "2-5")),
       ("sov-25a", "25 Army", "infantry", 3, "3-5", (1, "1-5")), ("sov-35a", "35 Army", "infantry", 3, "3-5", (1, "1-5")),
       ("sov-2rb", "2 Red Banner", "infantry", 3, "3-5", (1, "1-5")), ("sov-15a", "15 Army", "infantry", 3, "3-5", (1, "1-5"))]
for cid, name, icon, steps, val, red in SOV:
    unit(S1, cid, "soviet", "Soviet grouping, " + name, icon, "army", steps, val, tc=name,
         mr="Sov-Mong" if "Pliyev" in name else None, lr="Flotilla" if "15" in name else None, reduced=red)
marker(S1, "sov-airborne", "soviet", "Soviet airborne detachment", '<emblem kind="parachute" color="white" scale="0.8"/>' + bottom("Airborne|detachment"), count=3)
marker(S1, "sov-directive", "soviet", "Soviet directive (axis choice)", centre_text("Soviet|directive"))
marker(S1, "sov-withdrawal", "soviet", "Soviet withdrawal track marker", centre_text("Withdrawal|track"))
marker(S1, "sov-fuel", "soviet", "Soviet fuel airlift pool", '<silhouette kind="aircraft" color="white" scale="0.7"/>' + bottom("Fuel|airlift"))
marker(S1, "sov-occupied", "soviet", "Soviet occupation", '<emblem kind="star" color="white" scale="0.8"/>' + bottom("Occupied"), count=5)

# ============================================================================ sheet 1: puppets and the US
for n in (1, 2, 3):
    unit(S1, "pup-nanjing%d" % n, "puppet", "Nanjing regime, %d Army" % n, "garrison", "corps", 1, "1-2", tc="Nanjing %d Army" % n)
for n in (1, 2):
    unit(S1, "pup-manchukuo%d" % n, "manchukuo", "Manchukuo Army %d" % n, "garrison", "corps", 1, "1-2", tc="Manchukuo Army")
unit(S1, "pup-mengjiang", "mengjiang", "Mengjiang Army", "cavalry", "corps", 1, "1-3", tc="Mengjiang Army")
marker(S1, "us-14af", "usa", "US Fourteenth Air Force support",
       '<text slot="UL" size="small" weight="bold">14 AF</text><glyph kind="range" slot="UR" text="4"/><silhouette kind="aircraft" color="ink"/>'
       + band("Air support"), count=2, fam="support")
marker(S1, "us-cacw", "usa", "Chinese-American Composite Wing support",
       '<text slot="UL" size="small" weight="bold">CACW</text><glyph kind="range" slot="UR" text="3"/><silhouette kind="aircraft" color="ink"/>'
       + band("Air support"), fam="support")
marker(S1, "us-b29", "usa", "B-29 bases at Chengdu", '<silhouette kind="bomber" color="ink" scale="0.8"/>' + bottom("B-29|Chengdu"))
marker(S1, "us-lift", "usa", "US Strategic Lift", '<silhouette kind="ship" color="ink" scale="0.8"/>' + bottom("Strategic|Lift"))
marker(S1, "us-hump", "usa", "Hump tonnage and the Ledo Road", '<silhouette kind="aircraft" color="ink" scale="0.7"/>' + bottom("Hump|tonnage"))
marker(S1, "us-mission", "usa", "US military mission", centre_text("US|mission"))
for name in ("Tianjin", "Qingdao", "Qinhuangdao"):
    marker(S1, "us-marines-%s" % name.lower(), "usa", "US Marines holding the port of " + name,
           '<text slot="TC" size="small" weight="bold">US Marines</text><emblem kind="anchor" color="ink" scale="0.7"/>' + bottom(name))

# ============================================================================ sheet 2: KMT
S2 = "sheet-2"
GA = [(15, "1 WA", None), (19, "1 WA", None), (28, "1 WA", None), (31, "1 WA", None), (36, "1 WA", None),
      (34, "8 WA", None), (37, "8 WA", None), (38, "8 WA", None),
      (6, "2 WA", "Shanxi"), (22, "5 WA", None), (33, "5 WA", None), (10, "6 WA", None), (26, "6 WA", None),
      (24, "9 WA", None), (27, "9 WA", None), (30, "9 WA", None), (16, "4 WA", "Guangxi"), (35, "4 WA", "Guangxi"),
      (23, "3 WA", None), (25, "3 WA", None), (32, "3 WA", None), (12, "7 WA", "Guangdong"), (21, "10 WA", "Guangxi")]
for num, wa, region in GA:
    if region:
        unit(S2, "kmt-ga%d" % num, "nationalist", "KMT %d Group Army, %s (regional troops)" % (num, region), "infantry", "army", 2, "1-3",
             ul="%d GA" % num, tc=region, mr=wa, fill="grey", reduced=(1, "1-3"))
    else:
        unit(S2, "kmt-ga%d" % num, "nationalist", "KMT %d Group Army, %s" % (num, wa), "infantry", "army", 3, "2-3",
             ul="%d GA" % num, mr=wa, reduced=(1, "1-3"))
unit(S2, "kmt-fu", "nationalist", "KMT 35 Army, Fu Zuoyi (Suiyuan)", "infantry", "army", 2, "1-3", ul="35 A", tc="Suiyuan", mr="12 WA", fill="grey", reduced=(1, "1-3"))
# US-equipped armies: two back from Burma, three of the Alpha divisions that went north in 1945
for cid, ul, name in (("kmt-n1a", "New 1", "New 1 Army"), ("kmt-n6a", "New 6", "New 6 Army"), ("kmt-13a", "13 A", "13 Army"),
                      ("kmt-52a", "52 A", "52 Army"), ("kmt-94a", "94 A", "94 Army")):
    unit(S2, cid, "nationalist", "KMT %s (US-equipped)" % name, "infantry", "army", 3, "4-4", ul=ul, tc="US-equipped", fill="gold", reduced=(1, "2-4"))
unit(S2, "kmt-ga11", "nationalist", "KMT 11 Group Army, Y-Force (returns 1945)", "infantry", "army", 3, "3-3", ul="11 GA", tc="Y-Force", fill="gold", reduced=(1, "1-3"))
unit(S2, "kmt-ga20", "nationalist", "KMT 20 Group Army, Y-Force (returns 1945)", "infantry", "army", 3, "3-3", ul="20 GA", tc="Y-Force", fill="gold", reduced=(1, "1-3"))
unit(S2, "kmt-ghq", "nationalist", "KMT National Military Council (Chongqing)", "hq", "high-command", 0, "0-4", ul="GHQ", tc="Chongqing")
unit(S2, "kmt-hq1", "nationalist", "KMT 1 War Area headquarters", "hq", "army-group", 0, "0-4", ul="1", tc="War Area")
unit(S2, "kmt-hq9", "nationalist", "KMT 9 War Area headquarters", "hq", "army-group", 0, "0-4", ul="9", tc="War Area")
unit(S2, "kmt-hqalpha", "nationalist", "Alpha Force headquarters (Kunming)", "hq", "army-group", 0, "0-4", ul="A", tc="Alpha HQ")
marker(S2, "kmt-init", "nationalist", "Initiative, KMT", '<emblem kind="hexagon" color="white" scale="0.8"/>' + bottom("Initiative|KMT"))
marker(S2, "kmt-legit", "nationalist", "Legitimacy, KMT", centre_text("Legitimacy"))
marker(S2, "kmt-position", "nationalist", "Postwar Position, KMT", centre_text("Postwar|Position"))
marker(S2, "kmt-fort", "nationalist", "KMT fortified city", '<symbol icon="text" text="Fort"/>' + bottom("Fortified|city"), count=2)

# ============================================================================ sheet 2: CCP
FIELD = [("ccp-sgn", "Shaan-Gan-Ning", "8RA"), ("ccp-js", "Jin-Sui", "8RA"), ("ccp-jcj", "Jin-Cha-Ji", "8RA"), ("ccp-jjly", "Jin-Ji-Lu-Yu", "8RA"),
         ("ccp-jjly2", "Jin-Ji-Lu-Yu", "8RA"), ("ccp-sd", "Shandong", "8RA"), ("ccp-sd2", "Shandong Binhai", "8RA"), ("ccp-jrl", "Ji-Re-Liao", "8RA"),
         ("ccp-n4a1", "N4A 1 Div", "N4A"), ("ccp-n4a2", "N4A 2 Div", "N4A"), ("ccp-n4a3", "N4A 3 Div", "N4A"), ("ccp-n4a4", "N4A 4 Div", "N4A"),
         ("ccp-n4a5", "N4A 5 Div", "N4A"), ("ccp-n4a6", "N4A 6 Div", "N4A"), ("ccp-n4a7", "N4A 7 Div", "N4A"), ("ccp-dj", "Dongjiang Col.", "SC")]
for cid, name, ul in FIELD:
    unit(S2, cid, "communist", "CCP field force, " + name, "infantry", "corps", 2, "2-4", ul=ul, tc=name, reduced=(1, "1-4"))
for n in range(1, 6):
    unit(S2, "ccp-ne%d" % n, "communist", "CCP Northeast column %d (formed in play)" % n, "infantry", "corps", 2, "2-4", ul="NE", tc="Column %d" % n, reduced=(1, "1-4"))
unit(S2, "ccp-hq", "communist", "CCP headquarters, Yan'an", "hq", "army-group", 0, "0-4", tc="Yan'an")
unit(S2, "ccp-hq-ne", "communist", "CCP Northeast Bureau (1945)", "hq", "army-group", 0, "0-4", ul="NE", tc="NE Bureau")
flip(S2, "ccp-presence", "CCP presence / base area",
     "communist", '<symbol icon="partisan"/>' + bottom("Presence"),
     "communist", '<symbol icon="partisan" fill="gold"/>' + band("Base area"), count=24)
marker(S2, "ccp-init", "communist", "Initiative, CCP", '<emblem kind="hexagon" color="white" scale="0.8"/>' + bottom("Initiative|CCP"))
marker(S2, "ccp-legit", "communist", "Legitimacy, CCP", centre_text("Legitimacy"))
marker(S2, "ccp-position", "communist", "Postwar Position, CCP", centre_text("Postwar|Position"))
marker(S2, "ccp-concentrate", "communist", "Presence concentrating into a field force", centre_text("Concen-|trate"), count=2)
marker(S2, "ccp-junks", "communist", "Bohai junk crossing", '<silhouette kind="ship" color="white" scale="0.8"/>' + bottom("Bohai|crossing"))

# ============================================================================ sheet 2: neutral markers
marker(S2, "mk-calendar", "plain", "Calendar", centre_text("Calendar"))
marker(S2, "mk-weather", "plain", "Weather", '<emblem kind="cloud" color="lightblue" scale="0.9"/>' + bottom("Weather"))
marker(S2, "mk-pacific", "plain", "Pacific War track", '<emblem kind="star" color="ink" scale="0.8"/>' + bottom("Pacific|War"))
flip(S2, "mk-control", "Control: KMT / CCP",
     "nationalist", '<emblem kind="flag" color="white" color2="white" scale="0.8"/>' + bottom("Control"),
     "communist", '<emblem kind="flag" color="white" color2="white" scale="0.8"/>' + bottom("Control"), count=8)
flip(S2, "mk-rail", "Rail segment interdicted / broken",
     "plain", '<silhouette kind="locomotive" color="ink" scale="0.8"/>' + band("Interdicted", "orange", "ink"),
     "plain", '<silhouette kind="locomotive" color="ink" scale="0.8"/>' + band("Broken"), count=10)
marker(S2, "mk-bridge-yellow", "plain", "Yellow River rail bridge", centre_text("Yellow River|bridge", "small") + band("Bridge"))
marker(S2, "mk-ferry-yangtze", "plain", "Yangtze ferry", centre_text("Yangtze|ferry", "small") + band("Ferry"), count=2)
flip(S2, "mk-port", "Port denied / open",
     "plain", '<emblem kind="anchor" color="ink" scale="0.7"/>' + bottom("Port|denied"),
     "plain", '<emblem kind="anchor" color="ink" scale="0.7"/>' + bottom("Port|open"), count=3)
marker(S2, "mk-oos", "plain", "Out of supply", centre_text("Out of|supply"), count=2)
marker(S2, "mk-halted", "plain", "Halted (fuel or water)", centre_text("Halted"))

# ============================================================================ sheets
COLS, ROWS = 12, 11
titles = {"sheet-1": "Circling Dragons, sheet 1: Japan, the Soviet Union, the puppets, the United States",
          "sheet-2": "Circling Dragons, sheet 2: the KMT, the CCP, neutral markers"}
total = 0
for sid in ("sheet-1", "sheet-2"):
    n = sum(c for _, c in sheets[sid])
    assert n <= COLS * ROWS, (sid, n)
    total += n
    x.append('  <sheet id="%s" title="%s" cols="%d" rows="%d" gutter="0.5" margin="8" mirror="horizontal">' % (sid, escape(titles[sid]), COLS, ROWS))
    for cid, c in sheets[sid]:
        x.append('    <place counter="%s"%s/>' % (cid, ' repeat="%d"' % c if 1 < c else ""))
    x.append('  </sheet>')
x.append('</counters>')
x.append('<!-- Copyright Ben Paul Wise. All Rights Reserved. -->')
open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(x) + "\n")
print("wrote", os.path.normpath(OUT), len(seen), "records,", total, "counters on", len(sheets), "sheets")
# Copyright Ben Paul Wise. All Rights Reserved.
