// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PlayLayers.h"

#include "hexview/Shapes.h"

#include <array>
#include <cmath>
#include <string>
#include <variant>
#include <vector>

namespace HexView {

  namespace {

    constexpr std::array<Color, 2> kSideColours{Color{176, 176, 170}, Color{196, 72, 56}};  // the first side, the second
    constexpr Color kLeaderColour{240, 196, 92};
    constexpr Color kInk{20, 20, 20};
    constexpr Color kMoved{20, 60, 220};
    constexpr Color kRetreated{230, 130, 0};
    constexpr Color kBattle{220, 0, 0};
    constexpr Color kEntered{0, 150, 60};

    double
    hexRadius(const MapFrame& frame, const HexCoord::HexId& id)
    {
      const Pixel centre = frame.centre(id);
      const Pixel corner = frame.corners(id)[0];
      return std::hypot(corner.x - centre.x, corner.y - centre.y);
    }

    std::string
    labelOf(const HexModel::UnitSpec& spec, const HexModel::UnitState& state, const PlayLayerOptions& options)
    {
      if (!state.flags.revealedP && !options.revealAllP) {
        return "?";
      }
      if (HexModel::UnitKind::Leader == spec.kind || HexModel::UnitKind::Hq == spec.kind) {
        return "HQ";
      }
      const HexModel::Strengths& face = HexModel::Face::Back == state.face && spec.back ? *spec.back : spec.front;
      const int attack = face.attack ? face.attack->value : 0;
      const int defence = face.defence ? face.defence->value : attack;
      return std::to_string(attack) + "-" + std::to_string(defence);
    }

    void
    addSquare(Scene& scene, Pixel centre, double width, const HexModel::UnitSpec& spec, const std::string& label)
    {
      const bool leaderP = HexModel::UnitKind::Leader == spec.kind || HexModel::UnitKind::Hq == spec.kind;
      const Color colour = leaderP ? kLeaderColour : kSideColours[spec.side.value % kSideColours.size()];
      PathShape square;
      square.commands = rectPath(centre.x - width / 2.0, centre.y - width / 2.0, width, width);
      square.fill = Fill{colour};
      square.stroke = Stroke{kInk, width / 16.0};
      scene.add(Layer::Units, Primitive{square, UnitHit{spec.id}});
      TextShape text;
      text.at = centre;
      text.text = label;
      text.font = Font{"Arial", width * 0.42};
      text.color = kInk;
      text.baseline = TextBaseline::Central;
      scene.add(Layer::Units, Primitive{text, UnitHit{spec.id}});
      return;
    }

    void
    addPath(Scene& scene, const MapFrame& frame, const HexModel::Board& board, const std::vector<HexModel::HexIndex>& hexes, Color colour)
    {
      if (2 > hexes.size()) {
        return;
      }
      PathShape path;
      path.commands.push_back(MoveTo{frame.centre(board.id(hexes.front()))});
      for (std::size_t i = 1; i < hexes.size(); ++i) {
        path.commands.push_back(LineTo{frame.centre(board.id(hexes[i]))});
      }
      path.stroke = Stroke{colour, hexRadius(frame, board.id(hexes.front())) / 7.0};
      scene.add(Layer::Highlights, Primitive{path, NoHit{}});
      return;
    }

    void
    addRing(Scene& scene, const MapFrame& frame, const HexModel::Board& board, HexModel::HexIndex hex, Color colour)
    {
      const HexCoord::HexId& id = board.id(hex);
      const Pixel centre = frame.centre(id);
      const double radius = hexRadius(frame, id);
      PathShape ring;
      ring.commands = circlePath(centre.x, centre.y, radius * 0.85);
      ring.stroke = Stroke{colour, radius / 7.0};
      scene.add(Layer::Highlights, Primitive{ring, NoHit{}});
      return;
    }

  }  // namespace

  void
  addUnits(Scene& scene, const MapFrame& frame, const HexModel::Board& board, const HexModel::Roster& roster,
           const HexModel::Position& position, const PlayLayerOptions& options)
  {
    for (std::size_t h = 0; h < board.hexCount(); ++h) {
      const HexModel::HexIndex hex{static_cast<std::uint32_t>(h)};
      std::vector<HexModel::UnitId> units;
      for (HexModel::UnitId unit : position.unitsAt(hex)) {
        if (HexModel::UnitKind::Marker != roster.unit(unit).kind) {
          units.push_back(unit);
        }
      }
      if (units.empty()) {
        continue;
      }
      const HexCoord::HexId& id = board.id(hex);
      const Pixel centre = frame.centre(id);
      const double width = hexRadius(frame, id) * 0.72;
      const int columns = 1 == units.size() ? 1 : 2;
      const int rows = (static_cast<int>(units.size()) + columns - 1) / columns;
      for (std::size_t k = 0; k < units.size(); ++k) {
        const int column = static_cast<int>(k) % columns;
        const int row = static_cast<int>(k) / columns;
        const Pixel at{centre.x + (column - (columns - 1) / 2.0) * width, centre.y + (row - (rows - 1) / 2.0) * width};
        addSquare(scene, at, width * 0.96, roster.unit(units[k]), labelOf(roster.unit(units[k]), position.unit(units[k]), options));
      }
    }
    return;
  }

  void
  addStepMarks(Scene& scene, const MapFrame& frame, const HexModel::Board& board, std::span<const HexEngine::Event> events)
  {
    for (const HexEngine::Event& event : events) {
      if (const HexEngine::UnitMoved* moved = std::get_if<HexEngine::UnitMoved>(&event)) {
        addPath(scene, frame, board, moved->path, kMoved);
      } else if (const HexEngine::Retreated* retreated = std::get_if<HexEngine::Retreated>(&event)) {
        addPath(scene, frame, board, retreated->path, kRetreated);
      } else if (const HexEngine::CombatDeclared* battle = std::get_if<HexEngine::CombatDeclared>(&event)) {
        addRing(scene, frame, board, battle->target, kBattle);
      } else if (const HexEngine::UnitPlaced* placed = std::get_if<HexEngine::UnitPlaced>(&event)) {
        if (const HexModel::HexIndex* hex = std::get_if<HexModel::HexIndex>(&placed->where)) {
          addRing(scene, frame, board, *hex, kEntered);
        }
      }
    }
    return;
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
