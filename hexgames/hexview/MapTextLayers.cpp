// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The labels layer, hexsheet2svg.py layer_labels: printed hex ids along their id side, region
// labels, and the sheet's authored labels (anchor, angle, weight, italic, spacing, halo).
// ----------------------------------------------
#include "hexview/MapLayers.h"

#include <cmath>
#include <numbers>
#include <stdexcept>

namespace HexView {

  namespace {

    constexpr Color kIdInk{0x44, 0x44, 0x44, 204};      // #444, fill-opacity 0.8
    constexpr Color kRegionInk{0x33, 0x33, 0x33, 204};  // #333, fill-opacity 0.8
    constexpr Color kInk{0x11, 0x11, 0x11, 255};
    constexpr Color kWhite{0xff, 0xff, 0xff, 255};

    TextAnchor
    anchorOf(const std::string& text, std::string_view at)
    {
      if ("start" == text) {
        return TextAnchor::Start;
      }
      if ("middle" == text) {
        return TextAnchor::Middle;
      }
      if ("end" == text) {
        return TextAnchor::End;
      }
      throw std::invalid_argument(std::string(at) + ": anchor '" + text +
                                  "' is not start, middle or end");
    }

    void
    addIds(const MapContext& ctx, Scene& scene)
    {
      for (std::size_t g = 0; g < ctx.frame.grids().size(); ++g) {
        const HexXml::SheetGridDoc& doc = ctx.sheet.grids[g];
        if (!doc.showIds) {
          continue;
        }
        const HexCoord::Grid& grid = ctx.frame.grids()[g];
        const double size = grid.spec().size;
        const std::string at =
            "sheet '" + ctx.sheet.id + "' grid '" + doc.id.value_or("g") + "' id-side";
        for (const HexId& id : cellOrder(grid)) {
          const double ang = ctx.edgeAngle(id, ctx.direction(id, doc.idSide, at));
          const Pixel c = ctx.frame.centre(id);
          const double a = ang * std::numbers::pi / 180.0;
          const Pixel p{c.x + size * 0.70 * std::cos(a), c.y + size * 0.70 * std::sin(a)};
          // The id runs along its hexside and is never upside down.
          double rot = std::fmod(ang + 90.0, 360.0);
          if (90.0 < rot && 270.0 > rot) {
            rot = std::fmod(rot + 180.0, 360.0);
          }
          scene.add(Layer::Labels,
                    Primitive{textShape(p, id.text, ctx.font(size * 0.22), kIdInk,
                                        TextAnchor::Middle, TextBaseline::Central, std::trunc(rot)),
                              ctx.hexHit(id)});
        }
      }
      return;
    }

    void
    addRegionLabels(const MapContext& ctx, Scene& scene)
    {
      for (const HexXml::SheetRegionDoc& reg : ctx.sheet.regions) {
        if (!reg.label.has_value() || !reg.labelAt.has_value()) {
          continue;
        }
        const HexId id{*reg.labelAt};
        Font f = ctx.font(ctx.sizeOf(id) * 0.32);
        f.italicP = true;
        scene.add(Layer::Labels,
                  Primitive{textShape(ctx.frame.centre(id), *reg.label, f, kRegionInk,
                                      TextAnchor::Middle, TextBaseline::Alphabetic),
                            NoHit{}});
      }
      return;
    }

    void
    addAuthoredLabels(const MapContext& ctx, Scene& scene)
    {
      for (const HexXml::SheetLabelDoc& lb : ctx.sheet.labels) {
        const std::string at = "sheet '" + ctx.sheet.id + "' label '" + lb.text + "'";
        if (lb.path.has_value()) {
          throw std::invalid_argument(at + ": text along a path (path=\"" + *lb.path +
                                      "\") is not drawn by hexview yet");
        }
        Font f = ctx.font(lb.size);
        f.weight = "bold" == lb.weight ? FontWeight::Bold : FontWeight::Normal;
        f.italicP = lb.italic;
        f.letterSpacing = lb.spacing;
        const Pixel p = lb.at.has_value() ? ctx.slotPoint(HexId{*lb.at}, lb.slot, 0.9)
                                          : Pixel{lb.x.value_or(0.0), lb.y.value_or(0.0)};
        TextShape t = textShape(p, lb.text, f, ctx.colorOr(lb.color, kInk, at),
                                anchorOf(lb.anchor, at), TextBaseline::Central, lb.angle);
        if (lb.halo) {
          t.halo = Stroke{kWhite, lb.size * 0.18, {}, 0.85, LineCap::Butt, LineJoin::Miter};
        }
        scene.add(Layer::Labels, Primitive{t, NoHit{}});
      }
      return;
    }

  }  // namespace

  void
  addLabelsLayer(const MapContext& ctx, Scene& scene)
  {
    addIds(ctx, scene);
    addRegionLabels(ctx, scene);
    addAuthoredLabels(ctx, scene);
    return;
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
