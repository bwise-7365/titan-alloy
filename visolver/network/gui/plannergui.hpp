// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// PlannerGui: the skeleton shared by the two planner viewers -- the map view,
// instance controls, plan-mode radios, busy bar, histogram, node rankings,
// and status line that the network and fleet main windows have in common,
// plus the behavior that is identical in both (seed rerolling, regenerate
// bookkeeping, the closest-links overlay, the busy bar, the flow lists).
// A derived window adds its model-specific controls through the assembly
// points and drives the skeleton through the virtual hooks.
// ----------------------------------------------
#ifndef VIMCP_NETWORK_PLANNERGUI_HPP
#define VIMCP_NETWORK_PLANNERGUI_HPP

#include "costhistogram.hpp"
#include "flowplanview.hpp"
#include "instance.hpp"
#include "nodelistwidget.hpp"
#include "plan.hpp"

#include <QMainWindow>
#include <QString>

#include <cstdint>
#include <string>
#include <vector>

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QFormLayout;
class QLabel;
class QProgressBar;
class QRadioButton;
class QSpinBox;
class QTimer;
class QVBoxLayout;

namespace VIMCP::Network {

  // "label" or "#node" when the labels do not cover it -- the display form
  // used by the node rankings and popups.
  QString nodeText(const std::vector<std::string>& labels, Index node);

  class PlannerGui : public QMainWindow
  {
    Q_OBJECT

  public:

  protected:
    // The texts that differ between the planners; everything else about the
    // shared skeleton is identical.
    struct Texts {
      QString windowTitle;
      QString nearestLinksTip;    // tooltip of the closest-links spinner
      QString histogramTitle;     // "Cost histogram" / "Distance histogram"
      QString histogramBinsTip;
      QString throughputHeader;   // header of the left node-ranking column
    };

    explicit PlannerGui(const Texts& texts, QWidget* parent = nullptr);

    // --- hooks the shared skeleton drives ---
    // Build a fresh instance from the control values: validate the profile,
    // generate, store, invalidate the plan caches, and rescope any derived
    // controls. Throws on an invalid profile; regenerate() reports it.
    virtual void rebuildInstance(std::uint64_t seed) = 0;
    // Node count of the current instance (rescopes the links spinner).
    virtual Index nodeCount() const = 0;
    // Push the current instance into the map view and histogram.
    virtual void refreshMap() = 0;
    // Apply the checked plan-mode radio to the current instance.
    virtual void applyPlanMode() = 0;
    // Set the status line from the current instance plus the given mode note.
    virtual void refreshStatus(const QString& modeNote) = 0;

    // --- shared behavior ---
    // Reroll the seed if it reads 0, rebuild the instance, stamp out stale
    // background results, rescope the links spinner, and re-apply the mode.
    void regenerate();
    // Push the closest-links spin value into the view (no regeneration).
    void applyNearestK();
    // Start / stop the busy bar. Operations of unknowable length cannot
    // report real progress, so the bar refills over and over while any of
    // them runs; a counter handles overlap.
    void startBusy();
    void stopBusy();
    // Fill both node lists from a plan and enable them / clear and grey them.
    void showFlowLists(const Plan& plan,
                       const std::vector<std::string>& labels);
    void hideFlowLists();
    // The node-class spinners and laydown as an InstanceProfile.
    InstanceProfile geometryProfile() const;

    // --- assembly points for the derived constructor ---
    void addInstanceRow(const QString& label, QWidget* field);  // above Regenerate
    void addInstanceRow(QWidget* row);
    void addPlanRadio(QRadioButton* radio);   // below Closest; joins the group
    void addSwapControl(QWidget* widget);
    void addRightPanelTop(QWidget* widget);   // above the node rankings

    // --- shared widgets the derived windows read ---
    FlowPlanView* view_ = nullptr;
    QComboBox* laydownBox_ = nullptr;
    QSpinBox* nearestBox_ = nullptr;
    QRadioButton* closestRadio_ = nullptr;
    CostHistogram* histogram_ = nullptr;
    QLabel* statusLabel_ = nullptr;

    // --- shared state ---
    std::uint64_t lastSeed_ = 0;    // the seed that produced the instance
    bool haveInstanceP_ = false;
    int solveToken_ = 0;   // bumped per instance; stamps background launches

  private:
    QSpinBox* seedBox_ = nullptr;
    QSpinBox* supplyOnlyBox_ = nullptr;
    QSpinBox* bothBox_ = nullptr;
    QSpinBox* demandOnlyBox_ = nullptr;
    QSpinBox* transitBox_ = nullptr;
    QCheckBox* labelsCheck_ = nullptr;
    QSpinBox* histBinsBox_ = nullptr;
    NodeListWidget* throughputList_ = nullptr;
    NodeListWidget* countList_ = nullptr;
    QProgressBar* busyBar_ = nullptr;
    QTimer* busyTimer_ = nullptr;
    int busyCount_ = 0;   // running background operations (bar shows > 0)
    QButtonGroup* linkGroup_ = nullptr;
    QFormLayout* instanceForm_ = nullptr;
    QVBoxLayout* modeLayout_ = nullptr;
    QVBoxLayout* swapLayout_ = nullptr;
    QVBoxLayout* rightLayout_ = nullptr;
  };

} // namespace VIMCP::Network

#endif // VIMCP_NETWORK_PLANNERGUI_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
