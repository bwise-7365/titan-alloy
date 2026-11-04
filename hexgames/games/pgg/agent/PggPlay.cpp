// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// pgg_play -- plays games of PGG with two agents, one after another, and prints one line for each: its
// seed, the points, the level of victory, the commands, the refusals and the fingerprint. A game that can
// go no further is reported as a fault, and the rest still play. Each game's seed is
// mixSeed(base seed, game number), so one game can be played again alone.
//
//   pgg_play [--games N] [--seed S] [--setup fixed|drawn] [--german nearest|consolidate]
//            [--soviet nearest|consolidate] [--first G] [--refusals] [--events] [--root hexgames-dir]
//
// --first starts the numbering at game G, --refusals prints each refused command, and --events prints each
// game's events. Exit codes: 0 all finished, 1 a game faulted or
// did not finish, 2 usage, 3 the data would not load.
// ----------------------------------------------
#include "PggGame.h"

#include "hexmodel/Seed.h"

#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

  struct Options {
    int games = 1;
    int first = 0;
    std::uint64_t seed = 19410703;
    Pgg::SetUp setUp = Pgg::SetUp::Fixed;
    HexAi::Tactic german = HexAi::AttackOrConsolidate{};
    HexAi::Tactic soviet = HexAi::AttackOrConsolidate{};
    bool refusalsP = false;
    bool eventsP = false;
    std::filesystem::path root = HEXGAMES_SOURCE_DIR;
  };

  Options
  optionsOf(int argc, char** argv)
  {
    Options options;
    for (int i = 1; i < argc; ++i) {
      const std::string name = argv[i];
      if ("--refusals" == name) {
        options.refusalsP = true;
        continue;
      }
      if ("--events" == name) {
        options.eventsP = true;
        continue;
      }
      if (argc <= i + 1) {
        throw std::invalid_argument("'" + name + "' needs a value");
      }
      const std::string value = argv[++i];
      if ("--games" == name) {
        options.games = std::stoi(value);
      } else if ("--first" == name) {
        options.first = std::stoi(value);
      } else if ("--seed" == name) {
        options.seed = std::stoull(value);
      } else if ("--setup" == name) {
        if ("fixed" != value && "drawn" != value) {
          throw std::invalid_argument("--setup is fixed or drawn, not '" + value + "'");
        }
        options.setUp = "drawn" == value ? Pgg::SetUp::Drawn : Pgg::SetUp::Fixed;
      } else if ("--german" == name) {
        options.german = HexAi::tacticNamed(value);
      } else if ("--soviet" == name) {
        options.soviet = HexAi::tacticNamed(value);
      } else if ("--root" == name) {
        options.root = value;
      } else {
        throw std::invalid_argument("unknown option '" + name + "'");
      }
    }
    return options;
  }

  int
  run(const Options& options)
  {
    const Pgg::PggShared shared = Pgg::loadShared(options.root);
    int faults = 0;
    std::map<std::string, int> levels;
    for (int game = options.first; game < options.first + options.games; ++game) {
      Pgg::PggGameTask task;
      task.seed = HexModel::mixSeed(options.seed, static_cast<std::uint64_t>(game));
      task.setUp = options.setUp;
      task.german = options.german;
      task.soviet = options.soviet;
      task.keepEventsP = options.eventsP;
      const auto began = std::chrono::steady_clock::now();
      try {
        const Pgg::PggGameResult result = Pgg::playGame(shared, task);
        const auto took = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - began);
        std::cout << "game " << game << " seed " << task.seed << " german " << result.germanPoints.value_or(-1) << " soviet "
                  << result.sovietPoints.value_or(-1) << " '" << result.level << "' turn " << result.lastTurn << " commands "
                  << result.play.commands.size() << " refused " << result.play.refusals.size() << " fingerprint "
                  << result.play.fingerprint << " " << took.count() << " ms\n";
        if (options.refusalsP) {
          for (const std::string& refusal : result.play.refusals) {
            std::cout << "  refused " << refusal << "\n";
          }
        }
        if (!result.play.finishedP) {
          std::cout << "  did not finish\n";
          faults += 1;
        }
        levels[result.level] += 1;
        for (const std::string& event : result.events) {
          std::cout << "  " << event << "\n";
        }
      } catch (const std::exception& fault) {
        std::cout << "game " << game << " seed " << task.seed << " FAULT: " << fault.what() << "\n";
        faults += 1;
      }
    }
    for (const auto& [level, count] : levels) {
      std::cout << count << " x '" << level << "'\n";
    }
    std::cout << faults << " of " << options.games << " faulted or did not finish\n";
    return 0 == faults ? 0 : 1;
  }

}  // namespace

int
main(int argc, char** argv)
{
  Options options;
  try {
    options = optionsOf(argc, argv);
  } catch (const std::exception& usage) {
    std::cerr << "pgg_play: " << usage.what() << "\n";
    return 2;
  }
  try {
    return run(options);
  } catch (const std::exception& error) {
    std::cerr << "pgg_play: " << error.what() << "\n";
    return 3;
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
