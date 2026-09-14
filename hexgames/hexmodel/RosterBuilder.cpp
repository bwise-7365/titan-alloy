// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Compiled into the hexrules static library (see BoardBuilder.cpp for why).
// ----------------------------------------------
#include "hexmodel/RosterBuilder.h"

#include "hexrules/RuleSet.h"

#include <map>
#include <regex>
#include <stdexcept>

namespace HexModel {

  namespace {

    bool
    bindingMatchesP(const HexXml::PackageUnitBindingDoc& binding, const std::string& counterId)
    {
      for (const std::string& explicitId : binding.counters) {
        if (explicitId == counterId) {
          return true;
        }
      }
      if (binding.match) {
        return std::regex_match(counterId, std::regex(*binding.match));
      }
      return false;
    }

    const HexXml::PackageUnitBindingDoc&
    findBinding(const HexXml::PackageDoc& package, const std::string& counterId)
    {
      const HexXml::PackageUnitBindingDoc* found = nullptr;
      for (const HexXml::PackageUnitBindingDoc& binding : package.unit) {
        if (bindingMatchesP(binding, counterId)) {
          if (nullptr != found) {
            throw std::invalid_argument("RosterBuilder: counter '" + counterId +
                                         "' matches more than one unit binding");
          }
          found = &binding;
        }
      }
      if (nullptr == found) {
        throw std::invalid_argument("RosterBuilder: counter '" + counterId +
                                     "' matches no unit binding");
      }
      return *found;
    }

    UnitKind
    toUnitKind(const std::string& kind, const std::string& counterId)
    {
      if ("ground" == kind) {
        return UnitKind::Ground;
      }
      if ("air" == kind) {
        return UnitKind::Air;
      }
      if ("naval" == kind) {
        return UnitKind::Naval;
      }
      if ("hq" == kind) {
        return UnitKind::Hq;
      }
      if ("leader" == kind) {
        return UnitKind::Leader;
      }
      if ("marker" == kind) {
        return UnitKind::Marker;
      }
      throw std::invalid_argument("RosterBuilder: counter '" + counterId + "': unknown unit-type kind '" +
                                   kind + "'");
    }

    // Counter style -> rules side, from the package's <side> elements. A style named by two
    // elements is a package error in its own right, reported here rather than silently taking the
    // last binding.
    std::map<std::string, SideId>
    sideByStyle(const HexXml::PackageDoc& package, const HexRules::RuleSet& rules)
    {
      std::map<std::string, SideId> bound;
      for (const HexXml::PackageSideBindingDoc& binding : package.side) {
        const SideId side = rules.side(binding.rules);
        for (const std::string& style : binding.styles) {
          const auto existing = bound.find(style);
          if (bound.end() != existing) {
            throw std::invalid_argument("RosterBuilder: counter style '" + style +
                                         "' is bound to more than one side");
          }
          bound[style] = side;
        }
      }
      return bound;
    }

    // The counter's printed ground colour names its side; the unit type's own @side mask, where the
    // rules give one, is a consistency check on that binding and never a source of its own.
    SideId
    resolveSide(const HexRules::UnitType& type, const HexXml::CounterDoc& counter,
                 const std::map<std::string, SideId>& bound, const HexRules::RuleSet& rules)
    {
      if (!counter.front.style) {
        throw std::invalid_argument("RosterBuilder: counter '" + counter.id +
                                     "' has no front style, so no side can be bound to it");
      }
      const auto it = bound.find(*counter.front.style);
      if (bound.end() == it) {
        throw std::invalid_argument("RosterBuilder: counter '" + counter.id + "' has style '" +
                                     *counter.front.style + "', which no package <side> binds");
      }
      const SideId side = it->second;
      if (type.sides.any() && !type.sides.test(side.value)) {
        throw std::invalid_argument("RosterBuilder: counter '" + counter.id + "' is bound to side '" +
                                     rules.sides()[side.value].id + "', which its unit type '" + type.id +
                                     "' excludes");
      }
      return side;
    }

    int
    maxStepsOf(const HexXml::CounterFaceDoc& front)
    {
      int maxSteps = 1;
      for (const HexXml::CounterStepsDoc& s : front.steps) {
        const int candidate = s.maxCount.value_or(s.count);
        if (candidate > maxSteps) {
          maxSteps = candidate;
        }
      }
      return maxSteps;
    }

    Strengths
    strengthsOfFace(const HexXml::CounterFaceDoc& face, UnitKind kind, const std::string&,
                     const ValueLineReader& reader)
    {
      // Not every counter prints a value line (Stuka/Sturmovik support counters show only text);
      // such a counter simply has an all-nullopt Strengths rather than a thrown error.
      if (face.values.empty()) {
        return Strengths{};
      }
      return reader(face.values.front().text, kind);
    }

    UnitSpec
    buildOne(const HexXml::CounterDoc& counter, const std::string& counterIdText, const HexRules::RuleSet& rules,
             const HexXml::PackageDoc& package, const std::map<std::string, SideId>& bound,
             const ValueLineReader& reader, std::uint32_t unitIndex)
    {
      const HexXml::PackageUnitBindingDoc& binding = findBinding(package, counter.id);
      const UnitTypeId typeId = rules.unitType(binding.type);
      const HexRules::UnitType& type = rules.unitTypes()[typeId.value];

      UnitSpec spec;
      spec.id = UnitId{unitIndex};
      spec.counter = CounterId{counterIdText};
      spec.type = typeId;
      spec.kind = toUnitKind(type.kind, counter.id);
      spec.side = resolveSide(type, counter, bound, rules);
      spec.nationality = counter.front.style.value_or("");
      spec.front = strengthsOfFace(counter.front, spec.kind, counter.id, reader);
      spec.maxSteps = maxStepsOf(counter.front);
      spec.hiddenP = type.concealment.has_value();

      if (counter.back) {
        if (!counter.back->face.values.empty()) {
          spec.back = strengthsOfFace(counter.back->face, spec.kind, counter.id, reader);
        } else if (counter.back->derived && "same" == *counter.back->derived) {
          spec.back = spec.front;
        }
        // "reduced" and "concealed" backs, and ref-based backs, carry no independently printed value
        // line here; a fuller model would recompute a reduced Strengths from the front, which needs
        // game-specific step-value tables this milestone does not have. Left as nullopt; see the task
        // log's open questions.
      }

      return spec;
    }

  }  // namespace

  Roster
  RosterBuilder::build(const HexXml::CounterSetDoc& counters, const HexRules::RuleSet& rules,
                        const HexXml::PackageDoc& package, const ValueLineReader& reader)
  {
    Roster roster;
    const std::map<std::string, SideId> bound = sideByStyle(package, rules);
    std::uint32_t nextUnit = 0;
    for (const HexXml::CounterDoc& counter : counters.counters) {
      if ("marker" == counter.family) {
        continue;
      }
      if (1 == counter.count) {
        UnitSpec spec = buildOne(counter, counter.id, rules, package, bound, reader, nextUnit);
        roster.byCounter_[spec.counter] = spec.id;
        roster.units_.push_back(std::move(spec));
        ++nextUnit;
      } else {
        for (int copy = 1; copy <= counter.count; ++copy) {
          const std::string counterIdText = counter.id + "#" + std::to_string(copy);
          UnitSpec spec = buildOne(counter, counterIdText, rules, package, bound, reader, nextUnit);
          roster.byCounter_[spec.counter] = spec.id;
          roster.units_.push_back(std::move(spec));
          ++nextUnit;
        }
      }
    }
    return roster;
  }

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
