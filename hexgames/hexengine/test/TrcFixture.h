// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The real TRC package and its engine test scenario, loaded once per test executable: the only
// board, roster and rules the hexengine tests need, and the position every session starts from.
// ----------------------------------------------
#pragma once
#include "hexengine/Defaults.h"
#include "hexengine/Session.h"
#include "hexmodel/PositionBuilder.h"
#include "hexrules/Package.h"
#include "hexxml/SaveDoc.h"
#include "hexxml/XmlDocument.h"

#include <filesystem>
#include <memory>
#include <string>

namespace TrcFixture {

  inline std::filesystem::path
  root()
  {
    return std::filesystem::path(HEXGAMES_SOURCE_DIR);
  }

  inline std::filesystem::path
  manifest()
  {
    return root() / "packages" / "xml" / "trc.package.xml";
  }

  inline std::shared_ptr<const HexRules::GameDefinition>
  definition()
  {
    return std::make_shared<const HexRules::GameDefinition>(
        HexRules::PackageLoader::load(manifest(), &HexEngine::defaultValueLine));
  }

  inline HexModel::Position
  scenario(const HexRules::GameDefinition& definition)
  {
    const HexXml::SaveDoc save = HexXml::SaveDoc::parse(
        HexXml::XmlDocument::load(root() / "game_records" / "xml" / "trc-test.xml"));
    const HexEngine::NoGameStateCodec state(*definition.rules);
    const HexEngine::NoObligationCodec obligations;
    return HexModel::PositionBuilder::build(save, *definition.board, *definition.roster, *definition.rules, state,
                                            obligations);
  }

  inline HexModel::UnitId
  unitOf(const HexRules::GameDefinition& definition, const std::string& counter)
  {
    const std::optional<HexModel::UnitId> found = definition.roster->find(HexModel::CounterId{counter});
    if (!found) {
      throw std::invalid_argument("TrcFixture: the roster has no counter '" + counter + "'");
    }
    return *found;
  }

  inline HexModel::HexIndex
  hexOf(const HexRules::GameDefinition& definition, const std::string& id)
  {
    return definition.board->indexOf(HexCoord::HexId{id});
  }

}  // namespace TrcFixture
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
