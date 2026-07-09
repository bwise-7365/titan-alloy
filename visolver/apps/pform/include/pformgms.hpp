// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// pform GMS reader: parse and validate a limited-subset GMS data file defining
// a PFORM instance (act, iss, weight, position, salience, unselectedProb).
// ----------------------------------------------
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// software is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this software is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
// ----------------------------------------------
#ifndef VINCP_APPS_PFORMGMS_HPP
#define VINCP_APPS_PFORMGMS_HPP

// readPformGms parses a small, fixed GMS data file (see apps/pform/doc/
// pform-example.gms) and returns the parliament instance plus the unselected-
// probability knob. It reuses the vincpgms front end (parser + buildGmsDatabase)
// and modifies nothing under gams/.
//
// The accepted file (limited GMS subset) declares, and only declares:
//   Set act ...      -- the parties  (columns of position/salience, of weight)
//   Set iss ...      -- the issues   (rows of position/salience)
//   Parameter weight(act)         -- party weights, all strictly positive
//   Table position(iss, act)      -- preferred positions, each in [0, 1]
//   Table salience(iss, act)      -- saliences, >= 0, each party's column >= 1
//   Parameter unselectedProb (scalar) in (0, (K-1)/K), K = |act|^|iss|
// No variables, equations, models, solves, aliases, or options are permitted.

#include "pformproblem.hpp"

#include <string>
#include <vector>

namespace VINCP::App {

  using std::string;
  using std::vector;

  // The validated contents of a pform GMS file. data.position / data.salience
  // are |iss| x |act| (issues x parties); data.weight is |act|.
  struct PformGmsInput {
    PformData data;
    double    unselectedProb = 0.05;
    vector<string> partyLabels;
    vector<string> issueLabels;
  };

  // Parse and validate 'path' as a pform GMS file. Throws std::invalid_argument
  // with a specific message if the file is not valid GMS (surfacing the parser's
  // own file:line:col), defines any symbol other than the expected six, has the
  // wrong shapes/domains, or carries out-of-range values (a position outside
  // [0, 1], a negative salience, a party whose total salience is below 1, a
  // non-positive weight, or unselectedProb outside (0, (K-1)/K)).
  PformGmsInput readPformGms(const string& path);

} // namespace VINCP::App

#endif // VINCP_APPS_PFORMGMS_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
