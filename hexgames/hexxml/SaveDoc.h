// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// A hexsave document (game_records/xml/hexsave.xsd), mirrored one to one. One schema, four flavours
// told apart by @kind (scenario, save, script, golden); parse() does not resolve any id against the
// rules/sheet/counters documents -- that is PositionBuilder's job in hexmodel, and readRecord's job
// in hexrecord, both of which report "file:line attribute -> target document" on failure. Types are
// prefixed Save* to avoid colliding with the same-named element of another hexxml document.
// ----------------------------------------------
#pragma once
#include "hexxml/XmlDocument.h"

#include <optional>
#include <string>
#include <vector>

namespace HexXml {

  struct SaveFileDoc {
    std::string role;  // package | rules | sheet | counters | cards | scenario
    std::string path;
    std::optional<std::string> sha256;
  };

  struct SaveCursorDoc {
    int turn = 0;
    std::string phase;
    std::optional<std::string> side;
    int moves = 0;
    bool over = false;
    std::optional<std::string> winner;
  };

  struct SaveRegisterDoc {
    std::string track;
    std::string value;
  };

  struct SaveFlagDoc {
    std::string name;
    std::string value;
  };

  struct SaveSideDoc {
    std::string id;
    std::vector<SaveRegisterDoc> registers;
    std::vector<SaveFlagDoc> flags;
  };

  struct SaveUnitDoc {
    std::string id;  // counter id, optionally "#k"
    std::string counter;
    std::string type;
    std::string owner;
    std::optional<std::string> hex;
    std::optional<std::string> space;
    std::string face = "front";  // front | back
    std::optional<int> steps;
    std::vector<std::string> status;
    std::optional<std::string> attached;
    std::optional<bool> revealed;
    bool moved = false;
    std::optional<int> delay;
    std::string text;
  };

  struct SaveControlHexDoc {
    std::string id;
    std::string side;
  };

  struct SaveControlLinkDoc {
    std::string network;
    std::vector<std::string> hexes;
    std::string side;
  };

  struct SaveRegionDoc {
    std::string layer;
    std::string id;
    std::optional<std::string> status;
    std::optional<std::string> alignment;
    std::optional<std::string> posture;
    std::optional<std::string> owner;
  };

  struct SavePileDoc {
    std::string randomizer;
    std::string kind;  // draw | hand | pending | current | discard | removed
    std::optional<std::string> side;
    std::vector<std::string> cards;
    std::optional<int> next;
  };

  struct SaveStreamDoc {
    std::string tag;
    int draws = 0;
    std::optional<std::string> check;
  };

  struct SaveArgDoc {
    std::string name;
    std::string value;
  };

  struct SaveResultDoc {
    std::string outcome;
    std::optional<std::string> odds;
    std::optional<std::string> column;
    std::optional<int> drm;
    std::string text;
  };

  struct SaveDrawDoc {
    std::optional<std::string> stream;
    std::optional<int> n;
    std::optional<std::string> value;
    std::optional<std::string> randomizer;
    std::optional<std::string> card;
  };

  struct SaveEventDoc {
    std::string kind;
    std::optional<std::string> unit;
    std::optional<std::string> hex;
    std::optional<std::string> side;
    std::optional<std::string> value;
    std::string text;
  };

  struct SaveMoveDoc {
    int n = 0;
    int turn = 0;
    std::string phase;
    std::string side;
    std::string by = "script";  // human | script | ai
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

    std::vector<SaveArgDoc> args;
    std::optional<SaveResultDoc> result;
    std::vector<SaveDrawDoc> draws;
    std::vector<SaveEventDoc> events;
  };

  struct SaveNoteDoc {
    std::optional<int> n;
    std::string text;
  };

  struct SaveDoc {
    std::string format;
    std::string kind;  // scenario | save | script | golden
    std::string game;
    std::string package;
    std::optional<std::string> scenario;
    std::uint64_t seed = 0;
    std::optional<std::string> engine;
    std::optional<std::string> created;
    std::optional<std::string> title;

    std::vector<SaveFileDoc> files;
    SaveCursorDoc cursor;
    std::vector<SaveSideDoc> sides;
    std::vector<SaveUnitDoc> units;
    std::vector<SaveControlHexDoc> controlHexes;
    std::vector<SaveControlLinkDoc> controlLinks;
    std::vector<SaveRegionDoc> regions;
    std::vector<SavePileDoc> piles;
    std::vector<SaveStreamDoc> streams;
    std::vector<SaveMoveDoc> log;
    std::vector<SaveNoteDoc> notes;

    static SaveDoc parse(const XmlDocument&);
  };

}  // namespace HexXml
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
