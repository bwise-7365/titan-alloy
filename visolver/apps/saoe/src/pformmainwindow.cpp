// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// pform GUI stub implementation: a placeholder central label. The interactive
// SAOE editor / result view is specified and built later.
// ----------------------------------------------
#include "pformmainwindow.hpp"

#include <QLabel>

namespace VINCP::App {

  PformMainWindow::PformMainWindow(QWidget* parent)
    : QMainWindow(parent)
  {
    setWindowTitle("pform -- SAOE (placeholder)");
    QLabel* const placeholder = new QLabel(
        "pform: the SAOE graphical app is not yet implemented.\n"
        "The pmatrix CLI is the working front end for now.", this);
    placeholder->setAlignment(Qt::AlignCenter);
    setCentralWidget(placeholder);
    return;
  }

} // namespace VINCP::App
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
