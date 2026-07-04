// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Main window for the instance viewer: a FlowPlanView plus a control panel to
// pick the laydown, seed, node-class counts, and the nearest-neighbour count.
// ----------------------------------------------
#ifndef VINCP_NETWORK_MAINWINDOW_HPP
#define VINCP_NETWORK_MAINWINDOW_HPP

#include "flowplanview.hpp"

#include <QMainWindow>

class QComboBox;
class QSpinBox;
class QLabel;

namespace VINCP::Network {

  class MainWindow : public QMainWindow
  {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget* parent = nullptr);

  private slots:
    // Rebuild the instance from the current control values and refresh the view.
    void regenerate();
    // Push the nearest-neighbour spin value into the view (no regeneration).
    void applyNearestK();

  private:
    // Assemble an InstanceProfile from the control values.
    InstanceProfile currentProfile() const;

    FlowPlanView* view_ = nullptr;

    QComboBox* laydownBox_ = nullptr;
    QSpinBox* seedBox_ = nullptr;
    QSpinBox* supplyOnlyBox_ = nullptr;
    QSpinBox* bothBox_ = nullptr;
    QSpinBox* demandOnlyBox_ = nullptr;
    QSpinBox* transitBox_ = nullptr;
    QSpinBox* nearestBox_ = nullptr;
    QLabel* statusLabel_ = nullptr;
  };

} // namespace VINCP::Network

#endif // VINCP_NETWORK_MAINWINDOW_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
