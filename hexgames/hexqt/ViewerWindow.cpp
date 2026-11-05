// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "ViewerWindow.h"

#include <QAction>
#include <QKeySequence>
#include <QMenu>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>

#include "MapView.h"

#include <exception>
#include <utility>

namespace HexQt {

  namespace {

    QString
    q(const std::string& s)
    {
      return QString::fromStdString(s);
    }

  }  // namespace

  ViewerWindow::ViewerWindow(QWidget* parent)
    : QMainWindow(parent),
      map_(new MapView(this)),
      slider_(new QSlider(Qt::Horizontal, this)),
      status_(new QLabel(this)),
      fingerprint_(new QLabel(this)),
      events_(new QPlainTextEdit(this))
  {
    setWindowTitle("pgg_trc_viewer");
    resize(1400, 900);

    QAction* open = menuBar()->addMenu("&File")->addAction("&Open...");
    connect(open, &QAction::triggered, this, [this] { chooseFile(); });
    reveal_ = menuBar()->addMenu("&View")->addAction("&Show untried strengths");
    reveal_->setCheckable(true);
    connect(reveal_, &QAction::toggled, this, [this] { showMove(slider_->value()); });
    QAction* fit = reveal_->parentWidget()->addAction("&Fit the map to the window");
    fit->setShortcut(QKeySequence("Ctrl+0"));
    connect(fit, &QAction::triggered, this, [this] { map_->fitAll(); });

    events_->setReadOnly(true);
    QSplitter* split = new QSplitter(this);
    split->addWidget(map_);
    split->addWidget(events_);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 0);
    split->setSizes({1100, 300});

    slider_->setTickPosition(QSlider::TicksBelow);
    slider_->setEnabled(false);
    connect(slider_, &QSlider::valueChanged, this, [this](int move) { showMove(move); });
    QPushButton* turnBack = new QPushButton("<< Turn", this);
    QPushButton* back = new QPushButton("<", this);
    QPushButton* forward = new QPushButton(">", this);
    QPushButton* turnForward = new QPushButton("Turn >>", this);
    connect(turnBack, &QPushButton::clicked, this, [this] { stepTurn(-1); });
    connect(back, &QPushButton::clicked, this, [this] { slider_->setValue(slider_->value() - 1); });
    connect(forward, &QPushButton::clicked, this, [this] { slider_->setValue(slider_->value() + 1); });
    connect(turnForward, &QPushButton::clicked, this, [this] { stepTurn(1); });
    QHBoxLayout* steps = new QHBoxLayout;
    steps->addWidget(turnBack);
    steps->addWidget(back);
    steps->addWidget(slider_, 1);
    steps->addWidget(forward);
    steps->addWidget(turnForward);

    status_->setWordWrap(true);
    fingerprint_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QWidget* central = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(central);
    layout->addWidget(split, 1);
    layout->addWidget(status_);
    layout->addWidget(fingerprint_);
    layout->addLayout(steps);
    setCentralWidget(central);

    warning_ = [this](const std::string& title, const std::string& text) { QMessageBox::warning(this, q(title), q(text)); };
    connect(&watcher_, &QFutureWatcher<Loaded>::finished, this, [this] { finished(); });
    status_->setText("File > Open: a hexsave script, or a run file from doc/pgg-runs.");
  }

  ViewerWindow::~ViewerWindow()
  {
    watcher_.waitForFinished();
  }

  bool
  ViewerWindow::loadingP() const
  {
    return watcher_.isRunning();
  }

  void
  ViewerWindow::setWarning(Warning warning)
  {
    warning_ = std::move(warning);
    return;
  }

  void
  ViewerWindow::chooseFile()
  {
    const QString path = QFileDialog::getOpenFileName(this, "Open a game", QString(HEXGAMES_SOURCE_DIR) + "/doc/pgg-runs",
                                                      "Runs and scripts (*.txt *.xml)");
    if (!path.isEmpty()) {
      openFile(path);
    }
    return;
  }

  void
  ViewerWindow::openFile(const QString& path)
  {
    if (!path.endsWith(".txt")) {
      openScript(path);
      return;
    }
    try {
      const RunFile run = readRunFile(path.toStdString());
      if (run.lines.empty()) {
        warning_("No games", path.toStdString() + " lists no finished game.");
        return;
      }
      QStringList items;
      for (const RunLine& line : run.lines) {
        items << q(line.text);
      }
      bool chosenP = false;
      const QString item = QInputDialog::getItem(this, "Choose a game", "Game:", items, 0, false, &chosenP);
      if (chosenP) {
        openRunGame(run, run.lines[static_cast<std::size_t>(items.indexOf(item))]);
      }
    } catch (const std::exception& error) {
      warning_("Cannot open the run file", error.what());
    }
    return;
  }

  void
  ViewerWindow::openScript(const QString& path)
  {
    const std::filesystem::path file = path.toStdString();
    load([file] { return ViewerGame::fromScript(file); }, path);
    return;
  }

  void
  ViewerWindow::openRunGame(const RunFile& run, const RunLine& line)
  {
    load([run, line] { return ViewerGame::fromRun(HEXGAMES_SOURCE_DIR, run, line); }, q(line.text));
    return;
  }

  void
  ViewerWindow::load(std::function<std::unique_ptr<ViewerGame>()> make, const QString& what)
  {
    if (loadingP()) {
      return;
    }
    slider_->setEnabled(false);
    status_->setText("Playing the game again: " + what);
    watcher_.setFuture(QtConcurrent::run([make = std::move(make)] {
      Loaded out;
      try {
        out.game = make();
      } catch (const std::exception& error) {
        out.error = error.what();
      }
      return out;
    }));
    return;
  }

  void
  ViewerWindow::finished()
  {
    const Loaded loaded = watcher_.result();
    if (!loaded.game) {
      status_->setText("The game could not be opened.");
      warning_("The game could not be opened", loaded.error);
      return;
    }
    map_->setDocument(nullptr);
    game_ = loaded.game;
    setWindowTitle(q("pgg_trc_viewer: " + game_->title()));
    map_->setDocument(&game_->document());
    slider_->setRange(0, static_cast<int>(game_->moveCount()));
    slider_->setTickInterval(100);
    slider_->setEnabled(true);
    QString text = "Fingerprint " + QString::number(game_->fingerprint());
    if (game_->fingerprintMismatchP()) {
      text += "  MISMATCH: the run file says " + QString::number(*game_->expectedFingerprint());
    } else if (game_->expectedFingerprint()) {
      text += "  (matches the run file)";
    }
    fingerprint_->setText(text);
    slider_->blockSignals(true);
    slider_->setValue(0);
    slider_->blockSignals(false);
    showMove(0);
    map_->fitAll();
    if (game_->fingerprintMismatchP()) {
      warning_("Fingerprint mismatch",
               "This is not the game the run file recorded.\n\nRun file: " + std::to_string(*game_->expectedFingerprint()) +
                   "\nPlayed now: " + std::to_string(game_->fingerprint()) +
                   "\n\nThe code has probably changed since the run. What you see is the game as the code plays it now.");
    }
    return;
  }

  void
  ViewerWindow::showMove(int move)
  {
    if (!game_) {
      return;
    }
    const std::size_t step = static_cast<std::size_t>(move);
    HexView::PlayLayerOptions options;
    options.revealAllP = reveal_->isChecked();
    map_->setOverlay(game_->overlay(step, options));
    status_->setText(QString("Step %1 of %2. ").arg(move).arg(game_->moveCount()) + q(game_->statusAt(step)) + ".  " + q(game_->commandOf(step)));
    QString lines;
    for (const std::string& line : game_->eventsOf(step)) {
      lines += q(line) + "\n";
    }
    events_->setPlainText(lines);
    return;
  }

  void
  ViewerWindow::stepTurn(int by)
  {
    if (game_) {
      const int turn = game_->turnOf(static_cast<std::size_t>(slider_->value()));
      slider_->setValue(static_cast<int>(game_->moveOfTurn(turn + by)));
    }
    return;
  }

}  // namespace HexQt
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
