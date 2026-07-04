// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// NodeListWidget implementation: forward the normal list behaviour, then emit
// nodePressed / pressEnded so the owner can drive the map's node-info popup.
// ----------------------------------------------
#include "nodelistwidget.hpp"

#include <QListWidgetItem>
#include <QMouseEvent>

namespace VINCP::Network {

  NodeListWidget::NodeListWidget(QWidget* parent)
    : QListWidget(parent)
  {
    return;
  }

  void
  NodeListWidget::mousePressEvent(QMouseEvent* event)
  {
    QListWidget::mousePressEvent(event);
    const QListWidgetItem* item = itemAt(event->position().toPoint());
    if (nullptr != item) {
      emit nodePressed(item->data(Qt::UserRole).toInt());
    }
    return;
  }

  void
  NodeListWidget::mouseReleaseEvent(QMouseEvent* event)
  {
    QListWidget::mouseReleaseEvent(event);
    emit pressEnded();
    return;
  }

} // namespace VINCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
