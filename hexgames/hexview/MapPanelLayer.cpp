// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The panels layer, hexsheet2svg.py layer_panels: each panel's frame, title, texts, boxes, tracks
// and tables, laid out in the panel's own frame and placed by its translation and rotation (the
// Scene has no groups, so every primitive is placed here). SheetPanelDoc keeps each kind of child
// in its own list, so a panel's children are drawn kind by kind; the reference draws them in
// document order, which is the same whenever a panel does not interleave kinds (true of every sheet
// in the golden).
// ----------------------------------------------
#include "hexview/MapLayers.h"

#include <algorithm>

namespace HexView {

  namespace {

    constexpr Color kInk{0x11, 0x11, 0x11, 255};
    constexpr Color kWhite{0xff, 0xff, 0xff, 255};
    constexpr Color kFrame{0x33, 0x33, 0x33, 255};
    constexpr Color kTableLine{0x66, 0x66, 0x66, 255};
    constexpr Color kPanelFill{0xf4, 0xf1, 0xe6, 255};

    // Python's len(): the number of code points of UTF-8 text.
    double
    codePoints(const std::string& text)
    {
      return static_cast<double>(std::count_if(text.begin(), text.end(), [](char c) {
        return 0x80 != (static_cast<unsigned char>(c) & 0xc0);
      }));
    }

    struct PanelDraw {
      const MapContext& ctx;
      Similarity place;
      HitTag hit;
      Scene& scene;

      void
      rect(Commands local, Color fill, Color stroke, double width)
      {
        scene.add(Layer::Panels,
                  pathPrimitive(place.apply(local), Fill{fill, 1.0},
                                Stroke{stroke, width, {}, 1.0, LineCap::Butt, LineJoin::Miter},
                                hit));
        return;
      }

      void
      text(double x, double y, const std::string& content, Font font, Color color,
           TextAnchor anchor = TextAnchor::Middle)
      {
        scene.add(Layer::Panels,
                  Primitive{textShape(place.apply(Pixel{x, y}), content, std::move(font), color,
                                      anchor, TextBaseline::Alphabetic, place.rotateDegrees),
                            hit});
        return;
      }
    };

    void
    drawTrack(PanelDraw& draw, const HexXml::SheetTrackDoc& t, const std::string& at)
    {
      std::vector<std::string> cells;
      std::string cell;
      for (const char c : t.cells + " ") {
        if (' ' == c || '\t' == c || '\n' == c || '\r' == c) {
          if (!cell.empty()) {
            cells.push_back(cell);
          }
          cell.clear();
        }
        else {
          cell += c;
        }
      }
      const int wrap = t.wrap.value_or(0);
      const Color fill = draw.ctx.colorOr(t.fill, kWhite, at);
      for (std::size_t i = 0; i < cells.size(); ++i) {
        const double k = static_cast<double>(i);
        double cx = t.x + k * t.cellW;
        double cy = t.y;
        if ("v" == t.direction) {
          cx = t.x;
          cy = t.y + k * t.cellH;
        }
        else if (0 != wrap) {
          cx = t.x + static_cast<double>(i % wrap) * t.cellW;
          cy = t.y + static_cast<double>(i / wrap) * t.cellH;
        }
        draw.rect(rectPath(cx, cy, t.cellW, t.cellH), fill, kFrame, 0.8);
        std::string label = cells[i];
        std::replace(label.begin(), label.end(), '_', ' ');
        const double fs = std::min(std::min(t.cellW, t.cellH) * 0.45,
                                   1.8 * t.cellW / std::max(1.0, codePoints(label)));
        draw.text(cx + t.cellW / 2, cy + t.cellH * 0.62, label, draw.ctx.font(fs), kInk);
      }
      return;
    }

    void
    drawPanel(const MapContext& ctx, const HexXml::SheetPanelDoc& p, Scene& scene)
    {
      const std::string at = where(ctx.sheet, p.sourceLine, "panel '" + p.id + "'");
      PanelDraw draw{ctx, Similarity{Pixel{p.x, p.y}, p.rotate, 1.0}, PanelHit{p.id}, scene};
      draw.rect(roundedRectPath(0, 0, p.w, p.h, 2), ctx.colorOr(p.fill, kPanelFill, at),
                ctx.colorOr(p.stroke, kFrame, at), 1.2);
      if (p.title.has_value()) {
        const double fs =
            std::min({p.h * 0.22, 11.0, 1.9 * p.w / std::max(1.0, codePoints(*p.title))});
        Font f = ctx.font(fs);
        f.weight = FontWeight::Bold;
        draw.text(p.w / 2, std::min(p.h * 0.25, 14.0), *p.title, f, kInk);
      }
      for (const HexXml::SheetPanelTextDoc& t : p.texts) {
        Font f = ctx.font(t.size);
        f.weight = "bold" == t.weight.value_or("normal") ? FontWeight::Bold : FontWeight::Normal;
        draw.text(t.x, t.y, t.text, f, ctx.colorOr(t.color, kInk, at), TextAnchor::Start);
      }
      for (const HexXml::SheetBoxDoc& b : p.boxes) {
        draw.rect(rectPath(b.x, b.y, b.w, b.h), ctx.colorOr(b.fill, kWhite, at), kFrame, 0.8);
        if (b.label.has_value()) {
          draw.text(b.x + b.w / 2, b.y + std::min(b.h * 0.4, 12.0), *b.label,
                    ctx.font(std::min(b.h * 0.3, 9.0)), kInk);
        }
      }
      for (const HexXml::SheetTrackDoc& t : p.tracks) {
        drawTrack(draw, t, at);
      }
      for (const HexXml::SheetPanelTableDoc& t : p.tables) {
        const double fs = t.size.value_or(t.cellH * 0.5);
        for (std::size_t i = 0; i < t.rows.size(); ++i) {
          for (std::size_t j = 0; j < t.rows[i].cells.size(); ++j) {
            const double cx = t.x + static_cast<double>(j) * t.cellW;
            const double cy = t.y + static_cast<double>(i) * t.cellH;
            draw.rect(rectPath(cx, cy, t.cellW, t.cellH), kWhite, kTableLine, 0.5);
            draw.text(cx + t.cellW / 2, cy + t.cellH * 0.66, t.rows[i].cells[j], ctx.font(fs),
                      kInk);
          }
        }
      }
      return;
    }

  }  // namespace

  void
  addPanelsLayer(const MapContext& ctx, Scene& scene)
  {
    for (const HexXml::SheetPanelDoc& p : ctx.sheet.panels) {
      drawPanel(ctx, p, scene);
    }
    return;
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
