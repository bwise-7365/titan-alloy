// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The hexsave document as a plain value: strings and numbers, one member per attribute or element,
// independent of HexModel/HexEngine. Record.h's Record needs a Board and Roster to resolve ids into
// a HexModel::Position; that conversion is M4 glue. Everything that only needs the document itself --
// reading, canonical writing, golden comparison -- works on this model alone.
// ----------------------------------------------
#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace HexRecord {

  struct SaveFile {
    std::string role;
    std::string path;
    std::optional<std::string> sha256;
    bool operator==(const SaveFile&) const = default;
  };

  struct SaveCursor {
    int turn = 0;
    std::string phase;
    std::optional<std::string> side;
    std::uint64_t moves = 0;
    bool overP = false;
    std::optional<std::string> winner;
    bool operator==(const SaveCursor&) const = default;
  };

  struct SaveRegister {
    std::string track;
    std::string value;
    bool operator==(const SaveRegister&) const = default;
  };

  struct SaveFlag {
    std::string name;
    std::string value;
    bool operator==(const SaveFlag&) const = default;
  };

  struct SaveSide {
    std::string id;
    std::vector<SaveRegister> registers;
    std::vector<SaveFlag> flags;
    bool operator==(const SaveSide&) const = default;
  };

  struct SaveUnit {
    std::string id;
    std::string counter;
    std::string type;
    std::string owner;
    std::optional<std::string> hex;
    std::optional<std::string> space;
    std::string face = "front";
    std::optional<int> steps;
    std::vector<std::string> status;
    std::optional<std::string> attached;
    std::optional<bool> revealedP;
    bool movedP = false;
    std::optional<int> delay;
    std::string text;  // element content: the printed name, if any
    bool operator==(const SaveUnit&) const = default;
  };

  struct SaveControlHex {
    std::string id;
    std::string side;
    bool operator==(const SaveControlHex&) const = default;
  };

  struct SaveControlLink {
    std::string network;
    std::vector<std::string> hexes;
    std::string side;
    bool operator==(const SaveControlLink&) const = default;
  };

  struct SaveRegion {
    std::string layer;
    std::string id;
    std::optional<std::string> status;
    std::optional<std::string> alignment;
    std::optional<std::string> posture;
    std::optional<std::string> owner;
    bool operator==(const SaveRegion&) const = default;
  };

  struct SavePile {
    std::string randomizer;
    std::string kind;
    std::optional<std::string> side;
    std::vector<std::string> cards;
    std::optional<int> next;
    bool operator==(const SavePile&) const = default;
  };

  struct SaveStream {
    std::string tag;
    std::uint64_t draws = 0;
    std::optional<std::string> check;
    bool operator==(const SaveStream&) const = default;
  };

  struct SaveArg {
    std::string name;
    std::string value;
    bool operator==(const SaveArg&) const = default;
  };

  struct SaveResult {
    std::string outcome;
    std::optional<std::string> odds;
    std::optional<std::string> column;
    std::optional<int> drm;
    std::string text;
    bool operator==(const SaveResult&) const = default;
  };

  struct SaveDraw {
    std::optional<std::string> stream;
    std::optional<int> n;
    std::optional<std::string> value;
    std::optional<std::string> randomizer;
    std::optional<std::string> card;
    bool operator==(const SaveDraw&) const = default;
  };

  struct SaveEvent {
    std::string kind;
    std::optional<std::string> unit;
    std::optional<std::string> hex;
    std::optional<std::string> side;
    std::optional<std::string> value;
    std::string text;
    bool operator==(const SaveEvent&) const = default;
  };

  struct SaveMove {
    int n = 0;
    int turn = 0;
    std::string phase;
    std::string side;
    std::string by = "script";
    std::string cmd;
    std::vector<std::string> units;
    std::optional<std::string> from;
    std::optional<std::string> to;
    std::vector<std::string> path;
    std::optional<std::string> target;
    std::optional<std::string> mode;
    std::vector<std::string> modifiers;
    std::optional<std::string> card;
    std::optional<std::string> value;
    std::optional<std::string> choice;
    std::vector<SaveArg> args;
    std::optional<SaveResult> result;
    std::vector<SaveDraw> draws;
    std::vector<SaveEvent> events;
    bool operator==(const SaveMove&) const = default;
  };

  struct SaveNote {
    std::optional<int> n;
    std::string text;
    bool operator==(const SaveNote&) const = default;
  };

  // The whole document. Section wrappers that hexsave.xsd makes optional (package, sides, units,
  // control, regions, piles, streams, log, notes) are represented by their child vectors alone: an
  // empty vector reads back the same whether the source omitted the wrapper or wrote it empty, and
  // writeCanonical omits an empty wrapper.
  struct SaveModel {
    std::string kind;  // "scenario" | "save" | "script" | "golden"
    std::string game;
    std::string package;
    std::optional<std::string> scenario;
    std::uint64_t seed = 0;
    std::optional<std::string> engine;
    std::optional<std::string> created;  // copied through unchanged; never generated or reformatted
    std::optional<std::string> title;
    std::string schemaLocation = "hexsave.xsd";  // xsi:noNamespaceSchemaLocation, relative as read

    std::vector<SaveFile> files;
    SaveCursor cursor;
    std::vector<SaveSide> sides;
    std::vector<SaveUnit> units;
    std::vector<SaveControlHex> controlHexes;
    std::vector<SaveControlLink> controlLinks;
    std::vector<SaveRegion> regions;
    std::vector<SavePile> piles;
    std::vector<SaveStream> streams;
    std::vector<SaveMove> log;
    std::vector<SaveNote> notes;

    bool operator==(const SaveModel&) const = default;

    // Reads any hexsave document into the plain value model. Throws std::invalid_argument naming
    // file:line and the attribute or id for the checks the XSD cannot express: exactly one of
    // unit/@hex and unit/@space; move numbers contiguous from 1; a "#k" copy suffix with k >= 1.
    // Cross-document reference checks (does the hex or counter exist) are M4 glue, not here.
    static SaveModel read(const std::filesystem::path&);
  };

  // Writes the canonical form: the XML declaration, xsi:noNamespaceSchemaLocation as stored on the
  // model, attributes in the order hexsave.xsd declares them, units sorted by @id, control hexes
  // sorted by @id, moves sorted by @n, two-space indent, LF line endings, no trailing whitespace.
  // Two equal models write identical bytes.
  void writeCanonical(const SaveModel&, const std::filesystem::path&);

  // The same canonical form as writeCanonical, as a string, for comparison and diffing.
  std::string canonicalText(const SaveModel&);

  // Text rendering of one draw or event, for logs and divergence messages: "combat#5=3" (a stream
  // draw), "deck=card-17" (a randomizer draw), "moved unit=... hex=..." (an event).
  std::string renderDraw(const SaveDraw&);
  std::string renderEvent(const SaveEvent&);

}  // namespace HexRecord
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
