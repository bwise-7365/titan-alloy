// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Loading the PGG package. PGG counters print four value-line shapes (3.4): a German combat
// strength and movement allowance ("9-7", "4-10"), a Soviet attack-defence-movement ("2-4-6"), an
// untried face with its movement only ("U-6"), and a Soviet Leader's rating and allowance
// ("(3)*10", printed with a star). The reader throws on any other text.
// ----------------------------------------------
#pragma once
#include "hexmodel/Roster.h"
#include "hexrules/Package.h"

#include <filesystem>
#include <memory>
#include <string_view>

namespace Pgg {

  // "9-7": attack and defence 9, allowance 7. "2-4-6": attack 2, defence 4, allowance 6. "U-6": no
  // strengths, allowance 6. "(3)*10": no attack, defence 3, range (the Leader's radius) 3, allowance 10.
  HexModel::Strengths valueLine(std::string_view, HexModel::UnitKind);

  std::shared_ptr<const HexRules::GameDefinition> loadPackage(const std::filesystem::path& manifest);

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
