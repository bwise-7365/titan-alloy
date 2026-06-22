// Copyright Ben Paul Wise. All Rights Reserved.
#include "MainWindow.h"

#include "board_params.h"    // BoardParams, stones_per_side, kMin/MaxRowsCols
#include "irregular_grid.h"  // square_to_notation
#include "menu_helpers.h"

#include <QAction>
#include <QActionGroup>
#include <QColorDialog>
#include <QComboBox>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QTimer>
//#include <QVBoxLayout>
#include <QWidget>
#include <QWidgetAction>
#include <iterator>
//#include <stdexcept>

namespace gb = games::board;

// ── Static data ───────────────────────────────────────────────────────────────

static const guicommon::MctsOption kMctsOptions[] = {
    {  5, "5 sec"  }, { 15, "15 sec" },
    { 30, "30 sec" }, { 60, "60 sec" },
    { 90, "90 sec" }, { 120, "120 sec" },
};

// ── Constructor ───────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent) : guicommon::GameMainWindow(parent) {
    setWindowTitle("Ludus Latrunculorum");

    seedTimer_ = new QTimer(this);
    connect(seedTimer_, &QTimer::timeout, this, &MainWindow::onSeedTick);
    seedRng_.seed(AbsGame::makeSeed(0));  // clock-derived: a different layout each run

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* outer = new QVBoxLayout(central);

    // Banner.
    bannerLabel_ = new QLabel("Ludus Latrunculorum", central);
    bannerLabel_->setAlignment(Qt::AlignCenter);
    {
        QFont f = bannerLabel_->font();
        f.setPointSize(24);
        f.setBold(true);
        bannerLabel_->setFont(f);
    }
    outer->addWidget(bannerLabel_);

    auto* row = new QWidget(central);
    auto* rowH = new QHBoxLayout(row);
    rowH->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(row, 1);

    // Board area (board + the two shared progress bars).
    auto* boardArea = new QWidget(row);
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
    rowH->addWidget(boardArea, 1);
    connect(boardWidget_, &BoardWidget::moveRequested,
            this, &MainWindow::onMoveRequested);

    // Right panel.
    auto* panel = new QWidget(row);
    panel->setFixedWidth(280);  // right column width
    auto* pv = new QVBoxLayout(panel);
    pv->setAlignment(Qt::AlignTop);
    rowH->addWidget(panel);

    auto* statusRow  = new QWidget(panel);
    auto* statusHBox = new QHBoxLayout(statusRow);
    statusHBox->setContentsMargins(0, 0, 0, 0);
    statusHBox->setSpacing(4);
    statusLabel_ = new QLabel("No game", statusRow);
    stopBtn_ = new QPushButton("Stop", statusRow);
    stopBtn_->setStyleSheet("QPushButton { background-color: #FFCC99; color: black; }");
    stopBtn_->setFixedWidth(44);
    guicommon::retainSizeWhenHidden(stopBtn_);
    stopBtn_->hide();
    statusHBox->addWidget(statusLabel_, 1);
    statusHBox->addWidget(stopBtn_);
    pv->addWidget(statusRow);
    connect(stopBtn_, &QPushButton::clicked, this, [this]() { search().cancelSearch(); });

    tallyLabel_ = new QLabel(panel);
    tallyLabel_->setTextFormat(Qt::PlainText);
    pv->addWidget(tallyLabel_);

    pv->addSpacing(8);
    pv->addWidget(new QLabel("Suggested:", panel));
    suggestedLog_ = new QTextEdit(panel);
    suggestedLog_->setReadOnly(true);
    suggestedLog_->setFixedHeight(48);
    pv->addWidget(suggestedLog_);
    clearSuggestBtn_ = new QPushButton("Clear", panel);
    pv->addWidget(clearSuggestBtn_);
    connect(clearSuggestBtn_, &QPushButton::clicked, this, [this]() {
        suggestedLog_->clear();
        boardWidget_->clearSuggestion();
    });

    pv->addSpacing(8);
    pv->addWidget(new QLabel("Move log:", panel));
    moveLog_ = new QTextEdit(panel);
    moveLog_->setReadOnly(true);
    {
        QFont font("Monospace");
        font.setStyleHint(QFont::TypeWriter);
        moveLog_->setFont(font);
    }
    pv->addWidget(moveLog_, 1);

    buildMenuBar();
    resize(1040, 720);

    const int perSide = gb::stones_per_side(gb::BoardParams{6, 8});
    newGame(6, 8, perSide);
}

void MainWindow::setBannerFont(const QString& family) {
    if (family.isEmpty()) {
        return;
    }
    QFont f(family);
    f.setPointSize(26);
    f.setBold(true);
    bannerLabel_->setFont(f);
}

// ── Menu bar ──────────────────────────────────────────────────────────────────

void MainWindow::buildMenuBar() {
    // File.
    auto* fileMenu = menuBar()->addMenu("File");
    connect(fileMenu->addAction("New Game"), &QAction::triggered, this, &MainWindow::onNewGame);
    fileMenu->addSeparator();
    connect(fileMenu->addAction("Save..."), &QAction::triggered, this, &MainWindow::onSave);
    connect(fileMenu->addAction("Load..."), &QAction::triggered, this, &MainWindow::onLoad);
    fileMenu->addSeparator();
    connect(fileMenu->addAction("Quit"), &QAction::triggered, this, &QWidget::close);

    // Board.
    auto* boardMenu = menuBar()->addMenu("Board");
    auto* bmw = new QWidget;
    bmw->setMinimumWidth(220);
    auto* vbox = new QVBoxLayout(bmw);
    vbox->setContentsMargins(8, 6, 8, 6);
    auto* form = new QFormLayout;
    form->setSpacing(6);
    vbox->addLayout(form);

    rowsSpin_ = new QSpinBox(bmw);
    rowsSpin_->setRange(gb::kMinRowsCols, gb::kMaxRowsCols);
    rowsSpin_->setValue(6);
    form->addRow("Rows:", rowsSpin_);

    colsSpin_ = new QSpinBox(bmw);
    colsSpin_->setRange(gb::kMinRowsCols, gb::kMaxRowsCols);
    colsSpin_->setValue(8);
    form->addRow("Columns:", colsSpin_);

    perSideSpin_ = new QSpinBox(bmw);
    perSideSpin_->setRange(1, (gb::kMaxRowsCols * gb::kMaxRowsCols) / 2);
    perSideSpin_->setValue(gb::stones_per_side(gb::BoardParams{6, 8}));
    form->addRow("Discs/side:", perSideSpin_);

    // When the board size changes, cap the per-side maximum to the area and reset the
    // count to the default fraction (kDefaultStoneFraction = 20/64) of the new area.
    auto syncPerSide = [this]() {
        const int r = rowsSpin_->value();
        const int c = colsSpin_->value();
        perSideSpin_->setMaximum((r * c) / 2);
        perSideSpin_->setValue(gb::stones_per_side(gb::BoardParams{r, c}));
    };
    connect(rowsSpin_, &QSpinBox::valueChanged, this, [syncPerSide](int) { syncPerSide(); });
    connect(colsSpin_, &QSpinBox::valueChanged, this, [syncPerSide](int) { syncPerSide(); });

    auto* colorABtn = new QPushButton("Side A colour...", bmw);
    auto* colorBBtn = new QPushButton("Side B colour...", bmw);
    auto* bgBtn     = new QPushButton("Background colour...", bmw);
    vbox->addWidget(colorABtn);
    vbox->addWidget(colorBBtn);
    vbox->addWidget(bgBtn);
    connect(colorABtn, &QPushButton::clicked, this, &MainWindow::onPickColorA);
    connect(colorBBtn, &QPushButton::clicked, this, &MainWindow::onPickColorB);
    connect(bgBtn,     &QPushButton::clicked, this, &MainWindow::onPickBackground);

    auto* sep = new QFrame(bmw);
    sep->setFrameShape(QFrame::HLine);
    vbox->addWidget(sep);
    auto* genBtn = new QPushButton("Generate", bmw);
    vbox->addWidget(genBtn);
    connect(genBtn, &QPushButton::clicked, this, &MainWindow::onNewGame);
    connect(genBtn, &QPushButton::clicked, boardMenu, &QWidget::close);

    auto* bwa = new QWidgetAction(boardMenu);
    bwa->setDefaultWidget(bmw);
    boardMenu->addAction(bwa);

    // Setup: seed the placement phase with random no-capture placements. Each
    // option pre-places that percentage of EACH side's quota, one disc at a time.
    // Plain one-shot actions (clicking re-seeds a fresh game); the chosen percentage
    // is carried as the action's data.
    auto* setupMenu = menuBar()->addMenu("Setup");
    static const struct { int pct; const char* label; } kSeedOptions[] = {
        {0, "0%"}, {25, "25%"}, {50, "50%"}, {75, "75%"}, {100, "100%"},
    };
    for (const auto& o : kSeedOptions) {
        setupMenu->addAction(o.label)->setData(o.pct);
    }
    connect(setupMenu, &QMenu::triggered, this, &MainWindow::onSeedSelected);

    // Play.
    auto* playMenu  = menuBar()->addMenu("Play");
    auto* playGroup = new QActionGroup(this);
    playGroup->setExclusive(true);
    manualAction_ = playMenu->addAction("Manual");
    manualAction_->setCheckable(true);
    manualAction_->setChecked(true);
    playGroup->addAction(manualAction_);
    {
        guicommon::NegaMaxMenuConfig nm{1, 8, 4, true, 1, 200, 4};
        auto* goBtn = guicommon::buildNegaMaxMenu(this, playMenu, playGroup, nm,
                                                  playDepthSpin_, playTurnsSpin_);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onPlayNegamaxGo);
    }
    {
        guicommon::MctsMenuConfig mc{kMctsOptions, std::size(kMctsOptions), true, 1, 200, 4};
        auto* goBtn = guicommon::buildMctsMenu(this, playMenu, playGroup, mc,
                                               playMctsSecCombo_, playMctsTurnsSpin_);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onPlayMctsGo);
    }

    // Suggest.
    auto* suggestMenu  = menuBar()->addMenu("Suggest");
    auto* suggestGroup = new QActionGroup(this);
    suggestGroup->setExclusive(true);
    {
        guicommon::NegaMaxMenuConfig nm{1, 8, 4};
        nm.withTurns = false;
        QSpinBox* unusedTurns = nullptr;
        auto* goBtn = guicommon::buildNegaMaxMenu(this, suggestMenu, suggestGroup, nm,
                                                  suggestDepthSpin_, unusedTurns);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onSuggestNegamaxGo);
    }
    {
        guicommon::MctsMenuConfig mc{kMctsOptions, std::size(kMctsOptions)};
        mc.withTurns = false;
        QSpinBox* unusedTurns = nullptr;
        auto* goBtn = guicommon::buildMctsMenu(this, suggestMenu, suggestGroup, mc,
                                               suggestMctsSecCombo_, unusedTurns);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onSuggestMctsGo);
    }

    // Keep the Play and Suggest depth spinboxes in sync.
    connect(playDepthSpin_,    &QSpinBox::valueChanged, suggestDepthSpin_, &QSpinBox::setValue);
    connect(suggestDepthSpin_, &QSpinBox::valueChanged, playDepthSpin_,    &QSpinBox::setValue);
}

// ── Game control ──────────────────────────────────────────────────────────────

void MainWindow::newGame(int rows, int columns, int perSide) {
    stopSeed();
    if (2 * perSide > rows * columns) {
        QMessageBox::warning(this, "Latrunculi",
            QString("Too many discs: 2 x %1 must be <= %2 x %3 squares.")
                .arg(perSide).arg(rows).arg(columns));
        return;
    }
    try {
        game_ = std::make_unique<Latrunculi::Game>(rows, columns, perSide);
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, "Latrunculi", ex.what());
        return;
    }
    search().cancelSearch();
    currentFilePath_.clear();
    moveLog_->clear();
    suggestedLog_->clear();
    // Repoint the board at the new game BEFORE any colour rebuild. The make_unique
    // above freed the previous game, leaving the widget's observer pointer dangling
    // until setGame() updates it; setSideColors/setBackgroundColor each rebuild, so
    // doing them first would dereference the freed game.
    boardWidget_->setGame(game_.get());
    boardWidget_->setSideColors(colorA_, colorB_);
    boardWidget_->setBackgroundColor(background_);
    updateControls();
}

void MainWindow::onNewGame() {
    newGame(rowsSpin_->value(), colsSpin_->value(), perSideSpin_->value());
}

// ── Placement seeding (analogous to IrrGo's stone setup) ──────────────────────

bool MainWindow::seeding() const {
    return seedTimer_ != nullptr && seedTimer_->isActive();
}

void MainWindow::stopSeed() {
    if (seedTimer_ != nullptr) {
        seedTimer_->stop();
    }
}

bool MainWindow::extraSearchBlock() const {
    return seeding();  // don't let a search start while the seed animation runs
}

void MainWindow::onSeedSelected(QAction* action) {
    const int pct = action->data().toInt();
    // Start a fresh game at the current board settings, then seed it. newGame()
    // calls stopSeed(), so any in-progress seeding is cancelled first.
    newGame(rowsSpin_->value(), colsSpin_->value(), perSideSpin_->value());
    if (!game_) {
        return;  // invalid board size; newGame already reported it
    }
    const int eachSide = (game_->perSide() * pct + 50) / 100;  // rounded
    seedTarget_ = 2 * eachSide;  // both sides, placed alternately
    seedPlaced_ = 0;
    if (seedTarget_ <= 0) {
        return;  // 0% -> leave the empty board
    }
    // <= 20 discs: one every 500 ms; more: pace so all land within 10 seconds.
    const int interval = (seedTarget_ <= 20) ? 500 : (10000 / seedTarget_);
    seedTimer_->setInterval(interval);
    seedTimer_->start();
    updateControls();
}

void MainWindow::onSeedTick() {
    if (!game_ || seedPlaced_ >= seedTarget_ ||
        game_->phase() != Latrunculi::Phase::Placement) {
        stopSeed();
        updateControls();
        return;
    }
    const std::vector<AbsGame::MoveId> moves = game_->getLegalMoves();
    if (moves.empty()) {
        stopSeed();  // no legal no-capture square left (rare); stop early
        updateControls();
        return;
    }
    const AbsGame::MoveId mv = moves[seedRng_() % moves.size()];
    game_->applyMove(mv);
    if (!game_->history().empty()) {
        logMove(game_->history().back());
    }
    ++seedPlaced_;
    if (seedPlaced_ >= seedTarget_) {
        stopSeed();
    }
    refreshBoard();
}

void MainWindow::onMoveRequested(AbsGame::MoveId mv) {
    if (!game_ || game_->isTerminal() || search().isSearching() || seeding()) {
        return;
    }
    if (!game_->isLegalMove(mv)) {
        return;
    }
    game_->applyMove(mv);
    if (!game_->history().empty()) {
        logMove(game_->history().back());
    }
    suggestedLog_->clear();
    refreshBoard();
}

// ── GameMainWindow hooks ──────────────────────────────────────────────────────

AbsGame::Game* MainWindow::currentGame() {
    return game_.get();
}

void MainWindow::applyComputedMove(AbsGame::MoveId mv) {
    if (!game_ || game_->isTerminal() || !game_->isLegalMove(mv)) {
        return;
    }
    game_->applyMove(mv);
    if (!game_->history().empty()) {
        logMove(game_->history().back());
    }
    suggestedLog_->clear();
    refreshBoard();
}

// ── AI play / suggest ─────────────────────────────────────────────────────────

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
    if (!game_ || game_->isTerminal() || search().isSearching() || seeding()) {
        return;
    }
    guicommon::SearchController::Params p;
    p.algo  = guicommon::SearchController::Algorithm::NegaMax;
    p.depth = suggestDepthSpin_->value();
    search().launch(game_->clone(), p, [this](AbsGame::MoveId mv, unsigned) {
        if (mv < 0) {
            suggestedLog_->setText("(no move)");
            return;
        }
        suggestedLog_->setText(describeMoveId(mv));
        boardWidget_->setSuggestion(mv);
    });
}

void MainWindow::onSuggestMctsGo() {
    if (!game_ || game_->isTerminal() || search().isSearching() || seeding()) {
        return;
    }
    guicommon::SearchController::Params p;
    p.algo    = guicommon::SearchController::Algorithm::Mcts;
    p.seconds = suggestMctsSecCombo_->currentData().toInt();
    search().launch(game_->clone(), p, [this](AbsGame::MoveId mv, unsigned) {
        if (mv < 0) {
            suggestedLog_->setText("(no move)");
            return;
        }
        suggestedLog_->setText(describeMoveId(mv));
        boardWidget_->setSuggestion(mv);
    });
}

void MainWindow::onPickColorA() {
    const QColor c = QColorDialog::getColor(colorA_, this, "Side A colour");
    if (c.isValid()) {
        colorA_ = c;
        boardWidget_->setSideColors(colorA_, colorB_);
    }
}

void MainWindow::onPickColorB() {
    const QColor c = QColorDialog::getColor(colorB_, this, "Side B colour");
    if (c.isValid()) {
        colorB_ = c;
        boardWidget_->setSideColors(colorA_, colorB_);
    }
}

void MainWindow::onPickBackground() {
    const QColor c = QColorDialog::getColor(background_, this, "Background colour");
    if (c.isValid()) {
        background_ = c;
        boardWidget_->setBackgroundColor(background_);
    }
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void MainWindow::refreshBoard() {
    boardWidget_->setGame(game_.get());  // rebuilds the SVG and resets selection
    updateControls();
}

void MainWindow::updateControls() {
    const bool searching = search().isSearching();
    stopBtn_->setVisible(searching);
    boardWidget_->setSearching(searching);
    menuBar()->setEnabled(!searching);
    clearSuggestBtn_->setEnabled(!searching);

    if (!game_) {
        statusLabel_->setText("No game");
        tallyLabel_->clear();
        return;
    }
    tallyLabel_->setText(
        QString("A: %1  (%2 free, %3 bound)\nB: %4  (%5 free, %6 bound)")
            .arg(game_->totalDiscs(0)).arg(game_->freeDiscs(0)).arg(game_->boundDiscs(0))
            .arg(game_->totalDiscs(1)).arg(game_->freeDiscs(1)).arg(game_->boundDiscs(1)));

    if (game_->isOver()) {
        const int w = game_->winner();
        statusLabel_->setText(w == 0 ? "A wins" : (w == 1 ? "B wins" : "Draw"));
        return;
    }
    if (searching) {
        statusLabel_->setText("Thinking...");
        return;
    }
    if (seeding()) {
        statusLabel_->setText("Seeding...");
        return;
    }
    const int cur = game_->currentPlayer();
    const QString who = (cur == 0) ? "A" : "B";
    if (game_->phase() == Latrunculi::Phase::Placement) {
        statusLabel_->setText(who + " to place");
    } else if (game_->boundDiscs(1 - cur) > 0) {
        // Mandatory: a captured enemy disc must be removed before moving.
        statusLabel_->setText(who + ": remove a captive, then move");
    } else {
        statusLabel_->setText(who + " to move");
    }
}

QString MainWindow::notate(int square) const {
    if (!game_) {
        return QString::number(square);
    }
    return QString::fromStdString(
        gb::square_to_notation(square, game_->rows(), game_->columns()));
}

QString MainWindow::moveDescription(const Latrunculi::Move& m) const {
    const char side = (m.player == 0) ? 'A' : 'B';
    QString s = QString("%1. %2: ").arg(m.turn).arg(side);
    if (m.from < 0) {
        s += "place " + notate(m.to);
        return s;
    }
    if (m.path.empty()) {
        s += notate(m.from) + " -> " + notate(m.to);
    } else {
        bool first = true;
        for (int sq : m.path) {
            if (!first) {
                s += " -> ";
            }
            first = false;
            s += notate(sq);
        }
    }
    if (m.removed >= 0) {
        s += "  (remove " + notate(m.removed) + ")";
    }
    return s;
}

QString MainWindow::describeMoveId(AbsGame::MoveId mv) const {
    if (!game_ || mv < 0) {
        return "(pass)";
    }
    if (game_->phase() == Latrunculi::Phase::Placement) {
        return "place " + notate(mv);
    }
    int rem = -1, from = -1, to = -1;
    game_->decodeMovement(mv, rem, from, to);
    QString s = notate(from) + " -> " + notate(to);
    if (rem >= 0) {
        s += "  (remove " + notate(rem) + ")";
    }
    return s;
}

void MainWindow::logMove(const Latrunculi::Move& m) {
    moveLog_->append(moveDescription(m));
}
// Copyright Ben Paul Wise. All Rights Reserved.
