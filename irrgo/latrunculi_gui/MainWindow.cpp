// Copyright Ben Paul Wise. All Rights Reserved.
#include "MainWindow.h"

#include "MoveListWidget.h"
#include "PlaybackBar.h"
#include "board_params.h"    // BoardParams, stones_per_side, kMin/MaxRowsCols
#include "irregular_grid.h"  // square_to_notation
#include "menu_helpers.h"

#include <QAction>
#include <QActionGroup>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QFile>
#include <QFont>
#include <QFontMetrics>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QStringList>
#include <QTextBrowser>
#include <QTextEdit>
#include <QTimer>
//#include <QVBoxLayout>
#include <QWidget>
#include <QWidgetAction>
#include <algorithm>
#include <iterator>
//#include <stdexcept>

namespace gb = games::board;

// ── Static data ───────────────────────────────────────────────────────────────

// Ceiling for iterative deepening. The wall-clock budget from the NegaMax menu is what
// actually stops the search; this only bounds the loop if a position is so shallow that
// every depth completes, which cannot happen on a real board.
static constexpr int kMaxNegamaxDepth = 64;

static const guicommon::TimeOption kMctsOptions[] = {
    {  5, "5 sec"  },
    {  10, "10 sec"  },{ 15, "15 sec" },
    { 30, "30 sec" }, { 60, "60 sec" },
    { 90, "90 sec" }, { 120, "2 min" },
    { 240, "4 min" }, { 480, "8 min" },
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
        f.setPointSize(latgui::kBannerPointSize);
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
    // The right column must be fairly wide because
    // a move might have several leaps and a capture.
    panel->setFixedWidth(latgui::kPanelWidth);  // right column width
    auto* pv = new QVBoxLayout(panel);
    pv->setAlignment(Qt::AlignTop);
    rowH->addWidget(panel);

    auto* statusRow  = new QWidget(panel);
    auto* statusHBox = new QHBoxLayout(statusRow);
    statusHBox->setContentsMargins(0, 0, 0, 0);
    statusHBox->setSpacing(4);
    statusLabel_ = new QLabel("No game", statusRow);
    stopBtn_ = new QPushButton("Stop", statusRow);
    stopBtn_->setStyleSheet(QString("QPushButton { background-color: %1; color: black; }")
                                .arg(latgui::kStopButtonBg.name()));
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

    // Per-side tallies, each led by a color swatch (~ the size of a capital "A")
    // that identifies the side; the counts sit two em-spaces to the right of it.
    auto* tallyWidget = new QWidget(panel);
    auto* tallyGrid   = new QGridLayout(tallyWidget);
    tallyGrid->setContentsMargins(0, 0, 0, 0);
    tallyGrid->setVerticalSpacing(2);
    {
        const QFontMetrics fm(tallyWidget->font());
        int em = fm.horizontalAdvance(QChar(0x2003));  // EM SPACE: advance == 1 em
        if (em <= 0) {
            em = fm.height();
        }
        tallyGrid->setHorizontalSpacing(0.5 * em);       // counts shifted right by 2 em
        int cap = 1.75 * fm.capHeight();                      // height of a capital "A"
        if (cap <= 0) {
            cap = fm.ascent();
        }
        swatchA_ = new QLabel(tallyWidget);
        swatchB_ = new QLabel(tallyWidget);
        swatchA_->setFixedSize(cap, cap);
        swatchB_->setFixedSize(cap, cap);
    }
    tallyA_ = new QLabel(tallyWidget);
    tallyB_ = new QLabel(tallyWidget);
    tallyA_->setTextFormat(Qt::PlainText);
    tallyB_->setTextFormat(Qt::PlainText);
    tallyGrid->addWidget(swatchA_, 0, 0);
    tallyGrid->addWidget(tallyA_,  0, 1);
    tallyGrid->addWidget(swatchB_, 1, 0);
    tallyGrid->addWidget(tallyB_,  1, 1);
    pv->addWidget(tallyWidget);
    updateSwatches();

    pv->addSpacing(8);
    pv->addWidget(new QLabel("Suggested:", panel));
    suggestedLog_ = new QTextEdit(panel);
    suggestedLog_->setReadOnly(true);
    suggestedLog_->setFixedHeight(latgui::kSuggestedLogHeight);
    pv->addWidget(suggestedLog_);
    clearSuggestBtn_ = new QPushButton("Clear", panel);
    pv->addWidget(clearSuggestBtn_);
    connect(clearSuggestBtn_, &QPushButton::clicked, this, [this]() {
        suggestedLog_->clear();
        boardWidget_->clearSuggestion();
    });

    pv->addSpacing(8);
    playback_ = new guicommon::PlaybackBar(panel);
    pv->addWidget(playback_);
    pv->addWidget(new QLabel("Move log:", panel));
    moveList_ = new guicommon::MoveListWidget(panel);
    pv->addWidget(moveList_, 1);
    registerPlayback(playback_, moveList_);

    buildMenuBar();
    resize(1430, 970); // 1175x760 looks nice, 1380x992 shows 40 moves
    // 1430, 990
    // 2149, 1496
    resize(835, 505);

    newGame(latgui::kStartRows, latgui::kStartColumns, latgui::kStartPerSide,
            selectedMoveStyle(), selectedKomi());
}

void MainWindow::setBannerFont(const QString& family) {
    if (family.isEmpty()) {
        return;
    }
    QFont f(family);
    f.setPointSize(latgui::kBannerCustomPtSize);
    f.setBold(true);
    bannerLabel_->setFont(f);
}

// ── Menu bar ──────────────────────────────────────────────────────────────────

void MainWindow::buildMenuBar() {
    // File.
    auto* fileMenu = menuBar()->addMenu("File");
    connect(fileMenu->addAction("New Game"), &QAction::triggered, this, &MainWindow::onNewGame);
    fileMenu->addSeparator();
    connect(fileMenu->addAction("Save As..."), &QAction::triggered, this, &MainWindow::onSave);
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
    rowsSpin_->setValue(latgui::kStartRows);
    form->addRow("Rows:", rowsSpin_);

    colsSpin_ = new QSpinBox(bmw);
    colsSpin_->setRange(gb::kMinRowsCols, gb::kMaxRowsCols);
    colsSpin_->setValue(latgui::kStartColumns);
    form->addRow("Columns:", colsSpin_);

    perSideSpin_ = new QSpinBox(bmw);
    perSideSpin_->setRange(1, (gb::kMaxRowsCols * gb::kMaxRowsCols) / 2);
    perSideSpin_->setValue(latgui::kStartPerSide);
    form->addRow("Discs/side:", perSideSpin_);

    // Movement rule set. Both entries are Dux-free and share every other rule, so the
    // two can be compared run against run; see Latrunculi::MoveStyle for the sources.
    movementCombo_ = new QComboBox(bmw);
    movementCombo_->addItem("Kharebga (slide)",
                            static_cast<int>(Latrunculi::MoveStyle::Slide));
    movementCombo_->addItem("Seneca (step + leap)",
                            static_cast<int>(Latrunculi::MoveStyle::StepLeap));
    movementCombo_->setCurrentIndex(
        movementCombo_->findData(static_cast<int>(Latrunculi::kDefaultMoveStyle)));
    form->addRow("Movement:", movementCombo_);

    // Komi credited to side B. Half-integers only: an integral komi would allow an exact
    // tie, which this engine has no draw to report (Latrunculi::validateKomi rejects it).
    // A combo rather than a spinbox so a whole number cannot be entered at all.
    komiCombo_ = new QComboBox(bmw);
    for (int whole = 0; whole < 6; ++whole) {
        const double k = whole + 0.5;
        komiCombo_->addItem(QString::number(k, 'f', 1), k);
    }
    komiCombo_->setCurrentIndex(komiCombo_->findData(Latrunculi::kDefaultKomi));
    form->addRow("Komi (B):", komiCombo_);

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

    auto* colorABtn = new QPushButton("Side A color...", bmw);
    auto* colorBBtn = new QPushButton("Side B color...", bmw);
    auto* bgBtn     = new QPushButton("Background color...", bmw);
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
    // Selecting Manual takes both sides back under the mouse: leave any versus game and
    // stop the computer.
    connect(manualAction_, &QAction::triggered, this, [this]() {
        endVersus();
        search().cancelSearch();
    });
    {
        // NegaMax is iterative-deepening, so it is budgeted by time exactly like MCTS
        // and offers the same choices; kMaxNegamaxDepth is only the ceiling the clock
        // almost never lets it reach.
        guicommon::TimeMenuConfig nm{kMctsOptions, std::size(kMctsOptions), true, 1, 200, 4};
        auto* goBtn = guicommon::buildNegaMaxTimeMenu(this, playMenu, playGroup, nm,
                                                      playNegamaxSecCombo_, playTurnsSpin_);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onPlayNegamaxGo);
    }
    {
        guicommon::MctsMenuConfig mc{kMctsOptions, std::size(kMctsOptions), true, 1, 200, 4};
        auto* goBtn = guicommon::buildMctsMenu(this, playMenu, playGroup, mc,
                                               playMctsSecCombo_, playMctsTurnsSpin_);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onPlayMctsGo);
    }
    {
        // Human vs computer: the same NegaMax time choices, plus which side (A/B) the
        // human takes; the computer plays the other and answers every turn.
        guicommon::ComputerMenuConfig cc{kMctsOptions, std::size(kMctsOptions), "A", "B", 0};
        auto* goBtn = guicommon::buildComputerMenu(this, playMenu, playGroup, cc,
                                                   playComputerSecCombo_, playComputerSideCombo_);
        connect(goBtn, &QPushButton::clicked, this, &MainWindow::onPlayComputerGo);
    }

    // Suggest.
    auto* suggestMenu  = menuBar()->addMenu("Suggest");
    auto* suggestGroup = new QActionGroup(this);
    suggestGroup->setExclusive(true);
    {
        guicommon::TimeMenuConfig nm{kMctsOptions, std::size(kMctsOptions)};
        nm.withTurns = false;
        QSpinBox* unusedTurns = nullptr;
        auto* goBtn = guicommon::buildNegaMaxTimeMenu(this, suggestMenu, suggestGroup, nm,
                                                      suggestNegamaxSecCombo_, unusedTurns);
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

    // Keep the Play and Suggest NegaMax time combos in sync.
    connect(playNegamaxSecCombo_, &QComboBox::currentIndexChanged,
            suggestNegamaxSecCombo_, &QComboBox::setCurrentIndex);
    connect(suggestNegamaxSecCombo_, &QComboBox::currentIndexChanged,
            playNegamaxSecCombo_, &QComboBox::setCurrentIndex);

    // About.
    auto* aboutMenu = menuBar()->addMenu("About");
    connect(aboutMenu->addAction("Rules"), &QAction::triggered, this, [this]() {
        showMarkdownResource("Ludus Latrunculorum -- Rules", ":/doc/rules.md");
    });
    connect(aboutMenu->addAction("Usage"), &QAction::triggered, this, [this]() {
        showMarkdownResource("Ludus Latrunculorum -- Usage", ":/doc/usage.md");
    });
    aboutMenu->addSeparator();
    connect(aboutMenu->addAction("About Latrunculi"), &QAction::triggered, this, [this]() {
        guicommon::showAboutDialog(this, "Latrunculi");
    });
}

// Loads a bundled markdown resource and renders it (headings, emphasis, block quotes,
// etc.) in a read-only popup dialog; the dialog is modeless and self-deletes on close.
void MainWindow::showMarkdownResource(const QString& title, const QString& resourcePath) {
    QFile file(resourcePath);
    QString text;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        text = QString::fromUtf8(file.readAll());
    } else {
        // A missing bundled resource is a build/packaging bug, not user-facing bad
        // input -- surface it rather than silently showing a blank dialog.
        text = QString("(Could not load bundled resource \"%1\".)").arg(resourcePath);
    }

    auto* dialog = new QDialog(this);
    dialog->setWindowTitle(title);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->resize(700, 800);
    auto* layout = new QVBoxLayout(dialog);
    auto* browser = new QTextBrowser(dialog);
    browser->setOpenExternalLinks(true);
    browser->setMarkdown(text);
    layout->addWidget(browser);
    dialog->show();
}

// ── Game control ──────────────────────────────────────────────────────────────

Latrunculi::MoveStyle MainWindow::selectedMoveStyle() const {
    return static_cast<Latrunculi::MoveStyle>(movementCombo_->currentData().toInt());
}

double MainWindow::selectedKomi() const {
    return komiCombo_->currentData().toDouble();
}

void MainWindow::newGame(int rows, int columns, int perSide, Latrunculi::MoveStyle style,
                         double komi) {
    stopSeed();
    if (2 * perSide > rows * columns) {
        QMessageBox::warning(this, "Latrunculi",
            QString("Too many discs: 2 x %1 must be <= %2 x %3 squares.")
                .arg(perSide).arg(rows).arg(columns));
        return;
    }
    placementPolicy_.reset(AbsGame::makeSeed(0));  // a different opening per new board
    randomPlies_.clear();
    try {
        game_ = std::make_unique<Latrunculi::Game>(rows, columns, perSide, style,
                                                   Latrunculi::kDefaultPayoffStyle, komi);
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, "Latrunculi", ex.what());
        return;
    }
    search().cancelSearch();
    endVersus();  // a fresh board leaves any human-vs-computer game
    currentFilePath_.clear();
    tlRows_ = rows;
    tlCols_ = columns;
    tlPerSide_ = perSide;
    tlStyle_ = style;
    tlKomi_ = komi;
    timeline_.clear();
    rebuildMoveList();
    suggestedLog_->clear();
    // Repoint the board at the new game BEFORE any color rebuild. The make_unique
    // above freed the previous game, leaving the widget's observer pointer dangling
    // until setGame() updates it; setSideColors/setBackgroundColor each rebuild, so
    // doing them first would dereference the freed game.
    boardWidget_->setGame(game_.get());
    boardWidget_->setSideColors(colorA_, colorB_);
    boardWidget_->setBackgroundColor(background_);
    updateControls();
    syncPlaybackToEnd();  // empty timeline -> the bar shows 0 / 0
}

void MainWindow::onNewGame() {
    newGame(rowsSpin_->value(), colsSpin_->value(), perSideSpin_->value(),
            selectedMoveStyle(), selectedKomi());
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
    newGame(rowsSpin_->value(), colsSpin_->value(), perSideSpin_->value(),
            selectedMoveStyle(), selectedKomi());
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
    ++seedPlaced_;
    if (seedPlaced_ >= seedTarget_) {
        stopSeed();
    }
    afterMoveApplied();
}

void MainWindow::onMoveRequested(AbsGame::MoveId mv) {
    if (!game_ || game_->isTerminal() || search().isSearching() || seeding()) {
        return;
    }
    if (!game_->isLegalMove(mv)) {
        return;
    }
    game_->applyMove(mv);
    afterMoveApplied();
    maybeComputerMove();  // in Computer mode, let the computer answer this move
}

// ── GameMainWindow hooks ──────────────────────────────────────────────────────

AbsGame::Game* MainWindow::currentGame() {
    return game_.get();
}

// Opening variety for auto-play: the placement phase alternates between one random
// placement and a run of searched ones (see Latrunculi::PlacementPolicy). Returning true
// hands the move straight to startPlay, skipping the search for this ply only. Movement
// plies always fall through and are searched.
bool MainWindow::autoPlayMoveOverride(AbsGame::MoveId& mv) {
    if (!game_ || game_->phase() != Latrunculi::Phase::Placement) {
        return false;
    }
    if (!placementPolicy_.nextIsRandom(game_->currentPlayer())) {
        return false;
    }
    const std::vector<AbsGame::MoveId> moves = game_->getLegalMoves();
    if (moves.empty()) {
        return false;  // no legal placement; let the normal path surface it
    }
    mv = placementPolicy_.pickRandomPlacement(*game_, moves);
    // Record the ply this will become (Move::turn is 1-based) so the move list can tag
    // it. The caller applies `mv` immediately, and it came from getLegalMoves, so the
    // entry cannot go stale.
    randomPlies_.insert(static_cast<int>(game_->history().size()) + 1);
    return true;
}

void MainWindow::applyComputedMove(AbsGame::MoveId mv) {
    if (!game_ || game_->isTerminal() || !game_->isLegalMove(mv)) {
        return;
    }
    game_->applyMove(mv);
    afterMoveApplied();
}

// ── AI play / suggest ─────────────────────────────────────────────────────────

void MainWindow::onPlayNegamaxGo() {
    endVersus();  // auto-play drives both sides; drop any human-vs-computer game
    guicommon::SearchController::Params p;
    p.algo          = guicommon::SearchController::Algorithm::NegaMax;
    p.depth         = kMaxNegamaxDepth;
    p.negamaxTimeMs = playNegamaxSecCombo_->currentData().toInt() * 1000;
    // The clock bounds this search, not kMaxNegamaxDepth, so the search bar can show the
    // real elapsed fraction instead of sweeping.
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
    if (!game_ || game_->isTerminal() || seeding()) {
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
    if (!game_ || game_->isTerminal() || search().isSearching() || seeding()) {
        return;
    }
    guicommon::SearchController::Params p;
    p.algo          = guicommon::SearchController::Algorithm::NegaMax;
    p.depth         = kMaxNegamaxDepth;
    p.negamaxTimeMs = suggestNegamaxSecCombo_->currentData().toInt() * 1000;
    p.negamaxTimeBudgeted = true;  // as in onPlayNegamaxGo: the clock bounds this search
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
    const QColor c = QColorDialog::getColor(colorA_, this, "Side A color");
    if (c.isValid()) {
        colorA_ = c;
        boardWidget_->setSideColors(colorA_, colorB_);
        updateSwatches();
    }
}

void MainWindow::onPickColorB() {
    const QColor c = QColorDialog::getColor(colorB_, this, "Side B color");
    if (c.isValid()) {
        colorB_ = c;
        boardWidget_->setSideColors(colorA_, colorB_);
        updateSwatches();
    }
}

void MainWindow::onPickBackground() {
    const QColor c = QColorDialog::getColor(background_, this, "Background color");
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

void MainWindow::updateSwatches() {
    const QString swatchCss = QString("background-color: %1; border: 1px solid %2;");
    if (swatchA_ != nullptr) {
        swatchA_->setStyleSheet(swatchCss.arg(colorA_.name(), latgui::kSwatchBorder.name()));
    }
    if (swatchB_ != nullptr) {
        swatchB_->setStyleSheet(swatchCss.arg(colorB_.name(), latgui::kSwatchBorder.name()));
    }
}

void MainWindow::updateControls() {
    const bool searching = search().isSearching();
    stopBtn_->setVisible(searching);
    boardWidget_->setSearching(searching);
    menuBar()->setEnabled(!searching);
    clearSuggestBtn_->setEnabled(!searching);
    updateSwatches();  // keep the tally squares in step with the side colors

    if (!game_) {
        statusLabel_->setText("No game");
        tallyA_->clear();
        tallyB_->clear();
        return;
    }
    tallyA_->setText(QString("A: %1  (%2 free, %3 bound)")
        .arg(game_->totalDiscs(0)).arg(game_->freeDiscs(0)).arg(game_->boundDiscs(0)));
    tallyB_->setText(QString("B: %1  (%2 free, %3 bound)")
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
    QString s = QString("%1. %2: ").arg(m.turn, 3, 10, QChar('0')).arg(side);
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

void MainWindow::rebuildMoveList() {
    QStringList rows;
    rows.reserve(static_cast<int>(timeline_.size()) + 1);
    for (const Latrunculi::Move& m : timeline_) {
        // Tag the placements the opening policy chose at random rather than searched,
        // matching the self-play move log. Provenance is GUI-side only and is not
        // saved, so a loaded game shows no tags.
        const bool wasRandom = randomPlies_.count(m.turn) > 0;
        rows << (wasRandom ? moveDescription(m) + "  [random]" : moveDescription(m));
    }
    // A final, non-ply row announcing the result. Playback stays correct: gotoPly
    // clamps to the move count and setCurrentPly never highlights this extra row.
    if (game_->isOver()) {
        rows << gameOverSummary();
    }
    moveList_->setMoves(rows);
}

QString MainWindow::gameOverSummary() const {
    const QString who = (game_->winner() == 0) ? "A" : "B";
    QString how;
    switch (game_->winReason()) {
        case Latrunculi::WinReason::Reduction: {
            how = "reduction";
            break;
        }
        case Latrunculi::WinReason::Immobilization: {
            how = "immobilization";
            break;
        }
        case Latrunculi::WinReason::QuietGame: {
            how = "the quiet-game rule";
            break;
        }
        case Latrunculi::WinReason::None: {
            how = "an unknown rule";  // cannot happen once isOver(); flag it if it does
            break;
        }
    }
    // s = 3M/(3M+2N): the winner's terminal score (the loser scores -s).
    return QString("Game over. %1 won by %2 (s = %3)")
        .arg(who, how, QString::number(game_->winnerScore(), 'f', 2));
}

void MainWindow::afterMoveApplied() {
    // game_ now holds the post-move position; adopt it as the (possibly truncated)
    // timeline, then refresh the move list, board, last-move dot and playback bar.
    timeline_.assign(game_->history().begin(), game_->history().end());
    rebuildMoveList();
    suggestedLog_->clear();
    refreshBoard();
    if (!timeline_.empty()) {
        boardWidget_->setLastMove(timeline_.back().to);
    }
    syncPlaybackToEnd();
}

int MainWindow::playbackPlyCount() const {
    return static_cast<int>(timeline_.size());
}

void MainWindow::rebuildToPly(int ply) {
    if (tlRows_ <= 0) {
        return;
    }
    auto g = std::make_unique<Latrunculi::Game>(tlRows_, tlCols_, tlPerSide_, tlStyle_,
                                                Latrunculi::kDefaultPayoffStyle, tlKomi_);
    const int k = std::min(ply, static_cast<int>(timeline_.size()));
    for (int i = 0; i < k; ++i) {
        const Latrunculi::Move& m = timeline_[static_cast<std::size_t>(i)];
        const AbsGame::MoveId mid =
            (m.from < 0) ? g->placementMove(m.to)
                         : g->movementMove(m.from, m.to, m.removed);
        if (!g->applyMove(mid)) {
            break;  // inconsistent saved record; stop the replay here
        }
    }
    game_ = std::move(g);                 // frees the old game...
    boardWidget_->setGame(game_.get());   // ...so repoint before any color rebuild
    boardWidget_->setLastMove(k > 0 ? timeline_[static_cast<std::size_t>(k - 1)].to : -1);
    updateControls();
}
// Copyright Ben Paul Wise. All Rights Reserved.
