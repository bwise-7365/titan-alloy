// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Eager evaluator for the GAMS subset (GP2): walks a parsed Program in
// statement order, building the GmsDatabase -- declarations create symbols,
// data blocks and tables fill arrays, assignments evaluate immediately
// (indexed lvalues loop over their domains), equation definitions are stored
// and symbolically validated (references resolvable, arities correct, every
// index bound), and Model/Option/Solve/Display are recorded, not executed.
// Post-Solve assignments therefore see the INITIAL variable levels; GP3+
// re-runs them after a real solve fills the levels.
// ----------------------------------------------
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// code is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this code is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
// ----------------------------------------------
#ifndef VINCP_GMS_GMSEVAL_HPP
#define VINCP_GMS_GMSEVAL_HPP

#include "gmsast.hpp"
#include "gmsdatabase.hpp"

namespace VINCP::Gms {

  // Build the database from a parsed program. Semantic problems (undeclared
  // symbols, arity mismatches, unknown labels, unbound indices) throw
  // std::invalid_argument; a computed non-finite value throws
  // std::runtime_error naming the assignment.
  GmsDatabase buildGmsDatabase(const Program& program);

} // namespace VINCP::Gms

#endif // VINCP_GMS_GMSEVAL_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
