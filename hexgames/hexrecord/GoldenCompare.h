// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Golden comparison at the document level: two SaveModels compared move by move, and a small line
// diff of their canonical texts. compareWithGolden in Record.h is the M4 glue that runs a Session and
// calls down to these; everything here is pure document-to-document comparison.
// ----------------------------------------------
#pragma once
#include "hexrecord/Record.h"
#include "hexrecord/SaveModel.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace HexRecord {

  // Compares two logs move by move: outcome, then draws in order, then events. Returns the first
  // divergence, naming its move number; nullopt if the logs match. A length mismatch is reported
  // against the move number one past the shorter log's last move.
  std::optional<Divergence> compareLogs(const SaveModel& golden, const SaveModel& actual);

  // A small LCS-based line diff: "  " a common line, "- " only in `expected`, "+ " only in `actual`;
  // capped at maxLines total lines, with a truncation marker past that.
  std::string unifiedDiff(const std::string& expected, const std::string& actual, std::size_t maxLines = 80);

  // Where writeCanonical(actual, ...) lands beside a golden file: "<slug>.golden.xml" ->
  // "<slug>.actual.xml"; any other file name gets ".actual" inserted before the extension.
  std::filesystem::path actualPathFor(const std::filesystem::path& goldenPath);

  // The document-level system test: writes `actual`'s canonical form to actualPathFor(goldenPath),
  // compares it byte for byte with `golden`'s canonical form, and on a mismatch finds the first
  // divergent move and a unified diff of the two texts. The actual file is always written.
  GoldenReport buildGoldenReport(const SaveModel& golden, const SaveModel& actual, const std::filesystem::path& goldenPath);

  // The printable report: on mismatch, the move, golden vs actual outcome/draw, the diff, and the
  // re-bless command; on a match, a one-line confirmation.
  std::string formatGoldenReport(const GoldenReport&, std::string_view game, std::string_view slug);

}  // namespace HexRecord
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
