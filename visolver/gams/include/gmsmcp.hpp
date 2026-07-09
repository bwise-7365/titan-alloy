// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// GP3: build a VINCP mixed complementarity problem from a GmsDatabase model.
// The Model statement's eq.var pairs drive everything: pairs with FREE
// variables become the H block (=e= rows), pairs with POSITIVE variables the
// G block (=g= or =e= rows -- the mixed-complementarity reading derives from
// the VARIABLE'S KIND; the relation gets a consistency check only). Each
// family expands row-major over its domain sets, and each equation family's
// expansion pairs elementwise with its variable family's (their per-dimension
// base sets must match). z0 packs the .L levels; .UP bounds are SURFACED on
// the result (VIModel's K is the orthant -- the caller decides; the corpus
// files' own comments say their bounds are redundant at solutions).
// ----------------------------------------------
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// software is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this software is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
// ----------------------------------------------
#ifndef VINCP_GMS_GMSMCP_HPP
#define VINCP_GMS_GMSMCP_HPP

#include "gmsdatabase.hpp"

#include "vincp.hpp"

#include <string>
#include <vector>

namespace VINCP::Gms {

  // Where one variable family landed in the packed z = [x | y]: offset is
  // within its block (x for free, y for positive), count is the family's
  // expanded size, and elements follow the variable array's row-major order.
  struct GmsMcpSlot {
    string key;    // variable key (lower-cased)
    string name;   // original spelling
    bool freeP = false;
    Index offset = 0;
    Index count = 0;
  };

  struct GmsMcp {
    VIModel model;          // n, m, H, G
    VectorXd z0;            // size n+m, packed [x | y] from the .L levels
    VectorXd upperBounds;   // size n+m; +inf where absent (surfaced, not applied)
    bool anyFiniteUpperP = false;
    vector<GmsMcpSlot> slots;   // free families first, then positive, pair order
  };

  // Build the MCP for one Model statement. The returned closures READ THE
  // DATABASE LIVE (parameters, set sizes): `db` must outlive the GmsMcp.
  // Throws std::invalid_argument on: an undefined or doubly-used equation, a
  // doubly-paired variable, an =l= row, a free variable paired with an
  // inequality, an equation/variable domain mismatch, or an equation that
  // references a variable no pair covers.
  GmsMcp buildGmsMcp(const GmsDatabase& db, const string& modelKey);

  // Write a solution back into the database: z's slots land in the paired
  // variables' .L arrays (the row-major order both sides share). Afterward
  // rerunPostSolveAssignments (gmseval.hpp) recomputes the report
  // parameters the way GAMS would have.
  void applyMcpSolution(GmsDatabase& db, const GmsMcp& mcp,
                        const VectorXd& z);

} // namespace VINCP::Gms

#endif // VINCP_GMS_GMSMCP_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
