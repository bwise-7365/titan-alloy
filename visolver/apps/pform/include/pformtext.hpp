// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Shared text rendering for the PFORM front ends: the instance/result/coalition
// reports both pform_cli prints and pform_gui displays, as returned strings.
// ----------------------------------------------
#ifndef VINCP_APPS_PFORMTEXT_HPP
#define VINCP_APPS_PFORMTEXT_HPP

// Extracted from pform_cli so the CLI and the GUI render from ONE pipeline:
// the CLI prints these strings verbatim (byte-identical to its pre-extraction
// output), and the GUI shows exactly the same text in its log pane. Every
// format string is preserved from the original printf code; do not "improve"
// the formatting here without diffing a captured CLI run.

#include "pformproblem.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace VINCP::App {

  // The instance-plus-provenance bundle both front ends render from: the data,
  // the q knob, the display labels, and where the instance came from.
  struct PformInstance {
    PformData data;
    double    unselectedProb = 0.05;
    std::vector<std::string> partyLabels;
    std::vector<std::string> issueLabels;
    bool          randomP = false;   // generated (vs read from a GMS file)
    std::uint64_t seed = 0;          // the PRNG seed actually used (random mode)
  };

  // "P0", "P1", ... / "I0", "I1", ... fallback labels.
  std::vector<std::string> pformDefaultLabels(const char* prefix, Index n);

  // House seed convention: 0 means "pick one for me". Returns the requested
  // seed unchanged when non-zero; otherwise draws a non-zero seed from
  // std::random_device so the caller can DISPLAY the seed actually used and
  // the run stays reproducible.
  std::uint64_t pformResolveSeed(std::uint64_t requested);

  // Render a parliament's matching as its controlling-party labels per issue,
  // e.g. [P0 P1 P0].
  std::string pformMatchingText(Index k, Index m, Index d,
                                const std::vector<std::string>& partyLabels);

  // Render a coalition pattern: a pinned issue shows its controlling party, a
  // free issue shows '*'. Every cell is right-justified to the widest party
  // label so the columns line up under pformMatchingText's, e.g. [P0  * P1].
  std::string pformPatternText(const std::vector<Index>& pattern,
                               const std::vector<std::string>& partyLabels);

  // Spreadsheet-style coalition label: A..Z, then AA, AB, ...
  std::string pformCoalitionLabel(std::size_t index);

  // The three report blocks. appTag is the parenthetical in the section
  // headers ("pform_cli" / "pform_gui"), e.g. "=== PFORM instance (pform_cli) ===".
  //
  // renderPformInputs: the weight vector and the position and salience
  // matrices (issues x parties), all with their labels.
  std::string renderPformInputs(const PformInstance& in,
                                const std::string& appTag);
  // renderPformResult: dimensions, q + derived eps, engine, solver telemetry,
  // the deterministic parliament, the supported-parliaments table ('*' marks
  // the deterministic one), party utilities, and the non-convergence warning.
  std::string renderPformResult(const PformInstance& in,
                                const PformParams& params, const VIResult& vi,
                                const PformResult& res,
                                const std::string& appTag);
  // renderPformCoalitions: the coalition table plus the party-vote-split block.
  std::string renderPformCoalitions(const PformInstance& in,
                                    const std::vector<PformCoalition>& coalitions);

} // namespace VINCP::App

#endif // VINCP_APPS_PFORMTEXT_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
