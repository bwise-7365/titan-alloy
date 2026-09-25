// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// hexview tests: where the sheets and the reference renderer are, and how to run it.
// ----------------------------------------------
#pragma once
#include "hexxml/SheetDoc.h"
#include "hexxml/XmlDocument.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace HexViewTest {

  inline std::filesystem::path
  sourceRoot()
  {
    return std::filesystem::path(HEXVIEW_SOURCE_ROOT);
  }

  inline std::filesystem::path
  sheetPath(const std::string& name)
  {
    return sourceRoot() / "map_graphics" / "xml" / (name + ".xml");
  }

  inline HexXml::SheetDoc
  loadSheet(const std::string& name)
  {
    return HexXml::SheetDoc::parse(HexXml::XmlDocument::load(sheetPath(name)));
  }

  inline std::filesystem::path
  outDir()
  {
    return std::filesystem::path(HEXVIEW_OUT_DIR);
  }

  inline std::string
  readFile(const std::filesystem::path& p)
  {
    std::ifstream in(p, std::ios::binary);
    if (!in) {
      throw std::invalid_argument("cannot read " + p.string());
    }
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
  }

  inline void
  writeFile(const std::filesystem::path& p, const std::string& text)
  {
    std::ofstream out(p, std::ios::binary);
    if (!out) {
      throw std::invalid_argument("cannot write " + p.string());
    }
    out << text;
    return;
  }

  // Runs test/svg-golden.py with the given arguments; returns its exit status.
  inline int
  runReferenceScript(const std::vector<std::string>& args)
  {
    const std::filesystem::path script = sourceRoot() / "hexview" / "test" / "svg-golden.py";
    std::string cmd = "\"" + std::string(HEXVIEW_PYTHON) + "\" \"" + script.string() + "\"";
    for (const std::string& a : args) {
      cmd += " \"" + a + "\"";
    }
#ifdef _WIN32
    cmd = "\"" + cmd + "\"";  // cmd.exe strips one outer pair of quotes
#endif
    return std::system(cmd.c_str());
  }

}  // namespace HexViewTest
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
