// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The names TRC keeps state under: side flags (hexsave sides/side/flag) and unit markers (hexsave
// unit/@status), each written in one place so a typo cannot open a second, silent store.
// ----------------------------------------------
#pragma once
#include "hexmodel/Position.h"

#include <string>
#include <vector>

namespace Trc::Flags {

  // Side flags. Counters ("rail-moves", "air-used", "south-entry") are reset by erasing them, so
  // an absent counter is zero; every other flag is read with Position::flag and an absence checked.
  inline const std::string kWeather = "weather";
  inline const std::string kWeatherDrm = "weather-drm";
  inline const std::string kRailMoves = "rail-moves";
  inline const std::string kRailTouched = "rail-touched";      // hex ids, space separated
  inline const std::string kAirUsed = "air-used";
  inline const std::string kLeaderLost = "leader-lost";         // "pending" | "active"
  inline const std::string kSouthEntry = "south-entry";
  inline const std::string kSeaUsed = "sea-used";               // sea areas, space separated
  inline const std::string kReplaced = "replaced";              // replacement categories used
  inline const std::string kReplacementPoints = "replacement-points";
  inline const std::string kSurrendered = "surrendered";        // nations, space separated (axis)
  inline const std::string kHelsinkiRussian = "helsinki-russian";
  inline const std::string kSuddenDeath = "sudden-death";       // the condition id that was met
  inline const std::string kGarrisonWarsaw = "garrison-warsaw";  // "due" | "done"

  // Unit markers.
  inline const std::string kAv = "av";          // took part in an automatic victory this impulse
  inline const std::string kAvFirst = "av-first";  // ... in the first impulse (16.2)
  inline const std::string kInvaded = "invaded";   // landed by sea invasion this turn (17.2)
  inline const std::string kDropped = "dropped";   // a paratroop corps that has dropped (18.0)
  inline const std::string kRailed = "railed";     // moved by rail this impulse (9.1)

  int counter(const HexModel::Position&, HexModel::SideId, const std::string&);
  void bump(HexModel::Position&, HexModel::SideId, const std::string&, int by);
  std::vector<std::string> list(const HexModel::Position&, HexModel::SideId, const std::string&);
  void add(HexModel::Position&, HexModel::SideId, const std::string&, const std::string& item);
  bool listedP(const HexModel::Position&, HexModel::SideId, const std::string&, const std::string& item);

  bool markedP(const HexModel::Position&, HexModel::UnitId, const std::string&);
  void mark(HexModel::Position&, HexModel::UnitId, const std::string&);
  void unmark(HexModel::Position&, HexModel::UnitId, const std::string&);

}  // namespace Trc::Flags
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
