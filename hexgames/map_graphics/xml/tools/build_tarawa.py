"""build_tarawa.py -- assemble the D-Day at Tarawa sheet (pointy-topped; fire dots as side glyphs)."""
import re

OUT = r"C:\repos\ghub-per\titan-alloy\hexgames\map_graphics\xml\d-day-at-tarawa.xml"
COLS = ["purple", "red", "olive", "green", "blue", "orange"]


def frag(name, subs=()):
    lines = [l for l in open(name, encoding="utf-8").read().splitlines() if l.startswith("<")]
    txt = "\n".join(lines)
    for a, b in subs:
        txt = re.sub(a, b, txt)
    return txt


def land_only(text):
    """Japanese positions sit on land: drop detections on water and reef hexes."""
    water = set()
    for l in open("tar_terrain.txt", encoding="utf-8"):
        m = re.match(r'<hexes terrain="(water|reef)" ids="([^"]*)"', l)
        if m:
            water.update(m.group(2).split())
    keep = []
    for l in text.splitlines():
        m = re.search(r'id="(\d+)"', l)
        if m and m.group(1) in water:
            continue
        keep.append(l)
    return "\n".join(keep)


K = 1786 / 1600.0     # positions read off a 1600-wide view of the 1786 x 1153 scan
beach_labels = [("R3C", 245), ("R3B", 325), ("R3D", 410), ("R3A", 490), ("R2C", 670), ("R2D", 750), ("R2B", 830),
                ("R2A", 900), ("R1C", 1010), ("R1B", 1090), ("R1A", 1170)]
cards = [("CARD DECK", 10, 145), ("US Amphibious Operations Phase", 150, 215), ("LVT LANDING CHECK CARDS", 220, 345),
         ("First Event Phase -- EVENT CARD", 350, 485), ("Japanese Fire Phase -- JAPANESE FIRE CARD", 495, 680),
         ("Second Event Phase -- EVENT CARD", 690, 820), ("US Engineer and HQ Phase", 825, 895), ("US Action Phase", 900, 975),
         ("ADDITIONAL CARD DRAWS", 980, 1085), ("End of Turn", 1090, 1145)]

x = ['<?xml version="1.0" encoding="UTF-8"?>',
     '<sheet xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="hexsheet.xsd"',
     '       id="tarawa" title="D-Day at Tarawa"',
     '       source="DDaT-map2.png, 1786 x 1153; Decision Games 2014, map redesign Delphine Echassoux"',
     '       width="1786" height="1153" background="paper" font="Arial, Helvetica, sans-serif">',
     '  <grid id="main" orientation="pointy" offset="odd" cols="42" rows="25" size="24.90" ox="8.59" oy="222.10"',
     '        id-format="{row:02}{col:02}" col-start="2" row-start="25" row-step="-1" terrain="water" id-side="w"/>',
     '''  <palette>
    <color id="paper" value="#efe8cf" name="sheet background"/>
    <color id="lagoon" value="#c0dce7"/>
    <color id="reefgreen" value="#cbdfdf"/>
    <color id="sand" value="#e5e1c1"/>
    <color id="white" value="#f8f8f4"/>
    <color id="beige" value="#eed9a8"/>
    <color id="palmgreen" value="#b9c9a6"/>
    <color id="tan" value="#d9c19b"/>
    <color id="concrete" value="#6a6a6a"/>
    <color id="grid" value="#8a8a80"/>
    <color id="gridw" value="#8fb3c4"/>
    <color id="seawall" value="#3a3a3a"/>
    <color id="ink" value="#111111"/>
    <color id="teal" value="#2aa198"/>
    <color id="cardred" value="#c8342b"/>
    <color id="cardblue" value="#2f7fc1"/>
    <color id="panel" value="#f7f4e6"/>
    <color id="purple" value="#7476a5" name="position colour"/>
    <color id="red" value="#d23a3a" name="position colour"/>
    <color id="olive" value="#8f8f2c" name="position colour"/>
    <color id="green" value="#78b23c" name="position colour"/>
    <color id="blue" value="#3b8ed0" name="position colour"/>
    <color id="orange" value="#f2b021" name="position colour"/>
  </palette>
  <terrains>
    <terrain id="water" name="Water" fill="lagoon" stroke="gridw"/>
    <terrain id="reef" name="Coral reef" fill="reefgreen" stroke="gridw" pattern="dots"/>
    <terrain id="clear" name="Clear" fill="sand" stroke="grid"/>
    <terrain id="airstrip" name="Airstrip" fill="white" stroke="grid"/>
    <terrain id="beach" name="Beach" fill="beige" stroke="grid"/>
    <terrain id="palm" name="Palm trees" fill="palmgreen" stroke="grid" pattern="palms"/>
    <terrain id="rough" name="Rough / crater" fill="tan" stroke="grid" pattern="mottle"/>
    <terrain id="building" name="Building" fill="concrete" stroke="grid" pattern="hatch"/>
  </terrains>
  <lines>
    <line id="seawall-line" stroke="seawall" width="4" dash="5 2"/>
    <line id="pier-line" stroke="concrete" width="6"/>
    <line id="fz-purple" stroke="purple" width="3"/>
    <line id="fz-red" stroke="red" width="3"/>
    <line id="fz-olive" stroke="olive" width="3"/>
    <line id="fz-green" stroke="green" width="3"/>
    <line id="fz-blue" stroke="blue" width="3"/>
    <line id="fz-orange" stroke="orange" width="3"/>
  </lines>''',
     "  <!-- terrain, sampled from the scan at every hex centre -->",
     frag("tar_terrain.txt"),
     "  <!-- seawall and water-fire-zone boundary lines along hexsides, traced from the scan -->",
     frag("tar_edges.txt", [(r"line=\"seawall\"", "line=\"seawall-line\""), (r"line=\"pier\"", "line=\"pier-line\""), (r"line=\"gridline\"", 'line="seawall-line"')] + [(r'line="%s"' % c, 'line="fz-%s"' % c) for c in COLS]),
     "  <!-- position perimeter rings, in the position colour -->",
     land_only(frag("tar_rings.txt")),
     "  <!-- position badges at hex centres (the printed ID text is not recovered from the scan; A14 is known) -->",
     land_only(frag("tar_centres.txt", [(r'symbol="(%s)"' % "|".join(COLS), 'symbol="position-badge"')])).replace(
         '<hex id="2336"><glyph symbol="position-badge" color="orange"/></hex>',
         '<hex id="2336" name="A14"><glyph symbol="position-badge" color="orange" text="A14"/></hex>'),
     "  <!-- fire dots: one side glyph per (hex, hexside), in the projecting position's colour -->",
     frag("tar_sides.txt"),
     "  <!-- beach approach labels along the south edge -->"]
for name, sx in beach_labels:
    x.append('  <label text="%s" x="%.0f" y="%.0f" size="11" weight="bold" color="teal" halo="true"/>' % (name, sx * K, 1005 * K))
x.append('  <label text="The Pier" x="%.0f" y="%.0f" size="10" italic="true" color="white" angle="65" halo="true"/>' % (855 * K, 880 * K))
x.append('  <label text="Betio" x="%.0f" y="%.0f" size="20" italic="true" color="ink" halo="true"/>' % (700 * K, 470 * K))
x.append("  <!-- furniture: sequence-of-play cards across the top, turn track, holding boxes, legends -->")
for i, (title, x0, x1) in enumerate(cards):
    fill = "panel"
    stroke = "cardred" if "EVENT" in title or "FIRE" in title else "cardblue"
    x.append('  <panel id="card-%d" x="%.0f" y="%.0f" w="%.0f" h="%.0f" fill="%s" stroke="%s" title="%s"/>' % (
        i, x0 * K, 5 * K, (x1 - x0) * K, 185 * K, fill, stroke, title))
x.append('''  <panel id="turn-track" x="%.0f" y="%.0f" w="%.0f" h="%.0f" fill="panel" stroke="ink" title="TURN TRACK">
    <track x="6" y="24" cell-w="48" cell-h="58" wrap="10" fill="white"
           cells="1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30"/>
  </panel>''' % (1155 * K, 5 * K, 440 * K, 185 * K))
x.append('  <panel id="jp-eliminated" x="%.0f" y="%.0f" w="%.0f" h="%.0f" fill="panel" stroke="cardred" title="Japanese Eliminated Units"/>' % (115 * K, 195 * K, 170 * K, 65 * K))
x.append('  <panel id="depth-coastal" x="%.0f" y="%.0f" w="%.0f" h="%.0f" fill="panel" stroke="cardred" title="Japanese Coastal Depth Markers"/>' % (290 * K, 195 * K, 135 * K, 65 * K))
x.append('  <panel id="depth-inland" x="%.0f" y="%.0f" w="%.0f" h="%.0f" fill="panel" stroke="cardred" title="Japanese Inland Depth Markers"/>' % (430 * K, 195 * K, 140 * K, 65 * K))
x.append('  <panel id="jp-reserve" x="%.0f" y="%.0f" w="%.0f" h="%.0f" fill="panel" stroke="cardred" title="Japanese Reserve Units"/>' % (575 * K, 195 * K, 135 * K, 65 * K))
x.append('''  <panel id="cp-range" x="%.0f" y="%.0f" w="%.0f" h="%.0f" fill="cardred" stroke="cardred">
    <text x="4" y="14" size="8" weight="bold" color="white">US COMMAND POST RANGE</text>
    <track x="150" y="4" cell-w="26" cell-h="36" cells="1 2 2 3 3 3 4 4 4 5 5 5" fill="panel"/>
  </panel>''' % (770 * K, 195 * K, 380 * K, 40 * K))
x.append('''  <panel id="title" x="%.0f" y="%.0f" w="%.0f" h="%.0f" fill="paper" stroke="paper">
    <text x="0" y="34" size="34" weight="bold" color="cardred">D-DAY AT TARAWA</text>
    <text x="0" y="48" size="8">Game design: John Butterfield   Map graphics: Joe Youst   Map redesign: Delphine Echassoux (2014)</text>
  </panel>''' % (715 * K, 240 * K, 300 * K, 55 * K))
x.append('''  <panel id="allowed-actions" x="%.0f" y="%.0f" w="%.0f" h="%.0f" fill="panel" stroke="ink" title="JAPANESE ALLOWED ACTIONS">
    <track x="6" y="20" cell-w="50" cell-h="30" cells="Fire Turn_3 Turn_5 Turn_7 Turn_9 [I]_Turn_11" fill="white"/>
  </panel>''' % (330 * K, 285 * K, 290 * K, 55 * K))
x.append('  <panel id="shibasaki" x="%.0f" y="%.0f" w="%.0f" h="%.0f" fill="panel" stroke="ink" title="Adm. Shibasaki"/>' % (650 * K, 285 * K, 60 * K, 55 * K))
x.append('  <panel id="infantry-losses" x="%.0f" y="%.0f" w="%.0f" h="%.0f" fill="green" stroke="ink" title="US 2nd Marine Division Infantry Losses"/>' % (10 * K, 595 * K, 85 * K, 125 * K))
x.append('''  <panel id="lvts" x="%.0f" y="%.0f" w="%.0f" h="%.0f" fill="green" stroke="ink" title="Available LVTs">
    <box label="2 steps" x="6" y="30" w="46" h="90" fill="panel"/>
    <box label="1 step" x="58" y="30" w="46" h="90" fill="panel"/>
  </panel>''' % (100 * K, 595 * K, 100 * K, 125 * K))
x.append('''  <panel id="terrain-table" x="%.0f" y="%.0f" w="%.0f" h="%.0f" fill="panel" stroke="ink">
    <table x="4" y="4" cell-w="50" cell-h="22" size="5">
      <row><cell>Terrain</cell><cell>US Inf/Eng/HQ</cell><cell>Other US</cell><cell>Japanese Defense</cell></row>
      <row><cell>Beach, Clear, Airstrip</cell><cell>Yes</cell><cell>Yes</cell><cell>--</cell></row>
      <row><cell>Palm Trees</cell><cell>Yes</cell><cell>Yes</cell><cell>Halve ranged fire through</cell></row>
      <row><cell>Building</cell><cell>Yes</cell><cell>Yes</cell><cell>Unit doubled</cell></row>
      <row><cell>Fortified Building</cell><cell>Yes</cell><cell>Yes</cell><cell>Unit and depth doubled</cell></row>
      <row><cell>Rough / Crater</cell><cell>Yes</cell><cell>Yes, must stop</cell><cell>Unit doubled</cell></row>
      <row><cell>Seawall Hexside</cell><cell>Yes</cell><cell>Yes</cell><cell>Unit doubled</cell></row>
    </table>
  </panel>''' % (10 * K, 730 * K, 190 * K, 175 * K))
x.append('''  <panel id="water-legend" x="%.0f" y="%.0f" w="%.0f" h="%.0f" fill="panel" stroke="ink">
    <text x="6" y="12" size="7" weight="bold">Water Features</text>
    <text x="6" y="24" size="6">Water / Coral Reef Hexside / Pier Head / Pier Hexside (may not cross)</text>
    <text x="6" y="36" size="6">Water Fire Zone Boundary Line / Potential LVT Wreck (5.12) / USMC Arrival Box</text>
    <text x="110" y="12" size="7" weight="bold">Japanese Symbols</text>
    <text x="110" y="24" size="6">Position / Intense Fire Dot / Steady Fire Dots</text>
    <text x="110" y="36" size="6">Fire Position Connector</text>
  </panel>''' % (10 * K, 935 * K, 190 * K, 90 * K))
x.append("</sheet>")
open(OUT, "w", encoding="utf-8").write("\n".join(x))
print("wrote", OUT)
