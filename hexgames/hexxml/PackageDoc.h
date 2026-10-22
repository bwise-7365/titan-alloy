// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// A hexpackage document (packages/xml/hexpackage.xsd), mirrored one to one. Paths are kept exactly as
// written (relative to the manifest file); PackageLoader (hexrules) resolves them and applies every
// binding, reporting the offending id on failure. Types are prefixed Package* to avoid colliding with
// the same-named element of another hexxml document.
// ----------------------------------------------
#pragma once
#include "hexxml/XmlDocument.h"

#include <optional>
#include <string>
#include <vector>

namespace HexXml {

  struct PackagePathDoc {
    std::string path;
  };

  struct PackageCardsDoc {
    std::string randomizer;
    std::string path;
  };

  struct PackageScenarioDoc {
    std::string id;
    std::string path;
    std::optional<std::string> title;
  };

  // sheet: a sheet terrain id; symbol: a sheet glyph symbol that marks the terrain; rules: required.
  struct PackageTerrainBindingDoc {
    std::optional<std::string> sheet;
    std::optional<std::string> symbol;
    std::string rules;
  };

  struct PackageHexsideBindingDoc {
    std::optional<std::string> line;
    std::optional<std::string> kind;
    std::string rules;
  };

  struct PackageNetworkBindingDoc {
    std::string kind;
    std::string rules;
  };

  struct PackageSpaceBindingDoc {
    std::string rules;
    std::optional<std::string> panel;
    bool offSheet = false;
  };

  struct PackageLayerBindingDoc {
    std::string sheet;
    std::string rules;
  };

  // Counter style to rules side: the printed ground colour is the side on every sheet. One element
  // per side; every unit, support and leader counter's style must be bound to exactly one of them.
  struct PackageSideBindingDoc {
    std::string rules;
    std::vector<std::string> styles;
  };

  // Counter to unit type: an explicit id list, or a regular expression over counter ids.
  struct PackageUnitBindingDoc {
    std::string type;
    std::vector<std::string> counters;
    std::optional<std::string> match;
  };

  struct PackageDoc {
    std::string id;
    std::optional<std::string> title;
    std::optional<std::string> version;

    PackagePathDoc rules;
    std::vector<PackagePathDoc> sheets;
    std::vector<PackagePathDoc> counters;
    std::vector<PackageCardsDoc> cards;
    std::vector<PackageScenarioDoc> scenarios;
    std::vector<PackageSideBindingDoc> side;
    std::vector<PackageTerrainBindingDoc> terrain;
    std::vector<PackageHexsideBindingDoc> hexside;
    std::vector<PackageNetworkBindingDoc> network;
    std::vector<PackageSpaceBindingDoc> space;
    std::vector<PackageLayerBindingDoc> layer;
    std::vector<PackageUnitBindingDoc> unit;

    static PackageDoc parse(const XmlDocument&);
  };

}  // namespace HexXml
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
