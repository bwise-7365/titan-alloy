// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// PformMainWindow implementation (see pformmainwindow.hpp).
// ----------------------------------------------
#include "pformmainwindow.hpp"

#include "pformgms.hpp"

#include <QtConcurrent/QtConcurrent>

#include <QAction>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTableWidget>
#include <QTextCursor>
#include <QVBoxLayout>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>

namespace VIMCP::App {

  namespace {
    // Exemplar-derived control ranges: the C# window offered 3..15 parties and
    // 1..6 issues; the K = M^D cap (kMaxParliaments = 8192) gates the corner
    // of that box that overflows it.
    const int kPartiesMin = 3;
    const int kPartiesMax = 15;
    const int kPartiesDefault = 5;
    const int kIssuesMin = 1;
    const int kIssuesMax = 6;
    const int kIssuesDefault = 3;
    const double kDefaultQ = 0.05;
  } // namespace

  PformMainWindow::PformMainWindow(QWidget* parent)
    : QMainWindow(parent)
  {
    setWindowTitle("pform_gui");

    // --- menus ---------------------------------------------------------------
    QMenu* fileMenu = menuBar()->addMenu("&File");
    openAction = fileMenu->addAction("&Open GMS...");
    openAction->setToolTip("Read a limited-subset GMS instance through "
                           "readPformGms, exactly as pform_cli <file.gms>.");
    connect(openAction, &QAction::triggered, this, &PformMainWindow::onOpenGms);
    fileMenu->addSeparator();
    QAction* quitAction = fileMenu->addAction("&Quit");
    connect(quitAction, &QAction::triggered, this, &PformMainWindow::close);

    // --- top band: generation and solve controls ------------------------------
    partiesBox = new QSpinBox(this);
    partiesBox->setRange(kPartiesMin, kPartiesMax);
    partiesBox->setValue(kPartiesDefault);
    partiesBox->setToolTip("Number of parties M for the next Reset.");

    issuesBox = new QSpinBox(this);
    issuesBox->setRange(kIssuesMin, kIssuesMax);
    issuesBox->setValue(kIssuesDefault);
    issuesBox->setToolTip("Number of issues D for the next Reset.");

    seedEdit = new QLineEdit(this);
    seedEdit->setText("0");
    seedEdit->setMaximumWidth(170);
    seedEdit->setValidator(
        new QRegularExpressionValidator(QRegularExpression("[0-9]{0,20}"), this));
    seedEdit->setToolTip("PRNG seed for Reset. 0 (or empty) draws a fresh seed "
                         "and writes it back here, so every run is reproducible.");

    qBox = new QDoubleSpinBox(this);
    qBox->setDecimals(4);
    qBox->setSingleStep(0.01);
    qBox->setRange(0.0001, 0.9999);
    qBox->setValue(kDefaultQ);
    qBox->setToolTip("unselectedProb q in (0, (K-1)/K): the probability mass "
                     "left on unselected parliaments; sets the effort floor.");

    engineCombo = new QComboBox(this);
    engineCombo->addItem("Default (SAOE chain)",
                         static_cast<int>(ProblemBase::Engine::Default));
    for (const ProblemBase::Engine engine : SAOE::honoredEngines()) {
      engineCombo->addItem(engineName(engine), static_cast<int>(engine));
    }
    engineCombo->setToolTip("Solver engine, exactly pform_cli's --engine list.");

    kLabel = new QLabel(this);

    busyBar = new QProgressBar(this);
    busyBar->setMaximumWidth(120);
    busyBar->setRange(0, 0);   // indeterminate while shown
    busyBar->hide();

    resetButton = new QPushButton("Reset", this);
    resetButton->setToolTip("Generate a fresh random instance at the sizes "
                            "above (the exemplar's Reset).");
    solveButton = new QPushButton("Solve", this);
    solveButton->setEnabled(false);
    solveButton->setToolTip(
        "Solve the instance in the grid on a worker thread: PForm::solve, "
        "then coalition extraction on the raw result -- the pform_cli "
        "pipeline exactly. The log pane receives the CLI-identical report.");

    QHBoxLayout* topBand = new QHBoxLayout();
    topBand->addWidget(new QLabel("Parties", this));
    topBand->addWidget(partiesBox);
    topBand->addWidget(new QLabel("Issues", this));
    topBand->addWidget(issuesBox);
    topBand->addWidget(new QLabel("Seed", this));
    topBand->addWidget(seedEdit);
    topBand->addWidget(new QLabel("q", this));
    topBand->addWidget(qBox);
    topBand->addWidget(new QLabel("Engine", this));
    topBand->addWidget(engineCombo);
    topBand->addWidget(kLabel);
    topBand->addWidget(busyBar);
    topBand->addStretch(1);
    topBand->addWidget(resetButton);
    topBand->addWidget(solveButton);

    // --- center: the party grid beside the result panes -----------------------
    partyTable = new PformPartyTable(this);

    logPane = new QPlainTextEdit(this);
    logPane->setReadOnly(true);
    logPane->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    logPane->setLineWrapMode(QPlainTextEdit::NoWrap);

    deterministicLabel = new QLabel(this);

    coalitionTable = new QTableWidget(this);
    coalitionTable->setColumnCount(8);
    coalitionTable->setHorizontalHeaderLabels(
        { "id", "parties", "pattern", "contributions", "prob (each)", "seats",
          "prob (total)", "notes" });
    coalitionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    coalitionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    coalitionTable->setSelectionMode(QAbstractItemView::SingleSelection);
    coalitionTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    coalitionTable->verticalHeader()->hide();

    QWidget* resultPanel = new QWidget(this);
    QVBoxLayout* resultLayout = new QVBoxLayout(resultPanel);
    resultLayout->setContentsMargins(0, 0, 0, 0);
    resultLayout->addWidget(deterministicLabel);
    resultLayout->addWidget(coalitionTable);

    QSplitter* rightSplit = new QSplitter(Qt::Vertical, this);
    rightSplit->addWidget(logPane);
    rightSplit->addWidget(resultPanel);
    rightSplit->setStretchFactor(0, 2);
    rightSplit->setStretchFactor(1, 1);

    QSplitter* centerSplit = new QSplitter(Qt::Horizontal, this);
    centerSplit->addWidget(partyTable);
    centerSplit->addWidget(rightSplit);
    centerSplit->setStretchFactor(0, 3);
    centerSplit->setStretchFactor(1, 2);

    QWidget* central = new QWidget(this);
    QVBoxLayout* centralLayout = new QVBoxLayout(central);
    centralLayout->addLayout(topBand);
    centralLayout->addWidget(centerSplit);
    setCentralWidget(central);

    connect(resetButton, &QPushButton::clicked, this, &PformMainWindow::onReset);
    connect(solveButton, &QPushButton::clicked, this, &PformMainWindow::onSolve);
    connect(partiesBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &PformMainWindow::onCountsChanged);
    connect(issuesBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &PformMainWindow::onCountsChanged);
    connect(partyTable, &QTableWidget::itemChanged, this,
            &PformMainWindow::onCellEdited);
    connect(coalitionTable, &QTableWidget::itemSelectionChanged, this,
            &PformMainWindow::onCoalitionSelected);

    solveWatcher = new QFutureWatcher<SolveOutcome>(this);
    connect(solveWatcher, &QFutureWatcher<SolveOutcome>::finished, this,
            &PformMainWindow::onSolveFinished);

    refreshKDisplay();
    onReset();   // start with a real instance, exemplar-style
    return;
  }

  void
  PformMainWindow::onSolve()
  {
    if (solveBusyP) {
      return;
    }
    // Read + validate the grid on the GUI thread: with editable cells this is
    // the user-error surface, so violations report as a dialog, not a solve
    // failure.
    PformInstance in = current;
    try {
      in.data = partyTable->instanceFromCells();
      validatePformData(in.data);
    }
    catch (const std::exception& ex) {
      QMessageBox::warning(this, "Instance not solvable", ex.what());
      return;
    }
    in.unselectedProb = qBox->value();

    PformParams params;
    params.unselectedProb = qBox->value();
    params.engine =
        static_cast<ProblemBase::Engine>(engineCombo->currentData().toInt());

    current = in;   // the exact inputs being solved, for provenance
    setBusy(true);
    statusBar()->showMessage("solving (SAOE equilibrium of the parliament game)...");

    // The worker owns copies and touches no widget; the token stamps the
    // launch so a result arriving after Reset/Open/edits is discarded.
    const int token = solveToken;
    solveWatcher->setFuture(QtConcurrent::run([in, params, token]() {
      SolveOutcome out;
      out.token = token;
      out.instance = in;
      out.params = params;
      try {
        const PForm problem(in.data);
        const auto [vi, res] = problem.solve(params);
        out.vi = vi;
        out.result = res;
        // Coalitions from the RAW result (the even split over a coalition is
        // the signal; sparsify would erase it) -- also off the GUI thread.
        out.coalitions = pformCoalitions(res, in.data.weight.size(),
                                         in.data.position.rows());
        out.okP = true;
      }
      catch (const std::exception& ex) {
        out.error = QString::fromUtf8(ex.what());
      }
      return out;
    }));
    return;
  }

  void
  PformMainWindow::onSolveFinished()
  {
    setBusy(false);
    const SolveOutcome out = solveWatcher->result();
    if (out.token != solveToken) {
      statusBar()->showMessage(
          "solve result discarded (the instance changed while solving)");
      return;
    }
    if (!out.okP) {
      QMessageBox::warning(this, "Solve failed", out.error);
      statusBar()->showMessage("solve failed");
      return;
    }

    const Index M = out.instance.data.weight.size();
    const Index D = out.instance.data.position.rows();

    // The log pane receives exactly the CLI report (appTag aside).
    appendLog(renderPformInputs(out.instance, "pform_gui"));
    appendLog(renderPformResult(out.instance, out.params, out.vi, out.result,
                                "pform_gui"));
    appendLog(renderPformCoalitions(out.instance, out.coalitions));

    partyTable->setUtilities(out.result.utilities);

    const Index kStar = out.result.deterministic;
    deterministicMatching = pformMatching(kStar, M, D);
    deterministicLabel->setText(
        QString("Deterministic parliament: k = %1  %2   (eta %3, phi %4)")
            .arg(static_cast<long long>(kStar))
            .arg(QString::fromStdString(
                pformMatchingText(kStar, M, D, out.instance.partyLabels)))
            .arg(out.result.eta(kStar), 0, 'f', 4)
            .arg(out.result.phi(kStar), 0, 'f', 4));

    coalitions = out.coalitions;
    coalitionTable->setRowCount(static_cast<int>(coalitions.size()));
    for (std::size_t g = 0; g < coalitions.size(); ++g) {
      const PformCoalition& c = coalitions[g];
      QString parties;
      QString contribs;
      for (std::size_t i = 0; i < c.members.size(); ++i) {
        const QString label = QString::fromStdString(
            out.instance.partyLabels[static_cast<std::size_t>(c.members[i])]);
        if (0 < i) {
          parties += ",";
          contribs += " ";
        }
        parties += label;
        contribs += QString("%1:%2").arg(label).arg(
            c.effortPer(static_cast<Index>(i)), 0, 'f', 2);
      }
      const int row = static_cast<int>(g);
      const auto put = [this, row](int column, const QString& text) {
        coalitionTable->setItem(row, column, new QTableWidgetItem(text));
      };
      put(0, QString::fromStdString(pformCoalitionLabel(g)));
      put(1, parties);
      put(2, QString::fromStdString(
                 pformPatternText(c.pattern, out.instance.partyLabels)));
      put(3, contribs);
      put(4, QString::number(c.probEach, 'f', 4));
      put(5, QString::number(static_cast<qulonglong>(c.parliaments.size())));
      put(6, QString::number(c.probTotal, 'f', 4));
      put(7, c.regularP ? QString() : QString("irregular"));
    }

    partyTable->clearHighlights();
    partyTable->highlightDeterministic(deterministicMatching);

    statusBar()->showMessage(
        out.vi.converged
            ? QString("solved: %1 coalition(s); residual^2 = %2")
                  .arg(static_cast<qulonglong>(coalitions.size()))
                  .arg(out.vi.residual, 0, 'e', 2)
            : "NOT converged: best-visited point shown (see the log WARNING)");
    return;
  }

  void
  PformMainWindow::onCoalitionSelected()
  {
    // Repaint from scratch: deterministic marks first, then the selected
    // coalition's pinned pattern over them.
    partyTable->clearHighlights();
    partyTable->highlightDeterministic(deterministicMatching);
    const int row = coalitionTable->currentRow();
    if (0 <= row && static_cast<std::size_t>(row) < coalitions.size()) {
      partyTable->highlightPattern(
          coalitions[static_cast<std::size_t>(row)].pattern);
    }
    return;
  }

  void
  PformMainWindow::onReset()
  {
    PformRandomSpec spec;
    spec.numParties = partiesBox->value();
    spec.numIssues = issuesBox->value();
    // 0 (or empty) = surprise-me: draw and DISPLAY the seed actually used.
    const std::uint64_t requested = seedEdit->text().toULongLong();
    const std::uint64_t seed = pformResolveSeed(requested);
    spec.seed = seed;

    PformInstance in;
    try {
      in.data = PForm::generate(spec);
    }
    catch (const std::exception& ex) {
      QMessageBox::warning(this, "Reset failed", ex.what());
      return;
    }
    in.unselectedProb = qBox->value();
    in.partyLabels = pformDefaultLabels("P", spec.numParties);
    in.issueLabels = pformDefaultLabels("I", spec.numIssues);
    in.randomP = true;
    in.seed = seed;

    seedEdit->setText(QString::number(seed));
    populateFromInstance(
        in, QString("random seed %1").arg(static_cast<qulonglong>(seed)));
    return;
  }

  void
  PformMainWindow::onOpenGms()
  {
    if (solveBusyP) {
      return;
    }
    const QString path = QFileDialog::getOpenFileName(
        this, "Open GMS instance", QString(),
        "GMS files (*.gms);;All files (*)");
    if (path.isEmpty()) {
      return;
    }

    PformInstance in;
    try {
      const PformGmsInput gms = readPformGms(path.toStdString());
      in.data = gms.data;
      in.unselectedProb = gms.unselectedProb;
      in.partyLabels = gms.partyLabels;
      in.issueLabels = gms.issueLabels;
      // Reject an over-cap instance HERE (with the library's message), not
      // halfway through installing it.
      pformParliamentCount(in.data.weight.size(), in.data.position.rows());
    }
    catch (const std::exception& ex) {
      // readPformGms reports file:line:col; show it verbatim.
      QMessageBox::warning(this, "Open failed", ex.what());
      return;
    }
    in.randomP = false;
    in.seed = 0;

    // Sync the generation spinners to the file's sizes (blocked: this is not
    // a request to Reset), then install. The spinners clamp to their ranges
    // if the file lies outside them; they only parameterize the NEXT Reset,
    // the grid holds the file's true dimensions either way.
    {
      const QSignalBlocker partiesBlock(partiesBox);
      const QSignalBlocker issuesBlock(issuesBox);
      partiesBox->setValue(static_cast<int>(in.data.weight.size()));
      issuesBox->setValue(static_cast<int>(in.data.position.rows()));
    }
    refreshKDisplay();
    populateFromInstance(in, QFileInfo(path).fileName());
    qBox->setValue(in.unselectedProb);   // after populate: the max is set there
    return;
  }

  void
  PformMainWindow::onCountsChanged()
  {
    // Spinners parameterize the NEXT Reset only; the grid keeps the current
    // instance (so nudging a spinner cannot destroy a loaded or edited one).
    refreshKDisplay();
    return;
  }

  void
  PformMainWindow::onCellEdited()
  {
    ++solveToken;   // any background result now belongs to a stale instance
    clearResults();
    statusBar()->showMessage("instance edited; results cleared");
    return;
  }

  void
  PformMainWindow::refreshKDisplay()
  {
    try {
      const Index parliaments = pformParliamentCount(partiesBox->value(),
                                                     issuesBox->value());
      kLabel->setText(QString("K = %1^%2 = %3")
                          .arg(partiesBox->value())
                          .arg(issuesBox->value())
                          .arg(static_cast<long long>(parliaments)));
      kLabel->setStyleSheet(QString());
      resetButton->setEnabled(true);
    }
    catch (const std::exception&) {
      // pformParliamentCount rejects M^D over its cap; the label carries the
      // verdict and Reset stays off until the spinners come back in range.
      kLabel->setText(QString("K = %1^%2 exceeds the parliament cap")
                          .arg(partiesBox->value())
                          .arg(issuesBox->value()));
      kLabel->setStyleSheet("color: red;");
      resetButton->setEnabled(false);
    }
    return;
  }

  void
  PformMainWindow::populateFromInstance(const PformInstance& in,
                                        const QString& source)
  {
    current = in;
    ++solveToken;

    const Index M = in.data.weight.size();
    const Index D = in.data.position.rows();
    partyTable->rebuild(M, D, in.partyLabels, in.issueLabels);
    partyTable->setInstance(in.data);

    // q may range up to just under (K-1)/K for THIS instance.
    const double parliaments =
        static_cast<double>(pformParliamentCount(M, D));
    qBox->setMaximum((parliaments - 1.0) / parliaments - 1.0e-4);

    setWindowTitle(QString("pform_gui - %1").arg(source));
    clearResults();
    solveButton->setEnabled(true);
    statusBar()->showMessage(
        QString("instance ready: %1 parties, %2 issues, %3 parliaments")
            .arg(static_cast<long long>(M))
            .arg(static_cast<long long>(D))
            .arg(static_cast<long long>(parliaments)));
    return;
  }

  void
  PformMainWindow::clearResults()
  {
    // Clear the stored structures BEFORE the table: shrinking the selection
    // fires onCoalitionSelected, which repaints from these vectors.
    coalitions.clear();
    deterministicMatching.clear();
    partyTable->clearUtilities();
    partyTable->clearHighlights();
    coalitionTable->setRowCount(0);
    deterministicLabel->setText("Deterministic parliament: (not solved)");
    return;
  }

  void
  PformMainWindow::appendLog(const std::string& text)
  {
    logPane->moveCursor(QTextCursor::End);
    logPane->insertPlainText(QString::fromStdString(text));
    logPane->ensureCursorVisible();
    return;
  }

  void
  PformMainWindow::setBusy(bool busyP)
  {
    solveBusyP = busyP;
    busyBar->setVisible(busyP);
    solveButton->setEnabled(!busyP);
    openAction->setEnabled(!busyP);
    partiesBox->setEnabled(!busyP);
    issuesBox->setEnabled(!busyP);
    seedEdit->setEnabled(!busyP);
    qBox->setEnabled(!busyP);
    engineCombo->setEnabled(!busyP);
    if (busyP) {
      resetButton->setEnabled(false);
    }
    else {
      refreshKDisplay();   // Reset re-enables subject to the K cap
    }
    return;
  }

} // namespace VIMCP::App
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
