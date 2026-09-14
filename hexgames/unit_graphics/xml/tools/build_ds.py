# Copyright Ben Paul Wise. All Rights Reserved.
"""build_ds.py -- Axis Empires: Dai Senso!  A representative set covering every counter family and every
presentation device in the inventory: ground units of major and minor countries, support units whose
backs are other counters, and the operational and political markers."""
import re

OUT = r"C:\repos\ghub-per\titan-alloy\hexgames\unit_graphics\xml\dai-senso.xml"


def slug(s):
    return re.sub(r"[^A-Za-z0-9]+", "-", s).strip("-").lower()


x = ['<?xml version="1.0" encoding="UTF-8"?>',
     '<counters xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="hexcounters.xsd"',
     '          id="dai-senso" title="Axis Empires: Dai Senso! -- representative counters" size="12.7" corner="0.05"',
     '          source="pic1753886.png, pic1753887.png (2011 fronts); Living Rules Feb 2014 pp. 6-8 for anatomy and backs">',
     '''  <palette>
    <color id="ink" value="#111111"/>
    <color id="white" value="#ffffff"/>
    <color id="jp" value="#e8542a" name="Japan"/>
    <color id="kwa" value="#a8202a" name="Kwantung Army"/>
    <color id="ru" value="#8a5a2a" name="Russia"/>
    <color id="gb" value="#d2b48c" name="Great Britain and Commonwealth"/>
    <color id="us" value="#9aae7a" name="United States"/>
    <color id="nat" value="#f0d040" name="Nationalist China"/>
    <color id="ccp" value="#d02020" name="Communist China"/>
    <color id="cyan" value="#40d0e0"/>
    <color id="siam" value="#8a5ab0" name="Siam"/>
    <color id="nei" value="#f09030" name="Netherlands East Indies"/>
    <color id="fra" value="#3050a0" name="France"/>
    <color id="man" value="#7a8a40" name="Manchukuo"/>
    <color id="phil" value="#f4f4f4" name="Philippines"/>
    <color id="green" value="#2a8a3a"/>
    <color id="red" value="#d02020"/>
    <color id="orange" value="#f08020"/>
    <color id="black" value="#000000"/>
    <color id="gold" value="#f0c020" name="elite box"/>
    <color id="grey" value="#9a9a9a"/>
    <color id="mud" value="#8a6a3a"/>
    <color id="storm" value="#4a5a8a"/>
    <color id="snow" value="#dde8f0"/>
  </palette>
  <styles>
    <style id="japan" ground="jp" text="ink" box-stroke="ink"/>
    <style id="kwantung" ground="kwa" text="white" box-stroke="white"/>
    <style id="russia" ground="ru" text="white" box-stroke="white"/>
    <style id="britain" ground="gb" text="ink" box-stroke="ink"/>
    <style id="usa" ground="us" text="ink" box-stroke="ink"/>
    <style id="nationalist" ground="nat" text="red" box-stroke="red"/>
    <style id="communist" ground="ccp" text="cyan" box-stroke="cyan"/>
    <style id="siamese" ground="siam" text="white" box-stroke="white"/>
    <style id="dutch" ground="nei" text="fra" box-stroke="fra"/>
    <style id="french" ground="fra" text="white" box-stroke="white"/>
    <style id="manchukuo" ground="man" text="ink" box-stroke="ink"/>
    <style id="philippine" ground="phil" text="green" box-stroke="green"/>
    <style id="plain" ground="white" text="ink" box-stroke="ink"/>
  </styles>''']
ids = []


def add(cid, fam, name, front, back, fstyle, bstyle=None):
    ids.append(cid)
    x.append('  <counter id="%s" family="%s" name="%s">' % (cid, fam, name))
    x.append('    <front style="%s">\n%s\n    </front>' % (fstyle, front))
    if back is None:
        x.append('    <back derived="same"/>')
    elif back.startswith("ref="):
        x.append('    <back %s/>' % back)
    elif back.startswith("derived="):
        x.append('    <back %s/>' % back)
    else:
        x.append('    <back style="%s">\n%s\n    </back>' % (bstyle or fstyle, back))
    x.append('  </counter>')


def ground(cid, style, icon, ech, steps, mark, val, code, hist, nat=None, delay=False, fill=None, mods=None, bval=None, bsteps=None):
    f = []
    f.append('      <symbol icon="%s"%s%s/>' % (icon, ' fill="%s"' % fill if fill else "", ' modifiers="%s"' % mods if mods else ""))
    if ech:
        f.append('      <echelon level="%s"/>' % ech)
    f.append('      <steps count="%d" mark="%s" slot="UR"/>' % (steps, mark))
    f.append('      <text slot="UL" size="small">%s</text>' % code)
    if hist:
        f.append('      <text slot="MR" rotate="90" size="small">%s</text>' % hist)
    if nat:
        f.append('      <text slot="ML" rotate="-90" size="small">%s</text>' % nat)
    if delay:
        f.append('      <band color="black" text-color="white"/>')
    f.append('      <value%s>%s</value>' % (' color="white"' if delay else "", val))
    back = None
    if bval:
        back = "derived=\"reduced\"><steps count=\"%d\" mark=\"%s\" slot=\"UR\"/><value%s>%s</value></back" % (bsteps, mark, ' color="white"' if delay else "", bval)
        # written verbatim below
    ids.append(cid)
    x.append('  <counter id="%s" family="unit" name="%s">' % (cid, cid))
    x.append('    <front style="%s">\n%s\n    </front>' % (style, "\n".join(f)))
    if bval:
        x.append('    <back derived="reduced"><steps count="%d" mark="%s" slot="UR"/><value%s>%s</value></back>' % (bsteps, mark, ' color="white"' if delay else "", bval))
    else:
        x.append('    <back derived="same"/>')
    x.append('  </counter>')


# ---- ground units
ground("jp-inf-16", "japan", "infantry", "army", 3, "dot", "7-6-2", "44", "16", bval="4-4-2", bsteps=2)
ground("jp-inf-2", "japan", "infantry", None, 1, "dot", "3-3-1", "A", "2")
ground("jp-guards", "japan", "infantry", None, 2, "dot", "4-4-1", "B", "Gds", fill="gold", bval="2-2-1", bsteps=1)
ground("jp-snlf", "japan", "infantry", None, 1, "dot", "2-2-1", "12", "SNLF", mods="marine")
ground("jp-armour", "japan", "armour", None, 2, "dot", "3-2-2", "18", "1", bval="1-1-2", bsteps=1)
ground("jp-airborne", "japan", "infantry", None, 1, "dot", "1-1-1", "27", "1Ab", mods="airborne")
ground("jp-fortress", "japan", "fortress", None, 3, "square", "0-3-0", "E", "Rab", delay=True)
ground("jp-garrison", "japan", "garrison", None, 1, "square", "0-1-1", "N", "Kor")
ground("kwa-hq", "kwantung", "hq-checker", "army", 3, "dot", "3-2-1", "44", "Kwa", nat="Kwa", delay=True, bval="1-1-1", bsteps=1)
ground("kwa-inf", "kwantung", "infantry", None, 1, "dot", "3-3-1", "44", "4", nat="Kwa")
ground("kwa-cav", "kwantung", "cavalry", None, 1, "dot", "1-1-2", "44", "1", nat="Kwa")
ground("ru-army", "russia", "infantry", "army", 3, "dot", "5-5-1", "3", "1FE", bval="3-3-1", bsteps=2)
ground("ru-mech", "russia", "mechanized", None, 2, "dot", "4-3-2", "ASR", "6", bval="2-1-2", bsteps=1)
ground("ru-cavmech", "russia", "cav-mech", None, 1, "dot", "2-1-3", "9", "KMG")
ground("gb-ind", "britain", "infantry", None, 1, "dot", "1-1-1", "5", "17", nat="Ind")
ground("gb-aus", "britain", "infantry", None, 2, "dot", "2-1-1", "2", "8", nat="Aus", fill="white", bval="1-1-1", bsteps=1)
ground("gb-nz", "britain", "infantry", None, 1, "dot", "1-1-1", "2", "Exp", nat="NZ")
ground("gb-hk", "britain", "garrison", None, 1, "square", "0-1-0", "E", "HK", nat="HK")
ground("us-army", "usa", "infantry", "army", 3, "dot", "7-7-2", "60", "6", bval="4-4-2", bsteps=2)
ground("us-marine", "usa", "infantry", None, 2, "dot", "3-2-2", "48", "1", mods="marine", bval="1-1-2", bsteps=1)
ground("us-arm", "usa", "armour", None, 1, "dot", "2-1-2", "55", "1", fill="gold")
ground("us-phil", "philippine", "infantry", None, 1, "dot", "1-1-1", "E", "Res", nat="Phil", fill="white")
ground("nat-warzone", "nationalist", "infantry", "army-group", 3, "dot", "3-4-1", "N", "5WZ", bval="1-2-1", bsteps=2)
ground("nat-inf", "nationalist", "infantry", None, 1, "dot", "1-2-1", "N", "Exp")
ground("nat-kan", "nationalist", "infantry", None, 1, "dot", "1-1-1", "A", "Res", nat="Kan")
ground("nat-yun", "nationalist", "infantry", None, 1, "dot", "1-1-1", "A", "Exp", nat="Yun")
ground("nat-sze", "nationalist", "infantry", None, 1, "dot", "1-2-1", "A", "Res", nat="Sze")
ground("ccp-inf", "communist", "infantry", None, 1, "dot", "1-2-1", "ASR", "8Rt")
ground("siam-inf", "siamese", "infantry", None, 1, "dot", "1-1-1", "C", "Exp")
ground("nei-inf", "dutch", "infantry", None, 1, "dot", "1-2-1", "E", "KNIL", nat="NEI", fill="white")
ground("fra-inf", "french", "infantry", None, 1, "dot", "1-1-1", "E", "Exp", nat="Fra", fill="white")
ground("man-cav", "manchukuo", "cavalry", None, 1, "dot", "1-1-2", "N", "Mong", nat="Mong")

# ---- support units: the back is another counter
add("jp-cv-strike", "support", "Japanese CV Strike",
    '      <text slot="UL" size="small">4</text><glyph kind="range" slot="UR" text="2"/><silhouette kind="aircraft" color="ink"/><band color="black" text-color="white">CV Strike</band>',
    "ref=\"jp-cv-fleet\"", "japan")
add("jp-cv-fleet", "support", "Japanese CV Fleet Kido Butai",
    '      <text slot="UL" size="small">4</text><text slot="TC" size="small">Kido Butai</text><silhouette kind="carrier" color="ink"/><band color="black" text-color="white">CV Fleet</band>',
    "ref=\"jp-cv-strike\"", "japan")
add("jp-airforce", "support", "Japanese Air Force",
    '      <text slot="UL" size="small">A</text><glyph kind="range" slot="UR" text="3"/><silhouette kind="aircraft" color="ink"/><text slot="MR" size="small">1</text><band color="black" text-color="white">Air Force</band>',
    "ref=\"jp-troop-convoy\"", "japan")
add("jp-troop-convoy", "marker", "Japanese Escort Troop Convoy",
    '      <emblem kind="anchor" color="ink" scale="0.7"/><text slot="BOTTOM" size="small" weight="bold">Escort|Troop Convoy</text>',
    "ref=\"jp-airforce\"", "japan")
add("us-bomber", "support", "US Bomber",
    '      <text slot="UL" size="small">57</text><glyph kind="range" slot="UR" text="4"/><silhouette kind="bomber" color="ink"/><glyph kind="arrow-down" slot="LL" color="ink"/><glyph kind="drm" slot="LR" color="green" text="-1"/><band color="black" text-color="white">Bomber</band>',
    '      <emblem kind="cloud" color="grey"/><text slot="BOTTOM" size="small" weight="bold">Devastation</text>', "usa")
add("us-interceptor", "support", "US Interceptor",
    '      <text slot="UL" size="small">57</text><glyph kind="range" slot="UR" text="2"/><silhouette kind="interceptor" color="ink"/><glyph kind="arrow-up" slot="LL" color="ink"/><band color="black" text-color="white">Interceptor</band>',
    None, "usa")
add("us-surf-fleet", "support", "US Surface Fleet TF 11/17",
    '      <text slot="UL" size="small">4</text><text slot="TC" size="small">TF 11/17</text><silhouette kind="ship" color="ink"/><band color="black" text-color="white">Surf Fleet</band>',
    '      <emblem kind="anchor" color="ink" scale="0.7"/><text slot="BOTTOM" size="small" weight="bold">Escort|Troop Convoy</text>', "usa")
add("us-sub-fleet", "support", "US Sub Fleet",
    '      <text slot="UL" size="small">B</text><silhouette kind="submarine" color="ink"/><glyph kind="arrow-down" slot="LL" color="ink"/><glyph kind="drm" slot="LR" color="black" text="+1"/><band color="black" text-color="white">Sub Fleet</band>',
    None, "usa")
add("ru-airforce", "support", "Russian Air Force",
    '      <text slot="UL" size="small">3</text><glyph kind="range" slot="UR" text="2"/><silhouette kind="aircraft" color="white"/><glyph kind="drm" slot="LR" color="red" text="-1"/><band color="black" text-color="white">Air Force</band>',
    None, "russia")

# ---- operational and political markers
add("jp-airdrop", "marker", "Japanese Airdrop", '      <emblem kind="parachute" color="ink"/><text slot="BOTTOM" size="small" weight="bold">Airdrop</text>', "ref=\"jp-airborne\"", "japan")
add("jp-detachment", "marker", "Japanese Detachment", '      <emblem kind="pennant" color="ink" color2="white"/><text slot="BOTTOM" size="small" weight="bold">Detachment</text>', "ref=\"jp-inf-2\"", "japan")
add("jp-logistics", "marker", "Japanese Logistics", '      <emblem kind="pennant" color="ink" color2="red"/><text slot="BOTTOM" size="small" weight="bold">Logistics</text>', None, "japan")
add("jp-beachhead-2", "marker", "Beachhead 2",
    '      <glyph kind="arrow-up" slot="TC" color="ink"/><emblem kind="anchor" color="ink" scale="0.6"/><text slot="LL" size="small">Port</text><text slot="LR" size="small">Air</text><value size="medium">Beachhead 2</value>',
    '      <glyph kind="arrow-up" slot="TC" color="ink"/><silhouette kind="ship" color="ink" scale="0.6"/><value size="medium">Beachhead 2</value>', "japan")
add("jp-convoy", "marker", "Japanese Convoy", '      <emblem kind="anchor" color="ink" scale="0.7"/><text slot="BOTTOM" size="small" weight="bold">Supply|Convoy</text>',
    '      <emblem kind="anchor" color="ink" scale="0.7"/><text slot="BOTTOM" size="small" weight="bold">Troop|Convoy</text>', "japan")
add("ccp-partisan-base", "marker", "Partisan Base", '      <glyph kind="disc" x="50" y="44" color="black"/><text slot="BOTTOM" size="small" weight="bold">Partisan|Base</text>', None, "communist")
add("mk-neutrality", "marker", "Neutrality", '      <text slot="CENTRE" size="medium" weight="bold">Neutrality</text><value size="medium">+1</value>', None, "plain")
add("mk-armistice", "marker", "Armistice", '      <text slot="CENTRE" size="medium" weight="bold">Armistice</text>', '      <text slot="CENTRE" size="medium" weight="bold">Enforced|Peace</text>', "plain")
add("mk-allied-collapse", "marker", "Allied Collapse", '      <glyph kind="hexagon" color="green"/><text slot="CENTRE" size="small" weight="bold">Allied|Collapse</text>', None, "plain")
add("mk-oil-embargo", "marker", "Oil Embargo", '      <glyph kind="hexagon" color="orange"/><text slot="CENTRE" size="small" weight="bold">Oil|Embargo</text>', None, "plain")
add("mk-british-entry", "marker", "British Entry", '      <emblem kind="roundel" color="fra" color2="red" scale="0.8"/><text slot="BOTTOM" size="small" weight="bold">British Entry</text>', None, "britain")
add("mk-us-entry", "marker", "US Entry", '      <emblem kind="star" color="white" scale="0.8"/><text slot="BOTTOM" size="small" weight="bold">US Entry</text>', None, "usa")
add("mk-soviet-influence", "marker", "Soviet Influence", '      <emblem kind="flag" color="ink" color2="red" scale="0.8"/><text slot="BOTTOM" size="small" weight="bold">Soviet|Influence</text>', None, "plain")
add("mk-western-influence", "marker", "Western Influence", '      <emblem kind="flag" color="ink" color2="green" scale="0.8"/><text slot="BOTTOM" size="small" weight="bold">Western|Influence</text>', None, "plain")
add("mk-a-bomb", "marker", "A-bomb", '      <emblem kind="cloud" color="ink"/><text slot="BOTTOM" size="small" weight="bold">A-Bomb</text>', None, "usa")
add("mk-govt-army", "marker", "Government Army", '      <emblem kind="roundel" color="red" color2="white" scale="0.7"/><text slot="BOTTOM" size="small" weight="bold">Government|Army</text>', None, "japan")
add("mk-war-production", "marker", "War Production -1", '      <text slot="TC" size="small" weight="bold">War|Production</text><glyph kind="drm" x="50" y="70" color="black" text="-1"/>', None, "plain")
add("mk-minor-prod", "marker", "Minor Country Prod", '      <text slot="CENTRE" size="small" weight="bold">Minor|Country|Prod</text>', None, "plain")
x[-3] = x[-3].replace('<front style="plain">', '<front style="plain" split="orange">')
add("mk-hainan", "marker", "Japanese Dependent Hainan", '      <text slot="CENTRE" size="small" weight="bold">Japanese|Dependent|Hainan</text>', None, "japan")
x[-3] = x[-3].replace('<front style="japan">', '<front style="japan" split="gold">')
add("mk-blitz", "marker", "BLITZ", '      <text slot="CENTRE" size="large" weight="bold" color="white">BLITZ</text>', None, "japan")
x[-3] = x[-3].replace('<front style="japan">', '<front style="japan" ground="black">')
add("mk-mud", "marker", "MUD", '      <text slot="CENTRE" size="large" weight="bold" color="white">MUD</text>', None, "plain")
x[-3] = x[-3].replace('<front style="plain">', '<front style="plain" ground="mud">')
add("mk-storms", "marker", "STORMS", '      <text slot="CENTRE" size="medium" weight="bold" color="white">STORMS</text>', '      <text slot="CENTRE" size="medium" weight="bold">SNOW</text>', "plain", "plain")
x[-3] = x[-3].replace('<front style="plain">', '<front style="plain" ground="storm">')
x[-2] = x[-2].replace('<back style="plain">', '<back style="plain" ground="snow">')
add("mk-vj-day", "marker", "V-J Day", '      <text slot="CENTRE" size="medium" weight="bold">V-J|Day</text>', None, "plain")
add("mk-war-state", "marker", "Pacific War State", '      <text slot="TC" size="small" weight="bold">Pacific|War State</text><text slot="BOTTOM" size="medium" weight="bold">Limited War</text>',
    '      <text slot="TC" size="small" weight="bold">Pacific|War State</text><text slot="BOTTOM" size="medium" weight="bold">Total War</text>', "plain")
add("mk-open-city", "marker", "Open City", '      <text slot="TC" size="medium" weight="bold">Open City</text><text slot="BOTTOM" size="small">in this hex</text>', None, "plain")
add("mk-kamikazes", "marker", "Kamikazes", '      <emblem kind="skull" color="ink" color2="white" scale="0.7"/><text slot="BOTTOM" size="small" weight="bold">Kamikazes</text>', None, "japan")

cols = 8
rows = -(-len(ids) // cols)
x.append('  <sheet id="sheet" title="Dai Senso! representative counters" cols="%d" rows="%d" gutter="0.5" margin="8" mirror="horizontal">' % (cols, rows))
for cid in ids:
    x.append('    <place counter="%s"/>' % cid)
x.append('  </sheet>')
x.append('</counters>')
open(OUT, "w", encoding="utf-8").write("\n".join(x))
print("wrote", OUT, len(ids), "counters")
# Copyright Ben Paul Wise. All Rights Reserved.
