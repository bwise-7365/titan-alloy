// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcBinding.h"

#include "hexengine/Defaults.h"

#include <stdexcept>
#include <string>

namespace Trc {

  HexModel::Strengths
  valueLine(std::string_view line, HexModel::UnitKind kind)
  {
    const HexModel::Strengths read = HexEngine::defaultValueLine(line, kind);
    if (!read.attack) {
      throw std::invalid_argument("Trc::valueLine: '" + std::string(line) +
                                   "' is not a TRC value line (a factor, or a factor and a movement factor)");
    }
    return read;
  }

  std::shared_ptr<const HexRules::GameDefinition>
  loadPackage(const std::filesystem::path& manifest)
  {
    return std::make_shared<const HexRules::GameDefinition>(HexRules::PackageLoader::load(manifest, &valueLine));
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
