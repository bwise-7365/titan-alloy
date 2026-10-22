// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// A game package: the rules, sheet and counters documents of one game plus the bindings between
// their id spaces (hexpackage.xsd). Loading a package yields the immutable trio a Session needs.
// ----------------------------------------------
#pragma once
#include "hexmodel/Board.h"
#include "hexmodel/Roster.h"
#include "hexrules/RuleSet.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace HexRules {

  struct PackageProblem {
    std::string file;
    int line = 0;
    std::string message;  // always names the offending id
  };

  // Everything immutable about a game, shared read-only by every Session.
  struct GameDefinition {
    std::shared_ptr<const RuleSet> rules;
    std::shared_ptr<const Board> board;
    std::shared_ptr<const Roster> roster;
    std::filesystem::path packagePath;
  };

  class PackageLoader {
  public:
    // Parses the manifest and the three documents, applies the bindings, builds the Board and
    // Roster. Throws std::invalid_argument with the first problem; `check` collects all of them.
    static GameDefinition load(const std::filesystem::path& manifest, const ValueLineReader&);
    static std::vector<PackageProblem> check(const std::filesystem::path& manifest, const ValueLineReader&);
  };

}  // namespace HexRules
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
