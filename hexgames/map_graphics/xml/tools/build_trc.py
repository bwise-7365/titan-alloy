# Copyright Ben Paul Wise. All Rights Reserved.
"""build_trc.py -- assemble The Russian Campaign sheet (pointy-topped, lettered rows)."""
import sys

sys.path.insert(0, r"C:\repos\ghub-per\titan-alloy\hexgames\map_graphics\xml")
from lxml import etree
import hexsheet2svg as H

OUT = r"C:\repos\ghub-per\titan-alloy\hexgames\map_graphics\xml\the-russian-campaign.xml"
el = etree.Element("grid", terrain="clear", orientation="pointy", offset="odd", size="20.65", ox="22.27", oy="155.40",
                   cols="33", rows="43")
el.set("id-format", "{rowletter}{col}"); el.set("col-start", "33"); el.set("col-step", "-1"); el.set("row-start", "1")
g = H.Grid(el)


def near(x, y):
    return min(g.cells.items(), key=lambda kv: (g.centre(*kv[0])[0] - x) ** 2 + (g.centre(*kv[0])[1] - y) ** 2)[1]


def frag(name, subs=()):
    lines = [l for l in open(name, encoding="utf-8").read().splitlines() if l.startswith("<")]
    txt = "\n".join(lines)
    for a, b in subs:
        txt = txt.replace(a, b)
    return txt


# positions were read off a 1332 x 1600 view of the 1224 x 1483 scan
KX, KY = 1224 / 1332.0, 1483 / 1600.0
major = [("BERLIN", 100, 300), ("WARSAW", 522, 682), ("RIGA", 660, 330), ("HELSINKI", 760, 215), ("LENINGRAD", 912, 297),
         ("MINSK", 562, 637), ("MOSCOW", 962, 725), ("KIEV", 562, 890), ("KHARKOV", 760, 1035), ("VORONEZH", 962, 1035),
         ("SARATOV", 1140, 1090), ("STALINGRAD", 1022, 1305), ("DNEPROPETROVSK", 632, 1145), ("ODESSA", 388, 1140),
         ("SEVASTOPOL", 398, 1358), ("ROSTOV", 768, 1300), ("STALINO", 692, 1205), ("ASTRAKHAN", 1122, 1560),
         ("BUCHAREST", 120, 1073), ("ARCHANGEL", 1252, 238), ("GORKI", 1250, 830)]
minor = [("Posen", 322, 445), ("Breslau", 222, 622), ("Brest", 415, 632), ("Kaunas", 472, 505), ("Königsberg", 430, 378),
         ("Tallinn", 745, 250), ("Vitebsk", 735, 530), ("Smolensk", 778, 668), ("Kalinin", 995, 620), ("Bryansk", 778, 808),
         ("Tula", 948, 832), ("Lvov", 320, 770), ("Kursk", 818, 940), ("Krasnodar", 752, 1490)]
ports = {"RIGA": "n", "HELSINKI": "s", "LENINGRAD": "w", "ODESSA": "s", "SEVASTOPOL": "s", "ROSTOV": "s", "Tallinn": "n"}
oil = [("Ploesti Oil Fields", 135, 1040), ("Maikop Oil Fields", 830, 1500), ("Grozny Oil Fields", 1060, 1560)]
areas = [("GERMANY", 120, 250, -35, 40), ("POLAND", 340, 560, 0, 36), ("HUNGARY", 60, 780, -60, 34),
         ("RUMANIA", 110, 1010, 0, 34), ("FINLAND", 890, 175, 30, 32), ("RUSSIA", 1150, 930, 0, 60),
         ("Baltic Sea", 440, 250, 0, 40), ("White Sea", 1240, 160, 0, 30), ("Black Sea", 350, 1410, 0, 46),
         ("Sea of Azov", 605, 1330, 0, 22), ("Caspian Sea", 1230, 1580, 0, 30), ("Gulf of Finland", 860, 245, 0, 18),
         ("Lake Ladoga", 990, 268, 0, 15), ("Lake Onega", 1120, 268, 0, 15), ("Lake Peipus", 760, 365, 0, 15),
         ("Lake Ilmen", 870, 435, 0, 15), ("Lake Beloje", 1165, 400, 0, 15), ("Rybinsk Reservoir", 1105, 570, 0, 14)]
rivers = [("Dnieper", 560, 1000, -80), ("Don", 900, 1180, -60), ("Volga", 1150, 1200, -70), ("Dvina", 640, 420, -40),
          ("Narva R.", 830, 305, 0), ("Moskva R.", 1000, 770, 0), ("Pripet Marshes", 570, 740, -30)]

x = ['<?xml version="1.0" encoding="UTF-8"?>',
     '<sheet xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="hexsheet.xsd"',
     '       id="trc" title="The Russian Campaign, Deluxe Fifth Edition"',
     '       source="TRC map v1 adjusted.png, 1224 x 1483; Consim Press / GMT 2022"',
     '       width="1224" height="1483" background="frame" font="Georgia, Times New Roman, serif">',
     '  <grid id="main" orientation="pointy" offset="odd" cols="33" rows="43" size="20.65" ox="22.27" oy="155.40"',
     '        id-format="{rowletter}{col}" col-start="33" col-step="-1" row-start="1" terrain="clear" id-side="w"/>',
     '''  <palette>
    <color id="frame" value="#8c8c8c" name="sheet frame"/>
    <color id="steppe" value="#d5d59b"/>
    <color id="leaf" value="#6f9a4f"/>
    <color id="rock" value="#97915b"/>
    <color id="marsh" value="#b6c495"/>
    <color id="deep" value="#206f93"/>
    <color id="grid" value="#8f8f6a"/>
    <color id="gridw" value="#5f93b0"/>
    <color id="riverblue" value="#3f7fb3"/>
    <color id="railgrey" value="#6a6a6a"/>
    <color id="ink" value="#111111"/>
    <color id="cream" value="#f3edc0"/>
    <color id="citygrey" value="#8a8a8a"/>
    <color id="white" value="#ffffff"/>
    <color id="panel" value="#f4efd6"/>
    <color id="paneldark" value="#2d4a63"/>
    <color id="red" value="#b0201f"/>
    <color id="label" value="#333322"/>
  </palette>
  <terrains>
    <terrain id="clear" name="Clear" fill="steppe" stroke="grid"/>
    <terrain id="woods" name="Woods" fill="leaf" stroke="grid" pattern="mottle"/>
    <terrain id="mountain" name="Mountain" fill="rock" stroke="grid" pattern="mottle"/>
    <terrain id="swamp" name="Swamp" fill="marsh" stroke="grid" pattern="dots"/>
    <terrain id="sea" name="Sea" fill="deep" stroke="gridw"/>
    <terrain id="lake" name="Lake" fill="deep" stroke="gridw"/>
  </terrains>
  <lines>
    <line id="river" stroke="riverblue" width="3.2"/>
    <line id="rail" stroke="railgrey" width="1.6" dash="4 3"/>
    <line id="border" stroke="ink" width="2.4"/>
    <line id="district" stroke="railgrey" width="1.2" dash="6 2 1 2"/>
    <line id="blocked" stroke="white" width="2.2" dash="1 3"/>
    <line id="coast" stroke="ink" width="1.4"/>
  </lines>''',
     "  <!-- terrain, sampled from the scan; hexes under printed panels take the terrain around them -->",
     frag("trc_terrain.txt", [('terrain="panel"', 'terrain="clear"'), ('terrain="panel2"', 'terrain="clear"'), ('terrain="box"', 'terrain="sea"')]),
     "  <!-- rivers and country borders along hexsides, traced from the scan -->",
     frag("trc_edges.txt"),
     "  <!-- railroads, centre to centre, traced from the scan -->",
     frag("trc_links.txt"),
     "  <!-- the Kerch Strait: one hexside carrying five rule effects -->",
     '  <edge at="KK20:e" symbol="strait" color="white" label="Kerch Strait"/>',
     "  <!-- cities: cream ring, square for major, circle for minor; port anchors on the water side -->"]
for name, px, py in major:
    pid = near(px * KX, py * KY)
    gl = '<glyph symbol="city-major" color="citygrey"/>'
    if name in ports:
        gl += '<glyph symbol="port" slot="%s" color="white" scale="0.7"/>' % ports[name]
    x.append('  <hex id="%s" name="%s" ring="cream">%s</hex>' % (pid, name, gl))
    x.append('  <label text="%s" at="%s" slot="s" size="11" weight="bold" color="ink" halo="true"/>' % (name, pid))
for name, px, py in minor:
    pid = near(px * KX, py * KY)
    gl = '<glyph symbol="city-minor"/>'
    if name in ports:
        gl += '<glyph symbol="port" slot="%s" color="white" scale="0.7"/>' % ports[name]
    x.append('  <hex id="%s" name="%s" ring="cream">%s</hex>' % (pid, name, gl))
    x.append('  <label text="%s" at="%s" slot="s" size="9" color="ink" halo="true"/>' % (name, pid))
x.append("  <!-- oil fields -->")
for name, px, py in oil:
    pid = near(px * KX, py * KY)
    x.append('  <hex id="%s" name="%s" ring="cream"><glyph symbol="oil" color="ink"/></hex>' % (pid, name))
    x.append('  <label text="%s" at="%s" slot="s" size="6" color="ink"/>' % (name, pid))
x.append("  <!-- country, sea and lake names -->")
for name, px, py, ang, size in areas:
    italic = "true" if size <= 46 and not name.isupper() else "false"
    x.append('  <label text="%s" x="%.0f" y="%.0f" size="%d" angle="%d" spacing="%d" italic="%s" color="%s" halo="true"/>' % (
        name, px * KX, py * KY, size, ang, size // 6 if name.isupper() else 0, italic, "white" if "Sea" in name or "Lake" in name or "Gulf" in name or "Rybinsk" in name else "label"))
for name, px, py, ang in rivers:
    x.append('  <label text="%s" x="%.0f" y="%.0f" size="8" angle="%d" italic="true" color="riverblue"/>' % (name, px * KX, py * KY, ang))
x.append('''  <!-- furniture: the strip across the top, two panels over sea hexes, and the corner charts -->
  <panel id="title" x="8" y="8" w="300" h="68" fill="ink" stroke="ink">
    <text x="10" y="30" size="22" weight="bold" color="red">The Russian Campaign</text>
    <text x="10" y="56" size="11" color="white">Designer Signature Edition  --  John Edwards, Todd Davis</text>
  </panel>
  <panel id="axis-pool" x="322" y="8" w="140" h="68" fill="panel" stroke="ink" title="Axis Replacement Pool"/>
  <panel id="russian-surrendered" x="470" y="8" w="140" h="68" fill="panel" stroke="ink" title="Russian Surrendered Units"/>
  <panel id="weather" x="620" y="8" w="300" h="68" fill="panel" stroke="ink" title="Weather Chart">
    <table x="6" y="18" cell-w="32" cell-h="15" size="6">
      <row><cell>Months</cell><cell>0-</cell><cell>1</cell><cell>2</cell><cell>3</cell><cell>4</cell><cell>5</cell><cell>6</cell><cell>7+</cell></row>
      <row><cell>Mar/Apr</cell><cell>Clear</cell><cell>LtMud</cell><cell>LtMud</cell><cell>LtMud</cell><cell>LtMud</cell><cell>Mud</cell><cell>Mud</cell><cell>Snow</cell></row>
      <row><cell>Sep/Oct</cell><cell>Clear</cell><cell>Clear</cell><cell>Clear</cell><cell>LtMud</cell><cell>LtMud</cell><cell>Mud</cell><cell>Mud</cell><cell>Snow</cell></row>
    </table>
  </panel>
  <panel id="russian-pool" x="930" y="8" w="140" h="68" fill="panel" stroke="ink" title="Russian Replacement Pool"/>
  <panel id="axis-surrendered" x="1078" y="8" w="138" h="68" fill="panel" stroke="ink" title="Axis Surrendered Units"/>
  <panel id="turn-track" x="8" y="84" w="1208" h="42" fill="panel" stroke="ink">
    <text x="4" y="26" size="9" weight="bold">Turn Record Track</text>
    <track x="90" y="4" cell-w="44" cell-h="34" cells="0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25" fill="cream"/>
  </panel>
  <panel id="baltic-box" x="195" y="140" w="160" h="70" fill="paneldark" stroke="ink">
    <text x="6" y="14" size="8" weight="bold" color="white">Baltic Sea - Gulf of Finland Movement</text>
    <text x="6" y="28" size="6" color="white">1 unit/side per turn. No invasions.</text>
    <text x="6" y="40" size="6" color="white">Successful on die roll of 2 or less.</text>
    <text x="6" y="52" size="6" color="white">-1 for each of Helsinki, Leningrad, Riga, Tallinn controlled</text>
    <text x="6" y="64" size="6" color="white">+1 evacuation; +2 Russian, if Germans control Leningrad</text>
  </panel>
  <panel id="airpower" x="362" y="140" w="120" h="70" fill="panel" stroke="ink" title="Airpower Availability">
    <table x="4" y="18" cell-w="28" cell-h="10" size="5">
      <row><cell>Year</cell><cell>Clear</cell><cell>LtMud</cell><cell>Mud</cell></row>
      <row><cell>1941</cell><cell>3/0</cell><cell>1/0</cell><cell>1/0</cell></row>
      <row><cell>1942</cell><cell>2/0</cell><cell>1/0</cell><cell>1/0</cell></row>
      <row><cell>1943</cell><cell>1/1</cell><cell>0/0</cell><cell>0/0</cell></row>
      <row><cell>1944</cell><cell>0/2</cell><cell>0/1</cell><cell>0/0</cell></row>
    </table>
  </panel>
  <panel id="black-sea-box" x="470" y="1385" w="130" h="58" rotate="180" fill="paneldark" stroke="ink">
    <text x="6" y="14" size="7" weight="bold" color="white">Black Sea - Sea of Azov Movement</text>
    <text x="6" y="26" size="5" color="white">1 unit per side per turn. Successful on die roll of 3 or less.</text>
    <text x="6" y="36" size="5" color="white">-1 for each of Odessa, Sevastopol, Rostov controlled</text>
  </panel>
  <panel id="crt" x="214" y="1300" w="200" h="150" rotate="180" fill="panel" stroke="ink" title="Combat Results Table">
    <table x="4" y="18" cell-w="19" cell-h="13" size="6">
      <row><cell>Die</cell><cell>1-5</cell><cell>1-3</cell><cell>1-2</cell><cell>1-1</cell><cell>2-1</cell><cell>3-1</cell><cell>4-1</cell><cell>5-1</cell><cell>7-1</cell></row>
      <row><cell>1</cell><cell>AE</cell><cell>AE</cell><cell>AE</cell><cell>A1</cell><cell>A1</cell><cell>AR</cell><cell>C</cell><cell>EX</cell><cell>DR</cell></row>
      <row><cell>2</cell><cell>AE</cell><cell>AE</cell><cell>A1</cell><cell>A1</cell><cell>AR</cell><cell>C</cell><cell>EX</cell><cell>DR</cell><cell>D1</cell></row>
      <row><cell>3</cell><cell>AE</cell><cell>A1</cell><cell>AR</cell><cell>AR</cell><cell>C</cell><cell>EX</cell><cell>DR</cell><cell>D1</cell><cell>DE</cell></row>
      <row><cell>4</cell><cell>A1</cell><cell>AR</cell><cell>AR</cell><cell>C</cell><cell>EX</cell><cell>DR</cell><cell>D1</cell><cell>DE</cell><cell>DE</cell></row>
      <row><cell>5</cell><cell>AR</cell><cell>AR</cell><cell>C</cell><cell>EX</cell><cell>DR</cell><cell>D1</cell><cell>DE</cell><cell>DE</cell><cell>DS</cell></row>
      <row><cell>6</cell><cell>AR</cell><cell>C</cell><cell>EX</cell><cell>DR</cell><cell>D1</cell><cell>DE</cell><cell>DS</cell><cell>DS</cell><cell>DS</cell></row>
    </table>
  </panel>
  <panel id="tec" x="8" y="1220" w="200" h="230" rotate="180" fill="panel" stroke="ink" title="Terrain Effects Chart">
    <box label="Clear: 1 MP" x="8" y="22" w="184" h="20" fill="steppe"/>
    <box label="Woods: 1 MP, stop; no retreat" x="8" y="46" w="184" h="20" fill="leaf"/>
    <box label="Mountain: 1 MP, stop; defender x2" x="8" y="70" w="184" h="20" fill="rock"/>
    <box label="Swamp: 1 MP, stop; clear in Snow" x="8" y="94" w="184" h="20" fill="marsh"/>
    <box label="River: defender x2 if all attackers on river hexes" x="8" y="118" w="184" h="20" fill="riverblue"/>
    <box label="Major City: defender x2" x="8" y="142" w="184" h="20" fill="citygrey"/>
    <box label="Sea or Lake: prohibited" x="8" y="166" w="184" h="20" fill="deep"/>
    <box label="Blocked hexside: no movement, no ZOC" x="8" y="190" w="184" h="20"/>
  </panel>
</sheet>''')
open(OUT, "w", encoding="utf-8").write("\n".join(x))
print("wrote", OUT)
# Copyright Ben Paul Wise. All Rights Reserved.
