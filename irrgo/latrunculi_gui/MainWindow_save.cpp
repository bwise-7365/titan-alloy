// Copyright Ben Paul Wise. All Rights Reserved.
//
//
// NOTE: this is not a backup file.
//
// Save / load for the Latrunculi GUI. The file stores the authoritative board
// state (per-square cells, phase, side to move, placed counts) plus a move log
// for display; loading reconstructs the game through the engine's state-injecting
// constructor. The super-ko history is not serialised, so a loaded game restarts
// its repetition set from the restored position (a documented limitation).
#include "MainWindow.h"

#include "PlaybackBar.h"
#include <QColor>
#include <QFile>
#include <QFileDialog>
#include <QLatin1String>
#include <QMessageBox>
#include <QStringList>
#include <QTextEdit>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <vector>

namespace {

const char* cellName(Latrunculi::Cell c) {
    switch (c) {
        case Latrunculi::Cell::P0Free:  return "P0Free";
        case Latrunculi::Cell::P0Bound: return "P0Bound";
        case Latrunculi::Cell::P1Free:  return "P1Free";
        case Latrunculi::Cell::P1Bound: return "P1Bound";
        case Latrunculi::Cell::Empty:   return "Empty";
    }
    return "Empty";
}

bool parseCell(const QString& s, Latrunculi::Cell& out) {
    if (s == QLatin1String("P0Free"))  { out = Latrunculi::Cell::P0Free;  return true; }
    if (s == QLatin1String("P0Bound")) { out = Latrunculi::Cell::P0Bound; return true; }
    if (s == QLatin1String("P1Free"))  { out = Latrunculi::Cell::P1Free;  return true; }
    if (s == QLatin1String("P1Bound")) { out = Latrunculi::Cell::P1Bound; return true; }
    if (s == QLatin1String("Empty"))   { out = Latrunculi::Cell::Empty;   return true; }
    return false;
}

}  // namespace

void MainWindow::onSave() {
    if (!game_) {
        return;
    }
    // "Save As...": always let the user choose the destination file.
    const QString path = QFileDialog::getSaveFileName(this, "Save Game As", QString(),
        "Latrunculi XML files (*.xml);;All files (*)");
    if (path.isEmpty()) {
        return;
    }
    currentFilePath_ = path;
    saveToFile(path);
}

void MainWindow::onLoad() {
    const QString path = QFileDialog::getOpenFileName(this, "Load Game", QString(),
        "Latrunculi XML files (*.xml);;All files (*)");
    if (path.isEmpty()) {
        return;
    }
    if (loadFromFile(path)) {
        currentFilePath_ = path;
    }
}

void MainWindow::saveToFile(const QString& path) {
    if (!game_) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Latrunculi", "Could not open the file for writing.");
        return;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("LatrunculiBoard");
    xml.writeAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    xml.writeAttribute("xsi:noNamespaceSchemaLocation", "latrunculi.xsd");

    xml.writeEmptyElement("dims");
    xml.writeAttribute("rows", QString::number(game_->rows()));
    xml.writeAttribute("cols", QString::number(game_->columns()));

    xml.writeEmptyElement("perSide");
    xml.writeAttribute("val", QString::number(game_->perSide()));

    xml.writeEmptyElement("phase");
    xml.writeAttribute("val", game_->phase() == Latrunculi::Phase::Placement
                                  ? "Placement" : "Movement");

    xml.writeEmptyElement("sideToMove");
    xml.writeAttribute("val", QString::number(game_->currentPlayer()));

    // placed_ is needed to resume a placement-phase game; in movement it equals
    // perSide. Derive it from the board (no captures occur during placement).
    int placed0 = 0, placed1 = 0;
    if (game_->phase() == Latrunculi::Phase::Placement) {
        placed0 = game_->totalDiscs(0);
        placed1 = game_->totalDiscs(1);
    } else {
        placed0 = game_->perSide();
        placed1 = game_->perSide();
    }
    xml.writeEmptyElement("placed");
    xml.writeAttribute("p0", QString::number(placed0));
    xml.writeAttribute("p1", QString::number(placed1));

    // Side and background colors (the only cosmetic data stored; the hand-scratched
    // line/disc geometry is omitted, being reproducible from a PRNG seed).
    xml.writeEmptyElement("colors");
    xml.writeAttribute("sideA", colorA_.name());
    xml.writeAttribute("sideB", colorB_.name());
    xml.writeAttribute("background", background_.name());

    // Authoritative board state: non-empty cells only.
    xml.writeStartElement("Position");
    for (int s = 0; s < game_->squareCount(); ++s) {
        const Latrunculi::Cell c = game_->cellAt(s);
        if (c == Latrunculi::Cell::Empty) {
            continue;
        }
        xml.writeEmptyElement("cell");
        xml.writeAttribute("sq", QString::number(s));
        xml.writeAttribute("state", cellName(c));
    }
    xml.writeEndElement();  // Position

    // Move log (display only; not required to reconstruct state).
    xml.writeStartElement("move_list");
    for (const Latrunculi::Move& m : game_->history()) {
        xml.writeEmptyElement("move");
        xml.writeAttribute("turn", QString::number(m.turn));
        xml.writeAttribute("player", QString::number(m.player));
        xml.writeAttribute("from", QString::number(m.from));
        xml.writeAttribute("to", QString::number(m.to));
        xml.writeAttribute("removed", QString::number(m.removed));
        QStringList path;
        for (int sq : m.path) {
            path << QString::number(sq);
        }
        xml.writeAttribute("path", path.join(' '));
    }
    xml.writeEndElement();  // move_list

    xml.writeEndElement();  // LatrunculiBoard
    xml.writeEndDocument();
}

bool MainWindow::loadFromFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Latrunculi", "Could not open the file for reading.");
        return false;
    }

    int rows = 0, cols = 0, perSide = 0, current = 0, placed0 = 0, placed1 = 0;
    Latrunculi::Phase phase = Latrunculi::Phase::Placement;
    std::vector<Latrunculi::Cell> board;
    std::vector<Latrunculi::Move> history;
    bool haveDims = false;
    // Default to the current colors, so a file lacking <colors> keeps them.
    QColor loadedColorA = colorA_;
    QColor loadedColorB = colorB_;
    QColor loadedBg     = background_;

    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement()) {
            continue;
        }
        const auto name = xml.name();
        const QXmlStreamAttributes a = xml.attributes();
        if (name == QLatin1String("dims")) {
            rows = a.value(QLatin1String("rows")).toInt();
            cols = a.value(QLatin1String("cols")).toInt();
            if (rows > 0 && cols > 0) {
                board.assign(static_cast<std::size_t>(rows * cols), Latrunculi::Cell::Empty);
                haveDims = true;
            }
        } else if (name == QLatin1String("perSide")) {
            perSide = a.value(QLatin1String("val")).toInt();
        } else if (name == QLatin1String("phase")) {
            phase = (a.value(QLatin1String("val")) == QLatin1String("Movement"))
                        ? Latrunculi::Phase::Movement : Latrunculi::Phase::Placement;
        } else if (name == QLatin1String("sideToMove")) {
            current = a.value(QLatin1String("val")).toInt();
        } else if (name == QLatin1String("placed")) {
            placed0 = a.value(QLatin1String("p0")).toInt();
            placed1 = a.value(QLatin1String("p1")).toInt();
        } else if (name == QLatin1String("colors")) {
            const QColor ca(a.value(QLatin1String("sideA")).toString());
            const QColor cb(a.value(QLatin1String("sideB")).toString());
            const QColor bg(a.value(QLatin1String("background")).toString());
            if (ca.isValid()) { loadedColorA = ca; }
            if (cb.isValid()) { loadedColorB = cb; }
            if (bg.isValid()) { loadedBg = bg; }
        } else if (name == QLatin1String("cell")) {
            if (!haveDims) {
                continue;
            }
            const int sq = a.value(QLatin1String("sq")).toInt();
            Latrunculi::Cell c = Latrunculi::Cell::Empty;
            if (sq >= 0 && sq < static_cast<int>(board.size())
                && parseCell(a.value(QLatin1String("state")).toString(), c)) {
                board[static_cast<std::size_t>(sq)] = c;
            }
        } else if (name == QLatin1String("move")) {
            Latrunculi::Move m;
            m.turn    = a.value(QLatin1String("turn")).toInt();
            m.player  = a.value(QLatin1String("player")).toInt();
            m.from    = a.value(QLatin1String("from")).toInt();
            m.to      = a.value(QLatin1String("to")).toInt();
            m.removed = a.value(QLatin1String("removed")).toInt();
            const QStringList parts =
                a.value(QLatin1String("path")).toString().split(' ', Qt::SkipEmptyParts);
            for (const QString& part : parts) {
                m.path.push_back(part.toInt());
            }
            history.push_back(m);
        }
    }

    if (xml.hasError() || !haveDims) {
        QMessageBox::warning(this, "Latrunculi", "Invalid or unreadable game file.");
        return false;
    }

    try {
        game_ = std::make_unique<Latrunculi::Game>(
            rows, cols, perSide, std::move(board), phase, current,
            placed0, placed1, std::move(history));
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, "Latrunculi", ex.what());
        return false;
    }

    stopSeed();
    search().cancelSearch();
    // The loaded move list becomes the replay timeline; positions are rebuilt from a
    // fresh game, so Load opens at ply 0 (empty board) ready to step forward.
    timeline_.assign(game_->history().begin(), game_->history().end());
    tlRows_ = rows;
    tlCols_ = cols;
    tlPerSide_ = perSide;
    suggestedLog_->clear();
    // Adopt the loaded colors (or the retained defaults if the file had none).
    colorA_     = loadedColorA;
    colorB_     = loadedColorB;
    background_ = loadedBg;
    // Repoint the board at the (valid) final game before any color rebuild, then
    // step the replay back to the start.
    boardWidget_->setGame(game_.get());
    boardWidget_->setSideColors(colorA_, colorB_);
    boardWidget_->setBackgroundColor(background_);
    rebuildMoveList();
    playback_->setPlyCount(static_cast<int>(timeline_.size()));
    gotoPly(0);
    return true;
}
// Copyright Ben Paul Wise. All Rights Reserved.
