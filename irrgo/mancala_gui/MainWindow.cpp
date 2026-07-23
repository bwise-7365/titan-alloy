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

// Iterative-deepening ceiling for NegaMax. The clock is what actually stops the search;
// this only has to be past any depth the budget could reach, so it never binds.
static constexpr int kMaxNegamaxDepth = 64;

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
    connect(stopBtn_, &QPushButton::clicked, this, [this]() {
        search().cancelSearch();
        endVersus();                     // Stop drops a versus game back to manual play
        manualAction_->setChecked(true);  // reflect it in the Play menu
    });

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
    // Selecting Manual returns both sides to the mouse: leave any versus game.
    connect(manualAction_, &QAction::triggered, this, [this]() {
        endVersus();
        search().cancelSearch();
    });

    // NegaMax submenu
    {
        // Same time choices as MCTS: both are wall-clock budgets now.
        guicommon::TimeMenuConfig nm{kMctsOptions, std::size(kMctsOptions), true, 1, 200, 4};
        auto* goBtn = guicommon::buildNegaMaxTimeMenu(this, playMenu, playGroup, nm,
                                                      playNegamaxSecCombo_, playTurnsSpin_);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onPlayNegamaxGo);
    }

    // MCTS submenu
    {
        guicommon::MctsMenuConfig mc{kMctsOptions, std::size(kMctsOptions), true, 1, 200, 4};
        auto* goBtn = guicommon::buildMctsMenu(this, playMenu, playGroup, mc,
                                               playMctsSecCombo_, playMctsTurnsSpin_);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onPlayMctsGo);
    }

    // Computer submenu (human vs computer)
    {
        // The same NegaMax time choices, plus which side (P0/P1) the human takes; the
        // computer plays the other and answers every turn.
        guicommon::ComputerMenuConfig cc{kMctsOptions, std::size(kMctsOptions), "P0", "P1", 0};
        auto* goBtn = guicommon::buildComputerMenu(this, playMenu, playGroup, cc,
                                                   playComputerSecCombo_, playComputerSideCombo_);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onPlayComputerGo);
    }

    // ── Suggest menu ───────────────────────────────────────────────────────────
    auto* suggestMenu  = menuBar()->addMenu("Suggest");
    auto* suggestGroup = new QActionGroup(this);
    suggestGroup->setExclusive(true);

    // NegaMax suggest
    {
        guicommon::TimeMenuConfig nm{kMctsOptions, std::size(kMctsOptions)};
        nm.withTurns = false;
        QSpinBox* unusedTurns = nullptr;
        auto* goBtn = guicommon::buildNegaMaxTimeMenu(this, suggestMenu, suggestGroup, nm,
                                                      suggestNegamaxSecCombo_, unusedTurns);
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

    // Keep the NegaMax time combos in sync.
    connect(playNegamaxSecCombo_,    &QComboBox::currentIndexChanged,
            suggestNegamaxSecCombo_, &QComboBox::setCurrentIndex);
    connect(suggestNegamaxSecCombo_, &QComboBox::currentIndexChanged,
            playNegamaxSecCombo_,    &QComboBox::setCurrentIndex);

    // ── About ─────────────────────────────────────────────────────────────────
    auto* aboutMenu = menuBar()->addMenu("About");
    connect(aboutMenu->addAction("About Mancala"), &QAction::triggered, this, [this]() {
        guicommon::showAboutDialog(this, "Mancala");
    });
}

// ── Game control ──────────────────────────────────────────────────────────────

void MainWindow::onNewGame() {
    search().cancelSearch();
    endVersus();  // a fresh board leaves any human-vs-computer game
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
    maybeComputerMove();  // in Computer mode, let the computer answer this move
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
    endVersus();  // auto-play drives both sides; drop any human-vs-computer game
    guicommon::SearchController::Params p;
    p.algo          = guicommon::SearchController::Algorithm::NegaMax;
    p.depth         = kMaxNegamaxDepth;
    p.negamaxTimeMs = playNegamaxSecCombo_->currentData().toInt() * 1000;
    // The clock bounds this search, so the search bar shows the elapsed fraction.
    p.negamaxTimeBudgeted = true;
    startPlay(p, playTurnsSpin_->value());
}

void MainWindow::onPlayMctsGo() {
    endVersus();  // auto-play drives both sides; drop any human-vs-computer game
    guicommon::SearchController::Params p;
    p.algo    = guicommon::SearchController::Algorithm::Mcts;
    p.seconds = playMctsSecCombo_->currentData().toInt();
    startPlay(p, playMctsTurnsSpin_->value());
}

// Enter human-vs-computer mode: the human takes the side chosen in the Computer submenu and
// the computer answers each of its turns with a NegaMax search at the chosen think time.
void MainWindow::onPlayComputerGo() {
    if (!game_ || game_->isTerminal()) {
        return;
    }
    guicommon::SearchController::Params p;
    p.algo          = guicommon::SearchController::Algorithm::NegaMax;
    p.depth         = kMaxNegamaxDepth;
    p.negamaxTimeMs = playComputerSecCombo_->currentData().toInt() * 1000;
    p.negamaxTimeBudgeted = true;
    const int humanSide = playComputerSideCombo_->currentData().toInt();
    beginVersus(p, humanSide);  // if the computer holds the opening move, it starts now
}

void MainWindow::onSuggestNegamaxGo() {
    if (!game_ || game_->isTerminal() || search().isSearching()) return;

    int cp      = game_->currentPlayer();
    int numPits = game_->numPits();

    guicommon::SearchController::Params p;
    p.algo          = guicommon::SearchController::Algorithm::NegaMax;
    p.depth         = kMaxNegamaxDepth;
    p.negamaxTimeMs = suggestNegamaxSecCombo_->currentData().toInt() * 1000;
    p.negamaxTimeBudgeted = true;  // as in onPlayNegamaxGo: the clock bounds this search
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
