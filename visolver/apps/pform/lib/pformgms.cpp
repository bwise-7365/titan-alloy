// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// pform GMS reader implementation: build the vimcpgms database, verify the file
// defines exactly the PFORM symbols with sensible values, extract PformData.
// ----------------------------------------------
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// software is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this software is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
// ----------------------------------------------
#include "pformgms.hpp"

#include "gmsdatabase.hpp"
#include "gmseval.hpp"
#include "gmsparser.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <stdexcept>

namespace VIMCP::App {

  namespace {

    template <class MapT>
    vector<string>
    sortedKeys(const MapT& m)
    {
      vector<string> keys;
      for (const auto& entry : m) {
        keys.push_back(entry.first);
      }
      std::sort(keys.begin(), keys.end());
      return keys;
    }

    string
    joinKeys(const vector<string>& keys)
    {
      string out;
      for (size_t i = 0; i < keys.size(); ++i) {
        if (0 < i) {
          out += ", ";
        }
        out += keys[i];
      }
      return out;
    }

    void
    requireExactSymbols(const vector<string>& actual, vector<string> expected,
                        const char* kind)
    {
      std::sort(expected.begin(), expected.end());
      vector<string> missing;
      vector<string> extra;
      std::set_difference(expected.begin(), expected.end(),
                          actual.begin(), actual.end(),
                          std::back_inserter(missing));
      std::set_difference(actual.begin(), actual.end(),
                          expected.begin(), expected.end(),
                          std::back_inserter(extra));
      if (!missing.empty() || !extra.empty()) {
        string msg = "pform GMS: the " + string(kind)
                     + " declarations must be exactly {" + joinKeys(expected) + "}";
        if (!missing.empty()) {
          msg += "; missing: " + joinKeys(missing);
        }
        if (!extra.empty()) {
          msg += "; unexpected: " + joinKeys(extra);
        }
        throw std::invalid_argument(msg);
      }
      return;
    }

  } // namespace

  PformGmsInput
  readPformGms(const string& path)
  {
    const Gms::Program program = Gms::parseGmsFile(path);
    const Gms::GmsDatabase db  = Gms::buildGmsDatabase(program);

    // 1. No model-level constructs -- this is a pure data file.
    if (!db.variables.empty()) {
      throw std::invalid_argument("pform GMS: no variables permitted (found: "
                                  + joinKeys(sortedKeys(db.variables)) + ").");
    }
    if (!db.equations.empty()) {
      throw std::invalid_argument("pform GMS: no equations permitted (found: "
                                  + joinKeys(sortedKeys(db.equations)) + ").");
    }
    if (!db.models.empty()) {
      throw std::invalid_argument("pform GMS: no models permitted (found: "
                                  + joinKeys(sortedKeys(db.models)) + ").");
    }
    if (!db.aliases.empty()) {
      throw std::invalid_argument("pform GMS: no aliases permitted (found: "
                                  + joinKeys(sortedKeys(db.aliases)) + ").");
    }
    if (!db.solves.empty()) {
      throw std::invalid_argument("pform GMS: no Solve statements permitted.");
    }
    if (!db.options.empty()) {
      throw std::invalid_argument("pform GMS: no Option statements permitted.");
    }

    // 2. Exactly the expected sets and parameters, nothing else.
    requireExactSymbols(sortedKeys(db.sets), { "act", "iss" }, "set");
    requireExactSymbols(sortedKeys(db.parameters),
                        { "position", "salience", "unselectedprob", "weight" },
                        "parameter");

    // 3. Sets (dimensions and labels).
    const Gms::GmsSet& act = db.resolveSet("act");
    const Gms::GmsSet& iss = db.resolveSet("iss");
    const Index M = static_cast<Index>(act.size());
    const Index D = static_cast<Index>(iss.size());
    if (M < 2) {
      throw std::invalid_argument("pform GMS: set act must have at least 2 parties.");
    }
    if (D < 1) {
      throw std::invalid_argument("pform GMS: set iss must have at least 1 issue.");
    }

    // 4. Shapes and domains.
    const Gms::GmsParameter& weight   = db.parameter("weight");
    const Gms::GmsParameter& position = db.parameter("position");
    const Gms::GmsParameter& salience = db.parameter("salience");
    const Gms::GmsParameter& usProb   = db.parameter("unselectedprob");
    if (1 != weight.data.rank()
        || weight.data.shape[0] != static_cast<std::size_t>(M)) {
      throw std::invalid_argument("pform GMS: weight must be declared weight(act).");
    }
    const auto shapeIssAct = [D, M](const Gms::GmsParameter& p) {
      return 2 == p.data.rank()
             && p.data.shape[0] == static_cast<std::size_t>(D)
             && p.data.shape[1] == static_cast<std::size_t>(M);
    };
    if (!shapeIssAct(position)) {
      throw std::invalid_argument("pform GMS: position must be declared position(iss, act).");
    }
    if (!shapeIssAct(salience)) {
      throw std::invalid_argument("pform GMS: salience must be declared salience(iss, act).");
    }
    if (0 != usProb.data.rank()) {
      throw std::invalid_argument("pform GMS: unselectedProb must be a scalar.");
    }

    // 5. Values.
    PformGmsInput input;
    input.partyLabels = act.labels;
    input.issueLabels = iss.labels;
    input.data.weight.resize(M);
    input.data.position.resize(D, M);
    input.data.salience.resize(D, M);

    for (Index m = 0; m < M; ++m) {
      const double w = weight.data.values[static_cast<std::size_t>(m)];
      if (!(0.0 < w)) {
        throw std::invalid_argument(
            "pform GMS: weight(" + act.labels[static_cast<std::size_t>(m)]
            + ") must be strictly positive.");
      }
      input.data.weight(m) = w;
    }

    // position / salience are dense row-major with shape {D, M}: flat d*M + m.
    for (Index d = 0; d < D; ++d) {
      for (Index m = 0; m < M; ++m) {
        const std::size_t flat =
            static_cast<std::size_t>(d) * static_cast<std::size_t>(M)
            + static_cast<std::size_t>(m);
        const double p = position.data.values[flat];
        if (p < 0.0 || 1.0 < p) {
          throw std::invalid_argument("pform GMS: positions must lie in [0, 1].");
        }
        const double s = salience.data.values[flat];
        if (s < 0.0) {
          throw std::invalid_argument("pform GMS: saliences must be non-negative.");
        }
        input.data.position(d, m) = p;
        input.data.salience(d, m) = s;
      }
    }
    for (Index m = 0; m < M; ++m) {
      if (input.data.salience.col(m).sum() < 1.0) {
        throw std::invalid_argument(
            "pform GMS: each party's total salience (sum over issues) must be >= 1.");
      }
    }

    // unselectedProb must lie in (0, (K-1)/K); pformParliamentCount validates K.
    const double q = usProb.data.values.at(0);
    const Index K = pformParliamentCount(M, D);
    const double qMax = (static_cast<double>(K) - 1.0) / static_cast<double>(K);
    if (!(0.0 < q && q < qMax)) {
      throw std::invalid_argument(
          "pform GMS: unselectedProb must satisfy 0 < unselectedProb < (K-1)/K.");
    }
    input.unselectedProb = q;

    return input;
  }

} // namespace VIMCP::App
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
