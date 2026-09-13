// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexmodel/Position.h"

#include <algorithm>
#include <stdexcept>
#include <type_traits>

namespace HexModel {

  namespace {

    template <class T, class Index>
    T&
    atMut(std::vector<T>& v, Index i, const char* what)
    {
      if (i.value >= v.size()) {
        throw std::invalid_argument(std::string("Position: ") + what + " index out of range");
      }
      return v[i.value];
    }

    template <class T, class Index>
    const T&
    atConst(const std::vector<T>& v, Index i, const char* what)
    {
      if (i.value >= v.size()) {
        throw std::invalid_argument(std::string("Position: ") + what + " index out of range");
      }
      return v[i.value];
    }

    void
    eraseUnit(std::vector<UnitId>& stack, UnitId u)
    {
      const auto it = std::find(stack.begin(), stack.end(), u);
      if (stack.end() != it) {
        stack.erase(it);
      }
      return;
    }

    std::uint64_t
    fnv1a(std::string_view data)
    {
      std::uint64_t h = 14695981039346656037ull;
      constexpr std::uint64_t kPrime = 1099511628211ull;
      for (unsigned char c : data) {
        h ^= c;
        h *= kPrime;
      }
      return h;
    }

  }  // namespace

  void
  Position::removeFromStack(Location loc, UnitId u)
  {
    std::visit(
        [&](auto&& l) {
          using T = std::decay_t<decltype(l)>;
          if constexpr (std::is_same_v<T, HexIndex>) {
            eraseUnit(atMut(byHex_, l, "hex"), u);
          } else {
            eraseUnit(atMut(bySpace_, l, "space"), u);
          }
        },
        loc);
    return;
  }

  void
  Position::addToStack(Location loc, UnitId u)
  {
    std::visit(
        [&](auto&& l) {
          using T = std::decay_t<decltype(l)>;
          if constexpr (std::is_same_v<T, HexIndex>) {
            atMut(byHex_, l, "hex").push_back(u);
          } else {
            atMut(bySpace_, l, "space").push_back(u);
          }
        },
        loc);
    return;
  }

  const UnitState&
  Position::unit(UnitId id) const
  {
    if (id.value >= units_.size()) {
      throw std::invalid_argument("Position::unit: unit id out of range");
    }
    return units_[id.value];
  }

  const std::vector<UnitId>&
  Position::unitsAt(HexIndex h) const
  {
    return atConst(byHex_, h, "hex");
  }

  const std::vector<UnitId>&
  Position::unitsIn(SpaceId s) const
  {
    return atConst(bySpace_, s, "space");
  }

  void
  Position::place(UnitId id, Location loc)
  {
    if (id.value >= units_.size()) {
      throw std::invalid_argument("Position::place: unit id out of range");
    }
    UnitState& st = units_[id.value];
    if (st.where) {
      removeFromStack(*st.where, id);
    }
    addToStack(loc, id);
    st.where = loc;
    return;
  }

  void
  Position::remove(UnitId id)
  {
    if (id.value >= units_.size()) {
      throw std::invalid_argument("Position::remove: unit id out of range");
    }
    UnitState& st = units_[id.value];
    if (st.where) {
      removeFromStack(*st.where, id);
    }
    st.where = std::nullopt;
    return;
  }

  UnitState&
  Position::state(UnitId id)
  {
    if (id.value >= units_.size()) {
      throw std::invalid_argument("Position::state: unit id out of range");
    }
    return units_[id.value];
  }

  std::optional<SideId>
  Position::control(HexIndex h) const
  {
    if (h.value >= control_.size()) {
      throw std::invalid_argument("Position::control: hex index out of range");
    }
    return control_[h.value];
  }

  void
  Position::setControl(HexIndex h, std::optional<SideId> side)
  {
    if (h.value >= control_.size()) {
      throw std::invalid_argument("Position::setControl: hex index out of range");
    }
    control_[h.value] = side;
    return;
  }

  std::optional<SideId>
  Position::linkOwner(NetworkId n, std::size_t link) const
  {
    const std::vector<std::optional<SideId>>& owners = atConst(linkOwners_, n, "network");
    if (link >= owners.size()) {
      throw std::invalid_argument("Position::linkOwner: link index out of range");
    }
    return owners[link];
  }

  void
  Position::setLinkOwner(NetworkId n, std::size_t link, std::optional<SideId> side)
  {
    std::vector<std::optional<SideId>>& owners = atMut(linkOwners_, n, "network");
    if (link >= owners.size()) {
      throw std::invalid_argument("Position::setLinkOwner: link index out of range");
    }
    owners[link] = side;
    return;
  }

  const RegionState&
  Position::region(LayerId l, RegionId r) const
  {
    const std::vector<RegionState>& layer = atConst(regions_, l, "layer");
    if (r.value >= layer.size()) {
      throw std::invalid_argument("Position::region: region id out of range");
    }
    return layer[r.value];
  }

  RegionState&
  Position::region(LayerId l, RegionId r)
  {
    std::vector<RegionState>& layer = atMut(regions_, l, "layer");
    if (r.value >= layer.size()) {
      throw std::invalid_argument("Position::region: region id out of range");
    }
    return layer[r.value];
  }

  int
  Position::track(TrackId t) const
  {
    if (t.value >= tracks_.size()) {
      throw std::invalid_argument("Position::track: track id out of range");
    }
    return tracks_[t.value];
  }

  void
  Position::setTrack(TrackId t, int value)
  {
    if (t.value >= tracks_.size()) {
      throw std::invalid_argument("Position::setTrack: track id out of range");
    }
    tracks_[t.value] = value;
    return;
  }

  void
  Position::setPending(PendingDecision decision)
  {
    pending_ = std::move(decision);
    return;
  }

  std::uint64_t
  Position::digest() const
  {
    std::string s;
    s.reserve(4096);

    const auto appendI = [&s](long long v) {
      s += std::to_string(v);
      s += ';';
    };
    const auto appendS = [&s](std::string_view v) {
      s += v;
      s += ';';
    };
    const auto appendOptSide = [&](const std::optional<SideId>& side) {
      if (side) {
        appendI(side->value);
      } else {
        s += "-;";
      }
    };

    s += 'U';
    for (const UnitState& u : units_) {
      if (u.where) {
        std::visit(
            [&](auto&& loc) {
              using T = std::decay_t<decltype(loc)>;
              if constexpr (std::is_same_v<T, HexIndex>) {
                s += 'H';
                appendI(loc.value);
              } else {
                s += 'S';
                appendI(loc.value);
              }
            },
            *u.where);
      } else {
        s += "-;";
      }
      appendI(u.steps.current());
      appendI(u.steps.maximum());
      appendI(static_cast<int>(u.face));
      appendI(u.flags.movedP);
      appendI(u.flags.attackedP);
      appendI(u.flags.revealedP);
      appendI(u.flags.disruptedP);
      appendI(u.flags.isolatedP);
      if (u.delay) {
        appendI(*u.delay);
      } else {
        s += "-;";
      }
      for (const std::string& m : u.markers) {
        appendS(m);
      }
      s += '|';
    }

    s += 'C';
    for (const std::optional<SideId>& c : control_) {
      appendOptSide(c);
    }

    s += 'L';
    for (const std::vector<std::optional<SideId>>& net : linkOwners_) {
      for (const std::optional<SideId>& o : net) {
        appendOptSide(o);
      }
      s += '|';
    }

    s += 'R';
    for (const std::vector<RegionState>& layerStates : regions_) {
      for (const RegionState& r : layerStates) {
        appendS(r.status.value_or("-"));
        appendOptSide(r.alignment);
        appendS(r.posture.value_or("-"));
        appendOptSide(r.owner);
      }
      s += '|';
    }

    s += 'T';
    for (int t : tracks_) {
      appendI(t);
    }

    s += 'K';
    appendI(clock_.turn);
    appendI(clock_.phase.value);
    appendOptSide(clock_.actingSide);

    s += 'P';
    std::visit(
        [&](auto&& p) {
          using T = std::decay_t<decltype(p)>;
          if constexpr (std::is_same_v<T, NoDecision>) {
            s += "0;";
          } else if constexpr (std::is_same_v<T, ChooseLoss>) {
            s += "1;";
            for (UnitId id : p.candidates) {
              appendI(id.value);
            }
            appendI(p.count);
          } else if constexpr (std::is_same_v<T, ChooseRetreat>) {
            s += "2;";
            appendI(p.unit.value);
            for (HexIndex h : p.candidates) {
              appendI(h.value);
            }
          } else if constexpr (std::is_same_v<T, ChooseCard>) {
            s += "3;";
            appendI(p.deck.value);
            for (const std::string& c : p.candidates) {
              appendS(c);
            }
          } else if constexpr (std::is_same_v<T, GameChoice>) {
            s += "4;";
            appendS(p.verb);
            for (const std::string& o : p.options) {
              appendS(o);
            }
          }
        },
        pending_);

    return fnv1a(s);
  }

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
