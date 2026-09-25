// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#pragma once
// MapView -- shows a Document as hexview draws it: the sheet becomes a HexView::Scene through
// MapSceneBuilder (the same builder the SVG writer and, later, the game GUIs use) and ScenePainter
// puts that Scene into the QGraphicsScene. The view pans and zooms and reports where the mouse is in
// sheet pixels; it decides nothing, the window turns positions into edits. Presentation choices the
// editor adds on top: the "show ids" toggle and the hand-scratched waviness of roads, railways and
// rivers (HexView::MapStyle::scratch); neither touches the document.

#include "hexmaped/Document.h"
#include "hexview/SymbolLibrary.h"

#include <QGraphicsView>
#include <QString>

#include <optional>
#include <string>

namespace HexQt {

  class MapView final : public QGraphicsView {
    Q_OBJECT
  public:
    explicit MapView(QWidget* parent = nullptr);

    void setDocument(const HexMapEd::Document*);  // nullptr clears
    void rebuild();                                // after any edit
    void centreOnHex(const std::string& id);
    void highlight(const std::optional<std::string>& id);
    void setIdsVisible(bool);
    void zoomBy(double factor);
    void fitAll();
    void setPanningP(bool);  // Select mode pans with the left button; edit modes click
    // Presentation only: roads, railways and rivers drawn hand-scratched (hexview/Scratch.h), 0 = straight.
    static constexpr int kMaxWaviness = 10;
    void setWaviness(int level);

  signals:
    void hovered(QPointF sheetPixel);
    void clicked(QPointF sheetPixel, Qt::MouseButton button, Qt::KeyboardModifiers modifiers);

  protected:
    void wheelEvent(QWheelEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;

  private:
    const HexMapEd::Document* doc_ = nullptr;
    QGraphicsScene* scene_;
    QGraphicsItem* highlight_ = nullptr;
    HexView::SymbolLibrary symbols_;
    bool idsVisibleP_ = true;
    bool panningP_ = true;
    int waviness_ = 0;
  };

}  // namespace HexQt
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
