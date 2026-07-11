// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// pmatrix GMS reader: parse and validate a limited-subset GMS data file that
// defines an SAOE instance (act, opt, weight, reward, raFrac) into a SaoeData.
// ----------------------------------------------
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// software is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this software is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
// ----------------------------------------------
#ifndef VIMCP_APPS_PMATRIXGMS_HPP
#define VIMCP_APPS_PMATRIXGMS_HPP

// The pmatrix CLI reads its SAOE instance from a small, fixed GMS data file
// (see apps/saoe/doc/pmatrix-example.gms). Reusing the vimcpgms front end
// (parser + buildGmsDatabase), readPmatrixGms parses the file, verifies it
// defines EXACTLY the expected symbols with sensible values, and returns the
// reward matrix, strength vector, and risk-aversion fraction. Nothing under
// gms/ is modified; this reader only consumes its public API.
//
// The accepted file (limited GMS subset) declares, and only declares:
//   Set act ...     -- the actors      (rows of reward, entries of weight)
//   Set opt ...     -- the options     (columns of reward)
//   Parameter weight(act)   -- actor strengths, all strictly positive
//   Table reward(act, opt)  -- the reward matrix
//   Parameter raFrac (scalar), assigned a value in [0, 1] -- risk aversion
// No variables, equations, models, solves, aliases, or options are permitted.

#include "problem.hpp"

#include <string>
#include <vector>

namespace VIMCP::App {

  using std::string;
  using std::vector;

  // The validated contents of a pmatrix GMS file. R is |act| x |opt|; S is
  // |act|; raFrac is the risk-aversion fraction (fed to SaoeParams::riskAversion).
  // The label vectors carry the set members' original spellings, for display.
  struct PmatrixInput {
    MatrixXd R;
    VectorXd S;
    double   raFrac = 0.0;
    vector<string> actorLabels;
    vector<string> optionLabels;
  };

  // Parse and validate 'path' as a pmatrix GMS file. Throws std::invalid_argument
  // with a specific, human-readable message if the file is not valid GMS
  // (surfacing the parser's own file:line:col), defines any symbol other than
  // the expected five, has the wrong shapes/domains, or carries out-of-range
  // values (raFrac outside [0, 1], a non-positive weight, mismatched dimensions).
  PmatrixInput readPmatrixGms(const string& path);

} // namespace VIMCP::App

#endif // VIMCP_APPS_PMATRIXGMS_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
