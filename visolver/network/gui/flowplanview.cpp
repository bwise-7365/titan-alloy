// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// FlowPlanView implementation: cached cheapest-neighbour ordering, a live
// pan/zoom world->screen transform, and the QPainter drawing of links, plan
// flow arrows, nodes, labels, a legend, and the node-info popup.
// ----------------------------------------------
#include "flowplanview.hpp"

#include <QFontMetricsF>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPolygonF>
#include <QRectF>
#include <QStringList>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <utility>

namespace VIMCP::Network {

  // Fill colours per node class (kept together so the legend and nodes agree).
  namespace {
    const QColor kSupplyColor(46, 125, 50);     // green: pure source
    const QColor kBothColor(21, 101, 192);      // blue: source and sink
    const QColor kDemandColor(198, 40, 40);     // red: pure sink
    const QColor kTransitColor(120, 120, 120);  // grey: transit
    const QColor kLinkColor(255, 140, 0, 170);  // orange: closest-links overlay
    const QColor kFlowColor(0, 140, 140, 170);  // teal: plan flow arrows
    const QColor kLabelBg(255, 255, 255, 200);  // pill behind node labels
    const QColor kPopupBg(255, 255, 236, 240);  // node-info / swap popup fill
    const QColor kSwapColor(200, 0, 200);       // magenta: swapped-arc highlight
    const QColor kBackground(250, 250, 248);

    const double kMarginPx = 40.0;   // border around the fitted map
    const double kMinNodeRadiusPx = 4.0;
    const double kMaxNodeRadiusPx = 16.0;
    const double kMinFlowWidthPx = 1.2;
    const double kMaxFlowWidthPx = 7.0;
    const double kZoomStep = 1.15;   // wheel notch zoom factor
    const double kHitSlackPx = 2.0;  // extra click radius around a node

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

    // Draw a directed arrow from p0 to p1, backed off each node's radius so it
    // runs edge-to-edge, with a filled arrowhead at the p1 end.
    void
    drawArrow(QPainter& painter, const QPointF& p0, const QPointF& p1,
              double r0, double r1, double widthPx, const QColor& color)
    {
      const double dx = p1.x() - p0.x();
      const double dy = p1.y() - p0.y();
      const double len = std::hypot(dx, dy);
      if (len <= r0 + r1 + 1.0) {
        return;   // endpoints overlap; nothing meaningful to draw
      }
      const double ux = dx / len;
      const double uy = dy / len;
      const QPointF start(p0.x() + ux * r0, p0.y() + uy * r0);
      const QPointF tip(p1.x() - ux * r1, p1.y() - uy * r1);

      QPen pen(color);
      pen.setWidthF(widthPx);
      pen.setCapStyle(Qt::RoundCap);
      painter.setPen(pen);
      painter.setBrush(Qt::NoBrush);
      painter.drawLine(start, tip);

      const double head = std::max(5.0, widthPx * 2.2);
      const double nx = -uy;   // unit normal
      const double ny = ux;
      const QPointF base(tip.x() - ux * head, tip.y() - uy * head);
      const QPointF left(base.x() + nx * head * 0.5, base.y() + ny * head * 0.5);
      const QPointF right(base.x() - nx * head * 0.5, base.y() - ny * head * 0.5);
      QPolygonF headPoly;
      headPoly << tip << left << right;
      painter.setPen(Qt::NoPen);
      painter.setBrush(color);
      painter.drawPolygon(headPoly);
      return;
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
    setContextMenuPolicy(Qt::PreventContextMenu);   // right-click is "swap"
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
    // Bit-identical placement => keep the current pan/zoom. The fleet viewer
    // swaps per-asset SLICES of one geometry through here, and refitting on
    // every asset scroll would yank the view around.
    const auto sameVectorP = [](const VectorXd& a, const VectorXd& b) {
      if (a.size() != b.size()) {
        return false;
      }
      return 0 == a.size() || 0.0 == (a - b).cwiseAbs().maxCoeff();
    };
    const bool samePlacementP = instance_.numNodes == inst.numNodes
                                && 0 != inst.xCoord.size()
                                && sameVectorP(instance_.xCoord, inst.xCoord)
                                && sameVectorP(instance_.yCoord, inst.yCoord);

    instance_ = inst;
    if (nearestK_ > static_cast<int>(instance_.numNodes) - 1) {
      nearestK_ = std::max(0, static_cast<int>(instance_.numNodes) - 1);
    }
    rebuildNeighbours();
    // A new instance invalidates any prior plan overlay and popup; the owner
    // re-applies the plan (e.g. recomputes greedy) if that mode is active.
    showPlanP_ = false;
    popupNode_ = -1;
    swapShownP_ = false;
    swapArcs_.clear();
    if (!samePlacementP) {
      fitPendingP_ = true;
    }
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
  FlowPlanView::setPlan(const Plan& plan)
  {
    plan_ = plan;
    showPlanP_ = plan_.flow.rows() == instance_.numNodes
                 && plan_.flow.cols() == instance_.numNodes;
    update();
    return;
  }

  void
  FlowPlanView::clearPlan()
  {
    showPlanP_ = false;
    update();
    return;
  }

  void
  FlowPlanView::setShowLabels(bool onP)
  {
    showLabelsP_ = onP;
    update();
    return;
  }

  void
  FlowPlanView::showNodeInfo(int node)
  {
    popupNode_ = (0 <= node && node < instance_.numNodes) ? node : -1;
    swapShownP_ = false;   // the two popups are mutually exclusive
    update();
    return;
  }

  void
  FlowPlanView::hideNodeInfo()
  {
    popupNode_ = -1;
    update();
    return;
  }

  void
  FlowPlanView::setNodeInfoProvider(std::function<QStringList(Index)> provider)
  {
    infoProvider_ = std::move(provider);
    return;
  }

  void
  FlowPlanView::showSwapResult(int anchorNode, const QStringList& lines,
                               const vector<std::pair<int, int>>& highlightArcs)
  {
    popupNode_ = -1;   // the two popups are mutually exclusive
    swapAnchor_ = (0 <= anchorNode && anchorNode < instance_.numNodes)
                      ? anchorNode
                      : -1;
    swapLines_ = lines;
    swapArcs_ = highlightArcs;
    swapShownP_ = !lines.isEmpty();
    update();
    return;
  }

  void
  FlowPlanView::clearSwapResult()
  {
    swapShownP_ = false;
    swapArcs_.clear();
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
    peakTons_ = 1.0;
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
    double peak = 0.0;
    for (Index i = 0; i < m; ++i) {
      worldMinX_ = std::min(worldMinX_, instance_.xCoord(i));
      worldMaxX_ = std::max(worldMaxX_, instance_.xCoord(i));
      worldMinY_ = std::min(worldMinY_, instance_.yCoord(i));
      worldMaxY_ = std::max(worldMaxY_, instance_.yCoord(i));
      peak = std::max(peak, std::max(instance_.supplyCap(i),
                                     instance_.demand(i)));
    }
    peakTons_ = (peak > 0.0) ? peak : 1.0;

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

  double
  FlowPlanView::nodeRadius(Index node) const
  {
    const double tons =
        std::max(instance_.supplyCap(node), instance_.demand(node));
    const double frac = std::sqrt(std::max(tons, 0.0) / peakTons_);
    return kMinNodeRadiusPx + frac * (kMaxNodeRadiusPx - kMinNodeRadiusPx);
  }

  Index
  FlowPlanView::nodeAt(const QPoint& pos) const
  {
    const Index m = instance_.numNodes;
    const bool placedP =
        instance_.xCoord.size() == m && instance_.yCoord.size() == m;
    if (!placedP) {
      return -1;
    }
    // Reverse order so the topmost (last-drawn) node wins on overlap.
    for (Index i = m - 1; 0 <= i; --i) {
      const QPointF p = toScreen(instance_.xCoord(i), instance_.yCoord(i));
      const double reach = nodeRadius(i) + kHitSlackPx;
      if (std::hypot(pos.x() - p.x(), pos.y() - p.y()) <= reach) {
        return i;
      }
    }
    return -1;
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
    const QPoint pos = event->position().toPoint();
    if (Qt::LeftButton == event->button()) {
      swapShownP_ = false;   // any left click dismisses a swap-result popup
      const Index hit = nodeAt(pos);
      if (0 <= hit) {
        showNodeInfo(static_cast<int>(hit));   // popup; do not pan (calls update)
      }
      else {
        panningP_ = true;
        lastPanPos_ = pos;
        setCursor(Qt::ClosedHandCursor);
        update();
      }
    }
    else if (Qt::RightButton == event->button()) {
      const Index hit = nodeAt(pos);
      if (0 <= hit) {
        // Shift = drive the node to its swap optimum; plain = one best swap.
        if (event->modifiers().testFlag(Qt::ShiftModifier)) {
          emit nodeSwapToOptimumRequested(static_cast<int>(hit));
        }
        else {
          emit nodeSwapRequested(static_cast<int>(hit));
        }
      }
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
      if (panningP_) {
        panningP_ = false;
        unsetCursor();
      }
      if (0 <= popupNode_) {
        hideNodeInfo();
      }
    }
    return;
  }

  QStringList
  FlowPlanView::nodeInfoLines(Index node) const
  {
    if (infoProvider_) {
      return infoProvider_(node);
    }
    return QStringList{
        QString::fromStdString(nodeLabel(instance_, node)),
        QString("C = %1").arg(instance_.supplyCap(node), 0, 'f', 1),
        QString("D = %1").arg(instance_.demand(node), 0, 'f', 1),
    };
  }

  void
  FlowPlanView::drawTextBox(QPainter& painter, const QStringList& lines,
                            Index anchorNode) const
  {
    if (lines.isEmpty()) {
      return;
    }
    const QFontMetricsF fm(painter.font());
    const double pad = 6.0;
    const double lineH = fm.height();
    double textW = 0.0;
    for (const QString& line : lines) {
      textW = std::max(textW, fm.horizontalAdvance(line));
    }
    const double boxW = textW + 2.0 * pad;
    const double boxH = lines.size() * lineH + 2.0 * pad;

    // Anchor up-right of the node (or the widget centre if none), then clamp so
    // the whole box stays visible.
    QPointF anchor(width() * 0.5, height() * 0.5);
    double anchorR = 0.0;
    if (0 <= anchorNode && anchorNode < instance_.numNodes) {
      anchor = toScreen(instance_.xCoord(anchorNode),
                        instance_.yCoord(anchorNode));
      anchorR = nodeRadius(anchorNode);
    }
    double bx = anchor.x() + anchorR + 6.0;
    double by = anchor.y() - boxH - 6.0;
    bx = std::clamp(bx, 2.0, std::max(2.0, width() - boxW - 2.0));
    by = std::clamp(by, 2.0, std::max(2.0, height() - boxH - 2.0));
    const QRectF box(bx, by, boxW, boxH);

    painter.setPen(QPen(QColor(90, 90, 90), 1.0));
    painter.setBrush(kPopupBg);
    painter.drawRoundedRect(box, 4.0, 4.0);

    painter.setPen(Qt::black);
    double ty = by + pad + fm.ascent();
    for (const QString& line : lines) {
      painter.drawText(QPointF(bx + pad, ty), line);
      ty += lineH;
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

    // Links first (under everything): each node to its k cheapest neighbours.
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

    // Plan flow arrows (over the links, under the nodes): directed f_ij, width
    // scaled by flow against the plan's peak off-diagonal flow. The diagonal
    // (self-supply f_ii) is not drawn -- it has no direction on the map.
    if (showPlanP_ && plan_.flow.rows() == m && plan_.flow.cols() == m) {
      double peakFlow = 0.0;
      for (Index i = 0; i < m; ++i) {
        for (Index j = 0; j < m; ++j) {
          if (i != j) {
            peakFlow = std::max(peakFlow, plan_.flow(i, j));
          }
        }
      }
      const double flowScale = (peakFlow > 0.0) ? peakFlow : 1.0;
      for (Index i = 0; i < m; ++i) {
        const QPointF pi = toScreen(instance_.xCoord(i), instance_.yCoord(i));
        for (Index j = 0; j < m; ++j) {
          if (i != j && plan_.flow(i, j) > 0.0) {
            const QPointF pj =
                toScreen(instance_.xCoord(j), instance_.yCoord(j));
            const double frac = plan_.flow(i, j) / flowScale;
            const double widthPx =
                kMinFlowWidthPx + frac * (kMaxFlowWidthPx - kMinFlowWidthPx);
            drawArrow(painter, pi, pj, nodeRadius(i), nodeRadius(j), widthPx,
                      kFlowColor);
          }
        }
      }
    }

    // Nodes.
    for (Index i = 0; i < m; ++i) {
      const QPointF p = toScreen(instance_.xCoord(i), instance_.yCoord(i));
      const double radius = nodeRadius(i);
      const QColor fill = colorFor(classifyNode(instance_, i));
      painter.setPen(QPen(fill.darker(140), 1.0));
      painter.setBrush(fill);
      painter.drawEllipse(p, radius, radius);
    }

    // Labels centred on the dots (over the nodes), on a faint pill for contrast.
    if (showLabelsP_) {
      const QFontMetricsF fm(painter.font());
      const double lineH = fm.height();
      for (Index i = 0; i < m; ++i) {
        const QPointF p = toScreen(instance_.xCoord(i), instance_.yCoord(i));
        const QString lbl = QString::fromStdString(nodeLabel(instance_, i));
        const double w = fm.horizontalAdvance(lbl) + 4.0;
        const QRectF pill(p.x() - w * 0.5, p.y() - lineH * 0.5, w, lineH);
        painter.setPen(Qt::NoPen);
        painter.setBrush(kLabelBg);
        painter.drawRoundedRect(pill, 3.0, 3.0);
        painter.setPen(Qt::black);
        painter.drawText(pill, Qt::AlignCenter, lbl);
      }
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

    // Swapped-arc highlight (magenta), above the flows/nodes.
    if (swapShownP_ && !swapArcs_.empty()) {
      QPen swapPen(kSwapColor);
      swapPen.setWidthF(2.5);
      painter.setPen(swapPen);
      for (const std::pair<int, int>& arc : swapArcs_) {
        if (arc.first != arc.second && 0 <= arc.first && arc.first < m
            && 0 <= arc.second && arc.second < m) {
          painter.drawLine(
              toScreen(instance_.xCoord(arc.first), instance_.yCoord(arc.first)),
              toScreen(instance_.xCoord(arc.second),
                       instance_.yCoord(arc.second)));
        }
      }
    }

    // Popups last, so they sit above everything (only one is ever shown).
    if (0 <= popupNode_ && popupNode_ < m) {
      drawTextBox(painter, nodeInfoLines(popupNode_), popupNode_);
    }
    else if (swapShownP_) {
      drawTextBox(painter, swapLines_, swapAnchor_);
    }
    return;
  }

} // namespace VIMCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
