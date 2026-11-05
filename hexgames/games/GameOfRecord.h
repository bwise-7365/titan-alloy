// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// From a hexsave document to what it is played with: the package it names, and the policy set of the game's
// own module where there is one. hexgames_cli and pgg_trc_viewer both open records this way.
// ----------------------------------------------
#pragma once
#include "PggBinding.h"
#include "PggPolicySet.h"
#include "TrcPolicySet.h"

#include "hexengine/Defaults.h"
#include "hexengine/GameNames.h"
#include "hexrecord/SaveModel.h"
#include "hexrules/Package.h"

#include <filesystem>
#include <memory>

namespace HexGames {

  // The package a record names, resolved against the record's own directory.
  inline std::shared_ptr<const HexRules::GameDefinition>
  definitionOf(const std::filesystem::path& record)
  {
    const HexRecord::SaveModel model = HexRecord::SaveModel::read(record);
    const std::filesystem::path manifest = std::filesystem::weakly_canonical(record.parent_path() / model.package);
    const HexModel::ValueLineReader reader =
        "pgg" == model.game ? HexModel::ValueLineReader(&Pgg::valueLine) : HexModel::ValueLineReader(&HexEngine::defaultValueLine);
    return std::make_shared<const HexRules::GameDefinition>(HexRules::PackageLoader::load(manifest, reader));
  }

  // The policy set a record is played with: the game's own module where there is one.
  class PolicyChoice {
  public:
    PolicyChoice(const HexRules::GameDefinition& definition, bool defaultsP)
    {
      if (!defaultsP && "trc" == definition.rules->gameId()) {
        trc_ = std::make_unique<Trc::TrcPolicySet>(definition);
      } else if (!defaultsP && "pgg" == definition.rules->gameId()) {
        pgg_ = std::make_unique<Pgg::PggPolicySet>(definition);
      } else {
        defaults_ = std::make_unique<HexEngine::DefaultPolicySet>(definition, HexEngine::GameSteps::Withheld);
      }
    }
    const HexEngine::Policies&
    policies() const
    {
      if (trc_) {
        return trc_->policies();
      }
      return pgg_ ? pgg_->policies() : defaults_->policies();
    }
    const HexEngine::GameNames&
    names() const
    {
      if (trc_) {
        return trc_->names();
      }
      return pgg_ ? pgg_->names() : defaults_->names();
    }
    const Pgg::PggPolicySet* pgg() const { return pgg_.get(); }
    const HexEngine::DefaultPolicySet* defaults() const { return defaults_.get(); }

  private:
    std::unique_ptr<Trc::TrcPolicySet> trc_;
    std::unique_ptr<Pgg::PggPolicySet> pgg_;
    std::unique_ptr<HexEngine::DefaultPolicySet> defaults_;
  };

}  // namespace HexGames
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
