// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Hit testing is approximate where exactness buys nothing a pointer can feel: curves are flattened,
// text is its em box (0.6 em per character), and a symbol is a disc of half its scale.
// ----------------------------------------------
#include "hexview/Scene.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace HexView {

  namespace {

    using Ring = std::vector<Pixel>;

    Pixel
    lerp(Pixel a, Pixel b, double t)
    {
      return Pixel{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
    }

    // Subpaths as polylines; arcs are replaced by their chord and midpoint, which is close enough
    // for hit testing the small circles and rounded corners they draw.
    std::vector<Ring>
    flatten(const std::vector<PathCommand>& commands)
    {
      std::vector<Ring> rings;
      Pixel start;
      for (const PathCommand& c : commands) {
        if (const auto* m = std::get_if<MoveTo>(&c)) {
          rings.push_back(Ring{m->to});
          start = m->to;
        }
        else if (rings.empty()) {
          throw std::invalid_argument("path: drawing command before the first MoveTo");
        }
        else if (const auto* l = std::get_if<LineTo>(&c)) {
          rings.back().push_back(l->to);
        }
        else if (const auto* q = std::get_if<QuadTo>(&c)) {
          const Pixel from = rings.back().back();
          for (int k = 1; k <= 8; ++k) {
            const double t = k / 8.0;
            rings.back().push_back(lerp(lerp(from, q->control, t), lerp(q->control, q->to, t), t));
          }
        }
        else if (const auto* a = std::get_if<ArcTo>(&c)) {
          rings.back().push_back(a->to);
        }
        else {
          rings.back().push_back(start);
        }
      }
      return rings;
    }

    bool
    insideP(const std::vector<Ring>& rings, Pixel p)
    {
      int winding = 0;
      for (const Ring& ring : rings) {
        for (std::size_t i = 0; i < ring.size(); ++i) {
          const Pixel a = ring[i];
          const Pixel b = ring[(i + 1) % ring.size()];
          const double cross = (b.x - a.x) * (p.y - a.y) - (p.x - a.x) * (b.y - a.y);
          if (a.y <= p.y && b.y > p.y && cross > 0) {
            ++winding;
          }
          else if (a.y > p.y && b.y <= p.y && cross < 0) {
            --winding;
          }
        }
      }
      return 0 != winding;
    }

    double
    segmentDistance(Pixel p, Pixel a, Pixel b)
    {
      const double vx = b.x - a.x;
      const double vy = b.y - a.y;
      const double len2 = vx * vx + vy * vy;
      const double t =
          0.0 == len2 ? 0.0 : std::clamp(((p.x - a.x) * vx + (p.y - a.y) * vy) / len2, 0.0, 1.0);
      return std::hypot(p.x - (a.x + t * vx), p.y - (a.y + t * vy));
    }

    bool
    onStrokeP(const std::vector<Ring>& rings, Pixel p, double halfWidth)
    {
      for (const Ring& ring : rings) {
        for (std::size_t i = 1; i < ring.size(); ++i) {
          if (segmentDistance(p, ring[i - 1], ring[i]) <= halfWidth) {
            return true;
          }
        }
      }
      return false;
    }

    bool
    hitsP(const PathShape& s, Pixel p)
    {
      const std::vector<Ring> rings = flatten(s.commands);
      if ((s.fill.has_value() || s.pattern.has_value()) && insideP(rings, p)) {
        return true;
      }
      return s.stroke.has_value() && onStrokeP(rings, p, std::max(1.0, s.stroke->width / 2.0));
    }

    bool
    hitsP(const TextShape& s, Pixel p)
    {
      const double a = -s.angleDegrees * std::numbers::pi / 180.0;
      const double dx = p.x - s.at.x;
      const double dy = p.y - s.at.y;
      const double x = dx * std::cos(a) - dy * std::sin(a);
      const double y = dx * std::sin(a) + dy * std::cos(a);
      const double w = 0.6 * s.font.size * static_cast<double>(s.text.size());
      return std::fabs(x) <= w && std::fabs(y) <= s.font.size;
    }

    bool
    hitsP(const SymbolUse& s, Pixel p)
    {
      return std::hypot(p.x - s.at.x, p.y - s.at.y) <= s.scale / 2.0;
    }

  }  // namespace

  Scene::Scene(double width, double height) : width_(width), height_(height)
  {
    if (!(0.0 < width) || !(0.0 < height)) {
      throw std::invalid_argument("Scene: size " + std::to_string(width) + " x " +
                                  std::to_string(height) + " is not positive");
    }
  }

  void
  Scene::add(Layer layer, Primitive primitive)
  {
    layers_.at(static_cast<std::size_t>(layer)).push_back(std::move(primitive));
    return;
  }

  std::span<const Primitive>
  Scene::layer(Layer layer) const
  {
    return layers_.at(static_cast<std::size_t>(layer));
  }

  HitTag
  Scene::hitAt(Pixel p) const
  {
    for (std::size_t k = kLayerCount; k-- > 0;) {
      const std::vector<Primitive>& prims = layers_[k];
      for (auto it = prims.rbegin(); prims.rend() != it; ++it) {
        if (std::holds_alternative<NoHit>(it->hit)) {
          continue;
        }
        const bool hitP = std::visit([&](const auto& shape) { return hitsP(shape, p); }, it->shape);
        if (hitP) {
          return it->hit;
        }
      }
    }
    return NoHit{};
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
