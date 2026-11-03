# Copyright Ben Paul Wise. All Rights Reserved.
"""hexrules_path.py -- find the HexRules checkout and put its python/ directory on sys.path.

The schemas (hexsheet.xsd and the other four), validate.py and the reference renderers
hexsheet2svg.py and counters2svg.py are in the sibling repository HexRules, not in hexgames. A
script that needs them imports this module first, from this directory:

    import hexrules_path          # noqa: F401  (puts HexRules/python on sys.path)
    import hexsheet2svg as H

HexRules is the directory named by the environment variable HEXRULES_DIR, else the checkout beside
titan-alloy (C:/repos/ghub-per/HexRules beside C:/repos/ghub-per/titan-alloy). CMake's HEXRULES_DIR
cache variable does not reach Python run outside ctest; set the environment variable when HexRules
is elsewhere.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
HEXGAMES = os.path.normpath(os.path.join(HERE, "..", ".."))
HEXRULES_DIR = os.path.normpath(os.environ.get("HEXRULES_DIR") or os.path.join(HEXGAMES, "..", "..", "HexRules"))
PYTHON_DIR = os.path.join(HEXRULES_DIR, "python")
XSD_DIR = os.path.join(HEXRULES_DIR, "xsd")

if not os.path.isfile(os.path.join(XSD_DIR, "hexsheet.xsd")):
    raise ImportError("HexRules not found at %s (set HEXRULES_DIR to the HexRules checkout)" % HEXRULES_DIR)
if PYTHON_DIR not in sys.path:
    sys.path.insert(0, PYTHON_DIR)
# Copyright Ben Paul Wise. All Rights Reserved.
