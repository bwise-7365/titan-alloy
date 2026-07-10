// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// GmsDatabase implementation: array indexing and checked symbol lookups.
// ----------------------------------------------
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// software is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this software is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
// ----------------------------------------------
#include "gmsdatabase.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>

namespace VINCP::Gms {

  size_t
  GmsSet::ordinalOf(const string& label) const
  {
    string low = label;
    for (char& c : low) {
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    const auto found = ordinals.find(low);
    if (ordinals.end() == found) {
      throw std::invalid_argument("label '" + label + "' is not in set '"
                                  + name + "'");
    }
    return found->second;
  }

  GmsArray
  GmsArray::filled(const vector<size_t>& shape, double fill)
  {
    GmsArray array;
    array.shape = shape;
    size_t total = 1;
    for (const size_t extent : shape) {
      total *= extent;
    }
    array.values.assign(total, fill);
    return array;
  }

  size_t
  GmsArray::flatIndex(const vector<size_t>& ordinals) const
  {
    if (ordinals.size() != shape.size()) {
      std::ostringstream msg;
      msg << "array rank mismatch: " << shape.size() << " dimensions, "
          << ordinals.size() << " indices";
      throw std::invalid_argument(msg.str());
    }
    size_t flat = 0;
    for (size_t d = 0; d < shape.size(); ++d) {
      if (shape[d] <= ordinals[d]) {
        std::ostringstream msg;
        msg << "array index " << ordinals[d] << " out of range for extent "
            << shape[d] << " in dimension " << d;
        throw std::invalid_argument(msg.str());
      }
      flat = flat * shape[d] + ordinals[d];
    }
    return flat;
  }

  double
  GmsArray::at(const vector<size_t>& ordinals) const
  {
    return values[flatIndex(ordinals)];
  }

  double&
  GmsArray::at(const vector<size_t>& ordinals)
  {
    return values[flatIndex(ordinals)];
  }

  bool
  GmsDatabase::setP(const string& key) const
  {
    return 0 < sets.count(key) || 0 < aliases.count(key);
  }

  const GmsSet&
  GmsDatabase::resolveSet(const string& key) const
  {
    const auto alias = aliases.find(key);
    const string& baseKey = (aliases.end() != alias) ? alias->second : key;
    const auto found = sets.find(baseKey);
    if (sets.end() == found) {
      throw std::invalid_argument("'" + key + "' is not a set or alias");
    }
    return found->second;
  }

  const GmsParameter&
  GmsDatabase::parameter(const string& key) const
  {
    const auto found = parameters.find(key);
    if (parameters.end() == found) {
      throw std::invalid_argument("'" + key + "' is not a declared parameter");
    }
    return found->second;
  }

  const GmsVariable&
  GmsDatabase::variable(const string& key) const
  {
    const auto found = variables.find(key);
    if (variables.end() == found) {
      throw std::invalid_argument("'" + key + "' is not a declared variable");
    }
    return found->second;
  }

  const GmsEquation&
  GmsDatabase::equation(const string& key) const
  {
    const auto found = equations.find(key);
    if (equations.end() == found) {
      throw std::invalid_argument("'" + key + "' is not a declared equation");
    }
    return found->second;
  }

  const GmsModel&
  GmsDatabase::model(const string& key) const
  {
    const auto found = models.find(key);
    if (models.end() == found) {
      throw std::invalid_argument("'" + key + "' is not a declared model");
    }
    return found->second;
  }

} // namespace VINCP::Gms
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
