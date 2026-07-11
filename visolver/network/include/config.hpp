// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Runtime configuration (task E1): key=value files that adjust every tuning
// knob -- screen rules, certificate slacks, tie-break epsilon, solver
// tolerances, instance profiles -- without recompilation.
// ----------------------------------------------
#ifndef VIMCP_NETWORK_CONFIG_HPP
#define VIMCP_NETWORK_CONFIG_HPP

#include "flowplan.hpp"

#include <cstdint>
#include <map>
#include <string>

using std::map;
using std::string;

namespace VIMCP::Network {

  // ---------------------------------------------------------------------------
  // Parsing
  // ---------------------------------------------------------------------------

  // Format: one "key = value" per line; '#' starts a comment (anywhere on the
  // line); blank lines are fine; whitespace around key and value is trimmed.
  // Malformed lines, empty keys/values, and duplicate keys all throw
  // std::invalid_argument -- a knobs file must fail loudly, never silently.
  using ConfigEntries = map<string, string>;

  ConfigEntries parseConfigText(const string& text);
  ConfigEntries parseConfigFile(const string& path);   // throws if unreadable

  // ---------------------------------------------------------------------------
  // Typed consumption
  // ---------------------------------------------------------------------------

  // Each consume* looks up the key; if present, parses it (throwing
  // std::invalid_argument on garbage, with the key named), writes 'out',
  // ERASES the entry, and returns true. Absent keys leave 'out' untouched --
  // so defaults are simply the value already in 'out'.
  bool consumeDouble(ConfigEntries& entries, const string& key, double& out);
  bool consumeIndex(ConfigEntries& entries, const string& key, Index& out);
  bool consumeInt(ConfigEntries& entries, const string& key, int& out);
  bool consumeUint64(ConfigEntries& entries, const string& key,
                     std::uint64_t& out);
  bool consumeString(ConfigEntries& entries, const string& key, string& out);

  // ---------------------------------------------------------------------------
  // Section appliers
  // ---------------------------------------------------------------------------

  // Overlay recognized keys onto the target, consuming them. Recognized keys
  // (each optional; defaults are the struct's initializers):
  //   solver.engine (bshe94b | chain) solver.roughMagTol solver.roughIterMax
  //   solver.magTol solver.iterMax solver.iterFreq solver.epsilon
  //   screen.maxSourcesPerSink screen.gapFraction
  //   screen.maxCertificateRounds screen.certificateSlack
  void applyFlowPlanConfig(ConfigEntries& entries, FlowPlanParams& params);

  //   profile.numSupplyOnly profile.numBoth profile.numDemandOnly
  //   profile.numNeither profile.supplyLo profile.supplyHi profile.demandLo
  //   profile.demandHi profile.priorityLo profile.priorityHi
  //   profile.selfCostLo profile.selfCostHi profile.squareSide
  //   profile.costFloor profile.milesPerUnit profile.asymmetryMax
  //   profile.laydownType profile.bandXWidth profile.bandXStep
  //   profile.bandYLo profile.bandYHi profile.jitterHalfWidth
  //   profile.bandMinCostLo profile.bandMinCostHi
  void applyProfileConfig(ConfigEntries& entries, InstanceProfile& profile);

  // Call LAST: throws std::invalid_argument naming every leftover key (the
  // typo guard -- an unrecognized knob must never silently do nothing).
  void requireAllConsumed(const ConfigEntries& entries);

} // namespace VIMCP::Network

#endif // VIMCP_NETWORK_CONFIG_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
