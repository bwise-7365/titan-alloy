// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexview/Shapes.h"

#include <cctype>
#include <charconv>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <string>

namespace HexView {

  Commands
  rectPath(double x, double y, double w, double h)
  {
    return {MoveTo{{x, y}}, LineTo{{x + w, y}}, LineTo{{x + w, y + h}}, LineTo{{x, y + h}},
            ClosePath{}};
  }

  Commands
  roundedRectPath(double x, double y, double w, double h, double r)
  {
    const auto arc = [r](double px, double py) { return ArcTo{r, r, 0.0, false, true, {px, py}}; };
    return {MoveTo{{x + r, y}},    LineTo{{x + w - r, y}},
            arc(x + w, y + r),     LineTo{{x + w, y + h - r}},
            arc(x + w - r, y + h), LineTo{{x + r, y + h}},
            arc(x, y + h - r),     LineTo{{x, y + r}},
            arc(x + r, y),         ClosePath{}};
  }

  Commands
  circlePath(double cx, double cy, double r)
  {
    return ellipsePath(cx, cy, r, r);
  }

  Commands
  ellipsePath(double cx, double cy, double rx, double ry)
  {
    return {MoveTo{{cx + rx, cy}}, ArcTo{rx, ry, 0.0, false, true, {cx - rx, cy}},
            ArcTo{rx, ry, 0.0, false, true, {cx + rx, cy}}, ClosePath{}};
  }

  Commands
  polygonPath(const std::vector<Pixel>& points)
  {
    if (points.empty()) {
      throw std::invalid_argument("polygon: no points");
    }
    Commands out{MoveTo{points.front()}};
    for (std::size_t k = 1; k < points.size(); ++k) {
      out.push_back(LineTo{points[k]});
    }
    out.push_back(ClosePath{});
    return out;
  }

  namespace {

    class Reader {
    public:
      explicit Reader(std::string_view text) : text_(text) {}

      void
      skip()
      {
        while (pos_ < text_.size() &&
               (std::isspace(static_cast<unsigned char>(text_[pos_])) || ',' == text_[pos_])) {
          ++pos_;
        }
        return;
      }

      bool
      atEndP()
      {
        skip();
        return pos_ >= text_.size();
      }

      bool
      atNumberP()
      {
        skip();
        if (pos_ >= text_.size()) {
          return false;
        }
        const char c = text_[pos_];
        return std::isdigit(static_cast<unsigned char>(c)) || '-' == c || '+' == c || '.' == c;
      }

      char
      command()
      {
        skip();
        const char c = text_[pos_];
        if (!std::isalpha(static_cast<unsigned char>(c))) {
          throw bad("expected a command letter");
        }
        ++pos_;
        return c;
      }

      double
      number()
      {
        skip();
        std::size_t used = 0;
        const std::string rest(text_.substr(pos_, 40));
        double value = 0.0;
        try {
          value = std::stod(rest, &used);
        }
        catch (const std::exception&) {
          throw bad("expected a number");
        }
        pos_ += used;
        return value;
      }

      std::invalid_argument
      bad(const std::string& why) const
      {
        return std::invalid_argument("path data '" + std::string(text_) + "': " + why + " at " +
                                     std::to_string(pos_));
      }

    private:
      std::string_view text_;
      std::size_t pos_ = 0;
    };

    double
    radians(double degrees)
    {
      return degrees * std::numbers::pi / 180.0;
    }

  }  // namespace

  Commands
  parsePathData(std::string_view data)
  {
    Reader in(data);
    Commands out;
    Pixel cur;
    Pixel start;
    while (!in.atEndP()) {
      const char c = in.command();
      const bool relP = std::islower(static_cast<unsigned char>(c));
      const char up = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
      const auto point = [&]() {
        const double x = in.number();
        const double y = in.number();
        return relP ? Pixel{cur.x + x, cur.y + y} : Pixel{x, y};
      };
      if ('Z' == up) {
        out.push_back(ClosePath{});
        cur = start;
        continue;
      }
      bool firstP = true;
      do {
        switch (up) {
        case 'M':
          cur = point();
          if (firstP) {
            out.push_back(MoveTo{cur});
            start = cur;
          }
          else {
            out.push_back(LineTo{cur});
          }
          break;
        case 'L':
          cur = point();
          out.push_back(LineTo{cur});
          break;
        case 'H': {
          const double x = in.number();
          cur = Pixel{relP ? cur.x + x : x, cur.y};
          out.push_back(LineTo{cur});
          break;
        }
        case 'V': {
          const double y = in.number();
          cur = Pixel{cur.x, relP ? cur.y + y : y};
          out.push_back(LineTo{cur});
          break;
        }
        case 'Q': {
          const Pixel control = point();
          const Pixel to = point();
          cur = to;
          out.push_back(QuadTo{control, to});
          break;
        }
        case 'A': {
          ArcTo a;
          a.rx = in.number();
          a.ry = in.number();
          a.rotationDegrees = in.number();
          a.largeArcP = 0.0 != in.number();
          a.sweepP = 0.0 != in.number();
          a.to = point();
          cur = a.to;
          out.push_back(a);
          break;
        }
        default:
          throw in.bad(std::string("unsupported command '") + c + "'");
        }
        firstP = false;
      } while (in.atNumberP());
    }
    return out;
  }

  std::vector<Pixel>
  parsePoints(std::string_view points)
  {
    Reader in(points);
    std::vector<Pixel> out;
    while (!in.atEndP()) {
      const double x = in.number();
      const double y = in.number();
      out.push_back(Pixel{x, y});
    }
    return out;
  }

  Color
  svgColor(std::string_view text)
  {
    if ("currentColor" == text) {
      return kCurrentColor;
    }
    if (4 == text.size() && '#' == text[0]) {
      const std::string doubled{'#', text[1], text[1], text[2], text[2], text[3], text[3]};
      return Color::parse(doubled, text);
    }
    return Color::parse(text, "symbol colour");
  }

  Primitive
  symbolPart(Commands commands, std::string_view fill, std::string_view stroke, double width,
             std::vector<double> dash)
  {
    PathShape shape;
    shape.commands = std::move(commands);
    if (fill.empty()) {
      shape.fill = Fill{Color{0, 0, 0, 255}, 1.0};
    }
    else if ("none" != fill) {
      shape.fill = Fill{svgColor(fill), 1.0};
    }
    if ("none" != stroke) {
      shape.stroke =
          Stroke{svgColor(stroke), width, std::move(dash), 1.0, LineCap::Butt, LineJoin::Miter};
    }
    return Primitive{shape, NoHit{}};
  }

  Pixel
  Similarity::apply(Pixel p) const
  {
    const double a = radians(rotateDegrees);
    const double x = scale * p.x;
    const double y = scale * p.y;
    return Pixel{translate.x + x * std::cos(a) - y * std::sin(a),
                 translate.y + x * std::sin(a) + y * std::cos(a)};
  }

  Commands
  Similarity::apply(const Commands& commands) const
  {
    Commands out;
    for (const PathCommand& c : commands) {
      if (const auto* m = std::get_if<MoveTo>(&c)) {
        out.push_back(MoveTo{apply(m->to)});
      }
      else if (const auto* l = std::get_if<LineTo>(&c)) {
        out.push_back(LineTo{apply(l->to)});
      }
      else if (const auto* q = std::get_if<QuadTo>(&c)) {
        out.push_back(QuadTo{apply(q->control), apply(q->to)});
      }
      else if (const auto* a = std::get_if<ArcTo>(&c)) {
        out.push_back(ArcTo{a->rx * scale, a->ry * scale, a->rotationDegrees + rotateDegrees,
                            a->largeArcP, a->sweepP, apply(a->to)});
      }
      else {
        out.push_back(ClosePath{});
      }
    }
    return out;
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
