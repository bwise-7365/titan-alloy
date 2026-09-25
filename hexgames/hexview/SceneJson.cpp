// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// writeJson, a first faithful form: every layer with its primitives and hit tags, paths as SVG path
// data (two decimals), and the symbols the scene uses with their bodies. The HTML client's reader
// is not written yet; when it is, this form may grow (patterns as tiles, for one).
// ----------------------------------------------
#include "hexview/SceneWriters.h"

#include "hexview/WriterText.h"

#include <set>
#include <stdexcept>

namespace HexView {

  namespace {

    std::string
    quoted(std::string_view text)
    {
      std::string out = "\"";
      for (const char c : text) {
        if ('"' == c || '\\' == c) {
          out += '\\';
          out += c;
        }
        else if ('\n' == c) {
          out += "\\n";
        }
        else {
          out += c;
        }
      }
      return out + "\"";
    }

    std::string
    colorJson(const Color& c)
    {
      return "{\"rgb\":" + quoted(colorText(c)) + ",\"alpha\":" + shortest(alphaOf(c)) + "}";
    }

    std::string
    strokeJson(const Stroke& s)
    {
      std::string dash;
      for (const double v : s.dash) {
        dash += (dash.empty() ? "" : ",") + shortest(v);
      }
      return "{\"color\":" + colorJson(s.color) + ",\"width\":" + shortest(s.width) +
             ",\"dash\":[" + dash + "],\"opacity\":" + shortest(s.opacity) +
             ",\"cap\":" + quoted(capName(s.cap)) + ",\"join\":" + quoted(joinName(s.join)) + "}";
    }

    std::string
    shapeJson(const Shape& shape)
    {
      if (const auto* p = std::get_if<PathShape>(&shape)) {
        std::string out = "{\"kind\":\"path\",\"d\":" + quoted(pathData(p->commands, false));
        if (p->fill.has_value()) {
          out += ",\"fill\":{\"color\":" + colorJson(p->fill->color) +
                 ",\"opacity\":" + shortest(p->fill->opacity) + "}";
        }
        if (p->pattern.has_value()) {
          out += ",\"pattern\":" + quoted(patternName(*p->pattern));
        }
        if (p->stroke.has_value()) {
          out += ",\"stroke\":" + strokeJson(*p->stroke);
        }
        return out + "}";
      }
      if (const auto* t = std::get_if<TextShape>(&shape)) {
        std::string out =
            "{\"kind\":\"text\",\"x\":" + fixed(t->at.x, 2) + ",\"y\":" + fixed(t->at.y, 2) +
            ",\"text\":" + quoted(t->text) + ",\"family\":" + quoted(t->font.family) +
            ",\"size\":" + shortest(t->font.size) +
            ",\"bold\":" + (FontWeight::Bold == t->font.weight ? "true" : "false") +
            ",\"italic\":" + (t->font.italicP ? "true" : "false") +
            ",\"spacing\":" + shortest(t->font.letterSpacing) +
            ",\"color\":" + colorJson(t->color) + ",\"anchor\":" + quoted(anchorName(t->anchor)) +
            ",\"baseline\":" + std::to_string(static_cast<int>(t->baseline)) +
            ",\"angle\":" + shortest(t->angleDegrees);
        if (t->halo.has_value()) {
          out += ",\"halo\":" + strokeJson(*t->halo);
        }
        return out + "}";
      }
      const SymbolUse& u = std::get<SymbolUse>(shape);
      return "{\"kind\":\"symbol\",\"symbol\":" + quoted(u.symbol) + ",\"x\":" + fixed(u.at.x, 2) +
             ",\"y\":" + fixed(u.at.y, 2) + ",\"rotate\":" + shortest(u.rotateDegrees) +
             ",\"scale\":" + shortest(u.scale) + ",\"color\":" + colorJson(u.color) + "}";
    }

    std::string
    hitJson(const HitTag& hit)
    {
      if (const auto* h = std::get_if<HexHit>(&hit)) {
        return "{\"hex\":" + std::to_string(h->hex.value) + "}";
      }
      if (const auto* s = std::get_if<HexsideHit>(&hit)) {
        return "{\"hex\":" + std::to_string(s->hex.value) +
               ",\"dir\":" + std::to_string(static_cast<int>(s->dir)) + "}";
      }
      if (const auto* u = std::get_if<UnitHit>(&hit)) {
        return "{\"unit\":" + std::to_string(u->unit.value) + "}";
      }
      if (const auto* sp = std::get_if<SpaceHit>(&hit)) {
        return "{\"space\":" + std::to_string(sp->space.value) + "}";
      }
      if (const auto* p = std::get_if<PanelHit>(&hit)) {
        return "{\"panel\":" + quoted(p->panel) + "}";
      }
      return "null";
    }

  }  // namespace

  std::string
  writeJson(const Scene& scene, const SymbolLibrary& lib)
  {
    std::set<std::string> used;
    std::string layers;
    for (std::size_t k = 0; k < kLayerCount; ++k) {
      const Layer layer = static_cast<Layer>(k);
      std::string prims;
      for (const Primitive& p : scene.layer(layer)) {
        if (const auto* u = std::get_if<SymbolUse>(&p.shape)) {
          used.insert(u->symbol);
        }
        prims += (prims.empty() ? "" : ",\n") + std::string("{\"shape\":") + shapeJson(p.shape) +
                 ",\"hit\":" + hitJson(p.hit) + "}";
      }
      layers += (layers.empty() ? "" : ",\n") + std::string("{\"name\":") +
                quoted(layerName(layer)) + ",\"primitives\":[" + prims + "]}";
    }
    std::string symbols;
    for (const std::string& name : used) {
      std::string body;
      for (const Primitive& p : lib.symbol(name).body) {
        body += (body.empty() ? "" : ",") + shapeJson(p.shape);
      }
      symbols += (symbols.empty() ? "" : ",\n") + quoted(name) + ":[" + body + "]";
    }
    return "{\"width\":" + shortest(scene.width()) + ",\"height\":" + shortest(scene.height()) +
           ",\n\"layers\":[" + layers + "],\n\"symbols\":{" + symbols + "}}\n";
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
