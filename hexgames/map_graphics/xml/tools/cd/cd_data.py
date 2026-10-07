# Copyright Ben Paul Wise. All Rights Reserved.
"""cd_data.py -- the hand data of the Circling Dragons sheet: places, railways, strategic roads,
named ranges, lakes, labels.  Everything is (lat, lon); the lattice module turns it into hexes,
and PLACE_HEX moves the few places the 100 km lattice would otherwise put in the wrong hex
(the memorandum allows modest distortion to keep the topology of junctions and corridors).

Spellings follow the design memorandum (Peiping, Tientsin, Canton, Mukden; pinyin elsewhere).
kind: major (city-major), city, town, wp (a rail or road waypoint with no symbol or label).
flags: capital, port (sea), rport (river port), air (major airfield objective).
terrain: a ruling for the place's hex ("clear", "broken", "mountain"); None keeps the elevation class.
"""

# id, label, lat, lon, kind, flags, terrain ruling
PLACES = [
    # ---- North China
    ("peiping", "PEIPING", 39.90, 116.39, "major", "", "clear"),
    ("tientsin", "TIENTSIN", 39.08, 117.20, "major", "port", "clear"),
    ("baoding", "Baoding", 38.86, 115.47, "town", "", "clear"),
    ("shijiazhuang", "Shijiazhuang", 38.05, 114.48, "city", "", "clear"),
    ("taiyuan", "Taiyuan", 37.88, 112.54, "city", "", "broken"),
    ("datong", "Datong", 40.08, 113.30, "city", "", "broken"),
    ("kalgan", "Kalgan", 40.81, 114.85, "city", "", "broken"),
    ("guisui", "Guisui", 40.82, 111.66, "town", "", "broken"),
    ("baotou", "Baotou", 40.65, 109.82, "town", "", "broken"),
    ("jinan", "Jinan", 36.68, 116.99, "city", "", "clear"),
    ("qingdao", "Qingdao", 36.09, 120.33, "city", "port", "clear"),
    ("chefoo", "Chefoo", 37.53, 121.40, "town", "port", "clear"),
    ("weixian", "", 36.71, 119.10, "wp", "", None),
    ("puqi", "", 29.72, 113.88, "wp", "", None),        # the Yuehan railway runs south of the Yangtze by Puqi
    ("xingtai", "", 37.07, 114.50, "wp", "", "clear"),
    ("anyang", "Anyang", 36.08, 114.35, "town", "", "clear"),
    ("xinxiang", "Xinxiang", 35.30, 113.91, "town", "", "clear"),
    ("zhengzhou", "Zhengzhou", 34.76, 113.66, "city", "", "clear"),
    ("kaifeng", "Kaifeng", 34.79, 114.35, "city", "", "clear"),
    ("xuchang", "", 34.03, 113.85, "wp", "", None),
    ("luoyang", "Luoyang", 34.64, 112.44, "city", "", "clear"),
    ("xuzhou", "Xuzhou", 34.28, 117.18, "city", "", "clear"),
    ("shangqiu", "", 34.42, 115.65, "wp", "", None),
    ("haichow", "Haichow", 34.59, 119.23, "town", "port", "clear"),
    ("bengbu", "Bengbu", 32.95, 117.33, "town", "", "clear"),
    ("taian", "", 36.19, 117.12, "wp", "", None),
    ("tongguan", "Tongguan", 34.55, 110.25, "town", "", None),
    ("xian", "Xi'an", 34.28, 108.89, "city", "", "clear"),
    ("baoji", "Baoji", 34.38, 107.15, "town", "", "broken"),
    ("hanzhong", "Hanzhong", 33.13, 107.03, "town", "", "clear"),
    ("yanan", "Yan'an", 36.60, 109.50, "city", "capital", "broken"),
    ("lanzhou", "Lanzhou", 36.06, 103.79, "city", "", "broken"),
    ("pingliang", "", 35.54, 106.66, "wp", "", None),
    ("linfen", "Linfen", 36.09, 111.52, "town", "", "broken"),
    ("yuncheng", "", 35.03, 111.00, "wp", "", None),
    ("jining", "", 41.03, 113.08, "wp", "", None),
    ("shanhaiguan", "Shanhaiguan", 40.00, 119.75, "town", "port", "clear"),
    ("tangshan", "", 39.63, 118.19, "wp", "", "clear"),
    ("suizhong", "", 40.35, 120.30, "wp", "", None),   # the coastal corridor hex between Shanhaiguan and Jinzhou
    ("chengde", "Chengde", 40.96, 117.93, "town", "", None),
    ("gubeikou", "", 40.69, 117.16, "wp", "", None),
    ("yebaishou", "", 41.40, 119.65, "wp", "", None),
    ("chifeng", "Chifeng", 42.27, 118.95, "town", "", None),
    ("dolonnor", "Dolonnor", 42.20, 116.48, "town", "", None),
    # ---- Central China
    ("hankow", "WUHAN", 30.58, 114.27, "major", "rport", "clear"),
    ("yichang", "Yichang", 30.70, 111.28, "town", "rport", "clear"),
    ("changde", "Changde", 29.03, 111.68, "town", "", "clear"),
    ("yueyang", "Yueyang", 29.38, 113.10, "town", "", "clear"),
    ("changsha", "Changsha", 28.20, 112.97, "city", "", "clear"),
    ("zhuzhou", "Zhuzhou", 27.83, 113.15, "town", "", "clear"),
    ("hengyang", "Hengyang", 26.88, 112.59, "city", "air", "clear"),
    ("chenzhou", "", 25.80, 113.03, "wp", "", None),
    ("lingling", "Lingling", 26.42, 111.61, "town", "air", "broken"),
    ("baoqing", "Baoqing", 27.24, 111.47, "town", "", "broken"),
    ("zhijiang", "Chihchiang", 27.44, 109.68, "town", "air", "broken"),
    ("guilin", "Guilin", 25.28, 110.28, "city", "air", "broken"),
    ("liuzhou", "Liuzhou", 24.32, 109.41, "city", "air", "clear"),
    ("nanning", "Nanning", 22.82, 108.32, "city", "air", "clear"),
    ("hechi", "", 24.70, 108.08, "wp", "", None),
    ("dushan", "Dushan", 25.84, 107.55, "town", "", None),
    ("guiyang", "Guiyang", 26.58, 106.72, "city", "", "broken"),
    ("zunyi", "", 27.70, 106.92, "wp", "", None),
    ("qujing", "", 25.50, 103.80, "wp", "", None),
    ("zhenyuan", "", 27.05, 108.42, "wp", "", None),
    ("chongqing", "CHONGQING", 29.57, 106.59, "major", "capital rport", "broken"),
    ("chengdu", "Chengdu", 30.67, 104.07, "city", "air", "clear"),
    ("guangyuan", "", 32.44, 105.84, "wp", "", None),
    ("kunming", "Kunming", 25.04, 102.70, "city", "air", "broken"),
    ("wanxian", "Wanxian", 30.82, 108.40, "town", "rport", "broken"),
    ("enshi", "", 30.30, 109.48, "wp", "", None),
    ("laohekou", "Laohekou", 32.39, 111.68, "town", "air", "clear"),
    ("nanyang", "Nanyang", 33.00, 112.53, "town", "", "clear"),
    ("xinyang", "Xinyang", 32.13, 114.07, "town", "", None),
    ("nanchang", "Nanchang", 28.68, 115.88, "city", "", "clear"),
    ("jiujiang", "Jiujiang", 29.73, 115.98, "town", "rport", "clear"),
    ("anqing", "Anqing", 30.50, 117.05, "town", "rport", "clear"),
    ("wuhu", "Wuhu", 31.35, 118.37, "town", "rport", "clear"),
    ("nanking", "NANJING", 32.05, 118.78, "major", "capital rport", "clear"),
    ("zhenjiang", "", 32.18, 119.43, "wp", "", None),
    ("shanghai", "SHANGHAI", 31.22, 121.43, "major", "port", "clear"),
    ("hangzhou", "Hangzhou", 30.25, 120.17, "city", "", "clear"),
    ("ningbo", "Ningbo", 29.88, 121.55, "town", "port", "clear"),
    ("jinhua", "Jinhua", 29.12, 119.65, "town", "", None),
    ("shangrao", "", 28.47, 117.97, "wp", "", None),
    ("foochow", "Foochow", 26.08, 119.30, "city", "port", None),
    ("amoy", "Amoy", 24.45, 118.08, "town", "port", None),
    ("swatow", "Swatow", 23.37, 116.67, "town", "port", None),
    ("canton", "CANTON", 23.12, 113.26, "major", "port", "clear"),
    ("hongkong", "Hong Kong", 22.31, 114.18, "city", "port", None),
    ("shaoguan", "Shaoguan", 24.80, 113.58, "town", "", None),
    ("ganzhou", "Ganzhou", 25.92, 114.95, "town", "air", None),
    ("wuzhou", "Wuzhou", 23.48, 111.28, "town", "", None),
    ("guiping", "", 23.40, 110.08, "wp", "", None),
    # ---- Indochina
    ("hanoi", "Hanoi", 21.04, 105.85, "city", "", "clear"),
    ("haiphong", "Haiphong", 20.83, 106.68, "town", "port", "clear"),
    ("langson", "Lang Son", 21.85, 106.76, "town", "", None),
    ("laokay", "", 22.50, 103.97, "wp", "", None),
    # ---- Manchuria
    ("mukden", "MUKDEN", 41.80, 123.43, "major", "", "clear"),
    ("hsinking", "Changchun", 43.87, 125.34, "city", "capital", "clear"),
    ("harbin", "HARBIN", 45.75, 126.65, "major", "rport", "clear"),
    ("qiqihar", "Qiqihar", 47.35, 123.99, "city", "", "clear"),
    ("hailar", "Hailar", 49.20, 119.70, "town", "", None),
    ("manzhouli", "Manzhouli", 49.60, 117.43, "town", "sov", None),
    ("yakeshi", "", 49.28, 120.73, "wp", "", None),
    ("boketu", "", 48.76, 121.92, "wp", "", None),
    ("mudanjiang", "Mudanjiang", 44.58, 129.60, "city", "", "clear"),
    ("suifenhe", "Suifenhe", 44.40, 131.15, "town", "sov", None),
    ("jiamusi", "Jiamusi", 46.83, 130.35, "town", "rport", "clear"),
    ("linkou", "", 45.28, 130.25, "wp", "", None),
    ("kirin", "Kirin", 43.85, 126.55, "town", "", None),
    ("dunhua", "", 43.37, 128.23, "wp", "", None),
    ("tumen", "Tumen", 42.97, 129.82, "town", "sov", None),
    ("siping", "Siping", 43.17, 124.33, "town", "", "clear"),
    ("tieling", "", 42.29, 123.84, "wp", "", None),
    ("tongliao", "Tongliao", 43.62, 122.27, "town", "", None),
    ("baicheng", "", 45.62, 122.82, "wp", "", None),
    ("solun", "Solun", 46.60, 121.22, "town", "", None),
    ("jinzhou", "Jinzhou", 41.12, 121.10, "city", "port", "clear"),
    ("yingkou", "Yingkou", 40.67, 122.23, "town", "port", "clear"),
    ("liaoyang", "", 41.28, 123.18, "wp", "", None),
    ("dalian", "Dalian", 38.92, 121.63, "city", "port", "clear"),
    ("antung", "Antung", 40.15, 124.39, "town", "port", None),
    ("benxi", "", 41.33, 123.75, "wp", "", None),
    ("beian", "Beian", 48.25, 126.53, "town", "", None),
    ("suihua", "", 46.63, 126.98, "wp", "", None),
    ("heihe", "Heihe", 50.25, 127.45, "town", "sov", "clear"),
    # ---- Korea
    ("pyongyang", "Pyongyang", 39.02, 125.75, "city", "", None),
    ("seoul", "Seoul", 37.57, 127.00, "city", "", None),
    ("taegu", "", 35.87, 128.60, "wp", "", None),
    ("pusan", "Pusan", 35.10, 129.01, "city", "port", "clear"),
    ("wonsan", "Wonsan", 39.16, 127.43, "town", "port", "broken"),
    ("hamhung", "Hamhung", 39.91, 127.54, "town", "", "broken"),
    ("chongjin", "Chongjin", 41.78, 129.79, "town", "port", "broken"),
    # ---- USSR and Mongolia (staging; off-map in play). "sov" marks a Soviet entry hex of August 1945.
    ("borzya", "", 50.39, 116.52, "wp", "", None),
    ("khabarovsk", "Khabarovsk", 48.45, 135.12, "city", "sov", None),
    ("iman", "Iman", 45.93, 133.72, "town", "sov", None),
    ("vladivostok", "Vladivostok", 43.13, 131.91, "city", "port", None),
    ("choibalsan", "Choibalsan", 48.07, 114.51, "town", "", None),          # railhead from Borzya since 1939
    ("tamsag", "Tamsag Bulag", 47.23, 117.34, "town", "sov", None),         # forward base of the Khingan thrust
    ("sainshand", "Sain Shand", 44.90, 110.13, "town", "sov", None),        # Pliyev's Soviet-Mongolian group
    ("tongjiang", "Tongjiang", 47.65, 132.50, "town", "sov", None),         # Sungari mouth: 2nd Far Eastern Front
    # ---- Japan, Formosa
    ("nagasaki", "Nagasaki", 32.75, 129.88, "town", "port", None),
    ("taipei", "Taipei", 25.04, 121.57, "town", "port", "clear"),
]

# Places moved one hex from where the projection puts them, to keep corridors and ranges apart.
PLACE_HEX = {
    "taiyuan": "1415",    # into the Fen valley column, between the Luliang (col 13) and the Taihang (col 15)
    "hanzhong": "0820",   # into the Han valley row, between the Qinling (row 19) and the Daba (row 21)
    "baoqing": "1326",    # east of the Xuefeng wall, one hex from Hengyang
    "kaifeng": "1718",    # east of Zhengzhou on the Longhai (the two cities share a hex at 100 km)
    "suizhong": "2211",   # keeps the Peiping - Mukden line on the Liaoxi corridor, off the Bohai hex 2312
}
# The pins above are 100 km rulings; another scale has its own (the 75 km trial needs none so far).
PLACE_HEX_BY_SCALE = {100: PLACE_HEX, 75: {}}
# Coastal hexes ruled land at a scale because a decisive railway runs through them; each is just under half land
# in Natural Earth: the Liaodong spine south of Yingkou, the south shore of Hangzhou Bay, the Korean west coast.
LAND_HEXES_BY_SCALE = {100: "", 75: "3117 3131 3617"}

# River courses corrected by hand at a scale (hexsides as HEX:DIR, canonicalised when applied).  At 75 km the
# snapped Yangtze ran round the south of the Yueyang hex, so the Yueyang - Changsha railway crossed it; Yueyang
# and the railway are south of the river.  The river now passes north of the Yueyang hex, and the Xiang (with
# Dongting Lake) comes up the hex's west side to meet it (Ben's minor-river request, 2026-10-06).
# The same fault on the Yellow River between Tongguan and Zhengzhou: the snapped river ran round the south of
# the Sanmenxia hex 1724 and of the Luoyang hex 1924 and the north of 1824, so the Longhai railway crossed it
# four times there; Luoyang, Sanmenxia and the railway are south of the river.  The river now passes north of
# 1724 and 1924, and the only Longhai crossing left is the 1938 course between Zhengzhou and Kaifeng (Ben's
# ruling for Luoyang, 2026-10-07, carried one hex west to 1724 because the same railway crossed there).
RIVER_EDITS_BY_SCALE = {75: {
    "yangtze": dict(drop="1831:se 1832:ne 1932:s 1932:se", add="1932:n 1932:ne"),
    "xiang": dict(drop="1932:s", add="1832:ne 1831:se"),
    "yellow": dict(drop="1624:ne 1724:s 1724:se 1824:ne 1924:s 1924:se",
                   add="1724:nw 1724:n 1724:ne 1924:nw 1924:n 1924:ne"),
}}

# Minor rivers: printed only where they shaped a campaign, as explicit hexsides at a scale (Ben, 2026-10-06).
# The Xinqiang and the Miluo were Xue Yue's delay lines north of Changsha (1939-42).  At 75 km each crosses
# both the railway column and the lane east of it, and each joins the Xiang; the Laodao lies inside the
# Changsha hex and is left out.  key, printed name, hexsides from the mouth upstream.
MINOR_RIVERS_BY_SCALE = {75: [
    ("xinqiang", "Xinqiang", "1932:s 2032:sw 2032:s"),
    ("miluo", "Miluo", "1933:s 2033:sw 2033:s"),
]}
# labels that belong to one scale: text, lat, lon, size, angle, kind
LABELS_BY_SCALE = {75: [
    ("Xinqiang", 29.33, 113.32, 11, 0, "river"),
    ("Miluo", 28.66, 113.30, 11, 0, "river"),
]}

# Railways: id, printed name, list of place ids.  Consecutive places are joined by the straight
# hex line between their hexes.
RAILS = [
    ("pinghan", "Pinghan (Peiping - Hankow)", ["peiping", "baoding", "shijiazhuang", "xingtai", "anyang", "xinxiang",
                                              "zhengzhou", "xuchang", "xinyang", "hankow"]),
    ("yuehan", "Yuehan (Canton - Hankow)", ["hankow", "puqi", "yueyang", "changsha", "zhuzhou", "hengyang", "chenzhou",
                                           "shaoguan", "canton"]),
    ("xianggui", "Hunan - Guangxi", ["hengyang", "lingling", "guilin", "liuzhou"]),
    ("qiangui", "Guizhou - Guangxi", ["liuzhou", "hechi", "dushan"]),
    ("longhai", "Longhai", ["haichow", "xuzhou", "shangqiu", "kaifeng", "zhengzhou", "luoyang", "tongguan", "xian", "baoji"]),
    ("jinpu", "Tientsin - Pukow", ["tientsin", "jinan", "taian", "xuzhou", "bengbu", "nanking"]),
    ("pingsui", "Peiping - Suiyuan", ["peiping", "kalgan", "datong", "jining", "guisui", "baotou"]),
    ("tongpu", "Tongpu (Datong - Puzhou)", ["datong", "taiyuan", "linfen", "yuncheng", "tongguan"]),
    ("zhengtai", "Shijiazhuang - Taiyuan", ["shijiazhuang", "taiyuan"]),
    ("jiaoji", "Jinan - Qingdao", ["jinan", "weixian", "qingdao"]),
    ("beining", "Peiping - Mukden", ["peiping", "tientsin", "tangshan", "shanhaiguan", "suizhong", "jinzhou", "mukden"]),
    ("smr-south", "South Manchuria (Mukden - Dalian)", ["mukden", "liaoyang", "yingkou", "dalian"]),
    ("smr-north", "Mukden - Changchun - Harbin", ["mukden", "tieling", "siping", "hsinking", "harbin"]),
    ("cer-west", "Chinese Eastern, western line", ["manzhouli", "hailar", "yakeshi", "boketu", "qiqihar", "harbin"]),
    ("cer-east", "Chinese Eastern, eastern line", ["harbin", "mudanjiang", "suifenhe"]),
    ("beian", "Harbin - Beian - Heihe", ["harbin", "suihua", "beian", "heihe"]),
    ("suijia", "Suihua - Jiamusi", ["suihua", "jiamusi"]),
    ("tujia", "Tumen - Mudanjiang - Jiamusi", ["tumen", "mudanjiang", "linkou", "jiamusi"]),
    ("jingtu", "Changchun - Kirin - Tumen", ["hsinking", "kirin", "dunhua", "tumen"]),
    ("anfeng", "Mukden - Antung", ["mukden", "benxi", "antung"]),
    ("jingcheng", "Peiping - Chengde - Jinzhou", ["peiping", "gubeikou", "chengde", "yebaishou", "jinzhou"]),
    ("yechi", "Yebaishou - Chifeng", ["yebaishou", "chifeng"]),
    ("chifeng-siping", "Chifeng - Tongliao - Siping", ["chifeng", "tongliao", "siping"]),
    ("pingqi", "Tongliao - Baicheng - Qiqihar", ["tongliao", "baicheng", "qiqihar"]),
    ("solun", "Baicheng - Solun (Khingan line)", ["baicheng", "solun"]),
    ("huning", "Shanghai - Nanjing", ["shanghai", "zhenjiang", "nanking"]),
    ("huhang", "Shanghai - Hangzhou - Ningbo", ["shanghai", "hangzhou", "ningbo"]),
    ("zhegan", "Zhejiang - Jiangxi", ["hangzhou", "jinhua", "shangrao", "nanchang", "zhuzhou"]),
    ("nanxun", "Nanchang - Jiujiang", ["nanchang", "jiujiang"]),
    ("kcr", "Canton - Kowloon", ["canton", "hongkong"]),
    ("yunnan", "Yunnan - Indochina (metre gauge, cut 1940)", ["haiphong", "hanoi", "laokay", "kunming"]),
    ("langson", "Hanoi - Lang Son", ["hanoi", "langson"]),
    ("korea-trunk", "Pusan - Seoul - Pyongyang - Antung", ["pusan", "taegu", "seoul", "pyongyang", "antung"]),
    ("korea-east", "Seoul - Wonsan - Chongjin - Tumen", ["seoul", "wonsan", "hamhung", "chongjin", "tumen"]),
    ("transbaikal", "Trans-Siberian (Borzya - Manzhouli)", ["borzya", "manzhouli"]),
    ("choibalsan-rail", "Borzya - Choibalsan (1939)", ["borzya", "choibalsan"]),
    ("ussuri-rail", "Khabarovsk - Vladivostok", ["khabarovsk", "iman", "vladivostok"]),
    ("grodekovo", "Vladivostok - Suifenhe", ["vladivostok", "suifenhe"]),
    ("hanoi-south", "Hanoi - Saigon railway (exit)", ["hanoi"]),
]
# Lines that leave the map: rail id, the place at the end, and the direction of the exit hex when
# the place is not itself on the map edge.
RAIL_EXITS = [
    ("transbaikal", "borzya", None),       # Borzya is on the north edge: to Chita
    ("ussuri-rail", "khabarovsk", None),   # east edge: to the Trans-Siberian
    ("beian", "heihe", None),              # north edge: Blagoveshchensk and Belogorsk
    ("hanoi-south", "hanoi", "s"),         # one hex south, then the edge
]

ROADS = [
    ("burma", "Burma Road", ["kunming"]),                      # exits west
    ("dianqian", "Kunming - Guiyang", ["kunming", "qujing", "guiyang"]),
    ("qianchuan", "Guiyang - Chongqing", ["guiyang", "zunyi", "chongqing"]),
    ("chengyu", "Chongqing - Chengdu", ["chongqing", "chengdu"]),
    ("qiangui-road", "Guiyang - Dushan", ["guiyang", "dushan"]),
    ("xiangqian", "Hengyang - Chihchiang - Guiyang", ["hengyang", "baoqing", "zhijiang", "zhenyuan", "guiyang"]),
    ("chuanshaan", "Chengdu - Hanzhong - Baoji", ["chengdu", "guangyuan", "hanzhong", "baoji"]),
    ("xiyan", "Xi'an - Yan'an", ["xian", "yanan"]),
    ("xilan", "Xi'an - Lanzhou", ["xian", "pingliang", "lanzhou"]),
    ("nanning-road", "Liuzhou - Nanning - Lang Son", ["liuzhou", "nanning", "langson"]),
    ("xijiang", "Canton - Wuzhou - Liuzhou", ["canton", "wuzhou", "guiping", "liuzhou"]),
    ("chuane", "Wanxian - Enshi - Yichang", ["wanxian", "enshi", "yichang"]),
    ("yuxi", "Laohekou - Nanyang - Luoyang", ["laohekou", "nanyang", "luoyang"]),
    ("kalgan-dolonnor", "Kalgan - Dolonnor", ["kalgan", "dolonnor"]),
]
ROAD_EXITS = [
    ("burma", "kunming", "w"),
]

# Named ranges as explicit hex lists (decided against the elevation diagnostic): class, printed name, hexes.
RANGES = [
    ("mountain", "Taihang Shan", "1513 1514 1515 1516 1517 1713"),
    ("mountain", "Yan Shan", "1811 1912 2011"),
    ("mountain", "Luliang Shan", "1314 1315 1316"),
    ("mountain", "Qinling / Funiu", "0719 0819 0919 1019 1119 1219 1319 1419"),
    ("mountain", "Daba / Wu Shan", "0721 0821 0921 1021 1121 1122 1222"),
    ("mountain", "Dabie Shan", "1722 1822"),
    ("mountain", "Xuefeng Shan", "1225 1226 1127 1128"),
    ("mountain", "Nanling", "1228 1329 1429 1529 1629"),
    ("mountain", "Wuyi Shan", "2026 2126 1927 1827 1828"),
    ("mountain", "Greater Khingan", "2301 2302 2303 2203 2204 2205 2206 2107 2108 2008 2009"),
    ("mountain", "Lesser Khingan", "2602 2702 2802 2902 3003"),
    ("mountain", "Changbai Shan", "2811 2911 2910 3009 3008 3010"),
    ("mountain", "Zhangguangcai", "3005 3006 3007"),
    ("broken", "Wanda Shan", "3205 3206 3207"),
    ("broken", "Shandong hills", "1917 2016 2117"),
    ("broken", "Luoxiao Shan", "1624 1625 1626 1527 1528"),
]

# Lakes drawn as a mark in a marsh hex (only where the lake has a hex of its own at 100 km).
LAKES = [
    ("Tai Hu", 31.20, 120.20),
    ("Hongze", 33.30, 118.60),
    ("Khanka", 45.00, 132.40),
]

# Area labels: text, lat, lon, size, angle, kind (country | sea | range | river | area)
AREA_LABELS = [
    ("CHINA", 35.9, 116.1, 36, 0, "country"),
    ("MANCHUKUO", 45.6, 123.4, 26, 0, "country"),
    ("MONGOLIA", 44.2, 106.0, 26, 0, "country"),
    ("Gobi", 43.0, 109.5, 14, 0, "area"),
    ("U.S.S.R.", 50.0, 111.5, 26, 0, "country"),
    ("U.S.S.R.", 47.0, 135.0, 18, -75, "country"),
    ("KOREA", 36.6, 128.0, 20, 0, "country"),
    ("INDOCHINA", 20.9, 104.6, 16, 0, "country"),
    ("JAPAN", 31.6, 131.6, 18, 0, "country"),
    ("FORMOSA", 23.3, 122.4, 14, 0, "country"),
    ("Bohai", 38.6, 119.9, 16, 0, "sea"),
    ("Yellow Sea", 35.5, 123.0, 18, 0, "sea"),
    ("East China Sea", 27.3, 122.4, 18, 0, "sea"),
    ("South China Sea", 20.7, 112.6, 18, 0, "sea"),
    ("Sea of Japan", 40.5, 133.5, 18, 0, "sea"),
    ("Yangtze", 30.0, 108.6, 15, 0, "river"),
    ("Yangtze", 31.0, 119.0, 15, 0, "river"),
    ("Yellow River", 40.5, 108.4, 15, 0, "river"),
    ("Yellow River (1938 course)", 33.4, 116.6, 13, -35, "river"),
    ("Han", 32.4, 110.6, 13, 0, "river"),
    ("Xiang", 27.5, 112.9, 13, 0, "river"),
    ("Amur", 50.0, 130.5, 15, 0, "river"),
    ("Argun", 49.6, 118.7, 13, 0, "river"),
    ("Ussuri", 46.5, 133.8, 13, 0, "river"),
    ("Sungari", 46.7, 128.8, 13, 0, "river"),
    ("Nen", 48.3, 124.4, 13, 0, "river"),
    ("Liao", 42.2, 122.3, 13, 0, "river"),
    ("Yalu", 41.0, 125.4, 12, 0, "river"),
    ("Tumen", 42.6, 130.6, 12, 0, "river"),
    ("TAIHANG", 37.7, 113.3, 14, -80, "range"),
    ("QINLING", 33.95, 108.6, 14, -8, "range"),
    ("DABA SHAN", 32.1, 108.4, 13, -20, "range"),
    ("XUEFENG", 27.4, 110.5, 13, -55, "range"),
    ("NANLING", 25.2, 112.2, 14, -5, "range"),
    ("WUYI", 27.0, 117.0, 13, -55, "range"),
    ("GREATER KHINGAN", 47.4, 120.5, 14, -75, "range"),
    ("LESSER KHINGAN", 48.9, 127.6, 13, -40, "range"),
    ("CHANGBAI", 42.2, 127.9, 13, -45, "range"),
    ("LULIANG", 37.6, 111.2, 12, -85, "range"),
    ("DABIE", 31.5, 115.6, 12, 0, "range"),
    ("Ordos", 39.0, 108.6, 14, 0, "area"),
    ("Hulunbuir", 48.4, 118.4, 14, 0, "area"),
    ("Sanjiang plain", 47.0, 132.8, 13, 0, "area"),
    ("Loess plateau", 36.4, 108.3, 13, 0, "area"),
    ("Sichuan basin", 30.8, 105.3, 13, 0, "area"),
    ("Jehol", 41.8, 118.3, 13, 0, "area"),
    ("Chahar", 42.3, 114.5, 12, 0, "area"),
    ("Yellow River flood zone", 33.6, 115.6, 11, -35, "area"),
]

# Manchukuo, for the region tint: Chinese hexes inside this (lat, lon) polygon
MANCHUKUO = [(40.0, 119.9), (40.3, 118.9), (40.6, 117.3), (41.1, 116.6), (42.4, 116.1), (44.2, 116.9), (46.4, 118.6),
             (48.0, 116.0), (50.8, 116.8), (50.8, 136.0), (46.0, 136.0), (42.0, 131.0), (42.2, 128.9), (41.3, 126.8),
             (40.4, 125.1), (39.5, 124.3), (38.6, 122.5), (39.4, 121.0), (39.9, 120.4)]

# ---- map version 2: the Mongolian operations (build_cd.py VERSION >= 2) --------------------------------------
# kind "mark": a place drawn as its marks only, with a small label (a well, a pass, a fortified zone).
# flags: pass (a named mountain pass), fort (a Kwantung Army fortified zone), well (a water point of the Gobi),
# air (an airfield), autonomy (version 3).
PLACES_V2 = [
    ("ulanbator", "Ulan Bator", 47.92, 106.91, "town", "", None),             # Urga: the Soviet 17th Army's base
    ("erenhot", "Erenhot", 43.65, 111.98, "mark", "well", None),               # Dzamin Uud: the border well on the Urga road
    ("sonid", "Sonid", 42.75, 112.65, "mark", "well", None),                   # Saihan Tal, a Gobi water point
    ("matad", "Matad", 47.00, 115.50, "mark", "air", None),                    # a Soviet airfield of August 1945
    ("lubei", "Lubei", 44.58, 121.30, "mark", "well", None),                   # east foot of the Khingan, 6th Guards Tank Army
    ("lubei-pass", "Lubei trail", 44.90, 119.50, "mark", "pass", None),        # the tank army's crossing of the crest
    ("arshaan", "Arshaan", 47.17, 119.94, "mark", "pass fort", None),          # Halung-Arshaan: the Solun gap and its fortified zone
    ("sunwu", "Sunwu", 49.43, 127.33, "mark", "fort", None),
    ("hutou", "Hutou", 45.97, 133.65, "mark", "fort", None),                   # held out to 26 August 1945
    ("dongning", "Dongning", 44.06, 131.12, "mark", "fort", None),
]
# flags added to version-1 places in version 2
PLACE_FLAGS_V2 = {"hailar": "fort", "heihe": "fort", "boketu": "pass", "kalgan": "pass", "tamsag": "air", "choibalsan": "air"}
# the Gobi tracks and the Soviet supply road: link kind "track"
TRACKS_V2 = [
    ("kalgan-urga", "Kalgan - Urga road", ["kalgan", "erenhot", "sainshand", "ulanbator"]),
    ("gobi-trail", "Erenhot - Sonid - Dolonnor trail", ["erenhot", "sonid", "dolonnor"]),
    ("khingan-trail", "Tamsag Bulag - Lubei trail", ["tamsag", "lubei-pass", "lubei", "tongliao"]),
    ("supply-road", "Choibalsan - Tamsag Bulag supply road", ["choibalsan", "tamsag"]),
]
# waterless Gobi and Alashan: steppe hexes inside these (lat, lon) polygons become "desert"
DESERT = [
    [(45.6, 103.5), (45.2, 109.5), (44.4, 113.4), (43.1, 114.6), (41.7, 113.2), (41.5, 108.0), (41.9, 104.0)],
    [(37.6, 101.5), (42.2, 101.5), (42.0, 106.6), (39.9, 106.9), (38.2, 105.6)],
]
LABELS_V2 = [
    ("GOBI", 43.9, 109.0, 22, 0, "area"),
    ("Khingan crossings", 46.0, 121.6, 11, -75, "area"),
]

# ---- map version 3: the political layer (build_cd.py VERSION >= 3) ------------------------------------------
PLACES_V3 = [
    ("tonghua", "Tonghua", 41.68, 125.75, "town", "", None),                   # the Kwantung Army's planned redoubt
    ("yulin", "", 38.28, 109.73, "wp", "", None),
]
PLACE_FLAGS_V3 = {"sonid": "autonomy"}                                        # the Inner Mongolian declaration of September 1945
# Mengjiang, Prince De's Japanese-sponsored Inner Mongolia: Chinese hexes inside this polygon (Chahar and Suiyuan)
MENGJIANG = [(40.2, 108.6), (41.7, 108.2), (43.3, 112.0), (43.1, 116.5), (41.3, 116.7), (40.3, 115.5), (40.0, 112.5)]
# the Tonghua redoubt: the Changbai hexes along the Korean border the Kwantung Army meant to fall back on
REDOUBT_HEXES = "2811 2911 2910 3009 3010"
# the weather divide: land hexes north of this latitude are the winter zone, south of it the monsoon zone
WEATHER_DIVIDE_LAT = 33.5
# the speculative Soviet liaison route from Yan'an across the Ordos to the Mongolian border: link kind "courier"
COURIER_V3 = [("liaison", "Soviet liaison via Mongolia (speculative)", ["yanan", "yulin", "erenhot"])]
LABELS_V3 = [
    ("MENGJIANG", 41.7, 113.1, 18, 0, "country"),
    ("Tonghua redoubt", 41.5, 127.3, 11, -45, "area"),
    ("winter zone: cold Nov - Mar", 34.7, 105.4, 11, 0, "area"),
    ("monsoon zone: Jun - Sep", 32.4, 105.4, 11, 0, "area"),
]
# Copyright Ben Paul Wise. All Rights Reserved.
