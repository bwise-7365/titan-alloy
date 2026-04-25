// Copyright Ben Paul Wise. All Rights Reserved.
#include "MainWindow.h"
#include "AbsGame.h"
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
#include <QProgressBar>
#include <QPushButton>
#include <QIntValidator>
#include <QLineEdit>
#include <QSpinBox>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QWidgetAction>
#include <algorithm>
#include <numeric>
#include <thread>

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

static const struct { int secs; const char* label; } kMctsOptions[] = {
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
    : QMainWindow(parent), setupRng_(42)
{
    setWindowTitle("IrrGo");

    stoneTimer_ = new QTimer(this);
    connect(stoneTimer_, &QTimer::timeout, this, &MainWindow::onSetupTick);

    searchBarTimer_ = new QTimer(this);
    connect(searchBarTimer_, &QTimer::timeout, this, [this]() {
        if (searchBudgetMs_ > 0) {
            int pct = qMin(100, static_cast<int>(searchElapsed_.elapsed()) * 100
                                / searchBudgetMs_);
            searchProgress_->setValue(pct);
        } else {
            searchProgress_->setValue((searchProgress_->value() + 3) % 101);
        }
    });

    auto* central  = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QHBoxLayout(central);

    auto* boardArea = new QWidget(this);
    auto* boardVBox = new QVBoxLayout(boardArea);
    boardVBox->setContentsMargins(0, 0, 0, 0);
    boardVBox->setSpacing(4);
    boardWidget_ = new BoardWidget(boardArea);
    boardVBox->addWidget(boardWidget_, 1);
    turnProgress_ = new QProgressBar(boardArea);
    turnProgress_->setRange(0, 100);
    turnProgress_->setFixedHeight(14);
    turnProgress_->setTextVisible(false);
    {
        auto sp = turnProgress_->sizePolicy();
        sp.setRetainSizeWhenHidden(true);
        turnProgress_->setSizePolicy(sp);
    }
    turnProgress_->hide();
    boardVBox->addWidget(turnProgress_);
    searchProgress_ = new QProgressBar(boardArea);
    searchProgress_->setRange(0, 100);
    searchProgress_->setFixedHeight(14);
    searchProgress_->setTextVisible(false);
    {
        auto sp = searchProgress_->sizePolicy();
        sp.setRetainSizeWhenHidden(true);
        searchProgress_->setSizePolicy(sp);
    }
    searchProgress_->hide();
    boardVBox->addWidget(searchProgress_);
    root->addWidget(boardArea, 1);
    connect(boardWidget_, &BoardWidget::moveRequested,
            this, &MainWindow::onMoveRequested);
    connect(boardWidget_, &BoardWidget::clearSuggestionRequested,
            this, &MainWindow::clearSuggestion);

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
    { auto sp = stopBtn_->sizePolicy(); sp.setRetainSizeWhenHidden(true); stopBtn_->setSizePolicy(sp); }
    stopBtn_->hide();
    statusHBox->addWidget(currentPlayerLabel_, 1);
    statusHBox->addWidget(stopBtn_);
    pv->addWidget(statusRow);
    connect(stopBtn_, &QPushButton::clicked, this, &MainWindow::cancelSearch);
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

    pv->addWidget(new QLabel("Move log:", this));
    moveLog_ = new QTextEdit(this);
    moveLog_->setReadOnly(true);
    {
        QFont font("Monospace");
        font.setStyleHint(QFont::TypeWriter);
        moveLog_->setFont(font);
    }
    pv->addWidget(moveLog_, 1);

    buildMenuBar();
    resize(1000, 760);
    irregularCheck_->setChecked(true);
    generateBoard();
}

// ── Shared NegaMax submenu builder ────────────────────────────────────────────

QPushButton* MainWindow::buildNegaMaxMenu(QMenu* parent, QActionGroup* group,
                                          QSpinBox*& depthOut, QSpinBox*& turnsOut,
                                          bool withTurns)
{
    auto* action = new QAction("NegaMax", this);
    action->setCheckable(true);
    group->addAction(action);
    parent->addAction(action);

    auto* nmMenu  = new QMenu(this);
    auto* widget  = new QWidget;
    auto* vbox    = new QVBoxLayout(widget);
    vbox->setContentsMargins(8, 6, 8, 6);
    auto* form    = new QFormLayout;
    form->setSpacing(6);
    vbox->addLayout(form);

    depthOut = new QSpinBox(widget);
    depthOut->setRange(1, 6);
    depthOut->setValue(2);
    form->addRow("Depth:", depthOut);

    if (withTurns) {
        turnsOut = new QSpinBox(widget);
        turnsOut->setRange(1, 50);
        turnsOut->setValue(2);
        form->addRow("Turns:", turnsOut);
    } else {
        turnsOut = nullptr;
    }

    auto* sep = new QFrame(widget);
    sep->setFrameShape(QFrame::HLine);
    vbox->addWidget(sep);

    auto* goBtn = new QPushButton("Go!", widget);
    vbox->addWidget(goBtn);
    connect(goBtn, &QPushButton::clicked, nmMenu, &QMenu::hide);

    auto* wa = new QWidgetAction(nmMenu);
    wa->setDefaultWidget(widget);
    nmMenu->addAction(wa);
    action->setMenu(nmMenu);

    connect(nmMenu, &QMenu::aboutToShow, this, [action]() {
        action->setChecked(true);
    });
    return goBtn;
}

// ── Menu bar ──────────────────────────────────────────────────────────────────

void MainWindow::buildMenuBar() {
    // File
    auto* fileMenu = menuBar()->addMenu("File");
    connect(fileMenu->addAction("Save"),     &QAction::triggered, this, &MainWindow::onSave);
    connect(fileMenu->addAction("Save As..."), &QAction::triggered, this, &MainWindow::onSaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("Load");

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

    // ── Play (mode selection only; Go! is present but unconnected) ────────────
    auto* playMenu  = menuBar()->addMenu("Play");
    auto* playGroup = new QActionGroup(this);
    playGroup->setExclusive(true);

    manualAction_ = playMenu->addAction("Manual");
    manualAction_->setCheckable(true);
    manualAction_->setChecked(true);
    playGroup->addAction(manualAction_);

    {
        auto* goBtn = buildNegaMaxMenu(playMenu, playGroup, playDepthSpin_, playTurnsSpin_);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onPlayNegamaxGo);
    }

    {
        auto* mctsAction = new QAction("MCTS", this);
        mctsAction->setCheckable(true);
        playGroup->addAction(mctsAction);
        playMenu->addAction(mctsAction);

        auto* mctsMenu = new QMenu(this);
        auto* widget   = new QWidget;
        auto* vbox     = new QVBoxLayout(widget);
        vbox->setContentsMargins(8, 6, 8, 6);
        auto* form     = new QFormLayout;
        form->setSpacing(6);
        vbox->addLayout(form);

        playMctsSecCombo_ = new QComboBox(widget);
        for (const auto& o : kMctsOptions)
            playMctsSecCombo_->addItem(o.label, o.secs);
        form->addRow("Time:", playMctsSecCombo_);

        playMctsTurnsSpin_ = new QSpinBox(widget);
        playMctsTurnsSpin_->setRange(1, 999);
        playMctsTurnsSpin_->setValue(10);
        form->addRow("Turns:", playMctsTurnsSpin_);

        auto* sep = new QFrame(widget);
        sep->setFrameShape(QFrame::HLine);
        vbox->addWidget(sep);

        auto* goBtn = new QPushButton("Go!", widget);
        vbox->addWidget(goBtn);
        connect(goBtn, &QPushButton::clicked, mctsMenu, &QMenu::hide);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onPlayMctsGo);

        auto* wa = new QWidgetAction(mctsMenu);
        wa->setDefaultWidget(widget);
        mctsMenu->addAction(wa);
        mctsAction->setMenu(mctsMenu);

        connect(mctsMenu, &QMenu::aboutToShow, this, [mctsAction]() {
            mctsAction->setChecked(true);
        });
    }

    // ── Suggest (NegaMax Go! is wired up) ────────────────────────────────────
    auto* suggestMenu  = menuBar()->addMenu("Suggest");
    auto* suggestGroup = new QActionGroup(this);
    suggestGroup->setExclusive(true);

    {
        auto* goBtn = buildNegaMaxMenu(suggestMenu, suggestGroup,
                                       suggestDepthSpin_, suggestTurnsSpin_,
                                       /* withTurns= */ false);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onSuggestGo);
    }

    // MCTS suggest submenu
    {
        auto* mctsAction = new QAction("MCTS", this);
        mctsAction->setCheckable(true);
        suggestGroup->addAction(mctsAction);
        suggestMenu->addAction(mctsAction);

        auto* mctsMenu   = new QMenu(this);
        auto* widget     = new QWidget;
        auto* vbox       = new QVBoxLayout(widget);
        vbox->setContentsMargins(8, 6, 8, 6);
        auto* form       = new QFormLayout;
        form->setSpacing(6);
        vbox->addLayout(form);

        suggestMctsSecCombo_ = new QComboBox(widget);
        for (const auto& o : kMctsOptions)
            suggestMctsSecCombo_->addItem(o.label, o.secs);
        form->addRow("Time:", suggestMctsSecCombo_);

        auto* sep = new QFrame(widget);
        sep->setFrameShape(QFrame::HLine);
        vbox->addWidget(sep);

        auto* goBtn = new QPushButton("Go!", widget);
        vbox->addWidget(goBtn);
        connect(goBtn, &QPushButton::clicked, mctsMenu, &QMenu::hide);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onSuggestMctsGo);

        auto* wa = new QWidgetAction(mctsMenu);
        wa->setDefaultWidget(widget);
        mctsMenu->addAction(wa);
        mctsAction->setMenu(mctsMenu);

        connect(mctsMenu, &QMenu::aboutToShow, this, [mctsAction]() {
            mctsAction->setChecked(true);
        });
    }

    // Keep Play and Suggest depth spinboxes in sync
    connect(playDepthSpin_,    &QSpinBox::valueChanged,
            suggestDepthSpin_, &QSpinBox::setValue);
    connect(suggestDepthSpin_, &QSpinBox::valueChanged,
            playDepthSpin_,    &QSpinBox::setValue);

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
    cancelSearch();
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
    else
        graph_ = std::make_unique<RectangularGraph>(sz.rows, sz.cols);

    game_ = std::make_unique<Game>(*graph_);
    setupPlaced_ = 0;
    boardWidget_->setGame(game_.get());   // also resets DVR flags on the widget
    blackDvrCheck_->setChecked(false);
    whiteDvrCheck_->setChecked(false);
    boardWidget_->setBgColor(kBgColors[bgCombo_->currentIndex()].color);
    boardWidget_->setBoardInfo(QString("%1 x %2: %3")
        .arg(sz.rows).arg(sz.cols).arg(static_cast<int>(seed)));

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
    cancelSearch();
    stopStoneSetup();
    clearSuggestion();

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
            return;
        }
    }
    stoneTimer_->stop();
    game_->setSetupMode(false);
    boardWidget_->update();
    updateControls();
}

void MainWindow::stopStoneSetup() {
    stoneTimer_->stop();
    if (game_) game_->setSetupMode(false);
}

// ── Search progress indicator ──────────────────────────────────────────────────

void MainWindow::startSearchIndicator(int budgetSeconds) {
    searchBudgetMs_ = budgetSeconds * 1000;
    searchProgress_->setValue(0);
    searchProgress_->show();
    if (searchBudgetMs_ > 0) {
        searchElapsed_.start();
        searchBarTimer_->setInterval(250);
    } else {
        searchBarTimer_->setInterval(30);
    }
    searchBarTimer_->start();
    updateControls();
}

void MainWindow::stopSearchIndicator() {
    searchBarTimer_->stop();
    searchProgress_->hide();
}

void MainWindow::cancelSearch() {
    ++searchGen_;
    playTurnsRemaining_ = 0;
    playTurnsTotal_     = 0;
    turnProgress_->hide();
    if (isSearching_) {
        isSearching_ = false;
        stopSearchIndicator();
        updateControls();
    }
}

// ── Move handling ─────────────────────────────────────────────────────────────

void MainWindow::onMoveRequested(int nodeId) {
    if (!game_ || stoneTimer_->isActive() || isSearching_) return;
    if (game_->placeStone(nodeId)) {
        boardWidget_->setLastMove(nodeId);
        clearSuggestion();
        boardWidget_->update();
        updateControls();
        logLastMove();
    }
}

void MainWindow::onBlackPass() {
    if (!game_ || game_->toMove() != Player::Black || isSearching_) return;
    if (game_->pass()) {
        clearSuggestion();
        boardWidget_->update();
        updateControls();
        logLastMove();
    }
}

void MainWindow::onWhitePass() {
    if (!game_ || game_->toMove() != Player::White || isSearching_) return;
    if (game_->pass()) {
        clearSuggestion();
        boardWidget_->update();
        updateControls();
        logLastMove();
    }
}

// ── NegaMax suggestion ────────────────────────────────────────────────────────

void MainWindow::onSuggestGo() {
    if (!game_ || game_->isGameOver() || isSearching_ || stoneTimer_->isActive()) return;

    isSearching_ = true;
    startSearchIndicator();

    int      depth      = suggestDepthSpin_->value();
    bool     isBlack    = (game_->toMove() == Player::Black);
    int      turn       = static_cast<int>(game_->moveHistory().size()) + 1;
    unsigned gen        = searchGen_;
    auto     searchGame = game_->clone();

    std::thread([this, gen, depth, isBlack, turn,
                 searchGame = std::move(searchGame)]() {
        AbsGame::MoveId mv = AbsGame::Searcher::bestMove(*searchGame, depth, 10000);
        QMetaObject::invokeMethod(this, [this, mv, gen, isBlack, turn]() {
            if (gen != searchGen_) return;
            isSearching_ = false;
            stopSearchIndicator();
            updateControls();
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
        }, Qt::QueuedConnection);
    }).detach();
}

void MainWindow::onSuggestMctsGo() {
    if (!game_ || game_->isGameOver() || isSearching_) return;

    isSearching_ = true;
    int      seconds    = suggestMctsSecCombo_->currentData().toInt();
    startSearchIndicator(seconds);
    bool     isBlack    = (game_->toMove() == Player::Black);
    int      turn       = static_cast<int>(game_->moveHistory().size()) + 1;
    unsigned gen        = searchGen_;
    auto     searchGame = game_->clone();

    std::thread([this, gen, seconds, isBlack, turn,
                 searchGame = std::move(searchGame)]() {
        AbsGame::MoveId mv = AbsGame::Searcher::mcts(*searchGame, seconds);
        QMetaObject::invokeMethod(this, [this, mv, gen, isBlack, turn]() {
            if (gen != searchGen_) return;
            isSearching_ = false;
            stopSearchIndicator();
            updateControls();
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
        }, Qt::QueuedConnection);
    }).detach();
}

void MainWindow::applyComputedMove(AbsGame::MoveId mv) {
    if (!game_ || game_->isGameOver()) return;
    clearSuggestion();
    if (mv == AbsGame::kPass) {
        game_->pass();
    } else {
        game_->placeStone(mv);
        boardWidget_->setLastMove(mv);
    }
    boardWidget_->update();
    updateControls();
    logLastMove();
}

void MainWindow::onPlayNegamaxGo() {
    if (!game_ || game_->isGameOver() || isSearching_ || stoneTimer_->isActive()) return;

    if (playTurnsRemaining_ <= 0) {
        playTurnsRemaining_ = playTurnsSpin_->value();
        playTurnsTotal_     = playTurnsRemaining_;
        turnProgress_->setValue(0);
        turnProgress_->show();
    }

    isSearching_ = true;
    startSearchIndicator();

    auto     searchGame = game_->clone();
    int      depth      = playDepthSpin_->value();
    unsigned gen        = searchGen_;

    std::thread([this, gen, depth, searchGame = std::move(searchGame)]() {
        AbsGame::MoveId mv = AbsGame::Searcher::bestMove(*searchGame, depth, 10000);
        QMetaObject::invokeMethod(this, [this, mv, gen]() {
            if (gen != searchGen_) return;
            isSearching_ = false;
            stopSearchIndicator();
            applyComputedMove(mv);
            --playTurnsRemaining_;
            if (playTurnsTotal_ > 0)
                turnProgress_->setValue((playTurnsTotal_ - playTurnsRemaining_) * 100
                                        / playTurnsTotal_);
            if (!game_->isGameOver() && playTurnsRemaining_ > 0) {
                onPlayNegamaxGo();
            } else {
                turnProgress_->hide();
                playTurnsTotal_ = 0;
            }
        }, Qt::QueuedConnection);
    }).detach();
}

void MainWindow::onPlayMctsGo() {
    if (!game_ || game_->isGameOver() || isSearching_ || stoneTimer_->isActive()) return;

    if (playTurnsRemaining_ <= 0) {
        playTurnsRemaining_ = playMctsTurnsSpin_->value();
        playTurnsTotal_     = playTurnsRemaining_;
        turnProgress_->setValue(0);
        turnProgress_->show();
    }

    int      seconds = playMctsSecCombo_->currentData().toInt();
    isSearching_ = true;
    startSearchIndicator(seconds);

    auto     searchGame = game_->clone();
    unsigned gen        = searchGen_;

    std::thread([this, gen, seconds, searchGame = std::move(searchGame)]() {
        AbsGame::MoveId mv = AbsGame::Searcher::mcts(*searchGame, seconds);
        QMetaObject::invokeMethod(this, [this, mv, gen]() {
            if (gen != searchGen_) return;
            isSearching_ = false;
            stopSearchIndicator();
            applyComputedMove(mv);
            --playTurnsRemaining_;
            if (playTurnsTotal_ > 0)
                turnProgress_->setValue((playTurnsTotal_ - playTurnsRemaining_) * 100
                                        / playTurnsTotal_);
            if (!game_->isGameOver() && playTurnsRemaining_ > 0) {
                onPlayMctsGo();
            } else {
                turnProgress_->hide();
                playTurnsTotal_ = 0;
            }
        }, Qt::QueuedConnection);
    }).detach();
}

void MainWindow::clearSuggestion() {
    suggestedLog_->clear();
    boardWidget_->clearSuggestion();
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void MainWindow::updateControls() {
    stopBtn_->setVisible(isSearching_);
    boardWidget_->setSearching(isSearching_);
    bool idle = !isSearching_;
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
    if (isSearching_)
        currentPlayerLabel_->setText("Thinking...");
    else
        currentPlayerLabel_->setText(bt ? "Black to move" : "White to move");
    blackPassBtn_->setEnabled( bt && !animating && !isSearching_);
    whitePassBtn_->setEnabled(!bt && !animating && !isSearching_);
}

void MainWindow::logLastMove() {
    if (!game_ || game_->moveHistory().empty()) return;
    const Move& m = game_->moveHistory().back();
    // After a pass, toMove() has already flipped, so the passer is the other player.
    QString clr = (m.nodeId < 0)
        ? (game_->toMove() == Player::White ? "B" : "W")
        : (m.color == Color::Black ? "B" : "W");
    QString text = (m.nodeId < 0)
        ? QString("%1: %2 PASS").arg(m.turn, 3).arg(clr)
        : QString("%1: %2 %3")
              .arg(m.turn, 3)
              .arg(clr)
              .arg(QString::fromStdString(game_->graph().node(m.nodeId).label));
    moveLog_->append(text);
}

// Copyright Ben Paul Wise. All Rights Reserved.
