// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "Replay.h"

#include <stdexcept>
#include <string>

namespace HexView {

  ReplayController::ReplayController(std::shared_ptr<const HexRules::GameDefinition> definition, const HexEngine::Policies& policies,
                                     const HexRecord::Record& record, std::size_t cacheEvery, const OnApplied& onApplied)
    : record_(record), cacheEvery_(cacheEvery)
  {
    if (0 == cacheEvery) {
      throw std::invalid_argument("ReplayController: cacheEvery must be positive");
    }
    HexEngine::Session session(std::move(definition), policies, record.position, record.seed);
    cache_.emplace(0, session.fork());
    for (std::size_t i = 0; i < record.log.size(); ++i) {
      try {
        const HexEngine::Applied applied = session.apply(record.log[i].command);
        eventRanges_.emplace_back(applied.firstEvent, applied.eventCount);
        if (onApplied) {
          onApplied(session, record.log[i].command, applied);
        }
      } catch (const std::invalid_argument& refusal) {
        throw std::invalid_argument("the record does not replay at move " + std::to_string(i + 1) + ": " + refusal.what());
      }
      if (0 == (i + 1) % cacheEvery_) {
        cache_.emplace(i + 1, session.fork());
      }
    }
    for (const HexEngine::Event& event : session.events().events()) {
      log_.append(event);
    }
    return;
  }

  std::size_t
  ReplayController::moveCount() const
  {
    return record_.log.size();
  }

  const HexModel::Position&
  ReplayController::at(std::size_t move)
  {
    if (moveCount() < move) {
      throw std::invalid_argument("ReplayController: no move " + std::to_string(move) + " in a record of " +
                                  std::to_string(moveCount()));
    }
    // Forward from where the working session stands if that is on the way, else from the nearest copy behind.
    const std::size_t cached = move - move % cacheEvery_;
    if (!working_ || move < workingAt_ || workingAt_ < cached) {
      working_.emplace(cache_.at(cached).fork());
      workingAt_ = cached;
    }
    while (workingAt_ < move) {
      working_->apply(record_.log[workingAt_].command);
      workingAt_ += 1;
    }
    return working_->position();
  }

  std::span<const HexEngine::Event>
  ReplayController::eventsOf(std::size_t move) const
  {
    if (0 == move || moveCount() < move) {
      throw std::invalid_argument("ReplayController: no move " + std::to_string(move) + " in a record of " +
                                  std::to_string(moveCount()));
    }
    const auto& [first, count] = eventRanges_[move - 1];
    return std::span<const HexEngine::Event>(log_.events()).subspan(first, count);
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
