// Copyright Ben Paul Wise. All Rights Reserved.
#include "MainWindow.h"
#include "AbsGame.h"
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
#include <thread>

// ── Static data ───────────────────────────────────────────────────────────────

static const struct { int secs; const char* label; } kMctsOptions[] = {
    {  1, "1 sec" }, {  2, "2 sec" }, {  5, "5 sec" },
    {  10, "10 sec" }, {  30, "30 sec" }, {  60, "60 sec" },
};

// ── Constructor ───────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Mancala");

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

    turnProgress_ = new QProgressBar(boardArea);
    turnProgress_->setRange(0, 100);
    turnProgress_->setFixedHeight(14);
    turnProgress_->setTextVisible(false);
    { auto sp = turnProgress_->sizePolicy(); sp.setRetainSizeWhenHidden(true);
      turnProgress_->setSizePolicy(sp); }
    turnProgress_->hide();
    boardVBox->addWidget(turnProgress_);

    searchProgress_ = new QProgressBar(boardArea);
    searchProgress_->setRange(0, 100);
    searchProgress_->setFixedHeight(14);
    searchProgress_->setTextVisible(false);
    { auto sp = searchProgress_->sizePolicy(); sp.setRetainSizeWhenHidden(true);
      searchProgress_->setSizePolicy(sp); }
    searchProgress_->hide();
    boardVBox->addWidget(searchProgress_);

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
    { auto sp = stopBtn_->sizePolicy(); sp.setRetainSizeWhenHidden(true);
      stopBtn_->setSizePolicy(sp); }
    stopBtn_->hide();
    statusHBox->addWidget(statusLabel_, 1);
    statusHBox->addWidget(stopBtn_);
    pv->addWidget(statusRow);
    connect(stopBtn_, &QPushButton::clicked, this, &MainWindow::cancelSearch);

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
        auto* nmAction = new QAction("NegaMax", this);
        nmAction->setCheckable(true);
        playGroup->addAction(nmAction);
        playMenu->addAction(nmAction);

        auto* nmMenu  = new QMenu(this);
        auto* widget  = new QWidget;
        auto* vbox    = new QVBoxLayout(widget);
        vbox->setContentsMargins(8, 6, 8, 6);
        auto* form    = new QFormLayout;
        form->setSpacing(6);
        vbox->addLayout(form);

        playDepthSpin_ = new QSpinBox(widget);
        playDepthSpin_->setRange(1, 12);
        playDepthSpin_->setValue(6);
        form->addRow("Depth:", playDepthSpin_);

        playTurnsSpin_ = new QSpinBox(widget);
        playTurnsSpin_->setRange(1, 200);
        playTurnsSpin_->setValue(4);
        form->addRow("Turns:", playTurnsSpin_);

        auto* sep = new QFrame(widget);
        sep->setFrameShape(QFrame::HLine);
        vbox->addWidget(sep);

        auto* goBtn = new QPushButton("Go!", widget);
        vbox->addWidget(goBtn);
        connect(goBtn, &QPushButton::clicked, nmMenu, &QMenu::hide);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onPlayNegamaxGo);

        auto* wa = new QWidgetAction(nmMenu);
        wa->setDefaultWidget(widget);
        nmMenu->addAction(wa);
        nmAction->setMenu(nmMenu);
        connect(nmMenu, &QMenu::aboutToShow, this, [nmAction]() { nmAction->setChecked(true); });
    }

    // MCTS submenu
    {
        auto* mctsAction = new QAction("MCTS", this);
        mctsAction->setCheckable(true);
        playGroup->addAction(mctsAction);
        playMenu->addAction(mctsAction);

        auto* mctsMenu = new QMenu(this);
        auto* widget   = new QWidget;
        auto* vbox2    = new QVBoxLayout(widget);
        vbox2->setContentsMargins(8, 6, 8, 6);
        auto* form2    = new QFormLayout;
        form2->setSpacing(6);
        vbox2->addLayout(form2);

        playMctsSecCombo_ = new QComboBox(widget);
        for (const auto& o : kMctsOptions)
            playMctsSecCombo_->addItem(o.label, o.secs);
        form2->addRow("Time:", playMctsSecCombo_);

        playMctsTurnsSpin_ = new QSpinBox(widget);
        playMctsTurnsSpin_->setRange(1, 200);
        playMctsTurnsSpin_->setValue(4);
        form2->addRow("Turns:", playMctsTurnsSpin_);

        auto* sep2 = new QFrame(widget);
        sep2->setFrameShape(QFrame::HLine);
        vbox2->addWidget(sep2);

        auto* goBtn = new QPushButton("Go!", widget);
        vbox2->addWidget(goBtn);
        connect(goBtn, &QPushButton::clicked, mctsMenu, &QMenu::hide);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onPlayMctsGo);

        auto* wa = new QWidgetAction(mctsMenu);
        wa->setDefaultWidget(widget);
        mctsMenu->addAction(wa);
        mctsAction->setMenu(mctsMenu);
        connect(mctsMenu, &QMenu::aboutToShow, this, [mctsAction]() { mctsAction->setChecked(true); });
    }

    // ── Suggest menu ───────────────────────────────────────────────────────────
    auto* suggestMenu  = menuBar()->addMenu("Suggest");
    auto* suggestGroup = new QActionGroup(this);
    suggestGroup->setExclusive(true);

    // NegaMax suggest
    {
        auto* nmAction = new QAction("NegaMax", this);
        nmAction->setCheckable(true);
        suggestGroup->addAction(nmAction);
        suggestMenu->addAction(nmAction);

        auto* nmMenu  = new QMenu(this);
        auto* widget  = new QWidget;
        auto* vbox    = new QVBoxLayout(widget);
        vbox->setContentsMargins(8, 6, 8, 6);
        auto* form    = new QFormLayout;
        form->setSpacing(6);
        vbox->addLayout(form);

        suggestDepthSpin_ = new QSpinBox(widget);
        suggestDepthSpin_->setRange(1, 12);
        suggestDepthSpin_->setValue(6);
        form->addRow("Depth:", suggestDepthSpin_);

        auto* sep = new QFrame(widget);
        sep->setFrameShape(QFrame::HLine);
        vbox->addWidget(sep);

        auto* goBtn = new QPushButton("Go!", widget);
        vbox->addWidget(goBtn);
        connect(goBtn, &QPushButton::clicked, nmMenu, &QMenu::hide);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onSuggestNegamaxGo);

        auto* wa = new QWidgetAction(nmMenu);
        wa->setDefaultWidget(widget);
        nmMenu->addAction(wa);
        nmAction->setMenu(nmMenu);
        connect(nmMenu, &QMenu::aboutToShow, this, [nmAction]() { nmAction->setChecked(true); });
    }

    // MCTS suggest
    {
        auto* mctsAction = new QAction("MCTS", this);
        mctsAction->setCheckable(true);
        suggestGroup->addAction(mctsAction);
        suggestMenu->addAction(mctsAction);

        auto* mctsMenu = new QMenu(this);
        auto* widget   = new QWidget;
        auto* vbox2    = new QVBoxLayout(widget);
        vbox2->setContentsMargins(8, 6, 8, 6);
        auto* form2    = new QFormLayout;
        form2->setSpacing(6);
        vbox2->addLayout(form2);

        suggestMctsSecCombo_ = new QComboBox(widget);
        for (const auto& o : kMctsOptions)
            suggestMctsSecCombo_->addItem(o.label, o.secs);
        form2->addRow("Time:", suggestMctsSecCombo_);

        auto* sep2 = new QFrame(widget);
        sep2->setFrameShape(QFrame::HLine);
        vbox2->addWidget(sep2);

        auto* goBtn = new QPushButton("Go!", widget);
        vbox2->addWidget(goBtn);
        connect(goBtn, &QPushButton::clicked, mctsMenu, &QMenu::hide);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onSuggestMctsGo);

        auto* wa = new QWidgetAction(mctsMenu);
        wa->setDefaultWidget(widget);
        mctsMenu->addAction(wa);
        mctsAction->setMenu(mctsMenu);
        connect(mctsMenu, &QMenu::aboutToShow, this, [mctsAction]() { mctsAction->setChecked(true); });
    }

    // Keep depth spinboxes in sync.
    connect(playDepthSpin_,    &QSpinBox::valueChanged,
            suggestDepthSpin_, &QSpinBox::setValue);
    connect(suggestDepthSpin_, &QSpinBox::valueChanged,
            playDepthSpin_,    &QSpinBox::setValue);
}

// ── Game control ──────────────────────────────────────────────────────────────

void MainWindow::onNewGame() {
    cancelSearch();
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
    if (!game_ || game_->isTerminal() || isSearching_) return;
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

// ── Search progress ───────────────────────────────────────────────────────────

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

// ── AI play ───────────────────────────────────────────────────────────────────

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
    if (!game_ || game_->isTerminal() || isSearching_) return;

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
            if (!game_->isTerminal() && playTurnsRemaining_ > 0)
                onPlayNegamaxGo();
            else {
                turnProgress_->hide();
                playTurnsTotal_ = 0;
            }
        }, Qt::QueuedConnection);
    }).detach();
}

void MainWindow::onPlayMctsGo() {
    if (!game_ || game_->isTerminal() || isSearching_) return;

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
            if (!game_->isTerminal() && playTurnsRemaining_ > 0)
                onPlayMctsGo();
            else {
                turnProgress_->hide();
                playTurnsTotal_ = 0;
            }
        }, Qt::QueuedConnection);
    }).detach();
}

void MainWindow::onSuggestNegamaxGo() {
    if (!game_ || game_->isTerminal() || isSearching_) return;

    isSearching_ = true;
    startSearchIndicator();

    int      depth      = suggestDepthSpin_->value();
    int      cp         = game_->currentPlayer();
    int      numPits    = game_->numPits();
    unsigned gen        = searchGen_;
    auto     searchGame = game_->clone();

    std::thread([this, gen, depth, cp, numPits,
                 searchGame = std::move(searchGame)]() {
        AbsGame::MoveId mv = AbsGame::Searcher::bestMove(*searchGame, depth, 10000);
        QMetaObject::invokeMethod(this, [this, mv, gen, cp, numPits]() {
            if (gen != searchGen_) return;
            isSearching_ = false;
            stopSearchIndicator();
            updateControls();
            if (mv >= 0) {
                int pitNum = (cp == 0) ? mv + 1 : mv - numPits;
                suggestedLog_->setText(
                    QString("P%1 pit %2").arg(cp).arg(pitNum));
                boardWidget_->setSuggestion(mv);
            } else {
                suggestedLog_->setText("(no move)");
            }
        }, Qt::QueuedConnection);
    }).detach();
}

void MainWindow::onSuggestMctsGo() {
    if (!game_ || game_->isTerminal() || isSearching_) return;

    isSearching_ = true;
    int      seconds    = suggestMctsSecCombo_->currentData().toInt();
    startSearchIndicator(seconds);

    int      cp         = game_->currentPlayer();
    int      numPits    = game_->numPits();
    unsigned gen        = searchGen_;
    auto     searchGame = game_->clone();

    std::thread([this, gen, seconds, cp, numPits,
                 searchGame = std::move(searchGame)]() {
        AbsGame::MoveId mv = AbsGame::Searcher::mcts(*searchGame, seconds);
        QMetaObject::invokeMethod(this, [this, mv, gen, cp, numPits]() {
            if (gen != searchGen_) return;
            isSearching_ = false;
            stopSearchIndicator();
            updateControls();
            if (mv >= 0) {
                int pitNum = (cp == 0) ? mv + 1 : mv - numPits;
                suggestedLog_->setText(
                    QString("P%1 pit %2").arg(cp).arg(pitNum));
                boardWidget_->setSuggestion(mv);
            } else {
                suggestedLog_->setText("(no move)");
            }
        }, Qt::QueuedConnection);
    }).detach();
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void MainWindow::updateControls() {
    stopBtn_->setVisible(isSearching_);
    boardWidget_->setSearching(isSearching_);
    bool idle = !isSearching_;
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
    if (isSearching_) {
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
