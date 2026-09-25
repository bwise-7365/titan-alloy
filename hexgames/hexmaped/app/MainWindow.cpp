// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "MainWindow.h"

#include "hexmaped/Schema.h"

#include <QAction>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QSpinBox>
#include <QStatusBar>
#include <QTextEdit>
#include <QToolBar>

#include <stdexcept>

namespace HexQt {

  namespace {

    QString
    q(const std::string& s)
    {
      return QString::fromStdString(s);
    }

    std::string
    firstHexOf(const QString& at)
    {
      // "1638", "1831-1931", "1028:ne": the first printed id in the token
      QString s = at;
      const int dash = s.indexOf('-');
      if (dash > 0) {
        s = s.left(dash);
      }
      const int colon = s.indexOf(':');
      if (colon > 0) {
        s = s.left(colon);
      }
      return s.toStdString();
    }

  }  // namespace

  MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      view_(new MapView(this)),
      modeBox_(new QComboBox(this)),
      kindBox_(new QComboBox(this)),
      slotBox_(new QComboBox(this)),
      linkKindBox_(new QComboBox(this)),
      wavinessBox_(new QSpinBox(this)),
      status_(new QLabel(this)),
      doubts_(new QListWidget(this)),
      inspector_(new QTextEdit(this))
  {
    setWindowTitle("HexMapEd");
    resize(1400, 900);
    setCentralWidget(view_);
    buildMenus();
    buildToolbar();
    buildDocks();
    statusBar()->addWidget(status_, 1);
    connect(view_, &MapView::hovered, this, &MainWindow::onHovered);
    connect(view_, &MapView::clicked, this, &MainWindow::onClicked);
    refreshActions();
  }

  void
  MainWindow::buildMenus()
  {
    QMenu* file = menuBar()->addMenu("&File");
    file->addAction("&Open sheet...", QKeySequence::Open, [this] {
      const QString path = QFileDialog::getOpenFileName(this, "Open sheet", QString(HEXGAMES_SOURCE_DIR) + "/map_graphics/xml", "Sheets (*.xml)");
      if (!path.isEmpty()) {
        open(path);
      }
    });
    saveAction_ = file->addAction("&Save", QKeySequence::Save, [this] { save(); });
    file->addAction("Save &as...", QKeySequence::SaveAs, [this] { saveAs(); });
    file->addAction("&Validate saved file", [this] { validateSaved(); });
    file->addAction("Open &doubts...", [this] {
      const QString path = QFileDialog::getOpenFileName(this, "Open doubts", QString(), "Doubts (*.json)");
      if (!path.isEmpty()) {
        loadDoubtsBeside(path);
      }
    });
    file->addSeparator();
    file->addAction("&Quit", QKeySequence::Quit, [this] { close(); });

    QMenu* edit = menuBar()->addMenu("&Edit");
    undoAction_ = edit->addAction("&Undo", QKeySequence::Undo, [this] {
      if (doc_) {
        doc_->undo();
        view_->rebuild();
        refreshActions();
      }
    });
    redoAction_ = edit->addAction("&Redo", QKeySequence::Redo, [this] {
      if (doc_) {
        doc_->redo();
        view_->rebuild();
        refreshActions();
      }
    });

    QMenu* viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction("Zoom &in", QKeySequence::ZoomIn, [this] { view_->zoomBy(1.25); });
    viewMenu->addAction("Zoom &out", QKeySequence::ZoomOut, [this] { view_->zoomBy(0.8); });
    viewMenu->addAction("&Fit", QKeySequence(Qt::Key_F), [this] { view_->fitAll(); });
    QAction* ids = viewMenu->addAction("Show hex &ids");
    ids->setCheckable(true);
    ids->setChecked(true);
    connect(ids, &QAction::toggled, view_, &MapView::setIdsVisible);
    return;
  }

  void
  MainWindow::buildToolbar()
  {
    QToolBar* bar = addToolBar("Mode");
    bar->addWidget(new QLabel(" Mode "));
    modeBox_->addItems({"Select (pan)", "Terrain", "Hexside line", "Link", "Clip", "Glyph", "Name"});
    bar->addWidget(modeBox_);
    bar->addWidget(new QLabel(" Kind "));
    bar->addWidget(kindBox_);
    bar->addWidget(new QLabel(" Slot "));
    bar->addWidget(slotBox_);
    bar->addWidget(new QLabel(" Link kind "));
    bar->addWidget(linkKindBox_);
    linkKindBox_->setEditable(true);
    bar->addSeparator();
    bar->addWidget(new QLabel(" Waviness "));
    wavinessBox_->setRange(0, MapView::kMaxWaviness);
    wavinessBox_->setValue(0);
    wavinessBox_->setToolTip("Hand-scratched roads, railways and rivers, as irrgo draws its boards: 0 is straight. Presentation only.");
    bar->addWidget(wavinessBox_);
    connect(wavinessBox_, &QSpinBox::valueChanged, view_, &MapView::setWaviness);
    connect(modeBox_, &QComboBox::currentIndexChanged, this, [this](int) {
      linkStart_.reset();
      view_->setPanningP(Mode::Select == mode());
      refreshKinds();
    });
    for (const std::string_view s : HexMapEd::Schema::slotList()) {
      slotBox_->addItem(QString::fromUtf8(s.data(), static_cast<int>(s.size())));
    }
    return;
  }

  void
  MainWindow::buildDocks()
  {
    QDockWidget* doubtsDock = new QDockWidget("Doubts", this);
    doubtsDock->setWidget(doubts_);
    addDockWidget(Qt::RightDockWidgetArea, doubtsDock);
    connect(doubts_, &QListWidget::itemClicked, this, &MainWindow::onDoubtChosen);
    QDockWidget* inspectorDock = new QDockWidget("Hex", this);
    inspector_->setReadOnly(true);
    inspectorDock->setWidget(inspector_);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
    return;
  }

  MainWindow::Mode
  MainWindow::mode() const
  {
    return static_cast<Mode>(modeBox_->currentIndex());
  }

  void
  MainWindow::refreshKinds()
  {
    kindBox_->clear();
    if (!doc_) {
      return;
    }
    std::vector<std::string> kinds;
    switch (mode()) {
      case Mode::Select:
      case Mode::Clip:
      case Mode::Name:
        break;
      case Mode::Terrain:
        kinds = doc_->terrainIds();
        break;
      case Mode::Hexside:
      case Mode::Link:
        kinds = doc_->lineIds();
        break;
      case Mode::Glyph:
        for (const std::string_view s : HexMapEd::Schema::symbolList()) {
          kinds.emplace_back(s);
        }
        break;
    }
    for (const std::string& k : kinds) {
      kindBox_->addItem(q(k));
    }
    linkKindBox_->clear();
    for (const std::string& k : doc_->linkKinds()) {
      linkKindBox_->addItem(q(k));
    }
    return;
  }

  void
  MainWindow::refreshActions()
  {
    const bool haveP = static_cast<bool>(doc_);
    saveAction_->setEnabled(haveP && doc_->dirtyP());
    undoAction_->setEnabled(haveP && doc_->canUndoP());
    redoAction_->setEnabled(haveP && doc_->canRedoP());
    QString title = "HexMapEd";
    if (haveP) {
      title += " - " + QFileInfo(q(doc_->path().string())).fileName() + (doc_->dirtyP() ? " *" : "");
    }
    setWindowTitle(title);
    return;
  }

  void
  MainWindow::open(const QString& path)
  {
    try {
      doc_ = std::make_unique<HexMapEd::Document>(HexMapEd::Document::load(path.toStdString()));
    } catch (const std::exception& e) {
      QMessageBox::critical(this, "Cannot open", e.what());
      return;
    }
    view_->setDocument(doc_.get());
    refreshKinds();
    refreshActions();
    loadDoubtsBeside(path);
    return;
  }

  void
  MainWindow::loadDoubtsBeside(const QString& path)
  {
    doubts_->clear();
    QString doubtsPath = path;
    if (!path.endsWith(".json")) {
      doubtsPath = QFileInfo(path).absolutePath() + "/doubts.json";
    }
    QFile f(doubtsPath);
    if (!f.open(QIODevice::ReadOnly)) {
      return;
    }
    const QJsonDocument json = QJsonDocument::fromJson(f.readAll());
    for (const QJsonValue& v : json.array()) {
      const QJsonObject o = v.toObject();
      QListWidgetItem* item = new QListWidgetItem(
        QString("%1  %2  %3").arg(o.value("kind").toString(), o.value("at").toString(), o.value("why").toString()));
      item->setData(Qt::UserRole, o.value("at").toString());
      doubts_->addItem(item);
    }
    return;
  }

  void
  MainWindow::onDoubtChosen(QListWidgetItem* item)
  {
    if (!doc_) {
      return;
    }
    const std::string hex = firstHexOf(item->data(Qt::UserRole).toString());
    view_->centreOnHex(hex);
    view_->highlight(hex);
    describe(hex);
    return;
  }

  void
  MainWindow::describe(const std::string& hex)
  {
    if (!doc_ || !doc_->frame().printsP(hex)) {
      inspector_->clear();
      return;
    }
    QString text = "hex " + q(hex) + "\nterrain " + q(doc_->terrainOf(hex)) + "\n";
    const HexXml::SheetHexDoc* h = doc_->hexElement(hex);
    if (nullptr != h) {
      if (h->name.has_value()) {
        text += "name " + q(*h->name) + "\n";
      }
      if (h->ring.has_value()) {
        text += "ring " + q(*h->ring) + "\n";
      }
      for (const HexXml::SheetGlyphDoc& g : h->glyphs) {
        text += "glyph " + q(g.symbol.value_or("mark " + g.mark.value_or("?"))) + " at " + q(g.slot) + "\n";
      }
    }
    for (const std::string& e : doc_->edgeTokensAt(hex)) {
      text += "edge " + q(e) + "\n";
    }
    for (const HexXml::SheetLinkDoc& l : doc_->sheet().links) {
      if (std::find(l.hexes.begin(), l.hexes.end(), hex) != l.hexes.end()) {
        text += "link " + q(l.kind) + " ";
        for (const std::string& id : l.hexes) {
          text += q(id) + " ";
        }
        text += "\n";
      }
    }
    inspector_->setPlainText(text);
    return;
  }

  void
  MainWindow::onHovered(QPointF p)
  {
    if (!doc_) {
      return;
    }
    const std::optional<std::string> hex = doc_->frame().hexAt(HexMapEd::Pixel{p.x(), p.y()});
    if (hex.has_value()) {
      status_->setText(QString("%1  %2   (%3, %4)").arg(q(*hex), q(doc_->terrainOf(*hex))).arg(p.x(), 0, 'f', 0).arg(p.y(), 0, 'f', 0));
    } else {
      status_->setText(QString("(%1, %2)").arg(p.x(), 0, 'f', 0).arg(p.y(), 0, 'f', 0));
    }
    return;
  }

  void
  MainWindow::applyEdit(const std::function<void()>& edit)
  {
    try {
      edit();
    } catch (const std::invalid_argument& e) {
      status_->setText(QString("refused: ") + e.what());
      return;
    }
    view_->rebuild();
    refreshActions();
    return;
  }

  void
  MainWindow::onClicked(QPointF p, Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
  {
    if (!doc_) {
      return;
    }
    const HexMapEd::Pixel at{p.x(), p.y()};
    const std::optional<std::string> hex = doc_->frame().hexAt(at);
    const std::string kind = kindBox_->currentText().toStdString();
    const bool shiftP = modifiers.testFlag(Qt::ShiftModifier);
    if (hex.has_value()) {
      view_->highlight(hex);
      describe(*hex);
    }
    switch (mode()) {
      case Mode::Select:
        break;
      case Mode::Terrain:
        if (hex.has_value() && !kind.empty()) {
          applyEdit([&] { doc_->setTerrain(*hex, kind); });
        }
        break;
      case Mode::Hexside: {
        const double size = doc_->frame().grids().front().spec().size;
        const std::optional<HexMapEd::Hexside> side = doc_->frame().hexsideAt(at, 0.35 * size);
        if (side.has_value() && !kind.empty()) {
          applyEdit([&] { doc_->toggleEdge(*side, kind); });
        }
        break;
      }
      case Mode::Link:
        if (!hex.has_value()) {
          break;
        }
        if (!linkStart_.has_value()) {
          linkStart_ = hex;
          status_->setText("link: from " + q(*hex) + ", now click the next hex (shift-click removes the step)");
          break;
        }
        if (*linkStart_ == *hex) {
          linkStart_.reset();
          break;
        }
        {
          const std::string from = *linkStart_;
          const std::string linkKind = linkKindBox_->currentText().toStdString();
          if (shiftP) {
            applyEdit([&] { doc_->removeLinkStep(from, *hex); });
          } else {
            applyEdit([&] { doc_->addLinkStep(from, *hex, linkKind.empty() ? "rail" : linkKind, kind); });
          }
          linkStart_ = hex;  // the chain continues from here
        }
        break;
      case Mode::Clip:
        // a printed hex leaves the grid; an empty cell inside or just outside the grid joins it
        if (hex.has_value()) {
          applyEdit([&] { doc_->toggleClip(*hex); });
        } else {
          applyEdit([&] {
            const std::string added = doc_->addHexAt(at);
            status_->setText("added hex " + q(added));
          });
        }
        break;
      case Mode::Glyph:
        if (hex.has_value() && !kind.empty()) {
          if (shiftP) {
            const HexXml::SheetHexDoc* h = doc_->hexElement(*hex);
            if (nullptr != h && !h->glyphs.empty()) {
              applyEdit([&] { doc_->removeGlyph(*hex, h->glyphs.size() - 1); });
            }
          } else {
            applyEdit([&] { doc_->addGlyph(*hex, kind, slotBox_->currentText().toStdString(), std::nullopt); });
          }
        }
        break;
      case Mode::Name:
        if (hex.has_value()) {
          const HexXml::SheetHexDoc* h = doc_->hexElement(*hex);
          const QString current = (nullptr != h && h->name.has_value()) ? q(*h->name) : QString();
          bool okP = false;
          const QString name = QInputDialog::getText(this, "Name", "Name of " + q(*hex) + " (empty removes it)", QLineEdit::Normal, current, &okP);
          if (okP) {
            applyEdit([&] { doc_->setName(*hex, name.isEmpty() ? std::nullopt : std::optional<std::string>(name.toStdString())); });
          }
        }
        break;
    }
    (void)button;
    return;
  }

  void
  MainWindow::save()
  {
    if (!doc_) {
      return;
    }
    try {
      doc_->save();
    } catch (const std::exception& e) {
      QMessageBox::critical(this, "Cannot save", e.what());
      return;
    }
    refreshActions();
    validateSaved();
    return;
  }

  void
  MainWindow::saveAs()
  {
    if (!doc_) {
      return;
    }
    const QString path = QFileDialog::getSaveFileName(this, "Save sheet as", q(doc_->path().string()), "Sheets (*.xml)");
    if (path.isEmpty()) {
      return;
    }
    try {
      doc_->saveAs(path.toStdString());
    } catch (const std::exception& e) {
      QMessageBox::critical(this, "Cannot save", e.what());
      return;
    }
    refreshActions();
    validateSaved();
    return;
  }

  void
  MainWindow::validateSaved()
  {
    if (!doc_ || doc_->path().empty()) {
      return;
    }
    QProcess proc;
    QString python = QString(HEXGAMES_PYTHON);
    if (python.isEmpty()) {
      python = "python";
    }
    // validate-xml.py takes directories: every sheet beside the saved one is checked with it
    proc.start(python, {QString(HEXGAMES_SOURCE_DIR) + "/tools/validate-xml.py", q(doc_->path().parent_path().string())});
    if (!proc.waitForFinished(30000)) {
      status_->setText("validate-xml.py did not finish");
      return;
    }
    const QString out = QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError()).trimmed();
    if (0 == proc.exitCode()) {
      status_->setText("saved and valid: " + out);
    } else {
      QMessageBox::warning(this, "Saved file does not validate", out);
    }
    return;
  }

  void
  MainWindow::closeEvent(QCloseEvent* event)
  {
    if (doc_ && doc_->dirtyP()) {
      const auto answer = QMessageBox::question(this, "Unsaved changes", "Save before closing?",
                                                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
      if (QMessageBox::Cancel == answer) {
        event->ignore();
        return;
      }
      if (QMessageBox::Save == answer) {
        save();
      }
    }
    event->accept();
    return;
  }

}  // namespace HexQt
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
