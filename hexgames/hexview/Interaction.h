// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// [PROPOSED contract, M10] From pointer and keyboard intents to engine commands. EngineFacade is the
// only engine surface a view uses (hexqt calls it directly; the HTML client reaches the same calls
// over JSON). InteractionMachine emits a command only when the facade offers it -- a listed legal
// command, a path inside reachable(), a target inside attackTargets() -- so a view can never submit
// an illegal command; any other intent changes only the ViewState.
// ----------------------------------------------
#pragma once
#include "hexengine/Command.h"
#include "hexengine/Session.h"
#include "hexview/ViewState.h"

#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace HexView {

  // ---- intents -----------------------------------------------------------------------------------
  struct ClickHex { HexModel::HexIndex hex; };
  struct ClickUnit { HexModel::UnitId unit; bool addP = false; };  // addP: extend the selection
  struct DragTo { HexModel::HexIndex hex; };                       // extend the drafted path
  struct Confirm {};
  struct Cancel {};
  struct ChooseAnswer { std::string answer; };                     // for the pending decision
  struct RequestEndPhase {};
  struct IssueGameVerb { std::string verb; std::vector<std::string> args; };
  using Intent = std::variant<ClickHex, ClickUnit, DragTo, Confirm, Cancel, ChooseAnswer, RequestEndPhase, IssueGameVerb>;

  // ---- the engine as a view sees it --------------------------------------------------------------
  class EngineFacade {
  public:
    virtual ~EngineFacade() = default;
    virtual const HexRules::GameDefinition& definition() const = 0;
    virtual const HexModel::Position& position() const = 0;
    virtual HexEngine::Prompt prompt() const = 0;
    virtual std::vector<HexEngine::Command> legalCommands() const = 0;
    virtual HexEngine::Reachability reachable(std::span<const HexModel::UnitId>, HexModel::ModeId) const = 0;
    virtual std::vector<HexModel::HexIndex> attackTargets(std::span<const HexModel::UnitId>) const = 0;
    // Throws std::invalid_argument, naming the reason, for a command the engine refuses.
    virtual HexEngine::Applied apply(const HexEngine::Command&) = 0;
    virtual const HexEngine::EventLog& events() const = 0;
  };

  class SessionFacade final : public EngineFacade {
  public:
    explicit SessionFacade(HexEngine::Session&);
    const HexRules::GameDefinition& definition() const override;
    const HexModel::Position& position() const override;
    HexEngine::Prompt prompt() const override;
    std::vector<HexEngine::Command> legalCommands() const override;
    HexEngine::Reachability reachable(std::span<const HexModel::UnitId>, HexModel::ModeId) const override;
    std::vector<HexModel::HexIndex> attackTargets(std::span<const HexModel::UnitId>) const override;
    HexEngine::Applied apply(const HexEngine::Command&) override;
    const HexEngine::EventLog& events() const override;

  private:
    HexEngine::Session& session_;
  };

  // ---- the machine -------------------------------------------------------------------------------
  enum class InteractionMode : std::uint8_t { Idle, UnitsSelected, DraftingPath, DraftingAttack, AnsweringDecision, GameOver };

  struct Outcome {
    ViewState view;
    std::optional<HexEngine::Command> command;  // set only for a command the facade offers
    std::optional<std::string> refusal;         // why an intent did nothing, for the status line
  };

  class InteractionMachine {
  public:
    explicit InteractionMachine(const EngineFacade&);
    InteractionMode mode() const { return mode_; }
    // Pure in the facade: it asks, never applies. The caller applies `command` and feeds the next
    // intent with the returned view.
    Outcome feed(const Intent&, const ViewState&);

  private:
    const EngineFacade& engine_;
    InteractionMode mode_ = InteractionMode::Idle;
  };

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
