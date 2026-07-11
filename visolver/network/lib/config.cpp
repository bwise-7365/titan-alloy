// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Runtime configuration parsing and application.
// ----------------------------------------------
#include "config.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace VIMCP::Network {

  namespace {

    const char* const kWhitespace = " \t\r\n";

    string
    trim(const string& text)
    {
      const size_t first = text.find_first_not_of(kWhitespace);
      if (string::npos == first) {
        return string{};
      }
      const size_t last = text.find_last_not_of(kWhitespace);
      return text.substr(first, last - first + 1);
    }

    [[noreturn]] void
    badValue(const string& key, const string& value, const char* wanted)
    {
      throw std::invalid_argument("config: key '" + key + "' has value '"
                                  + value + "', which is not " + wanted + ".");
    }

    double
    parseDouble(const string& key, const string& value)
    {
      size_t consumed = 0;
      double parsed = 0.0;
      try {
        parsed = std::stod(value, &consumed);
      }
      catch (const std::exception&) {
        badValue(key, value, "a number");
      }
      if (consumed != value.size()) {
        badValue(key, value, "a number");
      }
      return parsed;
    }

    long long
    parseInteger(const string& key, const string& value)
    {
      size_t consumed = 0;
      long long parsed = 0;
      try {
        parsed = std::stoll(value, &consumed);
      }
      catch (const std::exception&) {
        badValue(key, value, "an integer");
      }
      if (consumed != value.size()) {
        badValue(key, value, "an integer");
      }
      return parsed;
    }

  } // namespace

  ConfigEntries
  parseConfigText(const string& text)
  {
    ConfigEntries entries;
    std::istringstream lines(text);
    string line;
    int lineNumber = 0;
    while (std::getline(lines, line)) {
      ++lineNumber;
      const size_t hash = line.find('#');
      if (string::npos != hash) {
        line = line.substr(0, hash);
      }
      line = trim(line);
      if (line.empty()) {
        continue;
      }
      const size_t equals = line.find('=');
      if (string::npos == equals) {
        throw std::invalid_argument(
            "config: line " + std::to_string(lineNumber)
            + " has no '=' (expected 'key = value').");
      }
      const string key = trim(line.substr(0, equals));
      const string value = trim(line.substr(equals + 1));
      if (key.empty() || value.empty()) {
        throw std::invalid_argument("config: line "
                                    + std::to_string(lineNumber)
                                    + " has an empty key or value.");
      }
      if (!entries.emplace(key, value).second) {
        throw std::invalid_argument("config: duplicate key '" + key + "'.");
      }
    }
    return entries;
  }

  ConfigEntries
  parseConfigFile(const string& path)
  {
    std::ifstream file(path);
    if (!file) {
      throw std::invalid_argument("config: cannot open file '" + path + "'.");
    }
    std::ostringstream text;
    text << file.rdbuf();
    return parseConfigText(text.str());
  }

  bool
  consumeDouble(ConfigEntries& entries, const string& key, double& out)
  {
    const auto found = entries.find(key);
    if (entries.end() == found) {
      return false;
    }
    out = parseDouble(key, found->second);
    entries.erase(found);
    return true;
  }

  bool
  consumeIndex(ConfigEntries& entries, const string& key, Index& out)
  {
    const auto found = entries.find(key);
    if (entries.end() == found) {
      return false;
    }
    out = static_cast<Index>(parseInteger(key, found->second));
    entries.erase(found);
    return true;
  }

  bool
  consumeInt(ConfigEntries& entries, const string& key, int& out)
  {
    const auto found = entries.find(key);
    if (entries.end() == found) {
      return false;
    }
    out = static_cast<int>(parseInteger(key, found->second));
    entries.erase(found);
    return true;
  }

  bool
  consumeUint64(ConfigEntries& entries, const string& key, std::uint64_t& out)
  {
    const auto found = entries.find(key);
    if (entries.end() == found) {
      return false;
    }
    const long long parsed = parseInteger(key, found->second);
    if (0 > parsed) {
      badValue(key, found->second, "a non-negative integer");
    }
    out = static_cast<std::uint64_t>(parsed);
    entries.erase(found);
    return true;
  }

  bool
  consumeString(ConfigEntries& entries, const string& key, string& out)
  {
    const auto found = entries.find(key);
    if (entries.end() == found) {
      return false;
    }
    out = found->second;
    entries.erase(found);
    return true;
  }

  void
  applyFlowPlanConfig(ConfigEntries& entries, FlowPlanParams& params)
  {
    consumeString(entries, "solver.engine", params.engine);
    consumeDouble(entries, "solver.roughMagTol", params.roughMagTol);
    consumeInt(entries, "solver.roughIterMax", params.roughIterMax);
    consumeString(entries, "solver.ipmNewton", params.ipmNewton);
    consumeDouble(entries, "solver.newtonCheckTol", params.newtonCheckTol);
    consumeDouble(entries, "solver.magTol", params.magTol);
    consumeInt(entries, "solver.iterMax", params.iterMax);
    consumeInt(entries, "solver.iterFreq", params.iterFreq);
    consumeDouble(entries, "solver.epsilon", params.epsilon);
    consumeIndex(entries, "screen.maxSourcesPerSink",
                 params.maxSourcesPerSink);
    consumeDouble(entries, "screen.gapFraction", params.gapFraction);
    consumeInt(entries, "screen.maxCertificateRounds",
               params.maxCertificateRounds);
    consumeDouble(entries, "screen.certificateSlack",
                  params.certificateSlack);
    return;
  }

  void
  applyProfileConfig(ConfigEntries& entries, InstanceProfile& profile)
  {
    consumeIndex(entries, "profile.numSupplyOnly", profile.numSupplyOnly);
    consumeIndex(entries, "profile.numBoth", profile.numBoth);
    consumeIndex(entries, "profile.numDemandOnly", profile.numDemandOnly);
    consumeIndex(entries, "profile.numNeither", profile.numNeither);
    consumeDouble(entries, "profile.supplyLo", profile.supplyLo);
    consumeDouble(entries, "profile.supplyHi", profile.supplyHi);
    consumeDouble(entries, "profile.demandLo", profile.demandLo);
    consumeDouble(entries, "profile.demandHi", profile.demandHi);
    consumeDouble(entries, "profile.priorityLo", profile.priorityLo);
    consumeDouble(entries, "profile.priorityHi", profile.priorityHi);
    consumeDouble(entries, "profile.selfCostLo", profile.selfCostLo);
    consumeDouble(entries, "profile.selfCostHi", profile.selfCostHi);
    consumeDouble(entries, "profile.squareSide", profile.squareSide);
    consumeDouble(entries, "profile.costFloor", profile.costFloor);
    consumeDouble(entries, "profile.milesPerUnit", profile.milesPerUnit);
    consumeDouble(entries, "profile.asymmetryMax", profile.asymmetryMax);
    consumeInt(entries, "profile.laydownType", profile.laydownType);
    consumeDouble(entries, "profile.bandXWidth", profile.bandXWidth);
    consumeDouble(entries, "profile.bandXStep", profile.bandXStep);
    consumeDouble(entries, "profile.bandYLo", profile.bandYLo);
    consumeDouble(entries, "profile.bandYHi", profile.bandYHi);
    consumeDouble(entries, "profile.jitterHalfWidth",
                  profile.jitterHalfWidth);
    consumeDouble(entries, "profile.bandMinCostLo", profile.bandMinCostLo);
    consumeDouble(entries, "profile.bandMinCostHi", profile.bandMinCostHi);
    return;
  }

  void
  requireAllConsumed(const ConfigEntries& entries)
  {
    if (entries.empty()) {
      return;
    }
    string leftovers;
    for (const auto& entry : entries) {
      if (!leftovers.empty()) {
        leftovers += ", ";
      }
      leftovers += "'" + entry.first + "'";
    }
    throw std::invalid_argument("config: unrecognized key(s): " + leftovers
                                + ".");
  }

} // namespace VIMCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
