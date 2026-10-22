// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Loading the TRC package. TRC counters print one combat factor, used in attack and defence, and a
// movement factor ("8-7", 2.4-2.5); workers print one number ("2") or a bracketed one ("(2)"). The
// engine's default reader already reads these; the TRC reader is the strict form of it and throws
// on a value line it does not understand instead of reading no strengths at all.
// ----------------------------------------------
#pragma once
#include "hexmodel/Roster.h"
#include "hexrules/Package.h"

#include <filesystem>
#include <memory>
#include <string_view>

namespace Trc {

  HexModel::Strengths valueLine(std::string_view, HexModel::UnitKind);

  std::shared_ptr<const HexRules::GameDefinition> loadPackage(const std::filesystem::path& manifest);

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
