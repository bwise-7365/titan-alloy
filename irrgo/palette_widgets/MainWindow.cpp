// Copyright Ben Paul Wise. All Rights Reserved.
#include "MainWindow.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "BoardPreview.h"
#include "ColorConvert.h"
#include "PaletteController.h"
#include "RybColorWheel.h"

namespace palette_widgets {

namespace {

palette::Harmony harmonyForIndex(int index) {
    switch (index) {
        case 0:
            return palette::Harmony::Complement;
        case 2:
            return palette::Harmony::Triad;
        case 1:
        default:
            return palette::Harmony::SplitComplement;
    }
}

QString harmonyName(palette::Harmony h) {
    switch (h) {
        case palette::Harmony::Complement:
            return "Complement";
        case palette::Harmony::SplitComplement:
            return "Split-Complement";
        case palette::Harmony::Triad:
            return "Triad";
        case palette::Harmony::Analogous:
            return "Analogous";
        case palette::Harmony::Tetrad:
            return "Tetrad";
    }
    return "?";
}

QString warningText(palette::Warning w) {
    switch (w) {
        case palette::Warning::PiecesHueTooClose:
            return "Pieces are too close in hue and luminance to tell apart.";
        case palette::Warning::PiecesLuminanceMidband:
            return "Both pieces sit in the mid-luminance band; spread them apart.";
        case palette::Warning::NoHarmonicBackground:
            return "The piece hues match no harmonic scheme; background neutralized.";
    }
    return "";
}

} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    controller_ = new PaletteController(this);
    buildUi();

    connect(controller_, &PaletteController::paletteReady, this,
            &MainWindow::onPaletteReady);

    // Seed the controller with the default state (Mode 1, beige board) and run
    // the first computation so nothing starts blank.
    controller_->setMode(PaletteController::Mode::Background);
    controller_->setHarmony(harmonyForIndex(harmonyCombo_->currentIndex()));
    controller_->setLuminanceSpread(spreadCheck_->isChecked());
    controller_->setBackgroundColor(toSrgb(backgroundColor_));
    controller_->setPiece1Color(toSrgb(piece1Color_));
    controller_->setPiece2Color(toSrgb(piece2Color_));
    setSwatch(inputBackground_, backgroundColor_);
    setSwatch(inputPiece1_, piece1Color_);
    setSwatch(inputPiece2_, piece2Color_);
    updateInputVisibility();
    controller_->recompute();
}

void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(8);

    // 1. Mode toggle.
    auto* modeRow = new QHBoxLayout;
    modeBackground_ = new QRadioButton("Pick background", central);
    modeOnePiece_ = new QRadioButton("Pick one piece", central);
    modeTwoPieces_ = new QRadioButton("Pick both pieces", central);
    modeBackground_->setChecked(true);
    auto* modeGroup = new QButtonGroup(this);
    modeGroup->addButton(modeBackground_);
    modeGroup->addButton(modeOnePiece_);
    modeGroup->addButton(modeTwoPieces_);
    modeRow->addWidget(modeBackground_);
    modeRow->addWidget(modeOnePiece_);
    modeRow->addWidget(modeTwoPieces_);
    modeRow->addStretch(1);
    root->addLayout(modeRow);

    // 2. Input row: native pick buttons + fixed-color swatches.
    auto* inputRow = new QHBoxLayout;
    pickBackground_ = new QPushButton("Background...", central);
    pickPiece1_ = new QPushButton("Piece 1...", central);
    pickPiece2_ = new QPushButton("Piece 2...", central);
    inputBackground_ = new QLabel(central);
    inputPiece1_ = new QLabel(central);
    inputPiece2_ = new QLabel(central);
    for (QLabel* s : {inputBackground_, inputPiece1_, inputPiece2_}) {
        s->setMinimumWidth(80);
    }
    inputRow->addWidget(pickBackground_);
    inputRow->addWidget(inputBackground_);
    inputRow->addWidget(pickPiece1_);
    inputRow->addWidget(inputPiece1_);
    inputRow->addWidget(pickPiece2_);
    inputRow->addWidget(inputPiece2_);
    inputRow->addStretch(1);
    root->addLayout(inputRow);

    // 3. Options row: harmony template + luminance-spread toggle.
    auto* optionRow = new QHBoxLayout;
    optionRow->addWidget(new QLabel("Harmony:", central));
    harmonyCombo_ = new QComboBox(central);
    harmonyCombo_->addItem("Complement");
    harmonyCombo_->addItem("Split-Complement");
    harmonyCombo_->addItem("Triad");
    harmonyCombo_->setCurrentIndex(2); // Triad default
    spreadCheck_ = new QCheckBox("Spread piece luminance", central);
    spreadCheck_->setChecked(true);
    optionRow->addWidget(harmonyCombo_);
    optionRow->addWidget(spreadCheck_);
    optionRow->addStretch(1);
    root->addLayout(optionRow);

    // 4 + 8. Board preview alongside the RYB wheel.
    auto* visualRow = new QHBoxLayout;
    preview_ = new BoardPreview(central);
    wheel_ = new RybColorWheel(central);
    visualRow->addWidget(preview_, 3);
    visualRow->addWidget(wheel_, 2);
    root->addLayout(visualRow);

    // 5. Result swatches.
    auto* resultRow = new QHBoxLayout;
    resultBackground_ = new QLabel(central);
    resultPiece1_ = new QLabel(central);
    resultPiece2_ = new QLabel(central);
    auto* resultForm = new QFormLayout;
    resultForm->addRow("Background", resultBackground_);
    resultForm->addRow("Piece 1", resultPiece1_);
    resultForm->addRow("Piece 2", resultPiece2_);
    resultRow->addLayout(resultForm);
    resultRow->addStretch(1);
    root->addLayout(resultRow);

    // 6. Diagnostics + 7. warnings.
    diagnostics_ = new QLabel(central);
    diagnostics_->setWordWrap(true);
    root->addWidget(diagnostics_);
    warnings_ = new QLabel(central);
    warnings_->setWordWrap(true);
    warnings_->setStyleSheet("color:#a00000;");
    root->addWidget(warnings_);

    connect(modeBackground_, &QRadioButton::toggled, this, &MainWindow::onModeChanged);
    connect(modeOnePiece_, &QRadioButton::toggled, this, &MainWindow::onModeChanged);
    connect(modeTwoPieces_, &QRadioButton::toggled, this, &MainWindow::onModeChanged);
    connect(pickBackground_, &QPushButton::clicked, this, &MainWindow::onPickBackground);
    connect(pickPiece1_, &QPushButton::clicked, this, &MainWindow::onPickPiece1);
    connect(pickPiece2_, &QPushButton::clicked, this, &MainWindow::onPickPiece2);
    connect(harmonyCombo_, &QComboBox::currentIndexChanged, this,
            &MainWindow::onHarmonyChanged);
    connect(spreadCheck_, &QCheckBox::toggled, this,
            &MainWindow::onLuminanceSpreadToggled);
}

void MainWindow::updateInputVisibility() {
    const bool bg = modeBackground_->isChecked();
    const bool one = modeOnePiece_->isChecked();
    const bool two = modeTwoPieces_->isChecked();
    pickBackground_->setVisible(bg);
    inputBackground_->setVisible(bg);
    pickPiece1_->setVisible(one || two);
    inputPiece1_->setVisible(one || two);
    pickPiece2_->setVisible(two);
    inputPiece2_->setVisible(two);
}

void MainWindow::setSwatch(QLabel* swatch, const QColor& color) {
    const QString textColor = (color.lightnessF() > 0.5) ? "black" : "white";
    swatch->setText(color.name());
    swatch->setAlignment(Qt::AlignCenter);
    swatch->setStyleSheet(
        QString("background-color:%1; color:%2; border:1px solid #888; padding:4px;")
            .arg(color.name(), textColor));
}

void MainWindow::onModeChanged() {
    PaletteController::Mode mode = PaletteController::Mode::Background;
    if (modeOnePiece_->isChecked()) {
        mode = PaletteController::Mode::OnePiece;
    } else if (modeTwoPieces_->isChecked()) {
        mode = PaletteController::Mode::TwoPieces;
    }
    controller_->setMode(mode);
    updateInputVisibility();
    controller_->recompute();
}

void MainWindow::onPickBackground() {
    const QColor c = QColorDialog::getColor(backgroundColor_, this, "Background color");
    if (!c.isValid()) {
        return;
    }
    backgroundColor_ = c;
    setSwatch(inputBackground_, c);
    controller_->setBackgroundColor(toSrgb(c));
    controller_->recompute();
}

void MainWindow::onPickPiece1() {
    const QColor c = QColorDialog::getColor(piece1Color_, this, "Piece 1 color");
    if (!c.isValid()) {
        return;
    }
    piece1Color_ = c;
    setSwatch(inputPiece1_, c);
    controller_->setPiece1Color(toSrgb(c));
    controller_->recompute();
}

void MainWindow::onPickPiece2() {
    const QColor c = QColorDialog::getColor(piece2Color_, this, "Piece 2 color");
    if (!c.isValid()) {
        return;
    }
    piece2Color_ = c;
    setSwatch(inputPiece2_, c);
    controller_->setPiece2Color(toSrgb(c));
    controller_->recompute();
}

void MainWindow::onHarmonyChanged(int index) {
    controller_->setHarmony(harmonyForIndex(index));
    controller_->recompute();
}

void MainWindow::onLuminanceSpreadToggled(bool enabled) {
    controller_->setLuminanceSpread(enabled);
    controller_->recompute();
}

void MainWindow::onPaletteReady(const palette::Palette& result) {
    const QColor bg = toQColor(result.background);
    const QColor p1 = toQColor(result.piece1);
    const QColor p2 = toQColor(result.piece2);

    setSwatch(resultBackground_, bg);
    setSwatch(resultPiece1_, p1);
    setSwatch(resultPiece2_, p2);
    preview_->setColors(bg, p1, p2);
    wheel_->setPalette(bg, p1, p2);

    const palette::Diagnostics& d = result.diagnostics;
    diagnostics_->setText(
        QString("Contrast  B:P1 %1   B:P2 %2   P1:P2 %3\n"
                "Template %4   target bg luminance %5")
            .arg(d.contrastBgP1, 0, 'f', 2)
            .arg(d.contrastBgP2, 0, 'f', 2)
            .arg(d.contrastP1P2, 0, 'f', 2)
            .arg(harmonyName(d.templateUsed))
            .arg(d.backgroundTargetLuminance, 0, 'f', 3));

    if (result.warnings.empty()) {
        warnings_->setText("");
    } else {
        QString text;
        for (const palette::Warning w : result.warnings) {
            if (!text.isEmpty()) {
                text += "\n";
            }
            text += "! " + warningText(w);
        }
        warnings_->setText(text);
    }
}

} // namespace palette_widgets
// Copyright Ben Paul Wise. All Rights Reserved.
