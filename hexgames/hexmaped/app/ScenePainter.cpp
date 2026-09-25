// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "ScenePainter.h"

#include "hexview/Shapes.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QPainterPath>
#include <QPen>
#include <QString>

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <variant>

namespace HexQt {

  namespace {

    using HexView::Color;
    using HexView::Pixel;

    QColor
    qcolor(const Color& c)
    {
      return QColor(c.r, c.g, c.b, c.a);
    }

    QPen
    qpen(const HexView::Stroke& s)
    {
      QColor c = qcolor(s.color);
      if (1.0 != s.opacity) {
        c.setAlphaF(c.alphaF() * s.opacity);
      }
      QPen pen(c, s.width);
      switch (s.cap) {
        case HexView::LineCap::Butt:
          pen.setCapStyle(Qt::FlatCap);
          break;
        case HexView::LineCap::Round:
          pen.setCapStyle(Qt::RoundCap);
          break;
        case HexView::LineCap::Square:
          pen.setCapStyle(Qt::SquareCap);
          break;
      }
      switch (s.join) {
        case HexView::LineJoin::Miter:
          pen.setJoinStyle(Qt::MiterJoin);
          break;
        case HexView::LineJoin::Round:
          pen.setJoinStyle(Qt::RoundJoin);
          break;
        case HexView::LineJoin::Bevel:
          pen.setJoinStyle(Qt::BevelJoin);
          break;
      }
      if (!s.dash.empty() && 0.0 < s.width) {
        QList<qreal> pattern;  // Qt dashes are in pen widths
        for (double d : s.dash) {
          pattern.push_back(std::max(0.05, d / s.width));
        }
        if (1 == pattern.size() % 2) {
          pattern.append(pattern);
        }
        pen.setDashPattern(pattern);
      }
      return pen;
    }

    QBrush
    qbrush(const HexView::Fill& f)
    {
      QColor c = qcolor(f.color);
      if (1.0 != f.opacity) {
        c.setAlphaF(c.alphaF() * f.opacity);
      }
      return QBrush(c);
    }

    // hexsheet2svg.py's four pattern tiles, as Qt brush textures over the fill (the SVG's tiles are
    // black at low opacity over the terrain colour).
    QBrush
    patternBrush(HexView::Pattern p)
    {
      switch (p) {
        case HexView::Pattern::Dots:
          return QBrush(QColor(0, 0, 0, 64), Qt::Dense6Pattern);
        case HexView::Pattern::Hatch:
          return QBrush(QColor(0, 0, 0, 51), Qt::BDiagPattern);
        case HexView::Pattern::Mottle:
          return QBrush(QColor(0, 0, 0, 31), Qt::Dense7Pattern);
        case HexView::Pattern::Palms:
          return QBrush(QColor(42, 122, 42, 153), Qt::DiagCrossPattern);
      }
      throw std::invalid_argument("paintScene: unknown pattern");
    }

    // An SVG endpoint-parameterised elliptical arc as Qt's centre-parameterised arcTo (SVG 1.1 F.6.5).
    void
    arcTo(QPainterPath& path, const HexView::ArcTo& a)
    {
      const QPointF p1 = path.currentPosition();
      const double x2 = a.to.x;
      const double y2 = a.to.y;
      double rx = std::fabs(a.rx);
      double ry = std::fabs(a.ry);
      if (0.0 == rx || 0.0 == ry || (p1.x() == x2 && p1.y() == y2)) {
        path.lineTo(x2, y2);
        return;
      }
      const double phi = a.rotationDegrees * std::numbers::pi / 180.0;
      const double cosPhi = std::cos(phi);
      const double sinPhi = std::sin(phi);
      const double dx = (p1.x() - x2) / 2.0;
      const double dy = (p1.y() - y2) / 2.0;
      const double x1p = cosPhi * dx + sinPhi * dy;
      const double y1p = -sinPhi * dx + cosPhi * dy;
      const double lambda = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry);
      if (1.0 < lambda) {
        rx *= std::sqrt(lambda);
        ry *= std::sqrt(lambda);
      }
      const double num = rx * rx * ry * ry - rx * rx * y1p * y1p - ry * ry * x1p * x1p;
      const double den = rx * rx * y1p * y1p + ry * ry * x1p * x1p;
      double coef = (0.0 < den && 0.0 < num) ? std::sqrt(num / den) : 0.0;
      if (a.largeArcP == a.sweepP) {
        coef = -coef;
      }
      const double cxp = coef * (rx * y1p / ry);
      const double cyp = coef * (-ry * x1p / rx);
      const double cx = cosPhi * cxp - sinPhi * cyp + (p1.x() + x2) / 2.0;
      const double cy = sinPhi * cxp + cosPhi * cyp + (p1.y() + y2) / 2.0;
      const auto angleOf = [](double ux, double uy, double vx, double vy) {
        const double dot = ux * vx + uy * vy;
        const double len = std::hypot(ux, uy) * std::hypot(vx, vy);
        double ang = std::acos(std::clamp(dot / len, -1.0, 1.0));
        if (ux * vy - uy * vx < 0.0) {
          ang = -ang;
        }
        return ang;
      };
      const double theta1 = angleOf(1.0, 0.0, (x1p - cxp) / rx, (y1p - cyp) / ry);
      double dtheta = angleOf((x1p - cxp) / rx, (y1p - cyp) / ry, (-x1p - cxp) / rx, (-y1p - cyp) / ry);
      if (!a.sweepP && 0.0 < dtheta) {
        dtheta -= 2.0 * std::numbers::pi;
      }
      else if (a.sweepP && dtheta < 0.0) {
        dtheta += 2.0 * std::numbers::pi;
      }
      // Qt's arcTo: angles in degrees, anticlockwise positive, on an unrotated rect. SVG's y is down,
      // so an SVG angle theta (clockwise on screen) is Qt's -theta.
      const double startDeg = -theta1 * 180.0 / std::numbers::pi;
      const double sweepDeg = -dtheta * 180.0 / std::numbers::pi;
      if (0.0 != phi) {
        // rotate about the centre: draw on an unrotated rect in a rotated frame
        QPainterPath arc;
        arc.arcMoveTo(QRectF(-rx, -ry, 2 * rx, 2 * ry), startDeg);
        arc.arcTo(QRectF(-rx, -ry, 2 * rx, 2 * ry), startDeg, sweepDeg);
        QTransform t;
        t.translate(cx, cy);
        t.rotate(a.rotationDegrees);
        path.connectPath(t.map(arc));
        return;
      }
      path.arcTo(QRectF(cx - rx, cy - ry, 2 * rx, 2 * ry), startDeg, sweepDeg);
      return;
    }

    QPainterPath
    painterPath(const std::vector<HexView::PathCommand>& commands)
    {
      QPainterPath path;
      for (const HexView::PathCommand& c : commands) {
        std::visit(
            [&](const auto& cmd) {
              using T = std::decay_t<decltype(cmd)>;
              if constexpr (std::is_same_v<T, HexView::MoveTo>) {
                path.moveTo(cmd.to.x, cmd.to.y);
              }
              else if constexpr (std::is_same_v<T, HexView::LineTo>) {
                path.lineTo(cmd.to.x, cmd.to.y);
              }
              else if constexpr (std::is_same_v<T, HexView::QuadTo>) {
                path.quadTo(cmd.control.x, cmd.control.y, cmd.to.x, cmd.to.y);
              }
              else if constexpr (std::is_same_v<T, HexView::ArcTo>) {
                arcTo(path, cmd);
              }
              else {
                path.closeSubpath();
              }
            },
            c);
      }
      return path;
    }

    QFont
    qfont(const HexView::Font& f)
    {
      // the family is a CSS list ("Arial, Helvetica, sans-serif"); Qt takes the first name
      const QString first = QString::fromStdString(f.family).split(',').value(0).trimmed();
      QFont font(first.isEmpty() ? QStringLiteral("Arial") : first);
      font.setPixelSize(std::max(1, static_cast<int>(std::lround(f.size))));
      font.setBold(HexView::FontWeight::Bold == f.weight);
      font.setItalic(f.italicP);
      if (0.0 != f.letterSpacing) {
        font.setLetterSpacing(QFont::AbsoluteSpacing, f.letterSpacing);
      }
      return font;
    }

    void
    addPath(QGraphicsScene& target, const HexView::PathShape& p)
    {
      const QPainterPath path = painterPath(p.commands);
      const QPen pen = p.stroke.has_value() ? qpen(*p.stroke) : QPen(Qt::NoPen);
      const QBrush brush = p.fill.has_value() ? qbrush(*p.fill) : QBrush(Qt::NoBrush);
      target.addPath(path, pen, brush);
      if (p.pattern.has_value()) {
        target.addPath(path, QPen(Qt::NoPen), patternBrush(*p.pattern));
      }
      return;
    }

    void
    addText(QGraphicsScene& target, const HexView::TextShape& t)
    {
      const QFont font = qfont(t.font);
      const auto place = [&](QGraphicsSimpleTextItem* item) {
        const QRectF box = item->boundingRect();
        double dx = -box.width() / 2.0;
        switch (t.anchor) {
          case HexView::TextAnchor::Start:
            dx = 0.0;
            break;
          case HexView::TextAnchor::Middle:
            dx = -box.width() / 2.0;
            break;
          case HexView::TextAnchor::End:
            dx = -box.width();
            break;
        }
        double dy = -box.height() / 2.0;
        switch (t.baseline) {
          case HexView::TextBaseline::Alphabetic:
            dy = -box.height() * 0.8;  // the baseline sits about 80% down the box
            break;
          case HexView::TextBaseline::Central:
          case HexView::TextBaseline::Middle:
            dy = -box.height() / 2.0;
            break;
        }
        item->setPos(t.at.x + dx, t.at.y + dy);
        item->setTransformOriginPoint(-dx, -dy);
        item->setRotation(t.angleDegrees);
        return;
      };
      if (t.halo.has_value()) {
        QGraphicsSimpleTextItem* halo = target.addSimpleText(QString::fromStdString(t.text), font);
        QPen pen = qpen(*t.halo);
        pen.setWidthF(pen.widthF() * 2.0);  // SVG paint-order=stroke shows half the stroke; Qt shows it all
        halo->setPen(pen);
        halo->setBrush(Qt::NoBrush);
        place(halo);
      }
      QGraphicsSimpleTextItem* item = target.addSimpleText(QString::fromStdString(t.text), font);
      item->setBrush(qcolor(t.color));
      place(item);
      return;
    }

    void
    addSymbol(QGraphicsScene& target, const HexView::SymbolLibrary& symbols, const HexView::SymbolUse& u)
    {
      const HexView::Symbol& sym = symbols.symbol(u.symbol);
      const HexView::Similarity s{u.at, u.rotateDegrees, u.scale};
      const auto colored = [&](const Color& c) { return c == HexView::kCurrentColor ? u.color : c; };
      for (const HexView::Primitive& part : sym.body) {
        if (const auto* path = std::get_if<HexView::PathShape>(&part.shape)) {
          HexView::PathShape placed = *path;
          placed.commands = s.apply(path->commands);
          if (placed.fill.has_value()) {
            placed.fill->color = colored(placed.fill->color);
          }
          if (placed.stroke.has_value()) {
            placed.stroke->color = colored(placed.stroke->color);
            placed.stroke->width *= u.scale;
            for (double& d : placed.stroke->dash) {
              d *= u.scale;
            }
          }
          addPath(target, placed);
        }
        else if (const auto* text = std::get_if<HexView::TextShape>(&part.shape)) {
          HexView::TextShape placed = *text;
          placed.at = s.apply(text->at);
          placed.font.size *= u.scale;
          placed.color = colored(placed.color);
          placed.angleDegrees += u.rotateDegrees;
          addText(target, placed);
        }
        else {
          throw std::invalid_argument("paintScene: symbol '" + u.symbol + "' nests a symbol use");
        }
      }
      return;
    }

  }  // namespace

  void
  paintScene(const HexView::Scene& scene, const HexView::SymbolLibrary& symbols, QGraphicsScene& target,
             const PaintOptions& options)
  {
    for (std::size_t li = 0; li < HexView::kLayerCount; ++li) {
      const HexView::Layer layer = static_cast<HexView::Layer>(li);
      for (const HexView::Primitive& prim : scene.layer(layer)) {
        if (const auto* path = std::get_if<HexView::PathShape>(&prim.shape)) {
          addPath(target, *path);
        }
        else if (const auto* text = std::get_if<HexView::TextShape>(&prim.shape)) {
          if (HexView::Layer::Grid == layer && !options.idsVisibleP) {
            continue;  // the printed ids
          }
          addText(target, *text);
        }
        else {
          addSymbol(target, symbols, std::get<HexView::SymbolUse>(prim.shape));
        }
      }
    }
    return;
  }

}  // namespace HexQt
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
