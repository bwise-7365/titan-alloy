// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// hexgames_cli -- the headless front end: check a package's bindings, replay a script and print its
// event log, compare a replay with its golden, or record a fresh golden. A record of a game with a
// module of its own (TRC since M6) is played with that module's policies; --defaults plays it with
// the engine's defaults instead, which is what the engine's own smoke golden is recorded with.
// ----------------------------------------------
#include "TrcPolicySet.h"

#include "hexengine/Defaults.h"
#include "hexengine/GameNames.h"
#include "hexengine/Session.h"
#include "hexrecord/GoldenCompare.h"
#include "hexrecord/Record.h"
#include "hexrecord/SaveModel.h"
#include "hexrules/Package.h"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

  int
  usage()
  {
    std::cerr << "usage: hexgames_cli --validate <package.xml>\n"
                  "       hexgames_cli --replay <script.xml> [--golden <golden.xml>] [--defaults]\n"
                  "       hexgames_cli --record <script.xml> <out.golden.xml> [--defaults]\n";
    return 2;
  }

  // The policy set a record is played with: the game's own module where there is one.
  class PolicyChoice {
  public:
    PolicyChoice(const HexRules::GameDefinition& definition, bool defaultsP)
    {
      if (!defaultsP && "trc" == definition.rules->gameId()) {
        trc_ = std::make_unique<Trc::TrcPolicySet>(definition);
      } else {
        defaults_ = std::make_unique<HexEngine::DefaultPolicySet>(definition, HexEngine::GameSteps::Withheld);
      }
    }
    const HexEngine::Policies& policies() const { return trc_ ? trc_->policies() : defaults_->policies(); }
    const HexEngine::GameNames& names() const { return trc_ ? trc_->names() : defaults_->names(); }

    // A run on engine defaults names every step behaviour of the rules it withheld (M6b).
    void
    reportWithheld() const
    {
      if (defaults_) {
        for (const auto& [does, why] : defaults_->steps().withheld()) {
          std::cerr << "hexgames_cli: withheld step behaviour '" << does << "': " << why << "\n";
        }
      }
      return;
    }

  private:
    std::unique_ptr<Trc::TrcPolicySet> trc_;
    std::unique_ptr<HexEngine::DefaultPolicySet> defaults_;
  };

  int
  validate(const std::filesystem::path& manifest)
  {
    const std::vector<HexRules::PackageProblem> problems =
        HexRules::PackageLoader::check(manifest, &HexEngine::defaultValueLine);
    for (const HexRules::PackageProblem& problem : problems) {
      std::cout << problem.file << ":" << problem.line << ": " << problem.message << "\n";
    }
    std::cout << manifest.string() << ": " << problems.size() << " problem"
              << (1 == problems.size() ? "" : "s") << "\n";
    return problems.empty() ? 0 : 1;
  }

  // The package a record names, resolved against the record's own directory.
  std::shared_ptr<const HexRules::GameDefinition>
  definitionOf(const std::filesystem::path& record)
  {
    const HexRecord::SaveModel model = HexRecord::SaveModel::read(record);
    const std::filesystem::path manifest =
        std::filesystem::weakly_canonical(record.parent_path() / model.package);
    return std::make_shared<const HexRules::GameDefinition>(
        HexRules::PackageLoader::load(manifest, &HexEngine::defaultValueLine));
  }

  int
  replay(const std::filesystem::path& script, const std::filesystem::path& golden, bool goldenGivenP, bool defaultsP)
  {
    const std::shared_ptr<const HexRules::GameDefinition> definition = definitionOf(script);
    const PolicyChoice choice(*definition, defaultsP);
    choice.reportWithheld();

    if (goldenGivenP) {
      const HexRecord::GoldenReport report = HexRecord::compareWithGolden(
          script, golden, definition, choice.policies(), *choice.policies().grammar);
      std::cout << HexRecord::formatGoldenReport(report, definition->rules->gameId(),
                                                  golden.stem().string());
      return report.matchP ? 0 : 1;
    }

    const HexRecord::Record record =
        HexRecord::readRecord(script, *definition, choice.policies());
    HexEngine::Session session =
        HexRecord::sessionFor(record, definition, choice.policies());
    HexRecord::playRecord(session, record);
    for (const HexEngine::Event& event : session.events().events()) {
      std::cout << HexEngine::TextEventEncoder::line(event, choice.names()) << "\n";
    }
    std::cout << "digest " << session.position().digest() << "\n";
    // A script that stops on a decision shows what the next answer may be, one line per option.
    if (session.prompt().decisionPendingP) {
      for (const HexEngine::Command& command : session.legalCommands()) {
        std::cout << "pending " << choice.policies().grammar->verb(command);
        for (const auto& [name, value] : choice.policies().grammar->arguments(command)) {
          std::cout << " " << name << "=" << value;
        }
        std::cout << "\n";
      }
    }
    return 0;
  }

  int
  record(const std::filesystem::path& script, const std::filesystem::path& out, bool defaultsP)
  {
    const std::shared_ptr<const HexRules::GameDefinition> definition = definitionOf(script);
    const PolicyChoice choice(*definition, defaultsP);
    choice.reportWithheld();
    const HexRecord::Record read =
        HexRecord::readRecord(script, *definition, choice.policies());
    HexEngine::Session session = HexRecord::sessionFor(read, definition, choice.policies());
    const std::vector<HexRecord::ScriptedMove> played = HexRecord::playRecord(session, read);
    HexRecord::writeRecord(out, HexRecord::Kind::Golden, session, played,
                            *choice.policies().grammar);
    std::cout << "wrote " << out.string() << " (" << played.size() << " moves, "
              << session.events().size() << " events)\n";
    return 0;
  }

}  // namespace

int
main(int argc, char** argv)
{
  std::vector<std::string> args(argv + 1, argv + argc);
  const auto defaultsAt = std::find(args.begin(), args.end(), "--defaults");
  const bool defaultsP = args.end() != defaultsAt;
  if (defaultsP) {
    args.erase(defaultsAt);
  }
  try {
    if (2 == args.size() && "--validate" == args[0]) {
      return validate(args[1]);
    }
    if (2 == args.size() && "--replay" == args[0]) {
      return replay(args[1], std::filesystem::path(), false, defaultsP);
    }
    if (4 == args.size() && "--replay" == args[0] && "--golden" == args[2]) {
      return replay(args[1], args[3], true, defaultsP);
    }
    if (3 == args.size() && "--record" == args[0]) {
      return record(args[1], args[2], defaultsP);
    }
    return usage();
  } catch (const std::exception& e) {
    std::cerr << "hexgames_cli: " << e.what() << "\n";
    return 3;
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
