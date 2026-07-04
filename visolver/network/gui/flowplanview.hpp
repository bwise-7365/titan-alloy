// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Qt widget that draws a flow-planning Instance as a map: nodes placed at their
// generated coordinates, coloured by class, sized by tonnage. Supports mouse
// pan (drag) and wheel zoom, recenter(), an optional "closest links" overlay,
// an optional directed-flow (plan) overlay, optional node labels, a node-info
// popup on left-press, and a right-press "swap this node" request whose result
// (swapped arcs + savings) is shown in a popup.
// ----------------------------------------------
#ifndef VINCP_NETWORK_FLOWPLANVIEW_HPP
#define VINCP_NETWORK_FLOWPLANVIEW_HPP

#include "instance.hpp"
#include "plan.hpp"

#include <QPoint>
#include <QPointF>
#include <QStringList>
#include <QWidget>

#include <utility>
#include <vector>

class QWheelEvent;
class QMouseEvent;
class QPaintEvent;

using std::vector;

namespace VINCP::Network {

  // Node classes, derived from (C_i, D_i); drives the fill colour and legend.
  enum class NodeClass
  {
    SupplyOnly,   // C > 0, D = 0
    Both,         // C > 0, D > 0
    DemandOnly,   // C = 0, D > 0
    Transit       // C = 0, D = 0 (pure transshipment / pass-through)
  };

  NodeClass classifyNode(const Instance& inst, Index node);

  // Read-only viewer: the owner sets an Instance and overlays, and the widget
  // repaints. It holds its own copy of the Instance (instances are small and
  // cheaply regenerated) so the owner may rebuild freely.
  class FlowPlanView : public QWidget
  {
    Q_OBJECT

  public:
    explicit FlowPlanView(QWidget* parent = nullptr);

    // Replace the displayed instance (must carry coordinates), rebuild caches,
    // clear any popup, and refit the view. Clamps nearestK.
    void setInstance(const Instance& inst);

    // Show, from each node, orange links to its k CHEAPEST neighbours by the
    // symmetrised cost (c_ij + c_ji)/2. k is clamped to [0, numNodes - 1].
    void setNearestK(int k);

    // Overlay a plan's directed flows f_ij as teal arrows (width ~ flow), or
    // hide them. The plan must match the current instance's node count.
    void setPlan(const Plan& plan);
    void clearPlan();

    // Draw each node's label centred on its dot (default off).
    void setShowLabels(bool onP);

    // Show / hide the node-info popup (name, C, D). Driven by a map left-press
    // or a list press; -1 or hideNodeInfo() dismisses it.
    void showNodeInfo(int node);
    void hideNodeInfo();

    // Show a swap-result popup (the given text lines) anchored near anchorNode
    // (centred if anchorNode < 0), highlighting the given arcs. Persistent until
    // cleared, a left-click, a new instance, or a mode change.
    void showSwapResult(int anchorNode, const QStringList& lines,
                        const vector<std::pair<int, int>>& highlightArcs);
    void clearSwapResult();

    Index nodeCount() const;

  signals:
    // A right-press landed on this node -- the owner computes and applies its
    // best local swap.
    void nodeSwapRequested(int node);

    // A Shift+right-press landed on this node -- drive it to its swap optimum.
    void nodeSwapToOptimumRequested(int node);

  public slots:
    // Restore the centred, fitted view (undo pan/zoom).
    void recenter();

  protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

  private:
    // Rebuild neighboursByCost_ and peakTons_ from the current instance.
    void rebuildNeighbours();

    // Fit and centre the world bounding box in the current widget rect,
    // resetting pan/zoom. Applied on setInstance and recenter().
    void computeFit();

    // Map a world (miles) point to a widget pixel using the live transform.
    QPointF toScreen(double worldX, double worldY) const;

    // On-screen radius of node i (tonnage-scaled). Shared by paint + hit test.
    double nodeRadius(Index node) const;

    // Index of the topmost node under a widget pixel, or -1 if none.
    Index nodeAt(const QPoint& pos) const;

    // The node-info lines (name, C, D) for a node.
    QStringList nodeInfoLines(Index node) const;

    // Draw a bordered text box of `lines` anchored near anchorNode (centred if
    // anchorNode < 0). Shared by the node-info and swap-result popups.
    void drawTextBox(QPainter& painter, const QStringList& lines,
                     Index anchorNode) const;

    Instance instance_;
    int nearestK_ = 0;

    // neighboursByCost_[i] = other node indices, ascending by (c_ij + c_ji)/2.
    vector<vector<int>> neighboursByCost_;

    // Optional directed-flow overlay (a greedy or solver plan).
    Plan plan_;
    bool showPlanP_ = false;

    bool showLabelsP_ = false;
    Index popupNode_ = -1;      // node whose info popup is shown, or -1
    double peakTons_ = 1.0;     // max(C_i, D_i) over the instance (radius scale)

    // Swap-result popup state.
    bool swapShownP_ = false;
    Index swapAnchor_ = -1;
    QStringList swapLines_;
    vector<std::pair<int, int>> swapArcs_;   // arcs (a,b) to highlight

    // World bounding box (miles).
    double worldMinX_ = 0.0, worldMinY_ = 0.0;
    double worldMaxX_ = 1.0, worldMaxY_ = 1.0;

    // Live world->screen transform: screen = offset + scale * localCoord, with
    // localCoord = (worldX - worldMinX_) for x and (worldMaxY_ - worldY) for y
    // (the y flip). Pan shifts the offsets; wheel zoom scales about the cursor.
    double viewScale_ = 1.0, viewOffsetX_ = 0.0, viewOffsetY_ = 0.0;
    bool fitPendingP_ = true;   // recompute the fit on the next paint

    // Pan (left-drag on empty space) state.
    bool panningP_ = false;
    QPoint lastPanPos_;
  };

} // namespace VINCP::Network

#endif // VINCP_NETWORK_FLOWPLANVIEW_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
