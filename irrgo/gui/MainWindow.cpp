// Copyright Ben Paul Wise. All Rights Reserved.
#include "MainWindow.h"
#include "IrregularGraph.h"
#include "RectangularGraph.h"
#include "utils.h"
#include <chrono>
#include <QAction>
#include <QActionGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QIntValidator>
#include <QLineEdit>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QWidgetAction>
#include <algorithm>
#include <numeric>

// ── Static data ───────────────────────────────────────────────────────────────

struct SizeEntry  { int rows, cols; const char* label; };
static const SizeEntry kSizes[] = {
    { 7,  7, "7 × 7"   }, { 7,  9, "7 × 9"   }, { 9,  9, "9 × 9"   },
    { 9, 13, "9 × 13"  }, {13, 13, "13 × 13"  }, {15, 15, "15 × 15" },
    {15, 17, "15 × 17" }, {17, 19, "17 × 19"  }, {19, 19, "19 × 19" },
    {21, 21, "21 × 21" },
};
static constexpr int kDefaultSizeIdx = 8; // 19 × 19

static const struct { QColor color; const char* label; } kBgColors[] = {
    { QColor("#DBAD6B"), "Tan"  },
    { QColor("#FFD169"), "Gold" },
    { QColor("#0C7F84"), "Teal" },
};

static const struct { double fraction; const char* label; } kStones[] = {
    { 0.00, "Empty"         },
    { 0.20, "Sparse (20%)"  },
    { 0.40, "Medium (40%)"  },
    { 0.75, "Dense (75%)"   },
    { 1.00, "Solid (100%)"  },
};

// ── Constructor ───────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), setupRng_(42)
{
    setWindowTitle("IrrGo");

    stoneTimer_ = new QTimer(this);
    connect(stoneTimer_, &QTimer::timeout, this, &MainWindow::onSetupTick);

    // Central layout: board (stretches) + right panel (fixed)
    auto* central  = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QHBoxLayout(central);

    boardWidget_ = new BoardWidget(this);
    root->addWidget(boardWidget_, 1);
    connect(boardWidget_, &BoardWidget::moveRequested,
            this, &MainWindow::onMoveRequested);

    auto* panel = new QWidget(this);
    panel->setFixedWidth(200);
    auto* pv = new QVBoxLayout(panel);
    pv->setAlignment(Qt::AlignTop);
    root->addWidget(panel);

    currentPlayerLabel_ = new QLabel("No game", this);
    currentPlayerLabel_->setAlignment(Qt::AlignCenter);
    pv->addWidget(currentPlayerLabel_);
    pv->addSpacing(8);

    blackPassBtn_ = new QPushButton("Black Pass", this);
    whitePassBtn_ = new QPushButton("White Pass", this);
    blackPassBtn_->setEnabled(false);
    whitePassBtn_->setEnabled(false);
    pv->addWidget(blackPassBtn_);
    pv->addWidget(whitePassBtn_);
    connect(blackPassBtn_, &QPushButton::clicked, this, &MainWindow::onBlackPass);
    connect(whitePassBtn_, &QPushButton::clicked, this, &MainWindow::onWhitePass);

    pv->addSpacing(8);
    pv->addWidget(new QLabel("Move log:", this));
    moveLog_ = new QTextEdit(this);
    moveLog_->setReadOnly(true);
    pv->addWidget(moveLog_, 1);

    buildMenuBar();
    resize(1000, 760);
}

// ── Menu bar ──────────────────────────────────────────────────────────────────

void MainWindow::buildMenuBar() {
    // File
    auto* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("Save");
    fileMenu->addAction("Save As...");
    fileMenu->addSeparator();
    fileMenu->addAction("Load");

    // Board — embedded widget action
    auto* boardMenu = menuBar()->addMenu("Board");

    auto* bmw  = new QWidget;
    bmw->setMinimumWidth(240);
    auto* vbox = new QVBoxLayout(bmw);
    vbox->setContentsMargins(8, 6, 8, 6);

    auto* form = new QFormLayout;
    form->setSpacing(6);
    vbox->addLayout(form);

    sizeCombo_ = new QComboBox(bmw);
    for (const auto& s : kSizes) sizeCombo_->addItem(s.label);
    sizeCombo_->setCurrentIndex(kDefaultSizeIdx);
    form->addRow("Size:", sizeCombo_);

    irregularCheck_ = new QCheckBox("Irregular", bmw);
    form->addRow("", irregularCheck_);

    bgCombo_ = new QComboBox(bmw);
    for (const auto& bg : kBgColors) bgCombo_->addItem(bg.label);
    form->addRow("Background:", bgCombo_);
    connect(bgCombo_, &QComboBox::currentIndexChanged,
            this, &MainWindow::onBgColorChanged);

    auto* sep = new QFrame(bmw);
    sep->setFrameShape(QFrame::HLine);
    vbox->addWidget(sep);

    auto* genBtn = new QPushButton("Generate", bmw);
    vbox->addWidget(genBtn);
    connect(genBtn, &QPushButton::clicked, this,     &MainWindow::generateBoard);
    connect(genBtn, &QPushButton::clicked, boardMenu, &QWidget::close);

    auto* bwa = new QWidgetAction(boardMenu);
    bwa->setDefaultWidget(bmw);
    boardMenu->addAction(bwa);

    // Stones
    auto* stonesMenu = menuBar()->addMenu("Stones");
    stonesGroup_ = new QActionGroup(this);
    stonesGroup_->setExclusive(true);
    for (const auto& s : kStones) {
        auto* a = stonesMenu->addAction(s.label);
        a->setCheckable(true);
        a->setData(s.fraction);
        stonesGroup_->addAction(a);
    }
    stonesGroup_->actions().first()->setChecked(true);
    connect(stonesGroup_, &QActionGroup::triggered,
            this, &MainWindow::onStonesSelected);

    // Play (no-op)
    auto* playMenu  = menuBar()->addMenu("Play");
    auto* playGroup = new QActionGroup(this);
    playGroup->setExclusive(true);
    for (const char* lbl : {"Manual Placement", "10 Seconds",
                             "30 Seconds",       "60 Seconds"}) {
        auto* a = playMenu->addAction(lbl);
        a->setCheckable(true);
        playGroup->addAction(a);
    }
    playGroup->actions().first()->setChecked(true);

    // Random — seed control
    auto* randomMenu = menuBar()->addMenu("Random");

    auto* rmw  = new QWidget;
    rmw->setMinimumWidth(210);
    auto* rvbox = new QVBoxLayout(rmw);
    rvbox->setContentsMargins(8, 6, 8, 6);
    auto* rform = new QFormLayout;
    rform->setSpacing(6);
    rvbox->addLayout(rform);

    randomSeedEdit_ = new QLineEdit("42", rmw);
    randomSeedEdit_->setValidator(new QIntValidator(0, INT_MAX, rmw));
    randomSeedEdit_->setPlaceholderText("0 = auto");
    rform->addRow("Seed:", randomSeedEdit_);

    auto* rsep = new QFrame(rmw);
    rsep->setFrameShape(QFrame::HLine);
    rvbox->addWidget(rsep);

    auto* resetBtn = new QPushButton("Reset", rmw);
    rvbox->addWidget(resetBtn);

    connect(resetBtn, &QPushButton::clicked, [this]() {
        if (randomSeedEdit_->text().toInt() == 0) {
            using namespace std::chrono;
            uint64_t us = static_cast<uint64_t>(
                duration_cast<microseconds>(
                    steady_clock::now().time_since_epoch()).count());
            uint64_t rotated = (us << 7) | (us >> 57);
            int s = static_cast<int>(utils::qTrans(rotated) & 0x7FFFFFFF);
            if (s == 0) s = 1;
            randomSeedEdit_->setText(QString::number(s));
        }
    });

    auto* rwa = new QWidgetAction(randomMenu);
    rwa->setDefaultWidget(rmw);
    randomMenu->addAction(rwa);
}

// ── Board generation ──────────────────────────────────────────────────────────

void MainWindow::generateBoard() {
    stopStoneSetup();

    const auto& sz  = kSizes[sizeCombo_->currentIndex()];
    bool        irr = irregularCheck_->isChecked();
    uint64_t   seed = static_cast<uint64_t>(randomSeedEdit_->text().toInt());

    game_.reset();
    graph_.reset();
    if (irr)
        graph_ = std::make_unique<IrregularGraph>(sz.rows, sz.cols, 4, seed);
    else
        graph_ = std::make_unique<RectangularGraph>(sz.rows, sz.cols);

    game_ = std::make_unique<Game>(*graph_);
    boardWidget_->setGame(game_.get());
    boardWidget_->setBgColor(kBgColors[bgCombo_->currentIndex()].color);

    stonesGroup_->actions().first()->setChecked(true);
    moveLog_->clear();
    updateControls();
}

void MainWindow::onBgColorChanged(int index) {
    boardWidget_->setBgColor(kBgColors[index].color);
}

// ── Stone setup animation ─────────────────────────────────────────────────────

void MainWindow::onStonesSelected(QAction* action) {
    if (!game_) return;
    stopStoneSetup();

    // Recreate game on same graph for a fresh board
    game_.reset();
    game_ = std::make_unique<Game>(*graph_);
    boardWidget_->setGame(game_.get());
    moveLog_->clear();
    updateControls();

    double fraction = action->data().toDouble();
    if (fraction <= 0.0) return;

    setupTarget_ = static_cast<int>(fraction * graph_->nodeCount());
    if (setupTarget_ <= 0) return;

    setupRng_ = std::mt19937_64(static_cast<uint64_t>(randomSeedEdit_->text().toInt()) ^ 0xABCDEF01ULL);
    pendingSetup_.resize(graph_->nodeCount());
    std::iota(pendingSetup_.begin(), pendingSetup_.end(), 0);
    std::shuffle(pendingSetup_.begin(), pendingSetup_.end(), setupRng_);

    setupIdx_    = 0;
    setupPlaced_ = 0;
    game_->setSetupMode(true);
    int interval = std::min(250, 30000 / setupTarget_);
    stoneTimer_->setInterval(interval);
    stoneTimer_->start();
}

void MainWindow::onSetupTick() {
    while (setupIdx_ < static_cast<int>(pendingSetup_.size())) {
        int nodeId = pendingSetup_[setupIdx_++];
        if (game_->placeStone(nodeId)) {
            ++setupPlaced_;
            boardWidget_->update();
            updateControls();
            logLastMove();
            if (setupPlaced_ >= setupTarget_) {
                stoneTimer_->stop();
                game_->setSetupMode(false);
                updateControls();
            }
            return; // wait 500 ms before next stone
        }
    }
    // exhausted all nodes before reaching target
    stoneTimer_->stop();
    game_->setSetupMode(false);
    boardWidget_->update();
    updateControls();
}

void MainWindow::stopStoneSetup() {
    stoneTimer_->stop();
    if (game_) game_->setSetupMode(false);
}

// ── Move handling ─────────────────────────────────────────────────────────────

void MainWindow::onMoveRequested(int nodeId) {
    if (!game_ || stoneTimer_->isActive()) return;
    if (game_->placeStone(nodeId)) {
        boardWidget_->update();
        updateControls();
        logLastMove();
    }
}

void MainWindow::onBlackPass() {
    if (!game_ || game_->currentPlayer() != Player::Black) return;
    int turn = static_cast<int>(game_->moveHistory().size()) + 1;
    if (game_->pass()) {
        moveLog_->append(QString("%1: B PASS").arg(turn));
        boardWidget_->update();
        updateControls();
    }
}

void MainWindow::onWhitePass() {
    if (!game_ || game_->currentPlayer() != Player::White) return;
    int turn = static_cast<int>(game_->moveHistory().size()) + 1;
    if (game_->pass()) {
        moveLog_->append(QString("%1: W PASS").arg(turn));
        boardWidget_->update();
        updateControls();
    }
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void MainWindow::updateControls() {
    if (!game_) {
        currentPlayerLabel_->setText("No game");
        blackPassBtn_->setEnabled(false);
        whitePassBtn_->setEnabled(false);
        return;
    }
    if (game_->isGameOver()) {
        currentPlayerLabel_->setText("Game over");
        blackPassBtn_->setEnabled(false);
        whitePassBtn_->setEnabled(false);
        return;
    }
    bool bt      = (game_->currentPlayer() == Player::Black);
    bool animating = stoneTimer_->isActive();
    currentPlayerLabel_->setText(bt ? "Black to move" : "White to move");
    blackPassBtn_->setEnabled( bt && !animating);
    whitePassBtn_->setEnabled(!bt && !animating);
}

void MainWindow::logLastMove() {
    if (!game_ || game_->moveHistory().empty()) return;
    const Move& m = game_->moveHistory().back();
    QString text = (m.color == Color::Empty)
        ? QString("%1: PASS").arg(m.turn)
        : QString("%1: %2 R%3C%4")
              .arg(m.turn)
              .arg(m.color == Color::Black ? "B" : "W")
              .arg(m.row).arg(m.col);
    moveLog_->append(text);
}
