# Copyright Ben Paul Wise. All Rights Reserved.
# printed panels over the hex field, in photo pixels (read off the overview); cells under them are not in play
PANELS = {
    "casualty-track": (55, 755, 225, 1190),
    "turn-track": (232, 1050, 562, 1195),
    "tec": (15, 1188, 580, 1568),
    "tec-2": (583, 1192, 868, 1568),
    "crt": (1995, 1195, 2396, 1545),
}


def underP(x, y):
    return any(x0 <= x <= x1 and y0 <= y <= y1 for x0, y0, x1, y1 in PANELS.values())
# Copyright Ben Paul Wise. All Rights Reserved.
