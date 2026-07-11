// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// CostHistogram implementation: equal-width binning of the N^2 cost entries and
// a simple bar-chart paint with 0 / maxCost / peak-count labels.
// ----------------------------------------------
#include "costhistogram.hpp"

#include <QPaintEvent>
#include <QPainter>

#include <algorithm>
#include <cmath>

namespace VIMCP::Network {

  namespace {
    const int kMinBins = 1;
    const int kMaxBins = 25;
    const QColor kBarColor(21, 101, 192);       // blue bars
    const QColor kAxisColor(140, 140, 140);
    const QColor kBackground(252, 252, 250);
  } // namespace

  CostHistogram::CostHistogram(QWidget* parent)
    : QWidget(parent)
  {
    setMinimumSize(200, 120);
    return;
  }

  void
  CostHistogram::setInstance(const Instance& inst)
  {
    instance_ = inst;
    recompute();
    update();
    return;
  }

  void
  CostHistogram::setBinCount(int k)
  {
    binCount_ = std::clamp(k, kMinBins, kMaxBins);
    recompute();
    update();
    return;
  }

  void
  CostHistogram::recompute()
  {
    counts_.assign(static_cast<size_t>(std::max(1, binCount_)), 0);
    maxCost_ = 1.0;
    const Index m = instance_.numNodes;
    if (0 == m || instance_.cost.rows() != m || instance_.cost.cols() != m) {
      return;
    }

    const double peak = instance_.cost.maxCoeff();
    if (!(peak > 0.0)) {
      return;
    }
    maxCost_ = peak;
    const double width = maxCost_ / binCount_;
    for (Index i = 0; i < m; ++i) {
      for (Index j = 0; j < m; ++j) {
        int idx = static_cast<int>(std::floor(instance_.cost(i, j) / width));
        // The maximum cost sits exactly on the top edge; keep it in the last
        // (closed) bin rather than spilling past it.
        idx = std::clamp(idx, 0, binCount_ - 1);
        counts_[static_cast<size_t>(idx)] += 1;
      }
    }
    return;
  }

  void
  CostHistogram::paintEvent(QPaintEvent* /*event*/)
  {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.fillRect(rect(), kBackground);

    const int bins = static_cast<int>(counts_.size());
    if (0 == bins || 0 == instance_.numNodes) {
      painter.setPen(Qt::darkGray);
      painter.drawText(rect(), Qt::AlignCenter, "No instance.");
      return;
    }

    int peakCount = 0;
    for (int c : counts_) {
      peakCount = std::max(peakCount, c);
    }
    const int scaleCount = (peakCount > 0) ? peakCount : 1;

    const double leftPad = 6.0;
    const double rightPad = 6.0;
    const double topPad = 16.0;   // room for the peak-count label
    const double botPad = 16.0;   // room for the x-axis labels
    const double plotW = std::max(width() - leftPad - rightPad, 1.0);
    const double plotH = std::max(height() - topPad - botPad, 1.0);
    const double baseline = topPad + plotH;
    const double slot = plotW / bins;

    // Bars.
    painter.setPen(Qt::NoPen);
    painter.setBrush(kBarColor);
    for (int b = 0; b < bins; ++b) {
      const double frac = counts_[static_cast<size_t>(b)]
                          / static_cast<double>(scaleCount);
      const double barH = frac * plotH;
      const double x = leftPad + b * slot;
      const QRectF bar(x + 0.5, baseline - barH,
                       std::max(slot - 1.0, 1.0), barH);
      painter.fillRect(bar, kBarColor);
    }

    // Baseline axis.
    painter.setPen(kAxisColor);
    painter.drawLine(QPointF(leftPad, baseline),
                     QPointF(leftPad + plotW, baseline));

    // Labels: peak count (top-left), 0 (bottom-left), maxCost (bottom-right).
    painter.setPen(Qt::black);
    painter.drawText(QPointF(2.0, 11.0),
                     QString("peak %1 / %2 links")
                         .arg(peakCount)
                         .arg(static_cast<int>(instance_.numNodes)
                              * static_cast<int>(instance_.numNodes)));
    painter.drawText(QPointF(leftPad, height() - 3.0), "0");
    const QString maxLabel = QString::number(maxCost_, 'g', 4);
    const double textW = painter.fontMetrics().horizontalAdvance(maxLabel);
    painter.drawText(QPointF(leftPad + plotW - textW, height() - 3.0), maxLabel);
    return;
  }

} // namespace VIMCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
