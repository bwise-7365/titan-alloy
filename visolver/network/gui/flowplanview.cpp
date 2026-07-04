// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// FlowPlanView implementation: cached cheapest-neighbour ordering, a live
// pan/zoom world->screen transform, and the QPainter drawing of links, nodes,
// and a small legend.
// ----------------------------------------------
#include "flowplanview.hpp"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace VINCP::Network {

  // Fill colours per node class (kept together so the legend and nodes agree).
  namespace {
    const QColor kSupplyColor(46, 125, 50);     // green: pure source
    const QColor kBothColor(21, 101, 192);      // blue: source and sink
    const QColor kDemandColor(198, 40, 40);     // red: pure sink
    const QColor kTransitColor(120, 120, 120);  // grey: transit
    const QColor kLinkColor(255, 140, 0, 170);  // orange: closest-links overlay
    const QColor kBackground(250, 250, 248);

    const double kMarginPx = 40.0;   // border around the fitted map
    const double kMinNodeRadiusPx = 4.0;
    const double kMaxNodeRadiusPx = 16.0;
    const double kZoomStep = 1.15;   // wheel notch zoom factor

    QColor
    colorFor(NodeClass cls)
    {
      switch (cls) {
        case NodeClass::SupplyOnly:
          return kSupplyColor;
        case NodeClass::Both:
          return kBothColor;
        case NodeClass::DemandOnly:
          return kDemandColor;
        case NodeClass::Transit:
          return kTransitColor;
      }
      return kTransitColor;
    }
  } // namespace

  NodeClass
  classifyNode(const Instance& inst, Index node)
  {
    const bool suppliesP = 0.0 < inst.supplyCap(node);
    const bool demandsP = 0.0 < inst.demand(node);
    if (suppliesP && demandsP) {
      return NodeClass::Both;
    }
    if (suppliesP) {
      return NodeClass::SupplyOnly;
    }
    if (demandsP) {
      return NodeClass::DemandOnly;
    }
    return NodeClass::Transit;
  }

  FlowPlanView::FlowPlanView(QWidget* parent)
    : QWidget(parent)
  {
    setMinimumSize(480, 360);
    return;
  }

  Index
  FlowPlanView::nodeCount() const
  {
    return instance_.numNodes;
  }

  void
  FlowPlanView::setInstance(const Instance& inst)
  {
    instance_ = inst;
    if (nearestK_ > static_cast<int>(instance_.numNodes) - 1) {
      nearestK_ = std::max(0, static_cast<int>(instance_.numNodes) - 1);
    }
    rebuildNeighbours();
    fitPendingP_ = true;
    update();
    return;
  }

  void
  FlowPlanView::setNearestK(int k)
  {
    const int hi = std::max(0, static_cast<int>(instance_.numNodes) - 1);
    nearestK_ = std::clamp(k, 0, hi);
    update();
    return;
  }

  void
  FlowPlanView::recenter()
  {
    fitPendingP_ = true;
    update();
    return;
  }

  void
  FlowPlanView::rebuildNeighbours()
  {
    neighboursByCost_.clear();
    const Index m = instance_.numNodes;
    const bool placedP =
        instance_.xCoord.size() == m && instance_.yCoord.size() == m;
    if (0 == m || !placedP) {
      worldMinX_ = worldMinY_ = 0.0;
      worldMaxX_ = worldMaxY_ = 1.0;
      return;
    }

    worldMinX_ = worldMaxX_ = instance_.xCoord(0);
    worldMinY_ = worldMaxY_ = instance_.yCoord(0);
    for (Index i = 1; i < m; ++i) {
      worldMinX_ = std::min(worldMinX_, instance_.xCoord(i));
      worldMaxX_ = std::max(worldMaxX_, instance_.xCoord(i));
      worldMinY_ = std::min(worldMinY_, instance_.yCoord(i));
      worldMaxY_ = std::max(worldMaxY_, instance_.yCoord(i));
    }

    // Rank each node's neighbours by the symmetrised cost (c_ij + c_ji)/2, so
    // the slight directional asymmetry of the cost matrix does not bias which
    // pair is "closest".
    neighboursByCost_.resize(m);
    for (Index i = 0; i < m; ++i) {
      vector<int>& order = neighboursByCost_[i];
      order.reserve(static_cast<size_t>(m - 1));
      for (Index j = 0; j < m; ++j) {
        if (i != j) {
          order.push_back(static_cast<int>(j));
        }
      }
      std::sort(order.begin(), order.end(), [&](int a, int b) {
        const double ca =
            0.5 * (instance_.cost(i, a) + instance_.cost(a, i));
        const double cb =
            0.5 * (instance_.cost(i, b) + instance_.cost(b, i));
        return ca < cb;
      });
    }
    return;
  }

  void
  FlowPlanView::computeFit()
  {
    const double spanX = std::max(worldMaxX_ - worldMinX_, 1.0e-9);
    const double spanY = std::max(worldMaxY_ - worldMinY_, 1.0e-9);
    const double usableW = std::max(width() - 2.0 * kMarginPx, 1.0);
    const double usableH = std::max(height() - 2.0 * kMarginPx, 1.0);
    viewScale_ = std::min(usableW / spanX, usableH / spanY);
    viewOffsetX_ = kMarginPx + 0.5 * (usableW - viewScale_ * spanX);
    viewOffsetY_ = kMarginPx + 0.5 * (usableH - viewScale_ * spanY);
    return;
  }

  QPointF
  FlowPlanView::toScreen(double worldX, double worldY) const
  {
    const double sx = viewOffsetX_ + viewScale_ * (worldX - worldMinX_);
    // Flip y so larger world-y is higher on screen (screen y grows downward).
    const double sy = viewOffsetY_ + viewScale_ * (worldMaxY_ - worldY);
    return QPointF(sx, sy);
  }

  void
  FlowPlanView::wheelEvent(QWheelEvent* event)
  {
    const double factor =
        (event->angleDelta().y() > 0) ? kZoomStep : 1.0 / kZoomStep;
    const QPointF focus = event->position();
    // Keep the world point under the cursor fixed: offset' = f - factor*(f - offset).
    viewOffsetX_ = focus.x() - factor * (focus.x() - viewOffsetX_);
    viewOffsetY_ = focus.y() - factor * (focus.y() - viewOffsetY_);
    viewScale_ *= factor;
    update();
    event->accept();
    return;
  }

  void
  FlowPlanView::mousePressEvent(QMouseEvent* event)
  {
    if (Qt::LeftButton == event->button()) {
      panningP_ = true;
      lastPanPos_ = event->position().toPoint();
      setCursor(Qt::ClosedHandCursor);
    }
    return;
  }

  void
  FlowPlanView::mouseMoveEvent(QMouseEvent* event)
  {
    if (panningP_) {
      const QPoint here = event->position().toPoint();
      const QPoint delta = here - lastPanPos_;
      viewOffsetX_ += delta.x();
      viewOffsetY_ += delta.y();
      lastPanPos_ = here;
      update();
    }
    return;
  }

  void
  FlowPlanView::mouseReleaseEvent(QMouseEvent* event)
  {
    if (Qt::LeftButton == event->button()) {
      panningP_ = false;
      unsetCursor();
    }
    return;
  }

  void
  FlowPlanView::paintEvent(QPaintEvent* /*event*/)
  {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), kBackground);

    const Index m = instance_.numNodes;
    const bool placedP =
        instance_.xCoord.size() == m && instance_.yCoord.size() == m;
    if (0 == m || !placedP) {
      painter.setPen(Qt::darkGray);
      painter.drawText(rect(), Qt::AlignCenter,
                       "No placed instance to display.");
      return;
    }

    if (fitPendingP_) {
      computeFit();
      fitPendingP_ = false;
    }

    // Node radius scales with tonnage max(C_i, D_i) against the instance peak.
    double peakTons = 0.0;
    for (Index i = 0; i < m; ++i) {
      peakTons = std::max(peakTons,
                          std::max(instance_.supplyCap(i), instance_.demand(i)));
    }
    const double tonScale = (peakTons > 0.0) ? peakTons : 1.0;

    // Links first (under the nodes): each node to its k cheapest neighbours.
    if (nearestK_ > 0) {
      QPen linkPen(kLinkColor);
      linkPen.setWidthF(1.4);
      painter.setPen(linkPen);
      for (Index i = 0; i < m; ++i) {
        const QPointF pi = toScreen(instance_.xCoord(i), instance_.yCoord(i));
        const vector<int>& order = neighboursByCost_[i];
        const int kk = std::min<int>(nearestK_, static_cast<int>(order.size()));
        for (int r = 0; r < kk; ++r) {
          const int j = order[static_cast<size_t>(r)];
          const QPointF pj =
              toScreen(instance_.xCoord(j), instance_.yCoord(j));
          painter.drawLine(pi, pj);
        }
      }
    }

    // Nodes.
    for (Index i = 0; i < m; ++i) {
      const QPointF p = toScreen(instance_.xCoord(i), instance_.yCoord(i));
      const double tons = std::max(instance_.supplyCap(i), instance_.demand(i));
      const double frac = std::sqrt(std::max(tons, 0.0) / tonScale);
      const double radius =
          kMinNodeRadiusPx + frac * (kMaxNodeRadiusPx - kMinNodeRadiusPx);
      const QColor fill = colorFor(classifyNode(instance_, i));
      painter.setPen(QPen(fill.darker(140), 1.0));
      painter.setBrush(fill);
      painter.drawEllipse(p, radius, radius);
    }

    // Legend, top-left.
    struct LegendRow
    {
      NodeClass cls;
      const char* label;
    };
    const LegendRow rows[] = {
        {NodeClass::SupplyOnly, "supply only (C>0, D=0)"},
        {NodeClass::Both, "supply + demand"},
        {NodeClass::DemandOnly, "demand only (C=0, D>0)"},
        {NodeClass::Transit, "transit (C=0, D=0)"},
    };
    const int swatch = 12;
    int y = 12;
    painter.setPen(Qt::black);
    for (const LegendRow& row : rows) {
      painter.setBrush(colorFor(row.cls));
      painter.setPen(QPen(colorFor(row.cls).darker(140), 1.0));
      painter.drawRect(12, y, swatch, swatch);
      painter.setPen(Qt::black);
      painter.drawText(12 + swatch + 6, y + swatch - 2, row.label);
      y += swatch + 6;
    }
    return;
  }

} // namespace VINCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
