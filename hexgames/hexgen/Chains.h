// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Steps between neighbouring hexes as chains, the form a sheet's links take (HexKrieg
// HexSheet.roadChains): a chain starts at every hex where the network does not simply pass through
// (degree other than 2) and runs until it reaches another such hex; the steps left over form closed
// loops, each a chain that returns to its start. Starts are taken in hex order, and each hex's steps
// in neighbour order, so the chains are the same for the same steps.
// ----------------------------------------------
#pragma once
#include <set>
#include <utility>
#include <vector>

namespace HexGen {

  std::vector<std::vector<int>> chainsOf(const std::set<std::pair<int, int>>& steps, int hexCount);

}  // namespace HexGen
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
