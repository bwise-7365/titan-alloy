// Copyright Ben Paul Wise. All Rights Reserved.

#include "AbsGame.h"

namespace AbsGame {

// Default rollout policy: a uniform-random ("light") playout move. Games that
// benefit from an informed playout override this; the ones that don't (irrgo,
// mancala) keep this behaviour unchanged.
MoveId Game::chooseRolloutMove(const std::vector<MoveId>& legal,
                               std::mt19937_64& rng) const {
    std::uniform_int_distribution<std::size_t> dist(0, legal.size() - 1);
    return legal[dist(rng)];
}

}  // namespace AbsGame

// Copyright Ben Paul Wise. All Rights Reserved.
