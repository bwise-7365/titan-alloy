# Copyright Ben Paul Wise. All Rights Reserved.
"""build_pgg.py -- assemble the Panzergruppe Guderian sheet (flat-topped test case)."""
import sys

sys.path.insert(0, r"C:\repos\ghub-per\titan-alloy\hexgames\map_graphics\xml")
from lxml import etree
import hexsheet2svg as H

OUT = r"C:\repos\ghub-per\titan-alloy\hexgames\map_graphics\xml\panzergruppe-guderian.xml"
GRID = dict(orientation="flat", offset="odd", size="61.65", ox="138.13", oy="125.02", cols="56", rows="31",
            terrain="clear")
GRID["id-format"] = "{col:02}{row:02}"; GRID["col-start"] = "1"; GRID["row-start"] = "1"
el = etree.Element("grid", **GRID)
g = H.Grid(el)


def near(x, y):
    """printed id of the hex whose centre is nearest scan pixel (x, y)"""
    return min(g.cells.items(), key=lambda kv: (g.centre(*kv[0])[0] - x) ** 2 + (g.centre(*kv[0])[1] - y) ** 2)[1]


def frag(name):
    return "\n".join(l for l in open(name, encoding="utf-8").read().splitlines() if l.startswith("<"))


K = 5615 / 1600.0   # small-image pixel -> scan pixel
cities = [  # (name, small x, small y, symbol)
    ("РЖЕВ", 1040, 45, "city-major"), ("ВИТЕБСК", 150, 395, "city-major"), ("ГЖАТСК", 1230, 215, "city-major"),
    ("ВЯЗЬМА", 1040, 462, "city-major"), ("СМОЛЕНСК", 560, 522, "city-major"), ("ОРША", 115, 615, "city-major"),
    ("МОГИЛЁВ", 110, 748, "city-major"), ("РОСЛАВЛЬ", 690, 795, "city-major"), ("КАЛУГА", 1560, 612, "city-major"),
    ("Велиж", 395, 282, "town"), ("Белый", 790, 172, "town"), ("Мстиславль", 392, 735, "town"), ("Кричев", 370, 842, "town"),
]
rivers = [("Днепр", 60, 545), ("Зап. Двина", 255, 200), ("Десна", 700, 880), ("Сож", 330, 790), ("Угра", 1130, 600),
          ("Волга", 1090, 25), ("Вопь", 640, 305), ("Остёр", 560, 860)]

x = []
x.append('<?xml version="1.0" encoding="UTF-8"?>')
x.append('<sheet xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="hexsheet.xsd"')
x.append('       id="pgg" title="Panzergruppe Guderian: The Battle of Smolensk, July 1941"')
x.append('       source="PGG - Cyrillic.png, 5615 x 3727; Russian-language redesign of the 1976 SPI map"')
x.append('       width="5615" height="3727" background="margin" font="Arial, Helvetica, sans-serif">')
x.append('  <grid id="main" orientation="flat" offset="odd" cols="56" rows="31" size="61.65" ox="138.13" oy="125.02"')
x.append('        id-format="{col:02}{row:02}" col-start="1" row-start="1" terrain="clear" id-side="n"/>')
x.append('''  <palette>
    <color id="margin" value="#aeae80" name="sheet margin"/>
    <color id="paper" value="#f4faf3"/>
    <color id="leaf" value="#7ea153"/>
    <color id="moss" value="#c8cfa8"/>
    <color id="water" value="#05bcf2"/>
    <color id="grid" value="#8a8a8a"/>
    <color id="gridw" value="#4a9ac8"/>
    <color id="riverblue" value="#2a7fb0"/>
    <color id="white" value="#ffffff"/>
    <color id="grey" value="#8c8c8c"/>
    <color id="ink" value="#111111"/>
    <color id="blue" value="#1f6fa8"/>
    <color id="panel" value="#f6f6ee"/>
  </palette>
  <terrains>
    <terrain id="clear" name="Открытая местность" fill="paper" stroke="grid"/>
    <terrain id="woods" name="Лес" fill="leaf" stroke="grid" pattern="mottle"/>
    <terrain id="swamp" name="Болото" fill="moss" stroke="grid" pattern="hatch"/>
    <terrain id="lake" name="Озеро" fill="water" stroke="gridw"/>
  </terrains>
  <lines>
    <line id="river" stroke="riverblue" width="7"/>
    <line id="rail" stroke="white" width="4" dash="10 10" casing="ink" casing-width="9"/>
    <line id="road" stroke="grey" width="5"/>
  </lines>''')
x.append("  <!-- terrain, sampled from the scan at every hex centre -->")
x.append(frag("pgg_terrain.txt"))
x.append("  <!-- rivers along hexsides, traced from the scan -->")
x.append(frag("pgg_edges.txt"))
x.append("  <!-- rail and road, centre to centre, traced from the scan -->")
x.append(frag("pgg_links.txt"))
x.append("  <!-- cities and towns, placed by hand -->")
for name, sx, sy, sym in cities:
    pid = near(sx * K, sy * K)
    x.append('  <hex id="%s" name="%s"><glyph symbol="%s" color="ink"/></hex>' % (pid, name, sym))
    x.append('  <label text="%s" at="%s" slot="s" size="%d" weight="%s"/>' % (
        name, pid, 34 if sym == "city-major" else 26, "bold" if sym == "city-major" else "normal"))
x.append("  <!-- river names -->")
for name, sx, sy in rivers:
    x.append('  <label text="%s" x="%.0f" y="%.0f" size="26" italic="true" color="blue" angle="-30"/>' % (name, sx * K, sy * K))
x.append('''  <!-- furniture -->
  <panel id="title" x="60" y="3470" w="1500" h="230" fill="margin" stroke="margin">
    <text x="20" y="90" size="86" weight="bold">Panzergruppe Guderian:</text>
    <text x="120" y="170" size="52" weight="bold">THE BATTLE OF SMOLENSK, JULY 1941</text>
  </panel>
  <panel id="turn-track" x="2240" y="3470" w="2360" h="230" fill="panel" stroke="ink" title="Дорожка хода">
    <track x="20" y="40" cell-w="190" cell-h="170" cells="1 2 3 4 5 6 7 8 9 10 11 12" fill="water"/>
  </panel>
  <panel id="key" x="4660" y="3455" w="920" h="250" fill="panel" stroke="ink" title="Тип местности">
    <box label="Открытая" x="20" y="40" w="170" h="90" fill="paper"/>
    <box label="Лес" x="200" y="40" w="170" h="90" fill="leaf"/>
    <box label="Болото" x="380" y="40" w="170" h="90" fill="moss"/>
    <box label="Озеро" x="560" y="40" w="170" h="90" fill="water"/>
    <box label="Большой город" x="20" y="145" w="240" h="90"/>
    <box label="Посёлок" x="280" y="145" w="200" h="90"/>
    <box label="Ж/д, дорога, река" x="500" y="145" w="400" h="90"/>
  </panel>
</sheet>''')
open(OUT, "w", encoding="utf-8").write("\n".join(x))
print("wrote", OUT)
# Copyright Ben Paul Wise. All Rights Reserved.
