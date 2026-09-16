# Copyright Ben Paul Wise. All Rights Reserved.
"""build_ds.py -- assemble the Dai Senso sheet (two independently numbered pointy-topped grids)."""
import re
import sys

sys.path.insert(0, r"C:\repos\ghub-per\titan-alloy\hexgames\map_graphics\xml")
from lxml import etree
import hexsheet2svg as H

OUT = r"C:\repos\ghub-per\titan-alloy\hexgames\map_graphics\xml\dai-senso.xml"
grids = []
for gid, ox, cols in (("west", "62.54", "27"), ("east", "1977.63", "29")):
    el = etree.Element("grid", terrain="sea", orientation="pointy", offset="odd", size="40.95", ox=ox, oy="-8.05", cols=cols, rows="53")
    el.set("id-format", gid[0] + "{row:02}{col:02}"); el.set("col-start", "1"); el.set("row-start", "61"); el.set("row-step", "-1")
    grids.append(H.Grid(el))


def near(x, y):
    best = None
    for g in grids:
        for (c, r), pid in g.cells.items():
            cx, cy = g.centre(c, r)
            d = (cx - x) ** 2 + (cy - y) ** 2
            if best is None or d < best[0]:
                best = (d, pid)
    return best[1]


def frag(name, subs=()):
    lines = [l for l in open(name, encoding="utf-8").read().splitlines() if l.startswith("<")]
    txt = "\n".join(lines)
    for a, b in subs:
        txt = re.sub(a, b, txt)
    return txt


def ports_on_land(ports, terrain):
    land = set()
    for l in open(terrain, encoding="utf-8"):
        m = re.match(r'<hexes terrain="(\w+)" ids="([^"]*)"', l)
        if m and m.group(1) in ("hills", "clear", "desert", "forest", "swamp", "city"):
            land.update(m.group(2).split())
    keep = [l for l in frag(ports, [('color="port"', 'color="cyan"')]).splitlines()
            if re.search(r'id="(\w+)"', l) and re.search(r'id="(\w+)"', l).group(1) in land]
    return "\n".join(keep)


K = 3990 / 1600.0   # positions read off a 1600-wide view of the 3990 x 3198 scan
cities = [("TOKYO", 930, 300), ("Osaka", 900, 320), ("Sapporo", 970, 210), ("PEKING", 690, 230), ("Nanking", 745, 335),
          ("SHANGHAI", 770, 345), ("Hong Kong", 700, 440), ("Canton", 690, 425), ("CHUNGKING", 610, 350), ("Hanoi", 640, 470),
          ("Saigon", 640, 545), ("BANGKOK", 555, 540), ("Rangoon", 480, 500), ("CALCUTTA", 380, 440), ("DELHI", 300, 350),
          ("Bombay", 265, 470), ("Colombo", 300, 590), ("SINGAPORE", 585, 625), ("Batavia", 600, 725), ("MANILA", 780, 520),
          ("Darwin", 770, 880), ("Sydney", 1010, 1145), ("Perth", 700, 1080), ("Rabaul", 960, 700), ("Port Moresby", 900, 790),
          ("VLADIVOSTOK", 860, 180), ("Harbin", 830, 110), ("Mukden", 800, 190), ("Keijo", 795, 250), ("Honolulu", 1490, 470),
          ("Auckland", 1210, 1120), ("Guam", 1010, 520), ("Truk", 1060, 600), ("Kwajalein", 1180, 560), ("Midway", 1290, 380)]
areas = [("JAPAN", 935, 320, 70, 30), ("CHINA", 640, 320, 0, 44), ("RUSSIA", 340, 70, 0, 36), ("MANCHUKUO", 780, 140, 0, 20),
         ("MONGOLIA", 640, 165, 0, 20), ("SINKIANG", 420, 235, 0, 18), ("TIBET", 380, 320, 0, 18), ("INDIA", 300, 460, 0, 34),
         ("BURMA", 465, 440, 0, 18), ("SIAM", 545, 500, 0, 18), ("INDOCHINA", 620, 470, 0, 16), ("PHILIPPINES", 780, 560, 0, 18),
         ("NETHERLANDS EAST INDIES", 640, 760, 0, 22), ("NEW GUINEA", 880, 780, 0, 18), ("AUSTRALIA", 830, 1010, 0, 50),
         ("NEW ZEALAND", 1210, 1170, 0, 18), ("Sea of Japan", 890, 220, 0, 16), ("Yellow Sea", 780, 280, 0, 14),
         ("South China Sea", 690, 560, 0, 16), ("Philippine Sea", 880, 470, 0, 16), ("Coral Sea", 1030, 800, 0, 16),
         ("Bay of Bengal", 420, 560, 0, 16), ("Indian Ocean", 400, 800, 0, 20), ("Central Pacific", 1300, 300, 0, 18),
         ("Sea of Okhotsk", 1010, 90, 0, 14), ("Tasman Sea", 1120, 1100, 0, 14), ("Gulf of Alaska", 1450, 130, 0, 14)]
zones = [  # naval zone boxes printed over the sea, at an angle (small-image x, y, rotation)
    ("West Indian Ocean", 75, 570, 0), ("Bay of Bengal", 330, 720, 0), ("Southeast Indian Ocean", 500, 1000, 0),
    ("South China Sea", 590, 560, 40), ("Philippine Sea", 800, 550, 0), ("Yellow Sea", 760, 340, 40),
    ("Sea of Japan", 855, 350, 0), ("Sea of Okhotsk", 990, 20, 0), ("Northwest Pacific", 1090, 130, 0),
    ("North Pacific", 1230, 160, 0), ("Gulf of Alaska", 1390, 150, 0), ("Central Pacific", 1330, 320, 0),
    ("The Bonins", 1040, 355, 0), ("Japanese Coast", 860, 370, 0), ("Micronesia", 1040, 560, 0), ("Marshall Islands", 1150, 610, 0),
    ("Bismarck Sea", 900, 690, 0), ("Coral Sea", 1010, 900, 0), ("Southeast Pacific", 1280, 600, 0), ("Tasman Sea", 1140, 1010, 0),
    ("South Pacific", 1230, 1120, 0), ("Great Australian Bight", 780, 1200, 0), ("Polynesia", 1250, 920, 0),
    ("Northeast Pacific", 1500, 350, 0), ("Eastern Pacific", 1500, 560, 0), ("Southeast Indian", 550, 1010, 0)]

TER = [('terrain="panel"', 'terrain="clear"'), ('terrain="pink"', 'terrain="sea"'), ('terrain="paleblue"', 'terrain="sea"'),
       ('terrain="city"', 'terrain="clear"'), ('terrain="deepblue"', 'terrain="sea"')]
x = ['<?xml version="1.0" encoding="UTF-8"?>',
     '<sheet xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="hexsheet.xsd"',
     '       id="dai-senso" title="Axis Empires: Dai Senso!"',
     '       source="Dai Senso game map adjusted.png, 3990 x 3198; Decision Games 2011. West Map and East Map side by side."',
     '       width="3990" height="3198" background="paper" font="Georgia, Times New Roman, serif" urban="symbol">',
     '  <grid id="west" orientation="pointy" offset="odd" cols="27" rows="53" size="40.95" ox="62.54" oy="-8.05"',
     '        id-format="w{row:02}{col:02}" col-start="1" row-start="61" row-step="-1" terrain="sea" id-side="w"/>',
     '  <grid id="east" orientation="pointy" offset="odd" cols="29" rows="53" size="40.95" ox="1977.63" oy="-8.05"',
     '        id-format="e{row:02}{col:02}" col-start="1" row-start="61" row-step="-1" terrain="sea" id-side="w"/>',
     '''  <palette>
    <color id="paper" value="#f3efe2" name="sheet background"/>
    <color id="seablue" value="#69b9ec"/>
    <color id="ochre" value="#ad8534"/>
    <color id="tan" value="#eadebe"/>
    <color id="sand" value="#ecd581"/>
    <color id="teal" value="#698380"/>
    <color id="greygreen" value="#b9c5b1"/>
    <color id="grid" value="#8f8a78"/>
    <color id="gridw" value="#4f93c4"/>
    <color id="white" value="#ffffff"/>
    <color id="brown" value="#7a4f1f"/>
    <color id="borderorange" value="#e8842a"/>
    <color id="ink" value="#111111"/>
    <color id="gold" value="#f2c40f"/>
    <color id="cyan" value="#29a8d8"/>
    <color id="orange" value="#e8762a" name="Axis strategic hex"/>
    <color id="green" value="#3aa04a" name="Western strategic hex"/>
    <color id="red" value="#d8302a" name="Soviet strategic hex"/>
    <color id="panel" value="#f7f4ea"/>
    <color id="navy" value="#2b5f9e"/>
    <color id="pinkpanel" value="#f2b9a3"/>
    <color id="label" value="#6b6656"/>
  </palette>
  <terrains>
    <terrain id="sea" name="All-Sea" fill="seablue" stroke="gridw"/>
    <terrain id="hills" name="Rough (Hills)" fill="ochre" stroke="grid"/>
    <terrain id="clear" name="Clear (Normal)" fill="tan" stroke="grid"/>
    <terrain id="desert" name="Clear (Desert)" fill="sand" stroke="grid"/>
    <terrain id="forest" name="Rough (Forest)" fill="teal" stroke="grid" pattern="mottle"/>
    <terrain id="swamp" name="Rough (Swamp)" fill="greygreen" stroke="grid" pattern="dots"/>
  </terrains>
  <lines>
    <line id="border" stroke="borderorange" width="5" dash="8 5"/>
    <line id="zone" stroke="white" width="5" dash="10 8"/>
    <line id="mountain" stroke="brown" width="7"/>
    <line id="road" stroke="white" width="4" casing="grid" casing-width="6"/>
  </lines>''',
     "  <!-- West Map terrain, sampled from the scan -->", frag("ds_terrain_w.txt", TER),
     "  <!-- East Map terrain -->", frag("ds_terrain_e.txt", TER),
     "  <!-- hexside features: country / dependent borders, naval zone borders, mountain hexsides -->",
     frag("ds_edges_W.txt", [('line="border2"', 'line="border"')]), frag("ds_edges_E.txt", [('line="border2"', 'line="border"')]),
     "  <!-- roads and rails, centre to centre (the scan does not let the tracer tell the two apart) -->",
     frag("ds_links_W.txt"), frag("ds_links_E.txt"),
     "  <!-- strategic hex rings in faction colours -->", frag("ds_rings_W.txt"), frag("ds_rings_E.txt"),
     "  <!-- cities and capitals -->",
     frag("ds_centres_W.txt", [('color="city"', 'color="gold"'), ('color="capital"', 'color="red"')]),
     frag("ds_centres_E.txt", [('color="city"', 'color="gold"'), ('color="capital"', 'color="red"')]),
     "  <!-- port anchors, offset toward the water; kept only on land hexes -->",
     ports_on_land("ds_ports_W.txt", "ds_terrain_w.txt"), ports_on_land("ds_ports_E.txt", "ds_terrain_e.txt"),
     "  <!-- place names -->"]
for name, sx, sy in cities:
    pid = near(sx * K, sy * K)
    up = name.isupper()
    x.append('  <label text="%s" at="%s" slot="s" size="%d" weight="%s" color="ink" halo="true"/>' % (name, pid, 22 if up else 17, "bold" if up else "normal"))
for name, sx, sy, ang, size in areas:
    sea = not name.isupper()
    x.append('  <label text="%s" x="%.0f" y="%.0f" size="%d" angle="%d" spacing="%d" italic="%s" color="%s" halo="true"/>' % (
        name, sx * K, sy * K, size, ang, 0 if sea else size // 5, "true" if sea else "false", "white" if sea else "label"))
x.append("  <!-- naval zone boxes, printed over live sea hexes, rotated to fit -->")
for i, (name, sx, sy, rot) in enumerate(zones):
    x.append('''  <panel id="nz-%d" x="%.0f" y="%.0f" w="170" h="92" rotate="%d" fill="navy" stroke="ink">
    <text x="6" y="16" size="12" weight="bold" color="white">%s</text>
    <box label="On Station" x="6" y="24" w="50" h="60" fill="seablue"/>
    <box label="Convoys" x="60" y="24" w="50" h="60" fill="panel"/>
    <box label="Used" x="114" y="24" w="50" h="60" fill="panel"/>
  </panel>''' % (i, sx * K, sy * K, rot, name))
x.append('''  <!-- off-map furniture -->
  <panel id="delay-drms" x="20" y="20" w="600" h="560" fill="panel" stroke="red" title="Delay DRMs">
    <text x="12" y="44" size="14" weight="bold">Axis Delay DRMs</text>
    <text x="12" y="64" size="10">-? for the number of VPs in the box containing the Rising Sun VP marker, if Total War is in effect.</text>
    <text x="12" y="80" size="10">-1 if the Axis War Production marker is in the Strategic Warfare Box.</text>
    <text x="12" y="200" size="14" weight="bold">Western Delay DRMs</text>
    <text x="12" y="360" size="14" weight="bold">Soviet Delay DRMs</text>
  </panel>
  <panel id="strategic-warfare" x="20" y="600" w="300" h="130" fill="panel" stroke="red" title="Strategic Warfare Box"/>
  <panel id="west-map-label" x="340" y="640" w="260" h="60" fill="paper" stroke="paper">
    <text x="0" y="40" size="40" weight="bold">West Map</text>
  </panel>
  <panel id="eastern-europe" x="640" y="120" w="60" h="480" rotate="0" fill="pinkpanel" stroke="red">
    <text x="8" y="240" size="14" weight="bold">Eastern Europe Box</text>
  </panel>
  <panel id="europe-africa" x="20" y="1590" w="330" h="300" fill="panel" stroke="green" title="Europe/Africa Box">
    <text x="10" y="40" size="10">Use the Port-to-Port procedure to move to/from a port in the West Indian Ocean Naval Zone.</text>
    <text x="10" y="58" size="10">Use the Off-Map Box to Off-Map Box procedure to move to/from the Panama Canal Box.</text>
    <text x="10" y="76" size="10">Replacement Location for British non-colonial and Afr colonial units.</text>
  </panel>
  <panel id="delay-box" x="20" y="1900" w="330" h="260" fill="panel" stroke="red" title="Delay Box"/>
  <panel id="naval-delay" x="20" y="2170" w="330" h="150" fill="panel" stroke="red" title="Naval Warfare Delay Box"/>
  <panel id="vp-track" x="360" y="2090" w="640" h="240" fill="panel" stroke="red" title="VP Track">
    <track x="120" y="40" cell-w="80" cell-h="50" cells="+1_to_+2 +3_to_+5 +6_to_+8 +9_to_+11 +12_to_+14 +15" fill="pinkpanel"/>
    <track x="120" y="100" cell-w="80" cell-h="50" cells="0 1_VP 2_VP 3_VP 4_VP Automatic" fill="white"/>
    <track x="120" y="160" cell-w="80" cell-h="50" cells="+2_to_+1 0_to_-5 -6_to_-10 -11_to_-13 -14_to_-16 -17" fill="seablue"/>
  </panel>
  <panel id="turn-track" x="20" y="2420" w="1080" h="760" fill="panel" stroke="red" title="Turn Track">
    <track x="20" y="40" cell-w="120" cell-h="110" wrap="9" fill="white"
           cells="1937/1943 Jan-Feb Mar-Apr Apr-May May-June June-July July-Aug Aug-Sept Sept-Oct Nov-Dec 1938/1944 Jan-Feb Mar-Apr Apr-May May-June June-July July-Aug Aug-Sept Sept-Oct Nov-Dec 1939/1945 Jan-Feb Mar-Apr Apr-May May-June June-July July-Aug Aug-Sept Sept-Oct Nov-Dec 1940/1946 Jan-Feb Mar-Apr Apr-May May-June June-July July-Aug Aug-Sept Sept-Oct Nov-Dec 1941/1947 Jan-Feb Mar-Apr Apr-May May-June June-July July-Aug Aug-Sept Sept-Oct Nov-Dec 1942/1948 Jan-Feb Mar-Apr Apr-May May-June June-July July-Aug Aug-Sept Sept-Oct Nov-Dec"/>
  </panel>
  <panel id="ceded-lands" x="1120" y="2830" w="320" h="180" fill="panel" stroke="red" title="Ceded Lands Box"/>
  <panel id="weather-areas" x="1120" y="3020" w="440" h="160" fill="panel" stroke="ink" title="Weather Areas">
    <text x="10" y="40" size="10">North: Aleutian Islands, Japan, Korea, Manchukuo, Russia</text>
    <text x="10" y="56" size="10">Desert: Sinkiang, Mongolia, Tibet   Central: Formosa, Hong Kong, Kiangsu, Kwangtung, Yunnan</text>
    <text x="10" y="72" size="10">North Monsoon: Burma, Ceylon, India, Indochina, Malaya, Nepal, Philippines, Siam</text>
    <text x="10" y="88" size="10">South Monsoon: Netherlands East Indies, New Guinea   South: Australia, New Zealand</text>
  </panel>
  <panel id="uscl" x="2900" y="20" w="520" h="120" fill="panel" stroke="red" title="USCL Track">
    <track x="140" y="30" cell-w="70" cell-h="70" cells="USCL_0 USCL_1 USCL_2 USCL_3 USCL_4" fill="white"/>
  </panel>
  <panel id="posture" x="3460" y="20" w="510" h="130" fill="panel" stroke="red" title="Posture Display">
    <track x="10" y="30" cell-w="80" cell-h="80" cells="U.S._and_Western_Minors Britain Nationalist_China Communist_China Russia Soviet_Minors" fill="white"/>
  </panel>
  <panel id="war-state" x="3700" y="200" w="270" h="140" fill="panel" stroke="red" title="War State Display">
    <track x="10" y="40" cell-w="80" cell-h="80" cells="Pre-War Limited_War Total_War" fill="white"/>
  </panel>
  <panel id="east-map-label" x="3680" y="410" w="260" h="60" fill="paper" stroke="paper">
    <text x="0" y="40" size="40" weight="bold">East Map</text>
  </panel>
  <panel id="western-us" x="3680" y="480" w="290" h="250" fill="panel" stroke="green" title="Western US Box"/>
  <panel id="panama" x="3680" y="1760" w="290" h="230" fill="panel" stroke="green" title="Panama Canal Box"/>
  <panel id="tec" x="3480" y="2020" w="490" h="600" fill="panel" stroke="red" title="Terrain Effects Chart">
    <table x="10" y="30" cell-w="150" cell-h="24" size="10">
      <row><cell>Hex Terrain Type</cell><cell>MP Cost</cell><cell>CRT Column Shift</cell></row>
      <row><cell>Clear (including Desert)</cell><cell>1 MP</cell><cell>No effect</cell></row>
      <row><cell>City or Capital</cell><cell>1 MP</cell><cell>1 left</cell></row>
      <row><cell>Port</cell><cell>1 MP</cell><cell>No effect</cell></row>
      <row><cell>Town</cell><cell>Other</cell><cell>No effect</cell></row>
      <row><cell>Rough (Hills, Forest, Swamp)</cell><cell>2 MP</cell><cell>1 left</cell></row>
      <row><cell>All-Sea</cell><cell>Prohibited</cell><cell>Prohibited</cell></row>
      <row><cell>Hexside Terrain Type</cell><cell>MP Cost</cell><cell>CRT Column Shift</cell></row>
      <row><cell>Mountain</cell><cell>+2 MP</cell><cell>2 left</cell></row>
      <row><cell>River</cell><cell>+1 MP</cell><cell>1 left</cell></row>
      <row><cell>Road or Rail (one-step unit)</cell><cell>1/2 MP</cell><cell>No effect</cell></row>
      <row><cell>Road or Rail (multi-step unit)</cell><cell>1 MP</cell><cell>No effect</cell></row>
      <row><cell>Strait (connected)</cell><cell>As Road or Rail</cell><cell>2 left</cell></row>
      <row><cell>Strait (not connected)</cell><cell>Entire MA</cell><cell>2 left</cell></row>
      <row><cell>Beachhead 2 / 1 / SNLF</cell><cell>Entire MA</cell><cell>2 / 1 / 0 left</cell></row>
      <row><cell>All-Sea or Lake</cell><cell>Entire MA</cell><cell>Marine only (+2 left)</cell></row>
    </table>
  </panel>
  <panel id="terrain-key" x="3480" y="2640" w="490" h="300" fill="panel" stroke="red" title="Terrain Key">
    <box label="Clear (Normal)" x="10" y="30" w="110" h="60" fill="tan"/>
    <box label="Clear (Desert)" x="130" y="30" w="110" h="60" fill="sand"/>
    <box label="Rough (Hills)" x="250" y="30" w="110" h="60" fill="ochre"/>
    <box label="Rough (Forest)" x="370" y="30" w="110" h="60" fill="teal"/>
    <box label="Rough (Swamp)" x="10" y="100" w="110" h="60" fill="greygreen"/>
    <box label="All-Sea" x="130" y="100" w="110" h="60" fill="seablue"/>
    <box label="Strategic Hex" x="250" y="100" w="110" h="60"/>
    <box label="Port / City / Capital" x="370" y="100" w="110" h="60"/>
    <box label="Country border" x="10" y="170" w="110" h="60" fill="borderorange"/>
    <box label="Naval Zone border" x="130" y="170" w="110" h="60" fill="white"/>
    <box label="Mountain hexside" x="250" y="170" w="110" h="60" fill="brown"/>
    <box label="Strait" x="370" y="170" w="110" h="60"/>
  </panel>
  <panel id="french-polynesia" x="3340" y="2960" w="380" h="200" fill="panel" stroke="green" title="French Polynesia Box"/>
  <panel id="title" x="3740" y="2960" w="230" h="200" fill="paper" stroke="paper">
    <text x="10" y="60" size="44" weight="bold" color="red">Dai Senso!</text>
    <text x="10" y="100" size="24" weight="bold">Axis Empires</text>
    <text x="10" y="130" size="12">(c) 2011 Decision Games</text>
  </panel>
</sheet>''')
open(OUT, "w", encoding="utf-8").write("\n".join(x))
print("wrote", OUT)
# Copyright Ben Paul Wise. All Rights Reserved.
