// Copyright Ben Paul Wise. All Rights Reserved.
#include "MainWindow.h"
#include "RectangularGraph.h"
#include <QFile>
#include <QFileDialog>
#include <QXmlStreamWriter>

using namespace IrrGo;

void MainWindow::onSave() {
    if (currentFilePath_.isEmpty())
        onSaveAs();
    else
        saveToFile(currentFilePath_);
}

void MainWindow::onSaveAs() {
    QString path = QFileDialog::getSaveFileName(
        this, "Save Game", QString(),
        "IrrGo XML files (*.xml);;All files (*)");
    if (path.isEmpty()) return;
    currentFilePath_ = path;
    saveToFile(currentFilePath_);
}

void MainWindow::saveToFile(const QString& path) {
    if (!game_) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    const auto& nodes   = graph_->nodes();
    const auto& history = game_->moveHistory();
    bool isRect = dynamic_cast<const RectangularGraph*>(graph_.get()) != nullptr;

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();

    xml.writeStartElement("IrrGoBoard");
    xml.writeAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    xml.writeAttribute("xsi:noNamespaceSchemaLocation", "irrgo.xsd");

    xml.writeEmptyElement("rectangularP");
    xml.writeAttribute("val", isRect ? "True" : "False");

    xml.writeStartElement("coord_list");
    for (const auto& nd : nodes) {
        xml.writeStartElement("coord");
        xml.writeEmptyElement("node");
        xml.writeAttribute("id", QString::fromStdString(nd.label));
        xml.writeEmptyElement("R");
        xml.writeAttribute("val", QString::number(nd.row));
        xml.writeEmptyElement("C");
        xml.writeAttribute("val", QString::number(nd.col));
        xml.writeEndElement(); // coord
    }
    xml.writeEndElement(); // coord_list

    xml.writeStartElement("edge_list");
    for (const auto& nd : nodes) {
        for (int nb : nd.neighbors) {
            if (nd.id < nb) {
                xml.writeStartElement("edge");
                xml.writeEmptyElement("node");
                xml.writeAttribute("id", QString::fromStdString(nd.label));
                xml.writeEmptyElement("node");
                xml.writeAttribute("id", QString::fromStdString(nodes[nb].label));
                xml.writeEndElement(); // edge
            }
        }
    }
    xml.writeEndElement(); // edge_list

    xml.writeStartElement("Game");

    xml.writeEmptyElement("sideToMove");
    xml.writeAttribute("color", game_->toMove() == Player::Black ? "Black" : "White");

    // Setup position: moves placed before play (first setupPlaced_ entries)
    xml.writeStartElement("Position");
    xml.writeAttribute("type", "setup");
    for (int i = 0; i < setupPlaced_ && i < static_cast<int>(history.size()); ++i) {
        const auto& mv = history[i];
        if (mv.nodeId >= 0) {
            xml.writeStartElement("stone");
            xml.writeEmptyElement("node");
            xml.writeAttribute("id", QString::fromStdString(nodes[mv.nodeId].label));
            xml.writeEmptyElement("color");
            xml.writeAttribute("val", mv.color == Color::Black ? "Black" : "White");
            xml.writeEndElement(); // stone
        }
    }
    xml.writeEndElement(); // Position setup

    // Play moves (everything after setup)
    xml.writeStartElement("move_list");
    for (int i = setupPlaced_; i < static_cast<int>(history.size()); ++i) {
        const auto& mv = history[i];
        xml.writeStartElement("move");
        xml.writeEmptyElement("node");
        xml.writeAttribute("id", mv.nodeId >= 0
            ? QString::fromStdString(nodes[mv.nodeId].label)
            : "PASS");
        xml.writeEmptyElement("color");
        xml.writeAttribute("val", mv.color == Color::Black ? "Black" : "White");
        xml.writeEmptyElement("movenum");
        xml.writeAttribute("num", QString::number(mv.turn));
        xml.writeEndElement(); // move
    }
    xml.writeEndElement(); // move_list

    // After-play position: current board state
    xml.writeStartElement("Position");
    xml.writeAttribute("type", "after-play");
    for (const auto& nd : nodes) {
        Color c = game_->colorAt(nd.id);
        if (c != Color::Empty) {
            xml.writeStartElement("stone");
            xml.writeEmptyElement("node");
            xml.writeAttribute("id", QString::fromStdString(nd.label));
            xml.writeEmptyElement("color");
            xml.writeAttribute("val", c == Color::Black ? "Black" : "White");
            xml.writeEndElement(); // stone
        }
    }
    xml.writeEndElement(); // Position after-play

    xml.writeEndElement(); // Game
    xml.writeEndElement(); // IrrGoBoard
    xml.writeEndDocument();
}
// Copyright Ben Paul Wise. All Rights Reserved.
