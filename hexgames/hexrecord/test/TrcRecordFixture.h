// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The TRC package, its engine test script and that script's golden: what the M4 replay and golden
// tests all start from.
// ----------------------------------------------
#pragma once
#include "hexengine/Defaults.h"
#include "hexrecord/Record.h"
#include "hexrules/Package.h"

#include <filesystem>
#include <memory>

namespace TrcRecord {

  inline std::filesystem::path
  root()
  {
    return std::filesystem::path(HEXGAMES_SOURCE_DIR);
  }

  inline std::filesystem::path
  script()
  {
    return root() / "game_records" / "xml" / "trc-test-script.xml";
  }

  inline std::filesystem::path
  golden()
  {
    return root() / "game_records" / "xml" / "trc-test.golden.xml";
  }

  inline std::shared_ptr<const HexRules::GameDefinition>
  definition()
  {
    return std::make_shared<const HexRules::GameDefinition>(HexRules::PackageLoader::load(
        root() / "packages" / "xml" / "trc.package.xml", &HexEngine::defaultValueLine));
  }

}  // namespace TrcRecord
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
