// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Qt widget that draws a flow-planning Instance as a map: nodes placed at their
// generated coordinates, coloured by class, sized by tonnage. Supports mouse
// pan (drag) and wheel zoom, with recenter() restoring the fitted view, and an
// optional "closest links" overlay (each node to its k cheapest neighbours).
// ----------------------------------------------
#ifndef VINCP_NETWORK_FLOWPLANVIEW_HPP
#define VINCP_NETWORK_FLOWPLANVIEW_HPP

#include "instance.hpp"

#include <QPoint>
#include <QPointF>
#include <QWidget>
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

  // Read-only viewer: the owner sets an Instance and a closest-neighbour count,
  // and the widget repaints. It holds its own copy of the Instance (instances
  // are small and cheaply regenerated) so the owner may rebuild freely.
  class FlowPlanView : public QWidget
  {
    Q_OBJECT

  public:
    explicit FlowPlanView(QWidget* parent = nullptr);

    // Replace the displayed instance (must carry coordinates), rebuild the
    // cached neighbour ordering, and refit the view. Clamps nearestK.
    void setInstance(const Instance& inst);

    // Show, from each node, orange links to its k CHEAPEST neighbours by the
    // symmetrised cost (c_ij + c_ji)/2. k is clamped to [0, numNodes - 1].
    void setNearestK(int k);

    Index nodeCount() const;

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
    // Rebuild neighboursByCost_ from the current instance cost matrix.
    void rebuildNeighbours();

    // Fit and centre the world bounding box in the current widget rect,
    // resetting any pan/zoom. Applied on setInstance and recenter().
    void computeFit();

    // Map a world (miles) point to a widget pixel using the live transform.
    QPointF toScreen(double worldX, double worldY) const;

    Instance instance_;
    int nearestK_ = 0;

    // neighboursByCost_[i] = other node indices, ascending by (c_ij + c_ji)/2.
    vector<vector<int>> neighboursByCost_;

    // World bounding box (miles).
    double worldMinX_ = 0.0, worldMinY_ = 0.0;
    double worldMaxX_ = 1.0, worldMaxY_ = 1.0;

    // Live world->screen transform: screen = offset + scale * localCoord, with
    // localCoord = (worldX - worldMinX_) for x and (worldMaxY_ - worldY) for y
    // (the y flip). Pan shifts the offsets; wheel zoom scales about the cursor.
    double viewScale_ = 1.0, viewOffsetX_ = 0.0, viewOffsetY_ = 0.0;
    bool fitPendingP_ = true;   // recompute the fit on the next paint

    // Pan (left-drag) state.
    bool panningP_ = false;
    QPoint lastPanPos_;
  };

} // namespace VINCP::Network

#endif // VINCP_NETWORK_FLOWPLANVIEW_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
