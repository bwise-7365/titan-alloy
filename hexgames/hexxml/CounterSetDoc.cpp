// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexxml/CounterSetDoc.h"

#include "hexxml/DocDetail.h"

namespace HexXml {

  using Detail::checkEnum;
  using Detail::requiredChild;
  using Detail::splitTokens;

  namespace {

    CounterColorDoc
    parseColor(const XmlNode& node)
    {
      CounterColorDoc c;
      c.id = node.required("id");
      c.value = node.required("value");
      c.name = node.optional("name");
      return c;
    }

    CounterStyleDoc
    parseStyle(const XmlNode& node)
    {
      CounterStyleDoc s;
      s.id = node.required("id");
      s.name = node.optional("name");
      s.ground = node.required("ground");
      s.text = node.required("text");
      s.boxStroke = node.required("box-stroke");
      s.boxFill = node.optional("box-fill");
      return s;
    }

    CounterSymbolDoc
    parseSymbol(const XmlNode& node)
    {
      CounterSymbolDoc s;
      s.icon = node.required("icon");
      if (const std::optional<std::string> m = node.optional("modifiers")) {
        s.modifiers = splitTokens(*m);
      }
      s.fill = node.optional("fill");
      s.stroke = node.optional("stroke");
      s.partial = node.optional("partial").value_or("none");
      checkEnum(node, "partial", s.partial, {"none", "left", "right"});
      s.text = node.optional("text");
      s.scale = node.optionalAs<double>("scale").value_or(1.0);
      return s;
    }

    CounterSilhouetteDoc
    parseSilhouette(const XmlNode& node)
    {
      CounterSilhouetteDoc s;
      s.kind = node.required("kind");
      s.color = node.optional("color");
      s.href = node.optional("href");
      s.scale = node.optionalAs<double>("scale").value_or(1.0);
      return s;
    }

    CounterEmblemDoc
    parseEmblem(const XmlNode& node)
    {
      CounterEmblemDoc e;
      e.kind = node.required("kind");
      e.color = node.optional("color");
      e.color2 = node.optional("color2");
      e.slot = node.optional("slot").value_or("CENTRE");
      e.scale = node.optionalAs<double>("scale").value_or(1.0);
      return e;
    }

    CounterEchelonDoc
    parseEchelon(const XmlNode& node)
    {
      CounterEchelonDoc e;
      e.level = node.required("level");
      e.color = node.optional("color");
      return e;
    }

    CounterValueDoc
    parseValue(const XmlNode& node)
    {
      CounterValueDoc v;
      v.color = node.optional("color");
      v.size = node.optional("size").value_or("large");
      checkEnum(node, "size", v.size, {"small", "medium", "large"});
      v.anchor = node.optional("anchor").value_or("middle");
      checkEnum(node, "anchor", v.anchor, {"start", "middle", "end"});
      v.text = node.text();
      return v;
    }

    CounterTextDoc
    parseText(const XmlNode& node)
    {
      CounterTextDoc t;
      t.slot = node.required("slot");
      t.x = node.optionalAs<double>("x");
      t.y = node.optionalAs<double>("y");
      t.rotate = node.optionalAs<double>("rotate").value_or(0.0);
      t.size = node.optional("size").value_or("small");
      checkEnum(node, "size", t.size, {"small", "medium", "large"});
      t.color = node.optional("color");
      t.weight = node.optional("weight").value_or("normal");
      checkEnum(node, "weight", t.weight, {"normal", "bold"});
      t.italic = node.optionalAs<bool>("italic").value_or(false);
      t.text = node.text();
      return t;
    }

    CounterStepsDoc
    parseSteps(const XmlNode& node)
    {
      CounterStepsDoc s;
      s.count = node.requiredAs<int>("count");
      s.maxCount = node.optionalAs<int>("max");
      s.mark = node.optional("mark").value_or("dot");
      checkEnum(node, "mark", s.mark, {"dot", "square"});
      s.slot = node.optional("slot").value_or("LCOL");
      s.color = node.optional("color");
      return s;
    }

    CounterGlyphDoc
    parseGlyph(const XmlNode& node)
    {
      CounterGlyphDoc g;
      g.kind = node.required("kind");
      g.slot = node.optional("slot").value_or("LL");
      g.color = node.optional("color");
      g.text = node.optional("text");
      g.x = node.optionalAs<double>("x");
      g.y = node.optionalAs<double>("y");
      return g;
    }

    CounterBandDoc
    parseBand(const XmlNode& node)
    {
      CounterBandDoc b;
      b.color = node.required("color");
      b.textColor = node.optional("text-color");
      b.height = node.optionalAs<double>("height").value_or(16.0);
      b.text = node.text();
      return b;
    }

    CounterTileDoc
    parseTile(const XmlNode& node)
    {
      CounterTileDoc t;
      t.fill = node.required("fill");
      t.color = node.optional("color");
      t.text = node.text();
      return t;
    }

    CounterFaceDoc
    parseFace(const XmlNode& node)
    {
      CounterFaceDoc f;
      f.style = node.optional("style");
      f.ground = node.optional("ground");
      f.text = node.optional("text");
      f.split = node.optional("split");
      for (const XmlNode& c : node.children()) {
        const std::string n = c.name();
        if ("symbol" == n) {
          f.symbols.push_back(parseSymbol(c));
        } else if ("silhouette" == n) {
          f.silhouettes.push_back(parseSilhouette(c));
        } else if ("emblem" == n) {
          f.emblems.push_back(parseEmblem(c));
        } else if ("echelon" == n) {
          f.echelons.push_back(parseEchelon(c));
        } else if ("value" == n) {
          f.values.push_back(parseValue(c));
        } else if ("text" == n) {
          f.texts.push_back(parseText(c));
        } else if ("steps" == n) {
          f.steps.push_back(parseSteps(c));
        } else if ("glyph" == n) {
          f.glyphs.push_back(parseGlyph(c));
        } else if ("band" == n) {
          f.bands.push_back(parseBand(c));
        } else if ("tile" == n) {
          f.tiles.push_back(parseTile(c));
        }
      }
      return f;
    }

    CounterBackDoc
    parseBack(const XmlNode& node)
    {
      CounterBackDoc b;
      b.face = parseFace(node);
      b.derived = node.optional("derived");
      if (b.derived) {
        checkEnum(node, "derived", *b.derived, {"reduced", "concealed", "same"});
      }
      b.ref = node.optional("ref");
      return b;
    }

    CounterDoc
    parseCounter(const XmlNode& node)
    {
      CounterDoc c;
      c.id = node.required("id");
      c.family = node.required("family");
      checkEnum(node, "family", c.family, {"unit", "support", "leader", "marker"});
      c.name = node.optional("name");
      c.count = node.optionalAs<int>("count").value_or(1);
      c.front = parseFace(requiredChild(node, "front"));
      if (const std::optional<XmlNode> back = node.child("back")) {
        c.back = parseBack(*back);
      }
      return c;
    }

    CounterPlaceDoc
    parsePlace(const XmlNode& node)
    {
      CounterPlaceDoc p;
      p.counter = node.optional("counter");
      p.blank = node.optionalAs<bool>("blank").value_or(false);
      p.repeat = node.optionalAs<int>("repeat").value_or(1);
      return p;
    }

    CounterSheetDoc
    parseSheet(const XmlNode& node)
    {
      CounterSheetDoc s;
      s.id = node.required("id");
      s.title = node.optional("title");
      s.cols = node.requiredAs<int>("cols");
      s.rows = node.requiredAs<int>("rows");
      s.gutter = node.optionalAs<double>("gutter").value_or(0.0);
      s.margin = node.optionalAs<double>("margin").value_or(10.0);
      s.mirror = node.optional("mirror").value_or("horizontal");
      checkEnum(node, "mirror", s.mirror, {"horizontal", "vertical", "none"});
      s.cropMarks = node.optionalAs<bool>("crop-marks").value_or(true);
      s.background = node.optional("background");
      for (const XmlNode& p : node.children("place")) {
        s.places.push_back(parsePlace(p));
      }
      return s;
    }

  }  // namespace

  CounterSetDoc
  CounterSetDoc::parse(const XmlDocument& doc)
  {
    const XmlNode root = doc.root();
    if ("counters" != root.name()) {
      throw std::invalid_argument(root.file() + ":" + std::to_string(root.line()) +
                                   ": expected root element 'counters', found '" + root.name() + "'");
    }

    CounterSetDoc c;
    c.id = root.required("id");
    c.title = root.required("title");
    c.source = root.optional("source");
    c.size = root.optionalAs<double>("size").value_or(12.7);
    c.corner = root.optionalAs<double>("corner").value_or(0.0);
    c.font = root.optional("font").value_or(c.font);

    const XmlNode palette = requiredChild(root, "palette");
    for (const XmlNode& color : palette.children("color")) {
      c.palette.push_back(parseColor(color));
    }

    const XmlNode styles = requiredChild(root, "styles");
    for (const XmlNode& style : styles.children("style")) {
      c.styles.push_back(parseStyle(style));
    }

    for (const XmlNode& counter : root.children("counter")) {
      c.counters.push_back(parseCounter(counter));
    }

    for (const XmlNode& sheet : root.children("sheet")) {
      c.sheets.push_back(parseSheet(sheet));
    }

    return c;
  }

}  // namespace HexXml
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
