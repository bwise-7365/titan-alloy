// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// hexgames_cli -- the headless front end: check a package's bindings, replay a script and print its
// event log, compare a replay with its golden, or record a fresh golden.
// ----------------------------------------------
#include "hexengine/Defaults.h"
#include "hexengine/GameNames.h"
#include "hexengine/Session.h"
#include "hexrecord/GoldenCompare.h"
#include "hexrecord/Record.h"
#include "hexrecord/SaveModel.h"
#include "hexrules/Package.h"

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
                  "       hexgames_cli --replay <script.xml> [--golden <golden.xml>]\n"
                  "       hexgames_cli --record <script.xml> <out.golden.xml>\n";
    return 2;
  }

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
  replay(const std::filesystem::path& script, const std::filesystem::path& golden, bool goldenGivenP)
  {
    const std::shared_ptr<const HexRules::GameDefinition> definition = definitionOf(script);
    const HexEngine::DefaultPolicySet defaults(*definition);

    if (goldenGivenP) {
      const HexRecord::GoldenReport report = HexRecord::compareWithGolden(
          script, golden, definition, defaults.policies(), *defaults.policies().grammar);
      std::cout << HexRecord::formatGoldenReport(report, definition->rules->gameId(),
                                                  golden.stem().string());
      return report.matchP ? 0 : 1;
    }

    const HexRecord::Record record =
        HexRecord::readRecord(script, *definition, *defaults.policies().grammar);
    HexEngine::Session session =
        HexRecord::sessionFor(record, definition, defaults.policies());
    HexRecord::playRecord(session, record);
    for (const HexEngine::Event& event : session.events().events()) {
      std::cout << HexEngine::TextEventEncoder::line(event, defaults.names()) << "\n";
    }
    std::cout << "digest " << session.position().digest() << "\n";
    return 0;
  }

  int
  record(const std::filesystem::path& script, const std::filesystem::path& out)
  {
    const std::shared_ptr<const HexRules::GameDefinition> definition = definitionOf(script);
    const HexEngine::DefaultPolicySet defaults(*definition);
    const HexRecord::Record read =
        HexRecord::readRecord(script, *definition, *defaults.policies().grammar);
    HexEngine::Session session = HexRecord::sessionFor(read, definition, defaults.policies());
    const std::vector<HexRecord::ScriptedMove> played = HexRecord::playRecord(session, read);
    HexRecord::writeRecord(out, HexRecord::Kind::Golden, session, played,
                            *defaults.policies().grammar);
    std::cout << "wrote " << out.string() << " (" << played.size() << " moves, "
              << session.events().size() << " events)\n";
    return 0;
  }

}  // namespace

int
main(int argc, char** argv)
{
  const std::vector<std::string> args(argv + 1, argv + argc);
  try {
    if (2 == args.size() && "--validate" == args[0]) {
      return validate(args[1]);
    }
    if (2 == args.size() && "--replay" == args[0]) {
      return replay(args[1], std::filesystem::path(), false);
    }
    if (4 == args.size() && "--replay" == args[0] && "--golden" == args[2]) {
      return replay(args[1], args[3], true);
    }
    if (3 == args.size() && "--record" == args[0]) {
      return record(args[1], args[2]);
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
