// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// hexgen_cli -- generate a map sheet from a hexgen.xsd settings document, and optionally draw it.
//   hexgen_cli <settings.xml> <out-sheet.xml> [--seed N] [--svg <out.svg>]
// --seed replaces the document's seed. Exit codes as hexgames_cli: 2 for usage, 3 for an error.
// ----------------------------------------------
#include "hexgen/Generate.h"
#include "hexgen/Settings.h"

#include "hexmaped/SheetWriter.h"
#include "hexview/MapFrame.h"
#include "hexview/MapSceneBuilder.h"
#include "hexview/Scene.h"
#include "hexview/SceneWriters.h"
#include "hexview/SymbolLibrary.h"

#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

  int
  usage()
  {
    std::cerr << "usage: hexgen_cli <settings.xml> <out-sheet.xml> [--seed N] [--svg <out.svg>]\n";
    return 2;
  }

  void
  writeSvg(const std::filesystem::path& path, const HexXml::SheetDoc& sheet)
  {
    const HexView::MapFrame frame = HexView::MapFrame::of(sheet);
    const HexView::SymbolLibrary symbols = HexView::SymbolLibrary::reference();
    HexView::Scene scene(frame.width(), frame.height());
    HexView::MapSceneBuilder(frame, symbols, HexView::MapStyle::reference()).build(sheet, scene);
    std::ofstream file(path, std::ios::binary);
    file << HexView::writeSvg(scene, symbols, sheet.title);
    if (!file) {
      throw std::invalid_argument("hexgen_cli: cannot write " + path.string());
    }
    return;
  }

}  // namespace

int
main(int argc, char** argv)
{
  const std::vector<std::string> args(argv + 1, argv + argc);
  if (2 > args.size()) {
    return usage();
  }
  std::optional<std::int64_t> seed;
  std::optional<std::filesystem::path> svg;
  for (std::size_t k = 2; k < args.size(); k += 2) {
    if (k + 1 >= args.size()) {
      return usage();
    }
    if ("--seed" == args[k]) {
      try {
        std::size_t used = 0;
        seed = std::stoll(args[k + 1], &used);
        if (used != args[k + 1].size()) {
          return usage();
        }
      }
      catch (const std::exception&) {
        return usage();
      }
    }
    else if ("--svg" == args[k]) {
      svg = args[k + 1];
    }
    else {
      return usage();
    }
  }
  try {
    HexGen::Settings settings = HexGen::Settings::load(args[0]);
    if (seed) {
      settings.seed = *seed;
    }
    const HexXml::SheetDoc sheet = HexGen::generate(settings);
    HexMapEd::writeSheet(std::filesystem::path(args[1]), sheet);
    if (svg) {
      writeSvg(*svg, sheet);
    }
    std::cout << args[1] << ": " << settings.cols << " x " << settings.rows << ", seed " << settings.seed << ", "
              << sheet.links.size() << " road links\n";
  }
  catch (const std::exception& e) {
    std::cerr << "hexgen_cli: " << e.what() << "\n";
    return 3;
  }
  return 0;
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
