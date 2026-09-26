# Copyright Ben Paul Wise. All Rights Reserved.
# places read by eye on the labelled tiles (P_ij.jpg): hex -> (name, kind); kind town | city
PLACES = {
    "1003": ("Makurazaki", "town"), "1108": ("Kure", "town"), "1509": ("Kagoshima", "city"),
    "1706": ("Kushikino", "town"), "2006": ("Sendai", "town"), "2509": ("Izumi", "town"),
    "2710": ("Marushima", "town"), "2811": ("Ashiki", "town"),
    "3905": ("Nagasaki", "city"), "4007": ("Isahaya", "town"), "4705": ("Sonogi", "town"), "4807": ("Imari", "town"),
    "5109": ("Karatsu", "town"),
    "0913": ("Kanoya", "town"), "0715": ("Kitano", "town"), "1116": ("Shibushi", "town"), "0918": ("Tokashi", "town"),
    "0818": ("Miyanoura", "town"), "1220": ("Obi", "town"), "1616": ("Miyakonojo", "town"), "1822": ("Miyazaki", "town"),
    "2214": ("Yokomachi", "town"), "2116": ("Kobayashi", "town"), "2515": ("Hitoyoshi", "town"),
    "2419": ("Murasho", "town"), "3012": ("Hinagu", "town"), "3114": ("Yatsushiro", "town"), "3320": ("Hama-Machi", "town"),
    "4612": ("Saga", "town"), "4213": ("Omuta", "town"), "3616": ("Kumamoto", "city"), "4716": ("Kurume", "town"),
    "4521": ("Hida", "town"), "5115": ("Fukuoka", "city"), "5215": ("Fukuoka", "city"), "5517": ("Tsuyazaki", "town"),
    "5620": ("Yawata", "city"), "5621": ("Yawata", "city"),
    "2325": ("Tsuno", "town"), "2928": ("Nobeoka", "town"), "4228": ("Beppu", "town"), "4130": ("Oita", "town"),
    "4926": ("Nakatsu", "town"), "5423": ("Kokura", "city"), "5523": ("Kokura", "city"),
}
# the lower photo edge cuts off the southern rows (Saeki and south): read from "olympic full map, tilted.jpg"
# Japanese deployment letters (C / L / 2L / 36C ...) read in hexes: hex -> text
DEPLOY = {
    "1003": "L", "1004": "C", "1005": "C", "0906": "C", "0907": "C", "1108": "L", "1305": "C", "1405": "C",
    "1705": "C", "1805": "C", "1606": "C", "1608": "2L", "2006": "L", "2105": "C", "2205": "C", "2306": "C",
    "2405": "C", "2509": "L", "2811": "L", "0911": "C", "3905": "L", "4807": "L", "5109": "L", "4612": "L",
    "5311": "C", "5411": "C", "5312": "C",
    "0913": "L", "0715": "L", "0815": "C", "0915": "C", "1015": "C", "1116": "CL", "1117": "C", "0918": "C", "1018": "L",
    "1120": "C", "1220": "L", "1522": "C", "1616": "2L", "1722": "C", "1812": "36C", "1714": "36L", "1814": "36L",
    "1715": "36L", "1615": "36C", "1815": "36L", "1916": "36L", "1917": "36C", "2214": "2L", "2016": "L", "2115": "C",
    "2515": "L", "3320": "2L", "4213": "L", "4414": "C", "3616": "L", "5114": "2L", "5215": "2L", "5517": "L",
    "5617": "C", "5618": "C", "5619": "C", "5412": "C",
    "1622": "C", "1923": "C", "2023": "C", "2124": "C", "2224": "C", "2325": "L", "2526": "C", "2527": "C", "2627": "C",
    "2728": "C", "2928": "L", "4228": "L", "5523": "L", "5624": "R",
}
# Copyright Ben Paul Wise. All Rights Reserved.
