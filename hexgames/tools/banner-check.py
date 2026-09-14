# Copyright Ben Paul Wise. All Rights Reserved.
"""banner-check.py ROOT -- fail unless every source and text file carries the copyright banner.

C++ (.h .hpp .cpp): the three-line banner as the first three and last three non-empty lines.
Markdown and text (.md .txt), CMake (CMakeLists.txt .cmake), Python (.py), PowerShell (.ps1) and
PlantUML (.puml): the one-line form (with the language's comment prefix) as the first and last
non-empty line. A Python script may keep a "#!" line above its first banner line.

XML, SVG and PNG are not checked (an XML file cannot put a comment before its declaration).
Exemptions are listed in EXEMPT below with their reason.
"""
import os
import sys

LINE = "Copyright Ben Paul Wise. All Rights Reserved."
RULER = "// ----------------------------------------------"
CPP = (".h", ".hpp", ".cpp")
ONE_LINE = {".md": "", ".txt": "", ".cmake": "# ", ".py": "# ", ".ps1": "# ", ".puml": "' "}
ROOTS = ["hexcoord", "hexxml", "hexmodel", "hexrules", "hexsearch", "hexengine", "hexrecord",
         "hexview", "hexqt", "games", "tools", "tasks", "tests", "cmake", "uml", "doc",
         "game_records", "packages", "game_rules", "map_graphics", "unit_graphics"]
TOP_FILES = ["PLAN.md", "BUGS.txt", "CMakeLists.txt", ".gitignore"]
EXEMPT = {
    "CLAUDE.md": "harness instruction file, as in irrgo",
    "CMakePresets.json": "JSON has no comments",
    "2026-09-12-2002-plan-request.txt": "Ben's request, not a project file",
    "military-unit-icons-handoff.md": "a hand-off document, not written for this project",
    "validate.py": "game_rules/xml/validate.py, the pre-project validator tools/validate-xml.py replaces",
}
SKIP_DIRS = {"cmake-build-debug", "cmake-build-release", "cmake-build-headless", "build", "out",
             "__pycache__", ".idea", ".vs", "_deps", "CMakeFiles", "golden"}


def nonempty(lines):
    return [l.rstrip("\r\n") for l in lines if l.strip()]


def check(path):
    name = os.path.basename(path)
    if name in EXEMPT:
        return None
    ext = os.path.splitext(name)[1].lower()
    with open(path, encoding="utf-8", errors="replace") as f:
        lines = nonempty(f.readlines())
    if ext in CPP:
        want = [RULER, "// " + LINE, RULER]
        if len(lines) < 6 or lines[:3] != want or lines[-3:] != want:
            return "missing C++ banner at top and bottom"
        return None
    if name == "CMakeLists.txt":
        prefix = "# "
    elif ext in ONE_LINE:
        prefix = ONE_LINE[ext]
    elif name == ".gitignore":
        prefix = "# "
    else:
        return None
    want = prefix + LINE
    if ".py" == ext and lines and lines[0].startswith("#!"):
        lines = lines[1:]
    if len(lines) < 2 or lines[0] != want or lines[-1] != want:
        return "missing '%s' as first and last line" % want
    return None


def main(root):
    failures = []
    candidates = [os.path.join(root, f) for f in TOP_FILES if os.path.exists(os.path.join(root, f))]
    for sub in ROOTS:
        top = os.path.join(root, sub)
        for dirpath, dirnames, filenames in os.walk(top):
            dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
            candidates.extend(os.path.join(dirpath, f) for f in filenames)
    for path in candidates:
        problem = check(path)
        if problem:
            failures.append("%s: %s" % (os.path.relpath(path, root), problem))
    for f in failures:
        print(f)
    print("banner-check: %d files, %d failures" % (len(candidates), len(failures)))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1] if len(sys.argv) > 1 else "."))
# Copyright Ben Paul Wise. All Rights Reserved.
