// Copyright Ben Paul Wise. All Rights Reserved.
#include "MainWindow.h"
#include "AbsGame.h"
#include "menu_helpers.h"
#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QWidgetAction>

// ── Static data ───────────────────────────────────────────────────────────────

static const guicommon::MctsOption kMctsOptions[] = {
    {  1, "1 sec" }, {  2, "2 sec" }, {  5, "5 sec" },
    {  10, "10 sec" }, {  30, "30 sec" }, {  60, "60 sec" },
};

// ── Constructor ───────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : guicommon::GameMainWindow(parent)
{
    setWindowTitle("Mancala");

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QHBoxLayout(central);

    // ── Board area ─────────────────────────────────────────────────────────────
    auto* boardArea = new QWidget(this);
    auto* boardVBox = new QVBoxLayout(boardArea);
    boardVBox->setContentsMargins(0, 0, 0, 0);
    boardVBox->setSpacing(4);

    boardWidget_ = new MancalaWidget(boardArea);
    boardVBox->addWidget(boardWidget_, 1);

    auto* turnBar = guicommon::makeStatusBar(boardArea);
    boardVBox->addWidget(turnBar);
    auto* searchBar = guicommon::makeStatusBar(boardArea);
    boardVBox->addWidget(searchBar);
    search().setProgressBars(searchBar, turnBar);

    root->addWidget(boardArea, 1);

    connect(boardWidget_, &MancalaWidget::moveRequested,
            this, &MainWindow::onMoveRequested);

    // ── Right panel ────────────────────────────────────────────────────────────
    auto* panel = new QWidget(this);
    panel->setFixedWidth(240);
    auto* pv = new QVBoxLayout(panel);
    pv->setAlignment(Qt::AlignTop);
    root->addWidget(panel);

    // Status row
    auto* statusRow  = new QWidget(this);
    auto* statusHBox = new QHBoxLayout(statusRow);
    statusHBox->setContentsMargins(0, 0, 0, 0);
    statusHBox->setSpacing(4);
    statusLabel_ = new QLabel("No game", statusRow);
    statusLabel_->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    stopBtn_ = new QPushButton("Stop", statusRow);
    stopBtn_->setStyleSheet("QPushButton { background-color: #FFCC99; color: black; }");
    stopBtn_->setFixedWidth(44);
    guicommon::retainSizeWhenHidden(stopBtn_);
    stopBtn_->hide();
    statusHBox->addWidget(statusLabel_, 1);
    statusHBox->addWidget(stopBtn_);
    pv->addWidget(statusRow);
    connect(stopBtn_, &QPushButton::clicked, this, [this]() { search().cancelSearch(); });

    pv->addSpacing(8);

    // Suggested move
    pv->addWidget(new QLabel("Suggested:", this));
    suggestedLog_ = new QTextEdit(this);
    suggestedLog_->setReadOnly(true);
    suggestedLog_->setFixedHeight(44);
    pv->addWidget(suggestedLog_);
    clearSuggestBtn_ = new QPushButton("Clear", this);
    pv->addWidget(clearSuggestBtn_);
    connect(clearSuggestBtn_, &QPushButton::clicked, this, [this]() {
        suggestedLog_->clear();
        boardWidget_->clearSuggestion();
    });

    pv->addSpacing(8);

    // Move log
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
    onNewGame();
}

// ── Menu bar ──────────────────────────────────────────────────────────────────

void MainWindow::buildMenuBar() {
    // ── Game menu ──────────────────────────────────────────────────────────────
    auto* gameMenu = menuBar()->addMenu("Game");

    // Stones per pit selector embedded in menu.
    auto* gmw  = new QWidget;
    gmw->setMinimumWidth(200);
    auto* gvbox = new QVBoxLayout(gmw);
    gvbox->setContentsMargins(8, 6, 8, 6);
    auto* gform = new QFormLayout;
    gform->setSpacing(6);
    gvbox->addLayout(gform);

    auto* pitsSpin = new QSpinBox(gmw);
    pitsSpin->setRange(3, 12);
    pitsSpin->setValue(6);
    gform->addRow("Pits/player:", pitsSpin);
    connect(pitsSpin, &QSpinBox::valueChanged, this, [this, pitsSpin](int) {
        numPits_ = pitsSpin->value();
    });

    auto* stoneSpin = new QSpinBox(gmw);
    stoneSpin->setRange(3, 12);
    stoneSpin->setValue(4);
    gform->addRow("Stones/pit:", stoneSpin);
    connect(stoneSpin, &QSpinBox::valueChanged, this, [this, stoneSpin](int) {
        stonesPerPit_ = stoneSpin->value();
    });

    auto* gsep = new QFrame(gmw);
    gsep->setFrameShape(QFrame::HLine);
    gvbox->addWidget(gsep);

    auto* newBtn = new QPushButton("New Game", gmw);
    gvbox->addWidget(newBtn);
    connect(newBtn, &QPushButton::clicked, this, &MainWindow::onNewGame);
    connect(newBtn, &QPushButton::clicked, gameMenu, &QWidget::close);

    auto* gwa = new QWidgetAction(gameMenu);
    gwa->setDefaultWidget(gmw);
    gameMenu->addAction(gwa);

    gameMenu->addSeparator();
    connect(gameMenu->addAction("Quit"), &QAction::triggered, this, &QWidget::close);

    // ── Play menu ──────────────────────────────────────────────────────────────
    auto* playMenu  = menuBar()->addMenu("Play");
    auto* playGroup = new QActionGroup(this);
    playGroup->setExclusive(true);

    manualAction_ = playMenu->addAction("Manual");
    manualAction_->setCheckable(true);
    manualAction_->setChecked(true);
    playGroup->addAction(manualAction_);

    // NegaMax submenu
    {
        guicommon::NegaMaxMenuConfig nm{1, 12, 6, true, 1, 200, 4};
        auto* goBtn = guicommon::buildNegaMaxMenu(this, playMenu, playGroup, nm,
                                                  playDepthSpin_, playTurnsSpin_);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onPlayNegamaxGo);
    }

    // MCTS submenu
    {
        guicommon::MctsMenuConfig mc{kMctsOptions, std::size(kMctsOptions), true, 1, 200, 4};
        auto* goBtn = guicommon::buildMctsMenu(this, playMenu, playGroup, mc,
                                               playMctsSecCombo_, playMctsTurnsSpin_);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onPlayMctsGo);
    }

    // ── Suggest menu ───────────────────────────────────────────────────────────
    auto* suggestMenu  = menuBar()->addMenu("Suggest");
    auto* suggestGroup = new QActionGroup(this);
    suggestGroup->setExclusive(true);

    // NegaMax suggest
    {
        guicommon::NegaMaxMenuConfig nm{1, 12, 6};
        nm.withTurns = false;
        QSpinBox* unusedTurns = nullptr;
        auto* goBtn = guicommon::buildNegaMaxMenu(this, suggestMenu, suggestGroup, nm,
                                                  suggestDepthSpin_, unusedTurns);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onSuggestNegamaxGo);
    }

    // MCTS suggest
    {
        guicommon::MctsMenuConfig mc{kMctsOptions, std::size(kMctsOptions)};
        mc.withTurns = false;
        QSpinBox* unusedTurns = nullptr;
        auto* goBtn = guicommon::buildMctsMenu(this, suggestMenu, suggestGroup, mc,
                                               suggestMctsSecCombo_, unusedTurns);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onSuggestMctsGo);
    }

    // Keep depth spinboxes in sync.
    connect(playDepthSpin_,    &QSpinBox::valueChanged,
            suggestDepthSpin_, &QSpinBox::setValue);
    connect(suggestDepthSpin_, &QSpinBox::valueChanged,
            playDepthSpin_,    &QSpinBox::setValue);
}

// ── Game control ──────────────────────────────────────────────────────────────

void MainWindow::onNewGame() {
    search().cancelSearch();
    suggestedLog_->clear();
    boardWidget_->clearSuggestion();
    moveLog_->clear();
    game_ = std::make_unique<Mancala::Game>(numPits_, stonesPerPit_);
    boardWidget_->setGame(game_.get());
    updateWindowSize();
    updateControls();
}

void MainWindow::updateWindowSize() {
    boardWidget_->setFixedSize(boardWidget_->preferredSize());
    // Defer one tick so the layout chain recalculates before we snap the
    // window to fit.  The window itself remains freely user-resizable.
    QTimer::singleShot(0, this, [this]() { adjustSize(); });
}

void MainWindow::onMoveRequested(int pitIndex) {
    if (!game_ || game_->isTerminal() || search().isSearching()) return;
    if (!game_->isLegalMove(pitIndex)) return;

    int player = game_->currentPlayer();
    game_->applyMove(pitIndex);
    boardWidget_->setLastMove(pitIndex);
    boardWidget_->clearSuggestion();
    suggestedLog_->clear();
    boardWidget_->update();
    logMove(pitIndex, player);
    updateControls();
}

// ── AI play ───────────────────────────────────────────────────────────────────

AbsGame::Game* MainWindow::currentGame() {
    return game_.get();
}

void MainWindow::applyComputedMove(AbsGame::MoveId mv) {
    if (!game_ || game_->isTerminal()) return;
    if (!game_->isLegalMove(mv)) return;

    int player = game_->currentPlayer();
    boardWidget_->clearSuggestion();
    suggestedLog_->clear();
    game_->applyMove(mv);
    boardWidget_->setLastMove(mv);
    boardWidget_->update();
    logMove(mv, player);
    updateControls();
}

void MainWindow::onPlayNegamaxGo() {
    guicommon::SearchController::Params p;
    p.algo  = guicommon::SearchController::Algorithm::NegaMax;
    p.depth = playDepthSpin_->value();
    startPlay(p, playTurnsSpin_->value());
}

void MainWindow::onPlayMctsGo() {
    guicommon::SearchController::Params p;
    p.algo    = guicommon::SearchController::Algorithm::Mcts;
    p.seconds = playMctsSecCombo_->currentData().toInt();
    startPlay(p, playMctsTurnsSpin_->value());
}

void MainWindow::onSuggestNegamaxGo() {
    if (!game_ || game_->isTerminal() || search().isSearching()) return;

    int cp      = game_->currentPlayer();
    int numPits = game_->numPits();

    guicommon::SearchController::Params p;
    p.algo  = guicommon::SearchController::Algorithm::NegaMax;
    p.depth = suggestDepthSpin_->value();
    search().launch(game_->clone(), p, [this, cp, numPits](AbsGame::MoveId mv, unsigned) {
        if (mv >= 0) {
            int pitNum = (cp == 0) ? mv + 1 : mv - numPits;
            suggestedLog_->setText(QString("P%1 pit %2").arg(cp).arg(pitNum));
            boardWidget_->setSuggestion(mv);
        } else {
            suggestedLog_->setText("(no move)");
        }
    });
}

void MainWindow::onSuggestMctsGo() {
    if (!game_ || game_->isTerminal() || search().isSearching()) return;

    int cp      = game_->currentPlayer();
    int numPits = game_->numPits();

    guicommon::SearchController::Params p;
    p.algo    = guicommon::SearchController::Algorithm::Mcts;
    p.seconds = suggestMctsSecCombo_->currentData().toInt();
    search().launch(game_->clone(), p, [this, cp, numPits](AbsGame::MoveId mv, unsigned) {
        if (mv >= 0) {
            int pitNum = (cp == 0) ? mv + 1 : mv - numPits;
            suggestedLog_->setText(QString("P%1 pit %2").arg(cp).arg(pitNum));
            boardWidget_->setSuggestion(mv);
        } else {
            suggestedLog_->setText("(no move)");
        }
    });
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void MainWindow::updateControls() {
    bool searching = search().isSearching();
    stopBtn_->setVisible(searching);
    boardWidget_->setSearching(searching);
    bool idle = !searching;
    menuBar()->setEnabled(idle);
    clearSuggestBtn_->setEnabled(idle);

    if (!game_) {
        statusLabel_->setText("No game");
        return;
    }
    if (game_->isTerminal()) {
        int s0 = game_->storeOf(0), s1 = game_->storeOf(1);
        statusLabel_->setText(
            s0 > s1 ? "P0 wins!" : s1 > s0 ? "P1 wins!" : "Draw!");
        return;
    }
    if (searching) {
        statusLabel_->setText("Thinking...");
    } else {
        int cp = game_->currentPlayer();
        statusLabel_->setText(
            QString("P%1  (%2–%3)")
            .arg(cp)
            .arg(game_->storeOf(0))
            .arg(game_->storeOf(1)));
    }
}

void MainWindow::logMove(int pitIndex, int player) {
    if (!game_) return;
    int pitNum = (player == 0) ? pitIndex + 1 : pitIndex - game_->numPits();
    QString extra = game_->isExtraTurnPending() ? " +turn" : "";
    moveLog_->append(
        QString("P%1 pit %2%3  (%4–%5)")
        .arg(player)
        .arg(pitNum)
        .arg(extra)
        .arg(game_->storeOf(0))
        .arg(game_->storeOf(1)));
}
// Copyright Ben Paul Wise. All Rights Reserved.
