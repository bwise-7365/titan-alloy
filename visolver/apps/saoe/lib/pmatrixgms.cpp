// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// pmatrix GMS reader implementation: build the vimcpgms database, verify the
// file defines exactly the SAOE symbols with sensible values, extract SaoeData.
// ----------------------------------------------
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// software is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this software is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
// ----------------------------------------------
#include "pmatrixgms.hpp"

#include "gmsdatabase.hpp"
#include "gmseval.hpp"
#include "gmsparser.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <stdexcept>

namespace VIMCP::App {

  namespace {

    // The keys (lower-cased symbol names) of a database map, sorted.
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

    // Require that 'actual' is exactly 'expected' (both are sorted key lists);
    // otherwise throw naming what is missing and what is unexpected.
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
        string msg = "pmatrix GMS: the " + string(kind)
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

  PmatrixInput
  readPmatrixGms(const string& path)
  {
    // Parse and evaluate through the unchanged vimcpgms front end. A malformed
    // file throws std::invalid_argument with the parser's own file:line:col;
    // a semantic error (unknown label, shape clash) throws from the evaluator.
    const Gms::Program program = Gms::parseGmsFile(path);
    const Gms::GmsDatabase db  = Gms::buildGmsDatabase(program);

    // 1. No model-level constructs -- this is a pure data file.
    if (!db.variables.empty()) {
      throw std::invalid_argument(
          "pmatrix GMS: no variables permitted (found: "
          + joinKeys(sortedKeys(db.variables)) + ").");
    }
    if (!db.equations.empty()) {
      throw std::invalid_argument(
          "pmatrix GMS: no equations permitted (found: "
          + joinKeys(sortedKeys(db.equations)) + ").");
    }
    if (!db.models.empty()) {
      throw std::invalid_argument(
          "pmatrix GMS: no models permitted (found: "
          + joinKeys(sortedKeys(db.models)) + ").");
    }
    if (!db.aliases.empty()) {
      throw std::invalid_argument(
          "pmatrix GMS: no aliases permitted (found: "
          + joinKeys(sortedKeys(db.aliases)) + ").");
    }
    if (!db.solves.empty()) {
      throw std::invalid_argument("pmatrix GMS: no Solve statements permitted.");
    }
    if (!db.options.empty()) {
      throw std::invalid_argument("pmatrix GMS: no Option statements permitted.");
    }

    // 2. Exactly the expected sets and parameters, nothing else.
    requireExactSymbols(sortedKeys(db.sets), { "act", "opt" }, "set");
    requireExactSymbols(sortedKeys(db.parameters), { "rafrac", "reward", "weight" },
                        "parameter");

    // 3. The sets (dimensions and display labels).
    const Gms::GmsSet& act = db.resolveSet("act");
    const Gms::GmsSet& opt = db.resolveSet("opt");
    const Index M = static_cast<Index>(act.size());
    const Index N = static_cast<Index>(opt.size());
    if (M <= 0 || N <= 0) {
      throw std::invalid_argument(
          "pmatrix GMS: sets act and opt must each have at least one member.");
    }

    // 4. Shapes and domains.
    const Gms::GmsParameter& weight = db.parameter("weight");
    const Gms::GmsParameter& reward = db.parameter("reward");
    const Gms::GmsParameter& rafrac = db.parameter("rafrac");
    if (1 != weight.data.rank()
        || weight.data.shape[0] != static_cast<std::size_t>(M)) {
      throw std::invalid_argument(
          "pmatrix GMS: weight must be declared weight(act).");
    }
    if (2 != reward.data.rank()
        || reward.data.shape[0] != static_cast<std::size_t>(M)
        || reward.data.shape[1] != static_cast<std::size_t>(N)) {
      throw std::invalid_argument(
          "pmatrix GMS: reward must be declared reward(act, opt).");
    }
    if (0 != rafrac.data.rank()) {
      throw std::invalid_argument("pmatrix GMS: raFrac must be a scalar.");
    }

    // 5. Values.
    const double raFrac = rafrac.data.values.at(0);
    if (!(0.0 <= raFrac && raFrac <= 1.0)) {
      throw std::invalid_argument(
          "pmatrix GMS: raFrac must lie in [0, 1].");
    }

    PmatrixInput input;
    input.raFrac       = raFrac;
    input.actorLabels  = act.labels;
    input.optionLabels = opt.labels;

    input.S.resize(M);
    for (Index i = 0; i < M; ++i) {
      const double w = weight.data.values[static_cast<std::size_t>(i)];
      if (!(0.0 < w)) {
        throw std::invalid_argument(
            "pmatrix GMS: weight(" + act.labels[static_cast<std::size_t>(i)]
            + ") must be strictly positive.");
      }
      input.S(i) = w;
    }

    // reward is dense row-major with shape {M, N}: flat index i*N + j.
    input.R.resize(M, N);
    for (Index i = 0; i < M; ++i) {
      for (Index j = 0; j < N; ++j) {
        input.R(i, j) =
            reward.data.values[static_cast<std::size_t>(i) * static_cast<std::size_t>(N)
                               + static_cast<std::size_t>(j)];
      }
    }

    return input;
  }

} // namespace VIMCP::App
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
