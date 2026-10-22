// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The sides, the counters and the ids; the map facts are in PggFactsMap.cpp.
// ----------------------------------------------
#include "PggFacts.h"

#include <regex>
#include <stdexcept>

namespace Pgg {

  namespace {

    Arm
    armOfType(const std::string& type)
    {
      if ("soviet-rifle" == type) {
        return Arm::SovietRifle;
      }
      if ("soviet-armour" == type) {
        return Arm::SovietArmour;
      }
      if ("soviet-leader" == type) {
        return Arm::SovietLeader;
      }
      if ("german-infantry" == type) {
        return Arm::GermanInfantry;
      }
      if ("german-panzer" == type) {
        return Arm::GermanPanzer;
      }
      if ("german-motorized" == type) {
        return Arm::GermanMotorized;
      }
      if ("german-cavalry" == type) {
        return Arm::GermanCavalry;
      }
      if ("german-air-interdiction-marker" == type || "soviet-interdiction-marker" == type) {
        return Arm::Marker;
      }
      throw std::invalid_argument("PggFacts: unit type '" + type + "' is not a PGG unit type");
    }

    // A counter id's division, read off its shape (2.0: regiment/division, e.g. 20/12).
    std::optional<Division>
    divisionOfCounter(const std::string& id)
    {
      static const std::regex regiment("^s2-[0-9]+-([0-9]+)-(arm|inf|mot)-[0-9]+-10$");
      static const std::regex dasReich("^s2-[0-9]+-(de|dr|gr)-ss-[0-9]+-10$");
      static const std::regex independent("^s2-(gd|lehr)-mot-[0-9]+-10$");
      static const std::regex infantry("^s2-([0-9]+)-inf-[29]-7$");
      static const std::regex cavalry("^s2-([0-9]+)-cav-[0-9]+-[0-9]+$");
      std::smatch match;
      if (std::regex_match(id, match, regiment)) {
        const DivisionKind kind = "mot" == match[2].str() ? DivisionKind::Motorized : DivisionKind::Panzer;
        return Division{kind, std::stoi(match[1].str())};
      }
      if (std::regex_match(id, match, dasReich)) {
        return Division{DivisionKind::DasReich, 0};
      }
      if (std::regex_match(id, match, independent)) {
        return Division{DivisionKind::Independent, "gd" == match[1].str() ? 0 : 1};
      }
      if (std::regex_match(id, match, infantry)) {
        return Division{DivisionKind::Infantry, std::stoi(match[1].str())};
      }
      if (std::regex_match(id, match, cavalry)) {
        return Division{DivisionKind::Cavalry, std::stoi(match[1].str())};
      }
      return std::nullopt;
    }

  }  // namespace

  PggFacts::PggFacts(const HexRules::GameDefinition& definition) : definition_(definition)
  {
    const HexRules::RuleSet& rules = *definition.rules;
    german_ = rules.side("german");
    soviet_ = rules.side("soviet");
    readCounters();
    readDivisions();
    readHexes();
    readLists();
    readAreas();
    readPhases();
    const HexModel::Board& board = *definition.board;
    roadNet_ = board.networkId("road-net");
    railNet_ = board.networkId("rail");
    deadPile_ = board.spaceId("dead-pile");
    sovietPool_ = board.spaceId("soviet-pool");
    sovietArrivals_ = board.spaceId("soviet-arrivals");
    germanArrivals_ = board.spaceId("german-arrivals");
    for (std::size_t m = 0; m < rules.movement().modes.size(); ++m) {
      const std::string& id = rules.movement().modes[m].id;
      if ("normal" == id) {
        normal_ = ModeId{static_cast<std::uint32_t>(m)};
      }
      if ("rail-move" == id) {
        rail_ = ModeId{static_cast<std::uint32_t>(m)};
      }
    }
    if (rules.movement().modes.size() < 2 || "normal" != rules.movement().modes[normal_.value].id ||
        "rail-move" != rules.movement().modes[rail_.value].id) {
      throw std::invalid_argument("PggFacts: the rules document needs movement modes 'normal' and 'rail-move'");
    }
  }

  SideId
  PggFacts::enemy(SideId side) const
  {
    return german_ == side ? soviet_ : german_;
  }

  void
  PggFacts::readCounters()
  {
    const HexModel::Roster& roster = *definition_.roster;
    for (const UnitSpec& spec : roster.units()) {
      arms_.push_back(armOfType(definition_.rules->unitTypes()[spec.type.value].id));
    }
    return;
  }

  void
  PggFacts::readDivisions()
  {
    const HexModel::Roster& roster = *definition_.roster;
    for (const UnitSpec& spec : roster.units()) {
      const std::optional<Division> division =
          german_ == spec.side ? divisionOfCounter(spec.counter.text) : std::nullopt;
      divisionOf_.push_back(division);
      if (division) {
        members_[*division].push_back(spec.id);
      }
    }
    for (const auto& [division, units] : members_) {
      if (DivisionKind::Infantry != division.kind) {
        continue;
      }
      const std::string first = "s2-" + std::to_string(division.number) + "-inf-9-7";
      const std::string second = "s2-" + std::to_string(division.number) + "-inf-2-7";
      const UnitId full = counter(first);
      const UnitId reduced = counter(second);
      successor_[full.value] = reduced;
      predecessor_[reduced.value] = full;
    }
    for (const UnitSpec& spec : roster.units()) {
      if (german_ == spec.side && !markerP(spec.id) && !divisionOf_[spec.id.value]) {
        throw std::invalid_argument("PggFacts: German counter '" + spec.counter.text + "' names no division (2.0)");
      }
    }
    return;
  }

  Arm
  PggFacts::arm(UnitId unit) const
  {
    if (arms_.size() <= unit.value) {
      throw std::invalid_argument("PggFacts: unit index " + std::to_string(unit.value) + " outside the roster");
    }
    return arms_[unit.value];
  }

  bool
  PggFacts::sovietDivisionP(UnitId unit) const
  {
    return Arm::SovietRifle == arm(unit) || Arm::SovietArmour == arm(unit);
  }

  MoveClass
  PggFacts::moveClass(UnitId unit) const
  {
    switch (arm(unit)) {
      case Arm::SovietRifle:
      case Arm::GermanInfantry:
      case Arm::GermanCavalry:
        return MoveClass::Foot;
      case Arm::SovietArmour:
      case Arm::GermanPanzer:
      case Arm::GermanMotorized:
        return MoveClass::Motor;
      case Arm::SovietLeader:
        return MoveClass::Leader;
      case Arm::Marker:
        break;
    }
    throw std::invalid_argument("PggFacts: counter '" + definition_.roster->unit(unit).counter.text +
                                "' is a marker and does not move");
  }

  std::optional<Division>
  PggFacts::division(UnitId unit) const
  {
    if (divisionOf_.size() <= unit.value) {
      throw std::invalid_argument("PggFacts: unit index " + std::to_string(unit.value) + " outside the roster");
    }
    return divisionOf_[unit.value];
  }

  const std::vector<UnitId>&
  PggFacts::members(Division division) const
  {
    const auto found = members_.find(division);
    if (members_.end() == found) {
      throw std::invalid_argument("PggFacts: no counter belongs to division " + std::to_string(division.number));
    }
    return found->second;
  }

  std::vector<Division>
  PggFacts::divisions() const
  {
    std::vector<Division> out;
    for (const auto& [division, units] : members_) {
      out.push_back(division);
    }
    return out;
  }

  bool
  PggFacts::independentRegimentP(UnitId unit) const
  {
    const std::optional<Division> of = division(unit);
    return of && DivisionKind::Independent == of->kind;
  }

  std::optional<UnitId>
  PggFacts::successor(UnitId unit) const
  {
    const auto found = successor_.find(unit.value);
    return successor_.end() == found ? std::nullopt : std::optional<UnitId>(found->second);
  }

  std::optional<UnitId>
  PggFacts::predecessor(UnitId unit) const
  {
    const auto found = predecessor_.find(unit.value);
    return predecessor_.end() == found ? std::nullopt : std::optional<UnitId>(found->second);
  }

  int
  PggFacts::leaderRating(UnitId unit) const
  {
    const UnitSpec& spec = definition_.roster->unit(unit);
    if (!leaderP(unit) || !spec.front.range) {
      throw std::invalid_argument("PggFacts: counter '" + spec.counter.text + "' is not a Leader with a rating");
    }
    return *spec.front.range;
  }

  int
  PggFacts::railPoints(UnitId unit) const
  {
    switch (arm(unit)) {
      case Arm::SovietRifle:
        return 1;
      case Arm::SovietArmour:
        return 3;
      case Arm::SovietLeader:
        return 0;
      case Arm::GermanInfantry:
      case Arm::GermanPanzer:
      case Arm::GermanMotorized:
      case Arm::GermanCavalry:
      case Arm::Marker:
        break;
    }
    throw std::invalid_argument("PggFacts: counter '" + definition_.roster->unit(unit).counter.text +
                                "' may not move by rail (6.36)");
  }

  UnitId
  PggFacts::counter(std::string_view id) const
  {
    const std::optional<UnitId> found = definition_.roster->find(CounterId{std::string(id)});
    if (!found) {
      throw std::invalid_argument("PggFacts: no counter '" + std::string(id) + "'");
    }
    return *found;
  }

  UnitTypeId
  PggFacts::unitType(std::string_view id) const
  {
    return definition_.rules->unitType(std::string(id));
  }

  void
  PggFacts::readPhases()
  {
    const auto add = [this](const char* id, PhaseKind kind, std::optional<SideId> side) {
      phases_[definition_.rules->phase(id).value] = PhaseInfo{kind, side};
      return;
    };
    add("game-turn", PhaseKind::Container, std::nullopt);
    add("set-up", PhaseKind::SetUp, german_);
    add("soviet-turn", PhaseKind::Container, soviet_);
    add("soviet-move", PhaseKind::Move, soviet_);
    add("soviet-combat", PhaseKind::Combat, soviet_);
    add("soviet-disruption-removal", PhaseKind::DisruptionRemoval, soviet_);
    add("soviet-interdiction", PhaseKind::SovietInterdiction, soviet_);
    add("german-turn", PhaseKind::Container, german_);
    add("german-move1", PhaseKind::Move, german_);
    add("german-combat", PhaseKind::Combat, german_);
    add("german-move2", PhaseKind::MechanizedMove, german_);
    add("german-disruption-removal", PhaseKind::DisruptionRemoval, german_);
    add("german-air-interdiction", PhaseKind::AirInterdiction, german_);
    if (phases_.size() != definition_.rules->phaseIds().size()) {
      throw std::invalid_argument("PggFacts: the rules document's phase tree is not PGG's sequence of play (4.0)");
    }
    return;
  }

  PhaseInfo
  PggFacts::phase(PhaseId id) const
  {
    const auto found = phases_.find(id.value);
    if (phases_.end() == found) {
      throw std::invalid_argument("PggFacts: phase index " + std::to_string(id.value) + " is not a PGG phase");
    }
    return found->second;
  }

  PhaseId
  PggFacts::phaseNamed(std::string_view id) const
  {
    return definition_.rules->phase(std::string(id));
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
