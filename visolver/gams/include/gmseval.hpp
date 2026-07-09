// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Eager evaluator for the GAMS subset (GP2) and the shared expression
// evaluator it is built on (also used by the GP3 model builder).
// buildGmsDatabase walks a parsed Program in statement order: declarations
// create symbols, data blocks and tables fill arrays, assignments evaluate
// immediately (indexed lvalues loop over their domains), equation
// definitions are stored and symbolically validated, and Model / Option /
// Solve / Display are recorded, not executed. Post-Solve assignments
// therefore see the INITIAL variable levels; GP3+ re-runs them after a real
// solve fills the levels.
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

#include <functional>

namespace VINCP::Gms {

  using std::function;

  // Numeric evaluation of Expr trees against a database, with loop-index
  // bindings and a pluggable reader for VARIABLE references -- the one thing
  // that differs between contexts: assignments read the stored attribute
  // arrays (and reject bare variables), while model equations read the
  // solver's current point.
  class GmsExprEvaluator {
  public:
    // Reads a variable reference. attrKey is empty for the variable itself
    // (equation bodies) or the lower-cased attribute ("l", "up", "lo").
    using VariableReader = function<double(const GmsVariable& var,
                                           const string& attrKey,
                                           const vector<size_t>& ordinals)>;

    GmsExprEvaluator(const GmsDatabase& db, VariableReader reader);

    // Loop-index bindings; the innermost binding of a name wins. Callers
    // push a binding per loop dimension, retarget its ordinal per lap with
    // setBindingOrdinal, and pop when the loop ends (sum() inside eval
    // manages its own bindings the same way).
    void pushBinding(const string& key, const GmsSet& set, size_t ordinal);
    void setBindingOrdinal(const string& key, size_t ordinal);
    void popBinding();

    double eval(const Expr& expr);

    // Ordinal tuple for a reference's indices under the current bindings:
    // each index is a bound name or a literal label of its domain set.
    vector<size_t> ordinalsFor(const string& symbolName,
                               const vector<string>& domainKeys,
                               const vector<string>& indices) const;

  protected:
  private:
    struct Binding {
      string key;
      const GmsSet* set = nullptr;
      size_t ordinal = 0;
    };

    const GmsDatabase& db_;
    VariableReader reader_;
    vector<Binding> env_;

    const Binding* findBinding(const string& key) const;
    double evalBinary(const Expr& expr);
    double evalCall(const Expr& expr);
    double sumOver(const Expr& expr, size_t dim);
  };

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
