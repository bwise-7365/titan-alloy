// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Qt widget: a histogram of all N^2 movement costs c_ij, binned into K equal
// cost-range buckets [0, w), [w, 2w), ... over [0, maxCost]. K is set by the
// owner and the widget recomputes and repaints on change.
// ----------------------------------------------
#ifndef VINCP_NETWORK_COSTHISTOGRAM_HPP
#define VINCP_NETWORK_COSTHISTOGRAM_HPP

#include "instance.hpp"

#include <QWidget>
#include <vector>

class QPaintEvent;

using std::vector;

namespace VINCP::Network {

  class CostHistogram : public QWidget
  {
    Q_OBJECT

  public:
    explicit CostHistogram(QWidget* parent = nullptr);

    // Bin the cost matrix of this instance (all N^2 entries, diagonal included).
    void setInstance(const Instance& inst);

    // Number of equal-width bins over [0, maxCost]; clamped to [1, 25].
    void setBinCount(int k);

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    // Refill counts_ / maxCost_ from the current instance and bin count.
    void recompute();

    Instance instance_;
    int binCount_ = 10;
    vector<int> counts_;     // size binCount_; counts_[b] = #costs in bin b
    double maxCost_ = 1.0;   // upper edge of the last bin
  };

} // namespace VINCP::Network

#endif // VINCP_NETWORK_COSTHISTOGRAM_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
