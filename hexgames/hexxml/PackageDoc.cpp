// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexxml/PackageDoc.h"

#include "hexxml/DocDetail.h"

namespace HexXml {

  using Detail::splitTokens;

  namespace {

    PackagePathDoc
    parsePath(const XmlNode& node)
    {
      return PackagePathDoc{node.required("path")};
    }

    PackageCardsDoc
    parseCards(const XmlNode& node)
    {
      PackageCardsDoc c;
      c.randomizer = node.required("randomizer");
      c.path = node.required("path");
      return c;
    }

    PackageScenarioDoc
    parseScenario(const XmlNode& node)
    {
      PackageScenarioDoc s;
      s.id = node.required("id");
      s.path = node.required("path");
      s.title = node.optional("title");
      return s;
    }

    PackageTerrainBindingDoc
    parseTerrainBinding(const XmlNode& node)
    {
      PackageTerrainBindingDoc t;
      t.sheet = node.optional("sheet");
      t.symbol = node.optional("symbol");
      t.rules = node.required("rules");
      return t;
    }

    PackageHexsideBindingDoc
    parseHexsideBinding(const XmlNode& node)
    {
      PackageHexsideBindingDoc h;
      h.line = node.optional("line");
      h.kind = node.optional("kind");
      h.rules = node.required("rules");
      return h;
    }

    PackageNetworkBindingDoc
    parseNetworkBinding(const XmlNode& node)
    {
      PackageNetworkBindingDoc n;
      n.kind = node.required("kind");
      n.rules = node.required("rules");
      return n;
    }

    PackageSpaceBindingDoc
    parseSpaceBinding(const XmlNode& node)
    {
      PackageSpaceBindingDoc s;
      s.rules = node.required("rules");
      s.panel = node.optional("panel");
      s.offSheet = node.optionalAs<bool>("off-sheet").value_or(false);
      return s;
    }

    PackageLayerBindingDoc
    parseLayerBinding(const XmlNode& node)
    {
      PackageLayerBindingDoc l;
      l.sheet = node.required("sheet");
      l.rules = node.required("rules");
      return l;
    }

    PackageSideBindingDoc
    parseSideBinding(const XmlNode& node)
    {
      PackageSideBindingDoc s;
      s.rules = node.required("rules");
      s.styles = splitTokens(node.required("styles"));
      return s;
    }

    PackageUnitBindingDoc
    parseUnitBinding(const XmlNode& node)
    {
      PackageUnitBindingDoc u;
      u.type = node.required("type");
      if (const std::optional<std::string> tokens = node.optional("counters")) {
        u.counters = splitTokens(*tokens);
      }
      u.match = node.optional("match");
      return u;
    }

  }  // namespace

  PackageDoc
  PackageDoc::parse(const XmlDocument& doc)
  {
    const XmlNode root = doc.root();
    if ("package" != root.name()) {
      throw std::invalid_argument(root.file() + ":" + std::to_string(root.line()) +
                                   ": expected root element 'package', found '" + root.name() + "'");
    }

    PackageDoc p;
    p.id = root.required("id");
    p.title = root.optional("title");
    p.version = root.optional("version");

    p.rules = parsePath(Detail::requiredChild(root, "rules"));

    for (const XmlNode& s : root.children("sheet")) {
      p.sheets.push_back(parsePath(s));
    }
    for (const XmlNode& c : root.children("counters")) {
      p.counters.push_back(parsePath(c));
    }
    for (const XmlNode& c : root.children("cards")) {
      p.cards.push_back(parseCards(c));
    }
    for (const XmlNode& s : root.children("scenario")) {
      p.scenarios.push_back(parseScenario(s));
    }
    for (const XmlNode& s : root.children("side")) {
      p.side.push_back(parseSideBinding(s));
    }
    for (const XmlNode& t : root.children("terrain")) {
      p.terrain.push_back(parseTerrainBinding(t));
    }
    for (const XmlNode& h : root.children("hexside")) {
      p.hexside.push_back(parseHexsideBinding(h));
    }
    for (const XmlNode& n : root.children("network")) {
      p.network.push_back(parseNetworkBinding(n));
    }
    for (const XmlNode& s : root.children("space")) {
      p.space.push_back(parseSpaceBinding(s));
    }
    for (const XmlNode& l : root.children("layer")) {
      p.layer.push_back(parseLayerBinding(l));
    }
    for (const XmlNode& u : root.children("unit")) {
      p.unit.push_back(parseUnitBinding(u));
    }

    return p;
  }

}  // namespace HexXml
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
