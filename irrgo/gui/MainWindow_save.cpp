// Copyright Ben Paul Wise. All Rights Reserved.
#include "MainWindow.h"
#include "LoadedGraph.h"
#include "RectangularGraph.h"
#include <QActionGroup>
#include <QCheckBox>
#include <QFile>
#include <QFileDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <unordered_map>

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

    if (isRect) {
        const auto* rg = static_cast<const RectangularGraph*>(graph_.get());
        xml.writeEmptyElement("rows");
        xml.writeAttribute("val", QString::number(rg->rows()));
        xml.writeEmptyElement("cols");
        xml.writeAttribute("val", QString::number(rg->cols()));
    } else {
        xml.writeEmptyElement("seed");
        xml.writeAttribute("val", QString::number(graph_->seed()));
    }

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

void MainWindow::onLoad() {
    QString path = QFileDialog::getOpenFileName(
        this, "Load Game", QString(),
        "IrrGo XML files (*.xml);;All files (*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QXmlStreamReader xml(&file);

    bool isRect   = false;
    uint64_t xmlSeed = 0;
    int xmlRows = 0, xmlCols = 0;

    struct NodeRec  { std::string label; int row = 0, col = 0; };
    struct EdgeRec  { std::string a, b; };
    struct StoneRec { std::string label; Color color = Color::Empty; };
    struct MoveRec  { std::string label; };

    std::vector<NodeRec>  nodeRecs;
    std::vector<EdgeRec>  edgeRecs;
    std::vector<StoneRec> setupStones;
    std::vector<MoveRec>  playMoves;

    enum State { TOP, IN_COORD, IN_EDGE, IN_STONE, IN_MOVE };
    State state   = TOP;
    bool  inSetup = false;
    bool  inMoveList = false;
    NodeRec  curNode;
    EdgeRec  curEdge;
    StoneRec curStone;
    MoveRec  curMove;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            const auto name = xml.name();
            if (name == QLatin1String("rectangularP")) {
                isRect = xml.attributes().value("val") == QLatin1String("True");
            } else if (name == QLatin1String("rows")) {
                xmlRows = xml.attributes().value("val").toInt();
            } else if (name == QLatin1String("cols")) {
                xmlCols = xml.attributes().value("val").toInt();
            } else if (name == QLatin1String("seed")) {
                xmlSeed = xml.attributes().value("val").toULongLong();
            } else if (name == QLatin1String("coord")) {
                state = IN_COORD; curNode = {};
            } else if (name == QLatin1String("edge")) {
                state = IN_EDGE; curEdge = {};
            } else if (name == QLatin1String("Position")) {
                inSetup = xml.attributes().value("type") == QLatin1String("setup");
            } else if (name == QLatin1String("move_list")) {
                inMoveList = true;
            } else if (name == QLatin1String("stone") && inSetup) {
                state = IN_STONE; curStone = {};
            } else if (name == QLatin1String("move") && inMoveList) {
                state = IN_MOVE; curMove = {};
            } else if (name == QLatin1String("node")) {
                auto id = xml.attributes().value("id").toString().toStdString();
                if      (state == IN_COORD) curNode.label = id;
                else if (state == IN_EDGE)  { if (curEdge.a.empty()) curEdge.a = id; else curEdge.b = id; }
                else if (state == IN_STONE) curStone.label = id;
                else if (state == IN_MOVE)  curMove.label  = id;
            } else if (name == QLatin1String("R") && state == IN_COORD) {
                curNode.row = xml.attributes().value("val").toInt();
            } else if (name == QLatin1String("C") && state == IN_COORD) {
                curNode.col = xml.attributes().value("val").toInt();
            } else if (name == QLatin1String("color") && state == IN_STONE) {
                curStone.color = (xml.attributes().value("val") == QLatin1String("Black"))
                    ? Color::Black : Color::White;
            }
        } else if (xml.isEndElement()) {
            const auto name = xml.name();
            if      (name == QLatin1String("coord"))     { nodeRecs.push_back(curNode); state = TOP; }
            else if (name == QLatin1String("edge"))      { edgeRecs.push_back(curEdge); state = TOP; }
            else if (name == QLatin1String("stone") && inSetup) { setupStones.push_back(curStone); state = TOP; }
            else if (name == QLatin1String("move") && inMoveList) { playMoves.push_back(curMove); state = TOP; }
            else if (name == QLatin1String("move_list")) inMoveList = false;
        }
    }

    if (xml.hasError() || nodeRecs.empty()) return;

    std::vector<LoadedGraph::NodeData> graphNodes;
    graphNodes.reserve(nodeRecs.size());
    for (const auto& nr : nodeRecs)
        graphNodes.push_back({nr.label, nr.row, nr.col});

    std::unordered_map<std::string, int> labelToId;
    for (int i = 0; i < static_cast<int>(graphNodes.size()); ++i)
        labelToId[graphNodes[i].label] = i;

    std::vector<LoadedGraph::EdgeData> graphEdges;
    graphEdges.reserve(edgeRecs.size());
    for (const auto& er : edgeRecs)
        graphEdges.push_back({er.a, er.b});

    std::unique_ptr<Graph> newGraph;
    if (isRect && xmlRows > 0 && xmlCols > 0)
        newGraph = std::make_unique<RectangularGraph>(xmlRows, xmlCols);
    else
        newGraph = std::make_unique<LoadedGraph>(graphNodes, graphEdges, xmlSeed);
    auto newGame = std::make_unique<Game>(*newGraph);

    int newSetupPlaced = 0;
    newGame->setSetupMode(true);
    for (const auto& s : setupStones) {
        auto it = labelToId.find(s.label);
        if (it != labelToId.end() && newGame->placeStone(it->second))
            ++newSetupPlaced;
    }
    newGame->setSetupMode(false);

    for (const auto& m : playMoves) {
        if (m.label == "PASS") {
            newGame->pass();
        } else {
            auto it = labelToId.find(m.label);
            if (it != labelToId.end())
                newGame->placeStone(it->second);
        }
    }

    cancelSearch();
    stopStoneSetup();
    clearSuggestion();

    graph_           = std::move(newGraph);
    game_            = std::move(newGame);
    setupPlaced_     = newSetupPlaced;
    currentFilePath_ = path;

    boardWidget_->setGame(game_.get());
    blackDvrCheck_->setChecked(false);
    whiteDvrCheck_->setChecked(false);
    boardWidget_->setBgColor(isRect ? QColor("#F2B06D") : QColor("#0C7F84"));
    boardWidget_->setBoardInfo(isRect
        ? QString("Loaded (%1x%2)").arg(xmlRows).arg(xmlCols)
        : QString("Loaded, seed %1").arg(xmlSeed));
    if (!isRect)
        randomSeedEdit_->setText(QString::number(xmlSeed));
    stonesGroup_->actions().first()->setChecked(true);

    moveLog_->clear();
    for (const auto& mv : game_->moveHistory()) {
        QString clr = (mv.turn % 2 == 1) ? "B" : "W";
        QString text = (mv.nodeId < 0)
            ? QString("%1: %2 PASS").arg(mv.turn, 3).arg(clr)
            : QString("%1: %2 %3")
                  .arg(mv.turn, 3).arg(clr)
                  .arg(QString::fromStdString(game_->graph().node(mv.nodeId).label));
        moveLog_->append(text);
    }

    updateControls();
}
// Copyright Ben Paul Wise. All Rights Reserved.
