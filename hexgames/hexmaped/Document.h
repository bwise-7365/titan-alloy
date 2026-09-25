// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#pragma once
// Document -- the sheet under edit: a SheetDoc (the schema's model), its SheetFrame, and the edits
// the editor offers. Every edit checks its arguments against the sheet's own declarations and the
// schema's vocabularies at the boundary and throws std::invalid_argument naming the offender; a
// Document therefore never holds a structure hexsheet.xsd would refuse. Undo and redo are whole
// snapshots of the model (a sheet is a few thousand small structs), which makes them exact.

#include "hexmaped/SheetFrame.h"
#include "hexxml/SheetDoc.h"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace HexMapEd {

  class Document {
  public:
    static Document load(const std::filesystem::path&);  // throws as SheetDoc::parse does
    explicit Document(HexXml::SheetDoc sheet, std::filesystem::path path = {});

    const HexXml::SheetDoc& sheet() const { return sheet_; }
    const SheetFrame& frame() const { return frame_; }
    const std::filesystem::path& path() const { return path_; }
    bool dirtyP() const { return dirtyP_; }

    void save();                                   // writes path(), clears dirty
    static std::string schemaLocationFor(const std::filesystem::path& file);  // hexsheet.xsd relative to the file
    void saveAs(const std::filesystem::path&);

    // ---- queries the panels need
    std::string terrainOf(std::string_view hex) const;             // the bulk assignment or the grid default
    std::vector<std::string> edgeTokensAt(std::string_view hex) const;  // edges written on this hex or its neighbours' shared sides
    const HexXml::SheetHexDoc* hexElement(std::string_view hex) const;  // nullptr if the hex has no content element
    std::optional<std::string> lineOfEdge(const Hexside&) const;
    std::vector<std::string> terrainIds() const;
    std::vector<std::string> lineIds() const;
    std::vector<std::string> colourIds() const;
    std::vector<std::string> linkKinds() const;  // kinds in use, "rail" and "road" always offered

    // ---- edits (each one undoable)
    void setTerrain(std::string_view hex, std::string_view terrainId);
    void toggleEdge(const Hexside&, std::string_view lineId);      // add the line on that hexside, or remove it
    void addLinkStep(std::string_view a, std::string_view b, std::string_view kind, std::string_view lineId);
    void removeLinkStep(std::string_view a, std::string_view b);   // the chain splits around the step
    void setName(std::string_view hex, std::optional<std::string> name);
    void addGlyph(std::string_view hex, std::string_view symbol, std::string_view slot, std::optional<std::string> colour);
    void removeGlyph(std::string_view hex, std::size_t index);
    void setRing(std::string_view hex, std::optional<std::string> colourId);
    void toggleClip(std::string_view hex);                         // a printed hex leaves the grid, or a clipped one returns
    // The hex the grid would print at this pixel joins the grid: a clipped cell inside the grid's
    // rectangle is unclipped; a cell just outside it grows the rectangle by one column or row (the
    // other new cells stay clipped, every existing id keeps its name and place). Returns the id.
    std::string addHexAt(Pixel);
    std::optional<std::string> cellAt(Pixel) const;                // the id of the printed or clipped cell there, none outside the rectangle

    bool canUndoP() const { return !undo_.empty(); }
    bool canRedoP() const { return !redo_.empty(); }
    void undo();
    void redo();

  private:
    void edit(const std::function<void(HexXml::SheetDoc&)>&);      // snapshot, apply, rebuild the frame
    void requireHex(std::string_view hex, std::string_view where) const;
    void requireTerrain(std::string_view id, std::string_view where) const;
    void requireLine(std::string_view id, std::string_view where) const;
    void requireColour(std::string_view id, std::string_view where) const;
    HexXml::SheetHexDoc& hexElementFor(HexXml::SheetDoc&, std::string_view hex) const;
    std::vector<std::string> clippedIds(const HexXml::SheetGridDoc&) const;  // the clip list with ranges expanded
    static void growTowards(HexXml::SheetGridDoc&, Pixel, const HexCoord::Grid& twin);

    HexXml::SheetDoc sheet_;
    SheetFrame frame_;
    std::filesystem::path path_;
    bool dirtyP_ = false;
    std::vector<HexXml::SheetDoc> undo_;
    std::vector<HexXml::SheetDoc> redo_;
  };

}  // namespace HexMapEd
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
