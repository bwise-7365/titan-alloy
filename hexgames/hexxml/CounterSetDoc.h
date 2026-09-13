// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// A hexcounters document (unit_graphics/xml/hexcounters.xsd), mirrored one to one. A Face's content
// is a choice of ten element kinds in any order, any number of times; parse() keeps each kind in its
// own vector, in document order (RosterBuilder only ever needs the value line, front.values.front(),
// so no cross-kind ordering is needed). Types are prefixed Counter* to avoid colliding with the
// same-named element of another hexxml document.
// ----------------------------------------------
#pragma once
#include "hexxml/XmlDocument.h"

#include <optional>
#include <string>
#include <vector>

namespace HexXml {

  struct CounterColorDoc {
    std::string id;
    std::string value;
    std::optional<std::string> name;
  };

  struct CounterStyleDoc {
    std::string id;
    std::optional<std::string> name;
    std::string ground;
    std::string text;
    std::string boxStroke;
    std::optional<std::string> boxFill;
  };

  struct CounterSymbolDoc {
    std::string icon;
    std::vector<std::string> modifiers;
    std::optional<std::string> fill;
    std::optional<std::string> stroke;
    std::string partial = "none";
    std::optional<std::string> text;
    double scale = 1.0;
  };

  struct CounterSilhouetteDoc {
    std::string kind;
    std::optional<std::string> color;
    std::optional<std::string> href;
    double scale = 1.0;
  };

  struct CounterEmblemDoc {
    std::string kind;
    std::optional<std::string> color;
    std::optional<std::string> color2;
    std::string slot = "CENTRE";
    double scale = 1.0;
  };

  struct CounterEchelonDoc {
    std::string level;
    std::optional<std::string> color;
  };

  struct CounterValueDoc {  // the value line: "8-7", "3-3-1", "4-U", "Hero RD"
    std::optional<std::string> color;
    std::string size = "large";
    std::string anchor = "middle";
    std::string text;
  };

  struct CounterTextDoc {
    std::string slot;
    std::optional<double> x;
    std::optional<double> y;
    double rotate = 0.0;
    std::string size = "small";
    std::optional<std::string> color;
    std::string weight = "normal";
    bool italic = false;
    std::string text;
  };

  struct CounterStepsDoc {
    int count = 0;
    std::optional<int> maxCount;
    std::string mark = "dot";
    std::string slot = "LCOL";
    std::optional<std::string> color;
  };

  struct CounterGlyphDoc {
    std::string kind;
    std::string slot = "LL";
    std::optional<std::string> color;
    std::optional<std::string> text;
    std::optional<double> x;
    std::optional<double> y;
  };

  struct CounterBandDoc {
    std::string color;
    std::optional<std::string> textColor;
    double height = 16.0;
    std::string text;
  };

  struct CounterTileDoc {
    std::string fill;
    std::optional<std::string> color;
    std::string text;
  };

  // One printed face: front, or a back that is a full face in its own right.
  struct CounterFaceDoc {
    std::optional<std::string> style;  // required on a front; a back without one inherits the front's
    std::optional<std::string> ground;
    std::optional<std::string> text;
    std::optional<std::string> split;
    std::vector<CounterSymbolDoc> symbols;
    std::vector<CounterSilhouetteDoc> silhouettes;
    std::vector<CounterEmblemDoc> emblems;
    std::vector<CounterEchelonDoc> echelons;
    std::vector<CounterValueDoc> values;
    std::vector<CounterTextDoc> texts;
    std::vector<CounterStepsDoc> steps;
    std::vector<CounterGlyphDoc> glyphs;
    std::vector<CounterBandDoc> bands;
    std::vector<CounterTileDoc> tiles;
  };

  struct CounterBackDoc {
    CounterFaceDoc face;
    std::optional<std::string> derived;  // reduced | concealed | same
    std::optional<std::string> ref;      // another counter, printed here instead
  };

  struct CounterDoc {
    std::string id;
    std::string family;  // unit | support | leader | marker
    std::optional<std::string> name;
    int count = 1;
    CounterFaceDoc front;
    std::optional<CounterBackDoc> back;
  };

  struct CounterPlaceDoc {
    std::optional<std::string> counter;
    bool blank = false;
    int repeat = 1;
  };

  struct CounterSheetDoc {
    std::string id;
    std::optional<std::string> title;
    int cols = 0;
    int rows = 0;
    double gutter = 0.0;
    double margin = 10.0;
    std::string mirror = "horizontal";
    bool cropMarks = true;
    std::optional<std::string> background;
    std::vector<CounterPlaceDoc> places;
  };

  struct CounterSetDoc {
    std::string id;
    std::string title;
    std::optional<std::string> source;
    double size = 12.7;
    double corner = 0.0;
    std::string font = "Arial Narrow, Liberation Sans Narrow, Arial, sans-serif";

    std::vector<CounterColorDoc> palette;
    std::vector<CounterStyleDoc> styles;
    std::vector<CounterDoc> counters;
    std::vector<CounterSheetDoc> sheets;

    static CounterSetDoc parse(const XmlDocument&);
  };

}  // namespace HexXml
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
