// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "MapView.h"

#include "ScenePainter.h"
#include "hexview/MapFrame.h"
#include "hexview/MapSceneBuilder.h"
#include "hexview/Scene.h"

#include <QBrush>
#include <QGraphicsPolygonItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPen>
#include <QWheelEvent>

#include <stdexcept>

namespace HexQt {

  namespace {

    QPolygonF
    polygonOf(const std::array<HexMapEd::Pixel, 6>& corners)
    {
      QPolygonF poly;
      for (const HexMapEd::Pixel& p : corners) {
        poly << QPointF(p.x, p.y);
      }
      return poly;
    }

  }  // namespace

  MapView::MapView(QWidget* parent)
    : QGraphicsView(parent), scene_(new QGraphicsScene(this)), symbols_(HexView::SymbolLibrary::reference())
  {
    setScene(scene_);
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::TextAntialiasing, true);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setMouseTracking(true);
    setBackgroundBrush(QColor("#777777"));
  }

  void
  MapView::setDocument(const HexMapEd::Document* doc)
  {
    doc_ = doc;
    rebuild();
    if (nullptr != doc_) {
      fitAll();
    }
    return;
  }

  void
  MapView::setPanningP(bool panningP)
  {
    panningP_ = panningP;
    setDragMode(panningP ? QGraphicsView::ScrollHandDrag : QGraphicsView::NoDrag);
    return;
  }

  void
  MapView::setIdsVisible(bool visible)
  {
    idsVisibleP_ = visible;
    rebuild();
    return;
  }

  void
  MapView::setWaviness(int level)
  {
    if (level < 0 || kMaxWaviness < level) {
      throw std::invalid_argument("MapView::setWaviness: level out of range");
    }
    waviness_ = level;
    rebuild();
    return;
  }

  void
  MapView::zoomBy(double factor)
  {
    scale(factor, factor);
    return;
  }

  void
  MapView::fitAll()
  {
    if (nullptr == doc_) {
      return;
    }
    fitInView(QRectF(0.0, 0.0, doc_->frame().width(), doc_->frame().height()), Qt::KeepAspectRatio);
    return;
  }

  void
  MapView::centreOnHex(const std::string& id)
  {
    if (nullptr == doc_ || !doc_->frame().printsP(id)) {
      return;
    }
    const HexMapEd::Pixel c = doc_->frame().centre(id);
    centerOn(QPointF(c.x, c.y));
    return;
  }

  void
  MapView::highlight(const std::optional<std::string>& id)
  {
    if (nullptr != highlight_) {
      scene_->removeItem(highlight_);
      delete highlight_;
      highlight_ = nullptr;
    }
    if (nullptr == doc_ || !id.has_value() || !doc_->frame().printsP(*id)) {
      return;
    }
    QGraphicsPolygonItem* item = scene_->addPolygon(polygonOf(doc_->frame().corners(*id, 0.04)),
                                                    QPen(QColor(235, 0, 170), 3.0), Qt::NoBrush);
    item->setZValue(100.0);
    highlight_ = item;
    return;
  }

  // The sheet drawn exactly as hexsheet2svg.py and hexview's SVG writer draw it, plus the editor's
  // two presentation choices. Waviness maps the spinner to irrgo's roughness (10 = raw deviation of
  // half a hex before relaxation) at irrgo's default smoothing of 0.95.
  void
  MapView::rebuild()
  {
    scene_->clear();
    highlight_ = nullptr;
    if (nullptr == doc_) {
      return;
    }
    const HexXml::SheetDoc& sheet = doc_->sheet();
    const HexView::MapFrame frame = HexView::MapFrame::of(sheet);
    HexView::MapStyle style = HexView::MapStyle::reference();
    if (0 < waviness_) {
      style.scratch = HexView::ScratchStyle{static_cast<double>(waviness_) / kMaxWaviness, 0.95};
    }
    HexView::Scene scene(frame.width(), frame.height());
    HexView::MapSceneBuilder(frame, symbols_, style).build(sheet, scene);
    scene_->setSceneRect(0.0, 0.0, frame.width(), frame.height());
    paintScene(scene, symbols_, *scene_, PaintOptions{idsVisibleP_});
    return;
  }

  void
  MapView::wheelEvent(QWheelEvent* event)
  {
    const double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    scale(factor, factor);
    event->accept();
    return;
  }

  void
  MapView::mouseMoveEvent(QMouseEvent* event)
  {
    emit hovered(mapToScene(event->pos()));
    QGraphicsView::mouseMoveEvent(event);
    return;
  }

  void
  MapView::mousePressEvent(QMouseEvent* event)
  {
    if (!panningP_ || Qt::LeftButton != event->button()) {
      emit clicked(mapToScene(event->pos()), event->button(), event->modifiers());
    }
    QGraphicsView::mousePressEvent(event);
    return;
  }

}  // namespace HexQt
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
