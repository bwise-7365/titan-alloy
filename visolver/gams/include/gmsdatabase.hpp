// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Symbol table for the GAMS subset (GP2): sets/aliases, dense numeric arrays
// for parameters and variable attributes, recorded equations/models/options/
// solves. Filled by buildGmsDatabase (gmseval.hpp), consumed by the GP3
// VIModel builder. All lookups are by lower-cased key.
// ----------------------------------------------
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// code is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this code is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
// ----------------------------------------------
#ifndef VINCP_GMS_GMSDATABASE_HPP
#define VINCP_GMS_GMSDATABASE_HPP

#include "gmsast.hpp"

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace VINCP::Gms {

  using std::map;
  using std::size_t;
  using std::string;
  using std::vector;

  // An ordered label list; ordinals are 0-based positions.
  struct GmsSet {
    string name;                    // original spelling
    vector<string> labels;          // original spellings, declaration order
    map<string, size_t> ordinals;   // lower-cased label -> position

    size_t size() const { return labels.size(); }
    // Position of a label (case-insensitive); throws std::invalid_argument
    // naming the set when the label is unknown.
    size_t ordinalOf(const string& label) const;
  };

  // Dense row-major numeric storage; rank 0 is a scalar (one value).
  struct GmsArray {
    vector<size_t> shape;
    vector<double> values;

    static GmsArray filled(const vector<size_t>& shape, double fill);
    size_t rank() const { return shape.size(); }
    // Flat position of an ordinal tuple; throws on rank or range mismatch.
    size_t flatIndex(const vector<size_t>& ordinals) const;
    double at(const vector<size_t>& ordinals) const;
    double& at(const vector<size_t>& ordinals);
  };

  struct GmsParameter {
    string name;
    vector<string> domainKeys;   // as declared (may be alias keys)
    GmsArray data;               // shape = base-set sizes; defaults 0
    // A parameter declared WITHOUT a domain takes its shape from its first
    // assignment (GAMS's universal-domain behavior; deploy_v09's LogUsedR).
    bool domainOpenP = false;
  };

  struct GmsVariable {
    string name;
    bool positiveP = false;
    vector<string> domainKeys;
    GmsArray level;   // .L, defaults 0
    GmsArray lower;   // .LO: 0 for positive, -inf for free
    GmsArray upper;   // .UP: +inf
  };

  struct GmsEquation {
    string name;
    vector<string> domainKeys;   // from the Equations declaration
    bool definedP = false;
    EquationDef def;             // valid iff definedP
  };

  struct GmsModel {
    string name;
    vector<ModelDecl::Pair> pairs;
    map<string, double> attrs;   // e.g. optfile
  };

  struct GmsDatabase {
    map<string, GmsSet> sets;        // base sets, by key
    map<string, string> aliases;     // alias key -> base set key
    map<string, GmsParameter> parameters;   // scalars are rank 0
    map<string, GmsVariable> variables;
    map<string, GmsEquation> equations;
    map<string, GmsModel> models;
    vector<OptionStmt> options;
    vector<SolveStmt> solves;

    bool setP(const string& key) const;
    // Base set behind a set or alias key; throws when neither.
    const GmsSet& resolveSet(const string& key) const;
    // Checked lookups; each throws std::invalid_argument naming the symbol.
    const GmsParameter& parameter(const string& key) const;
    const GmsVariable& variable(const string& key) const;
    const GmsEquation& equation(const string& key) const;
    const GmsModel& model(const string& key) const;
  };

} // namespace VINCP::Gms

#endif // VINCP_GMS_GMSDATABASE_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
