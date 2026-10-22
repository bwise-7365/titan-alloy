// Copyright Ben Paul Wise. All Rights Reserved.

#include "AbsGame.h"

namespace AbsGame {

// Default rollout policy: a uniform-random ("light") playout move over everything legal,
// kPass included. Games whose playouts need more than that override it -- Latrunculi for
// an aggression bias, IrrGo to refuse moves that fill its own eyes -- so in practice only
// Mancala, whose every legal move advances the game, still uses this.
MoveId Game::chooseRolloutMove(const std::vector<MoveId>& legal,
                               std::mt19937_64& rng) const {
    std::uniform_int_distribution<std::size_t> dist(0, legal.size() - 1);
    return legal[dist(rng)];
}

}  // namespace AbsGame

// Copyright Ben Paul Wise. All Rights Reserved.
