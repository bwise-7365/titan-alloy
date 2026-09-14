// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// [PROPOSED contract, M10] Counter faces: FaceResolver turns a counters document's face into a Face
// with its style and colours resolved (a back without a style inherits the front's; derived backs --
// reduced, concealed, same -- and backs printed on another counter become plain faces), and
// FaceLayout draws a Face the way counters2svg.py does, on its 100-unit square: the slot anchors
// (UL TC UR ML MR LL BOTTOM LR BAND CENTRE LCOL), the text sizes (small 8, medium 10, large 19), the
// symbol box, echelon marks, the value line, step marks, bands and tiles.
// ----------------------------------------------
#pragma once
#include "hexmodel/Ids.h"
#include "hexview/Scene.h"
#include "hexview/SymbolLibrary.h"
#include "hexxml/CounterSetDoc.h"

#include <optional>
#include <vector>

namespace HexView {

  enum class FaceSide : std::uint8_t { Front, Back };

  struct Face {
    Color ground;
    Color text;
    Color boxStroke;
    std::optional<Color> boxFill;
    HexXml::CounterFaceDoc elements;  // symbols, values, texts ... with the style's colours applied
  };

  class FaceResolver {
  public:
    // Throws std::invalid_argument naming the counter or style for a dangling reference.
    explicit FaceResolver(const HexXml::CounterSetDoc&);
    // Throws std::invalid_argument naming the counter when the set has none by that id.
    Face face(const HexModel::CounterId&, FaceSide) const;
    double counterSizeMillimetres() const;
    double cornerRadius() const;  // as a fraction of the size

  private:
    const HexXml::CounterSetDoc& set_;
  };

  class FaceLayout {
  public:
    explicit FaceLayout(const SymbolLibrary&);
    // A face drawn into a square of `size` sheet pixels with its top-left corner at `topLeft`; every
    // primitive carries `hit`.
    std::vector<Primitive> draw(const Face&, Pixel topLeft, double size, HitTag hit) const;

  private:
    const SymbolLibrary& symbols_;
  };

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
