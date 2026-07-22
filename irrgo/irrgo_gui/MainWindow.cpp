// Copyright Ben Paul Wise. All Rights Reserved.
#include "MainWindow.h"
#include "AbsGame.h"
#include "IrregularGraph.h"
#include "RectangularGraph.h"
#include "menu_helpers.h"
#include "MoveListWidget.h"
#include "PlaybackBar.h"
#include "../absgame/utils.h"
#include <QAction>
#include <QActionGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
//#include <QMenu>
#include <QMenuBar>
#include <QProgressBar>
#include <QPushButton>
#include <QIntValidator>
#include <QLineEdit>
#include <QSpinBox>
#include <QStringList>
#include <QTextEdit>
#include <QTimer>
//#include <QVBoxLayout>
#include <QWidget>
#include <QWidgetAction>
#include <algorithm>
#include <numeric>

using namespace IrrGo;

// ── Static data ───────────────────────────────────────────────────────────────

struct SizeEntry  { int rows, cols; const char* label; };
static const SizeEntry kSizes[] = {
    //{ 5,  5, "5 × 5"   }, // this size has low enough detail for ICO
    { 7,  7, "7 × 7"   }, { 7,  9, "7 × 9"   },
    { 9,  9, "9 × 9"   }, { 9, 13, "9 × 13"  },
    {13, 13, "13 × 13"  }, {13, 17, "13 × 17"  },
    {17, 19, "17 × 19"  },{17, 21, "17 × 21"  },
    {19, 19, "19 × 19" },
    {21, 21, "21 × 21" },
};

// I made it start at 17x21 because it is an interesting variant on 19x19.
// It is a slightly smaller area, and I expect tactics on the long edge
// to differ from those on the short edge.
static constexpr int kDefaultSizeIdx = 3; // 9 × 13

static const struct { QColor color; const char* label; } kBgColors[] = {
    { QColor("#F2B06D"), "Tan"  },
    { QColor("#FFD169"), "Gold" },
    { QColor("#0C7F84"), "Teal" },
};

static const int kMaxEdges[]       = { 3, 4, 5, 6 };
static constexpr int kDefaultMaxEdgesIdx = 1; // value 4

// Iterative-deepening ceiling for NegaMax. The clock is what actually stops the search;
// this only has to be past any depth the budget could reach, so it never binds.
static constexpr int kMaxNegamaxDepth = 64;

static const guicommon::MctsOption kMctsOptions[] = {
    {  10, "10 sec"   },
    {  30, "30 sec"   },
    {  45, "45 sec"   },
    {  60, "60 sec"   },
    {  90, "90 sec"   },
    { 120, "2 min"    },
    { 300, "5 min"    },
    { 450, "7.5 min"  },
    { 600, "10 min"   },
};

static const struct { double fraction; const char* label; } kStones[] = {
    { 0.00, "Empty"         },
    { 0.10, "Sparse (10%)"  },
    { 0.25, "Medium (25%)"  },
    { 0.50, "Dense (50%)"   },
    { 1.00, "Solid (100%)"  },
};

// ── Constructor ───────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : guicommon::GameMainWindow(parent), setupRng_(42)
{
    setWindowTitle("IrrGo");

    stoneTimer_ = new QTimer(this);
    connect(stoneTimer_, &QTimer::timeout, this, &MainWindow::onSetupTick);

    auto* central  = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QHBoxLayout(central);

    auto* boardArea = new QWidget(this);
    auto* boardVBox = new QVBoxLayout(boardArea);
    boardVBox->setContentsMargins(0, 0, 0, 0);
    boardVBox->setSpacing(4);
    boardWidget_ = new BoardWidget(boardArea);
    boardVBox->addWidget(boardWidget_, 1);
    auto* turnBar = guicommon::makeStatusBar(boardArea);
    boardVBox->addWidget(turnBar);
    auto* searchBar = guicommon::makeStatusBar(boardArea);
    boardVBox->addWidget(searchBar);
    search().setProgressBars(searchBar, turnBar);
    root->addWidget(boardArea, 1);
    connect(boardWidget_, &BoardWidget::moveRequested,
            this, &MainWindow::onMoveRequested);
    connect(boardWidget_, &BoardWidget::clearSuggestionRequested,
            this, &MainWindow::clearSuggestion);
    connect(boardWidget_, &BoardWidget::hoverChanged, this, [this](int nodeId) {
        if (nodeId >= 0 && game_) {
            hoverCoordLabel_->setText(QString::fromStdString(game_->graph().node(nodeId).label));
        } else {
            hoverCoordLabel_->setText("----");
        }
    });

    // ── Right panel ───────────────────────────────────────────────────────────
    auto* panel = new QWidget(this);
    panel->setFixedWidth(200);
    auto* pv = new QVBoxLayout(panel);
    pv->setAlignment(Qt::AlignTop);
    root->addWidget(panel);

    auto* statusRow  = new QWidget(this);
    auto* statusHBox = new QHBoxLayout(statusRow);
    statusHBox->setContentsMargins(0, 0, 0, 0);
    statusHBox->setSpacing(4);
    currentPlayerLabel_ = new QLabel("No game", statusRow);
    currentPlayerLabel_->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    stopBtn_ = new QPushButton("Stop", statusRow);
    stopBtn_->setStyleSheet("QPushButton { background-color: #FFCC99; color: black; }");
    stopBtn_->setFixedWidth(44);
    guicommon::retainSizeWhenHidden(stopBtn_);
    stopBtn_->hide();
    hoverCoordLabel_ = new QLabel("----", statusRow);
    hoverCoordLabel_->setFixedWidth(44);
    hoverCoordLabel_->setAlignment(Qt::AlignCenter);
    hoverCoordLabel_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    statusHBox->addWidget(currentPlayerLabel_, 1);
    statusHBox->addWidget(hoverCoordLabel_);
    statusHBox->addWidget(stopBtn_);
    pv->addWidget(statusRow);
    connect(stopBtn_, &QPushButton::clicked, this, [this]() { search().cancelSearch(); });
    pv->addSpacing(8);

    blackPassBtn_ = new QPushButton("Black Pass", this);
    whitePassBtn_ = new QPushButton("White Pass", this);
    blackPassBtn_->setEnabled(false);
    whitePassBtn_->setEnabled(false);
    pv->addWidget(blackPassBtn_);
    pv->addWidget(whitePassBtn_);
    connect(blackPassBtn_, &QPushButton::clicked, this, &MainWindow::onBlackPass);
    connect(whitePassBtn_, &QPushButton::clicked, this, &MainWindow::onWhitePass);

    labelsBtn_ = new QPushButton("Labels", this);
    pv->addWidget(labelsBtn_);
    connect(labelsBtn_, &QPushButton::pressed,  boardWidget_, &BoardWidget::showLabels);
    connect(labelsBtn_, &QPushButton::released, boardWidget_, &BoardWidget::hideLabels);

    pv->addSpacing(8);

    // Suggested move display
    pv->addWidget(new QLabel("Suggested:", this));
    suggestedLog_ = new QTextEdit(this);
    suggestedLog_->setReadOnly(true);
    suggestedLog_->setFixedHeight(54);  // ~2 lines
    pv->addWidget(suggestedLog_);
    clearSuggestBtn_ = new QPushButton("Clear", this);
    pv->addWidget(clearSuggestBtn_);
    connect(clearSuggestBtn_, &QPushButton::clicked, this, &MainWindow::clearSuggestion);

    {
        auto* radiusRow  = new QWidget(this);
        auto* radiusHBox = new QHBoxLayout(radiusRow);
        radiusHBox->setContentsMargins(0, 0, 0, 0);
        radiusHBox->setSpacing(4);
        radiusHBox->addWidget(new QLabel("Radius", radiusRow));
        dvrRadiusSpin_ = new QSpinBox(radiusRow);
        dvrRadiusSpin_->setRange(1, 999);
        dvrRadiusSpin_->setValue(4);
        radiusHBox->addWidget(dvrRadiusSpin_);
        pv->addWidget(radiusRow);
    }
    {
        auto* dvrRow  = new QWidget(this);
        auto* dvrHBox = new QHBoxLayout(dvrRow);
        dvrHBox->setContentsMargins(0, 0, 0, 0);
        dvrHBox->setSpacing(8);
        blackDvrCheck_ = new QCheckBox("Black DVR", dvrRow);
        whiteDvrCheck_ = new QCheckBox("White DVR", dvrRow);
        dvrHBox->addWidget(blackDvrCheck_);
        dvrHBox->addWidget(whiteDvrCheck_);
        pv->addWidget(dvrRow);
    }
    connect(dvrRadiusSpin_, &QSpinBox::valueChanged,
            boardWidget_,   &BoardWidget::setDvrRadius);
    connect(blackDvrCheck_, &QCheckBox::toggled,
            boardWidget_,   &BoardWidget::setShowBlackDvr);
    connect(whiteDvrCheck_, &QCheckBox::toggled,
            boardWidget_,   &BoardWidget::setShowWhiteDvr);

    neighborhoodBtn_ = new QPushButton("Neighborhood Size", this);
    pv->addWidget(neighborhoodBtn_);
    connect(neighborhoodBtn_, &QPushButton::pressed,
            boardWidget_,     &BoardWidget::showNeighborhoodSize);
    connect(neighborhoodBtn_, &QPushButton::released,
            boardWidget_,     &BoardWidget::hideNeighborhoodSize);

    pv->addSpacing(8);

    playback_ = new guicommon::PlaybackBar(this);
    pv->addWidget(playback_);
    pv->addWidget(new QLabel("Move log:", this));
    moveList_ = new guicommon::MoveListWidget(this);  // monospaced, set in its ctor
    pv->addWidget(moveList_, 1);
    registerPlayback(playback_, moveList_);

    buildMenuBar();
    resize(1000, 760);
    irregularCheck_->setChecked(true);
    generateBoard();
}

// ── Menu bar ──────────────────────────────────────────────────────────────────

void MainWindow::buildMenuBar() {
    // File
    auto* fileMenu = menuBar()->addMenu("File");
    connect(fileMenu->addAction("Save"),     &QAction::triggered, this, &MainWindow::onSave);
    connect(fileMenu->addAction("Save As..."), &QAction::triggered, this, &MainWindow::onSaveAs);
    fileMenu->addSeparator();
    connect(fileMenu->addAction("Load..."), &QAction::triggered, this, &MainWindow::onLoad);
    fileMenu->addSeparator();
    connect(fileMenu->addAction("Quit"), &QAction::triggered, this, &QWidget::close);

    // Board
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

    maxEdgesCombo_ = new QComboBox(bmw);
    for (int v : kMaxEdges) maxEdgesCombo_->addItem(QString::number(v));
    maxEdgesCombo_->setCurrentIndex(kDefaultMaxEdgesIdx);
    maxEdgesCombo_->setEnabled(false);
    form->addRow("Max Edges:", maxEdgesCombo_);
    connect(irregularCheck_, &QCheckBox::toggled,
            maxEdgesCombo_,  &QWidget::setEnabled);
    connect(irregularCheck_, &QCheckBox::toggled, this, [this](bool checked) {
        bgCombo_->setCurrentIndex(checked ? 2 : 0);  // 2=Teal, 0=Tan
    });

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

    // ── Play (mode selection only; Go! is present but unconnected) ────────────
    auto* playMenu  = menuBar()->addMenu("Play");
    auto* playGroup = new QActionGroup(this);
    playGroup->setExclusive(true);

    manualAction_ = playMenu->addAction("Manual");
    manualAction_->setCheckable(true);
    manualAction_->setChecked(true);
    playGroup->addAction(manualAction_);

    {
        // Same time choices as MCTS: both are wall-clock budgets now, and offering two
        // different scales for the same quantity would only invite confusion.
        guicommon::TimeMenuConfig nm;  // defaults: turns 1-999/10
        nm.options     = kMctsOptions;
        nm.optionCount = std::size(kMctsOptions);
        auto* goBtn = guicommon::buildNegaMaxTimeMenu(this, playMenu, playGroup, nm,
                                                      playNegamaxSecCombo_, playTurnsSpin_);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onPlayNegamaxGo);
    }

    {
        guicommon::MctsMenuConfig mc;  // defaults: turns 1-999/10
        mc.options     = kMctsOptions;
        mc.optionCount = std::size(kMctsOptions);
        auto* goBtn = guicommon::buildMctsMenu(this, playMenu, playGroup, mc,
                                               playMctsSecCombo_, playMctsTurnsSpin_);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onPlayMctsGo);
    }

    // ── Suggest (NegaMax Go! is wired up) ────────────────────────────────────
    auto* suggestMenu  = menuBar()->addMenu("Suggest");
    auto* suggestGroup = new QActionGroup(this);
    suggestGroup->setExclusive(true);

    {
        guicommon::TimeMenuConfig nm;
        nm.options     = kMctsOptions;
        nm.optionCount = std::size(kMctsOptions);
        nm.withTurns   = false;
        auto* goBtn = guicommon::buildNegaMaxTimeMenu(this, suggestMenu, suggestGroup, nm,
                                                      suggestNegamaxSecCombo_, suggestTurnsSpin_);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onSuggestGo);
    }

    // MCTS suggest submenu
    {
        guicommon::MctsMenuConfig mc;
        mc.options     = kMctsOptions;
        mc.optionCount = std::size(kMctsOptions);
        mc.withTurns   = false;
        QSpinBox* unusedTurns = nullptr;  // suggest MCTS has no Turns control
        auto* goBtn = guicommon::buildMctsMenu(this, suggestMenu, suggestGroup, mc,
                                               suggestMctsSecCombo_, unusedTurns);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onSuggestMctsGo);
    }

    // Keep the Play and Suggest NegaMax time combos in sync
    connect(playNegamaxSecCombo_,    &QComboBox::currentIndexChanged,
            suggestNegamaxSecCombo_, &QComboBox::setCurrentIndex);
    connect(suggestNegamaxSecCombo_, &QComboBox::currentIndexChanged,
            playNegamaxSecCombo_,    &QComboBox::setCurrentIndex);

    // ── Random ────────────────────────────────────────────────────────────────
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
            int s = static_cast<int>( AbsGame::msRandom());
            randomSeedEdit_->setText(QString::number(s));
        }
    });

    auto* rwa = new QWidgetAction(randomMenu);
    rwa->setDefaultWidget(rmw);
    randomMenu->addAction(rwa);

    // ── Theme ─────────────────────────────────────────────────────────────────
    auto* themeMenu = menuBar()->addMenu("Theme");

    auto* tmw  = new QWidget;
    tmw->setMinimumWidth(200);
    auto* tvbox = new QVBoxLayout(tmw);
    tvbox->setContentsMargins(8, 6, 8, 6);
    auto* tform = new QFormLayout;
    tform->setSpacing(6);
    tvbox->addLayout(tform);

    bgCombo_ = new QComboBox(tmw);
    for (const auto& bg : kBgColors) bgCombo_->addItem(bg.label);
    tform->addRow("Background:", bgCombo_);
    connect(bgCombo_, &QComboBox::currentIndexChanged,
            this, &MainWindow::onBgColorChanged);

    auto* tsep = new QFrame(tmw);
    tsep->setFrameShape(QFrame::HLine);
    tvbox->addWidget(tsep);

    auto* textureCheck = new QCheckBox("Texture", tmw);
    tvbox->addWidget(textureCheck);
    connect(textureCheck, &QCheckBox::toggled,
            boardWidget_,  &BoardWidget::setUseTexture);

    auto* twa = new QWidgetAction(themeMenu);
    twa->setDefaultWidget(tmw);
    themeMenu->addAction(twa);
}

// ── Board generation ──────────────────────────────────────────────────────────

void MainWindow::generateBoard() {
    search().cancelSearch();
    stopStoneSetup();
    clearSuggestion();

    const auto& sz  = kSizes[sizeCombo_->currentIndex()];
    bool        irr = irregularCheck_->isChecked();
    uint64_t   seed = static_cast<uint64_t>(randomSeedEdit_->text().toInt());

    game_.reset();
    graph_.reset();
    if (irr) {
        int maxDeg = kMaxEdges[maxEdgesCombo_->currentIndex()];
        graph_ = std::make_unique<IrregularGraph>(sz.rows, sz.cols, maxDeg, seed);
    }
    else {
        graph_ = std::make_unique<RectangularGraph>(sz.rows, sz.cols);
    }

    game_ = std::make_unique<Game>(*graph_);
    setupPlaced_ = 0;
    boardWidget_->setGame(game_.get());   // also resets DVR flags on the widget
    blackDvrCheck_->setChecked(false);
    whiteDvrCheck_->setChecked(false);
    boardWidget_->setBgColor(kBgColors[bgCombo_->currentIndex()].color);
    boardWidget_->setBoardInfo(QString("%1 x %2: %3")
        .arg(sz.rows).arg(sz.cols).arg(static_cast<int>(seed)));

    stonesGroup_->actions().first()->setChecked(true);
    setupNodes_.clear();
    timeline_.clear();
    rebuildMoveList();
    syncPlaybackToEnd();
    updateControls();
}

void MainWindow::onBgColorChanged(int index) {
    boardWidget_->setBgColor(kBgColors[index].color);
}

// ── Stone setup animation ─────────────────────────────────────────────────────

void MainWindow::onStonesSelected(QAction* action) {
    if (!game_) {
        return;
    }
    search().cancelSearch();
    stopStoneSetup();
    clearSuggestion();

    game_.reset();
    game_ = std::make_unique<Game>(*graph_);
    boardWidget_->setGame(game_.get());
    setupNodes_.clear();
    timeline_.clear();
    rebuildMoveList();
    syncPlaybackToEnd();
    updateControls();

    double fraction = action->data().toDouble();
    if (fraction <= 0.0) {
        return;
    }

    setupTarget_ = static_cast<int>(fraction * graph_->nodeCount());
    if (setupTarget_ <= 0) {
        return;
    }

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
            if (setupPlaced_ >= setupTarget_) {
                stoneTimer_->stop();
                game_->setSetupMode(false);
                captureSeed();
                updateControls();
            }
            return;
        }
    }
    stoneTimer_->stop();
    game_->setSetupMode(false);
    captureSeed();
    boardWidget_->update();
    updateControls();
}

void MainWindow::stopStoneSetup() {
    stoneTimer_->stop();
    if (game_) {
        game_->setSetupMode(false);
    }
}

// ── Move handling ─────────────────────────────────────────────────────────────

void MainWindow::onMoveRequested(int nodeId) {
    if (!game_ || stoneTimer_->isActive() || search().isSearching()) {
        return;
    }
    if (game_->placeStone(nodeId)) {
        clearSuggestion();
        afterPlayMove();
    }
}

void MainWindow::onBlackPass() {
    if (!game_ || game_->toMove() != Player::Black || search().isSearching()) {
        return;
    }
    if (game_->pass()) {
        clearSuggestion();
        afterPlayMove();
    }
}

void MainWindow::onWhitePass() {
    if (!game_ || game_->toMove() != Player::White || search().isSearching()) {
        return;
    }
    if (game_->pass()) {
        clearSuggestion();
        afterPlayMove();
    }
}

// ── NegaMax suggestion ────────────────────────────────────────────────────────

void MainWindow::onSuggestGo() {
    if (!game_ || game_->isGameOver() || search().isSearching() || stoneTimer_->isActive()) {
        return;
    }

    bool isBlack = (game_->toMove() == Player::Black);
    int  turn    = static_cast<int>(game_->moveHistory().size()) + 1;

    guicommon::SearchController::Params p;
    p.algo          = guicommon::SearchController::Algorithm::NegaMax;
    p.depth         = kMaxNegamaxDepth;
    p.negamaxTimeMs = suggestNegamaxSecCombo_->currentData().toInt() * 1000;
    // The clock bounds this search, so the search bar shows the elapsed fraction.
    p.negamaxTimeBudgeted = true;
    search().launch(game_->clone(), p, [this, isBlack, turn](AbsGame::MoveId mv, unsigned) {
        QString text;
        if (mv == AbsGame::kPass) {
            text = QString("%1: %2 PASS").arg(turn).arg(isBlack ? "B" : "W");
            boardWidget_->clearSuggestion();
        } else {
            const auto& nd = game_->graph().node(mv);
            text = QString("%1: %2 R%3C%4")
                       .arg(turn).arg(isBlack ? "B" : "W")
                       .arg(nd.row).arg(nd.col);
            boardWidget_->setSuggestion(mv, isBlack);
        }
        suggestedLog_->setText(text);
    });
}

void MainWindow::onSuggestMctsGo() {
    if (!game_ || game_->isGameOver() || search().isSearching()) {
        return;
    }

    bool isBlack = (game_->toMove() == Player::Black);
    int  turn    = static_cast<int>(game_->moveHistory().size()) + 1;

    guicommon::SearchController::Params p;
    p.algo    = guicommon::SearchController::Algorithm::Mcts;
    p.seconds = suggestMctsSecCombo_->currentData().toInt();
    search().launch(game_->clone(), p, [this, isBlack, turn](AbsGame::MoveId mv, unsigned) {
        QString text;
        if (mv == AbsGame::kPass) {
            text = QString("%1: %2 PASS").arg(turn).arg(isBlack ? "B" : "W");
            boardWidget_->clearSuggestion();
        } else {
            const auto& nd = game_->graph().node(mv);
            text = QString("%1: %2 %3")
                       .arg(turn).arg(isBlack ? "B" : "W")
                       .arg(QString::fromStdString(nd.label));
            boardWidget_->setSuggestion(mv, isBlack);
        }
        suggestedLog_->setText(text);
    });
}

void MainWindow::applyComputedMove(AbsGame::MoveId mv) {
    if (!game_ || game_->isGameOver()) {
        return;
    }
    clearSuggestion();
    if (mv == AbsGame::kPass) {
        game_->pass();
    } else {
        game_->placeStone(mv);
    }
    afterPlayMove();
}

void MainWindow::onPlayNegamaxGo() {
    guicommon::SearchController::Params p;
    p.algo          = guicommon::SearchController::Algorithm::NegaMax;
    p.depth         = kMaxNegamaxDepth;
    p.negamaxTimeMs = playNegamaxSecCombo_->currentData().toInt() * 1000;
    p.negamaxTimeBudgeted = true;  // as in onSuggestGo: the clock bounds this search
    startPlay(p, playTurnsSpin_->value());
}

void MainWindow::onPlayMctsGo() {
    guicommon::SearchController::Params p;
    p.algo    = guicommon::SearchController::Algorithm::Mcts;
    p.seconds = playMctsSecCombo_->currentData().toInt();
    startPlay(p, playMctsTurnsSpin_->value());
}

// ── GameMainWindow hooks ──────────────────────────────────────────────────────

AbsGame::Game* MainWindow::currentGame() {
    return game_.get();
}

bool MainWindow::extraSearchBlock() const {
    return stoneTimer_->isActive();
}

void MainWindow::clearSuggestion() {
    suggestedLog_->clear();
    boardWidget_->clearSuggestion();
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void MainWindow::updateControls() {
    bool searching = search().isSearching();
    stopBtn_->setVisible(searching);
    boardWidget_->setSearching(searching);
    bool idle = !searching;
    menuBar()          ->setEnabled(idle);
    labelsBtn_         ->setEnabled(idle);
    clearSuggestBtn_   ->setEnabled(idle);
    blackDvrCheck_     ->setEnabled(idle);
    whiteDvrCheck_     ->setEnabled(idle);
    dvrRadiusSpin_     ->setEnabled(idle);
    neighborhoodBtn_   ->setEnabled(idle);
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
    bool bt        = (game_->toMove() == Player::Black);
    bool animating = stoneTimer_->isActive();
    if (searching) {
        currentPlayerLabel_->setText("Thinking...");
    } else {
        currentPlayerLabel_->setText(bt ? "Black to move" : "White to move");
    }
    blackPassBtn_->setEnabled( bt && !animating && !searching);
    whitePassBtn_->setEnabled(!bt && !animating && !searching);
}

// ── Playback / replay ─────────────────────────────────────────────────────────

// Format a single play move as a log row. Turns are numbered globally from move 1
// and the side strictly alternates (every placeStone/pass toggles the player), so
// an odd turn was Black's and an even turn White's — for stones and passes alike.
QString MainWindow::moveText(const Move& m) const {
    if (!game_) {
        return QString();
    }
    const QString clr = (m.turn % 2 == 1) ? "B" : "W";
    if (m.nodeId < 0) {
        return QString("%1: %2 PASS").arg(m.turn, 3).arg(clr);
    }
    return QString("%1: %2 %3")
        .arg(m.turn, 3)
        .arg(clr)
        .arg(QString::fromStdString(game_->graph().node(m.nodeId).label));
}

void MainWindow::rebuildMoveList() {
    QStringList rows;
    rows.reserve(static_cast<int>(timeline_.size()));
    for (const Move& m : timeline_) {
        rows << moveText(m);
    }
    moveList_->setMoves(rows);
}

// Record the seed stones (the first setupPlaced_ history entries) as the fixed
// ply-0 position; the play timeline then starts empty.
void MainWindow::captureSeed() {
    setupNodes_.clear();
    const auto& h = game_->moveHistory();
    const int n = std::min(setupPlaced_, static_cast<int>(h.size()));
    for (int i = 0; i < n; ++i) {
        setupNodes_.push_back(h[i].nodeId);
    }
    timeline_.clear();
    rebuildMoveList();
    syncPlaybackToEnd();
}

// Common tail once a play move (or pass) has been applied to game_: refresh the
// timeline (everything past the seed), the move list, the board, and the bar. A
// take-over move made mid-replay truncates the superseded future automatically,
// since the timeline is taken from game_'s own (already-truncated) history.
void MainWindow::afterPlayMove() {
    const auto& h = game_->moveHistory();
    const int start = std::min(static_cast<int>(setupNodes_.size()),
                               static_cast<int>(h.size()));
    timeline_.assign(h.begin() + start, h.end());
    rebuildMoveList();
    boardWidget_->update();
    boardWidget_->setLastMove(timeline_.empty() ? -1 : timeline_.back().nodeId);
    updateControls();
    syncPlaybackToEnd();
}

int MainWindow::playbackPlyCount() const {
    return static_cast<int>(timeline_.size());
}

// Reconstruct the game at the cursor: a fresh game seeded with setupNodes_ (in
// setup mode), then the first `ply` play moves replayed. game_ becomes that
// position; the widget is repointed before any repaint to avoid a dangling game.
void MainWindow::rebuildToPly(int ply) {
    if (!graph_) {
        return;
    }
    auto g = std::make_unique<Game>(*graph_);
    g->setSetupMode(true);
    for (int nodeId : setupNodes_) {
        g->placeStone(nodeId);
    }
    g->setSetupMode(false);

    const int k = std::min(ply, static_cast<int>(timeline_.size()));
    for (int i = 0; i < k; ++i) {
        const Move& m = timeline_[i];
        if (m.nodeId >= 0) {
            g->placeStone(m.nodeId);
        } else {
            g->pass();
        }
    }

    game_ = std::move(g);
    boardWidget_->setGame(game_.get());   // resets DVR/feedback; re-apply the DVR view
    boardWidget_->setDvrRadius(dvrRadiusSpin_->value());
    boardWidget_->setShowBlackDvr(blackDvrCheck_->isChecked());
    boardWidget_->setShowWhiteDvr(whiteDvrCheck_->isChecked());
    boardWidget_->setLastMove(k > 0 ? timeline_[k - 1].nodeId : -1);
    boardWidget_->update();
    updateControls();
}

// Copyright Ben Paul Wise. All Rights Reserved.
