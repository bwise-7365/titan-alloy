// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// A QListWidget whose items carry a node index (in Qt::UserRole). It emits
// nodePressed(node) while the left button is held over an item and pressEnded()
// on release, mirroring the map's press-and-hold node popup so a list click can
// drive the same popup.
// ----------------------------------------------
#ifndef VINCP_NETWORK_NODELISTWIDGET_HPP
#define VINCP_NETWORK_NODELISTWIDGET_HPP

#include <QListWidget>

class QMouseEvent;

namespace VINCP::Network {

  class NodeListWidget : public QListWidget
  {
    Q_OBJECT

  public:
    explicit NodeListWidget(QWidget* parent = nullptr);

  signals:
    void nodePressed(int node);
    void pressEnded();

  protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
  };

} // namespace VINCP::Network

#endif // VINCP_NETWORK_NODELISTWIDGET_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
