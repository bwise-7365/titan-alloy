// Copyright Ben Paul Wise. All Rights Reserved.
#include "GameXml.h"

#include "RectangularGraph.h"

#include <locale>
#include <ostream>
#include <stdexcept>
#include <string>

namespace IrrGo {

namespace {

const char* colorName(Color c) {
    switch (c) {
        case Color::Black: return "Black";
        case Color::White: return "White";
        case Color::Empty: return "Vacant";
    }
    // Unreachable for a valid Color. Refuse rather than pick one: a stone written under
    // the wrong colour would reload as a different position.
    throw std::invalid_argument("writeGameXml: unrecognised stone colour");
}

// Node labels are "%02d%02d" of row and col, so they contain only digits today. Escape
// anyway: the writer should stay correct if labels ever carry anything else, and a
// silently malformed document is exactly what we cannot afford in a 1000-game batch.
std::string xmlAttr(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (const char ch : value) {
        switch (ch) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default:   out += ch;       break;
        }
    }
    return out;
}

void writeStone(std::ostream& out, const char* indent, const std::string& label,
                Color color) {
    out << indent << "<stone>\n"
        << indent << "    <node id=\"" << xmlAttr(label) << "\"/>\n"
        << indent << "    <color val=\"" << colorName(color) << "\"/>\n"
        << indent << "</stone>\n";
}

}  // namespace

void writeGameXml(std::ostream& out, const Game& game, const Graph& graph,
                  int setupCount) {
    const std::vector<Node>& nodes = graph.nodes();
    const std::vector<Move>& history = game.moveHistory();

    if ((setupCount < 0) || (setupCount > static_cast<int>(history.size()))) {
        throw std::invalid_argument("writeGameXml: setupCount " +
                                    std::to_string(setupCount) +
                                    " is outside the move history (" +
                                    std::to_string(history.size()) + " entries)");
    }

    out.imbue(std::locale::classic());

    const auto* rect = dynamic_cast<const RectangularGraph*>(&graph);

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<IrrGoBoard xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
        << " xsi:noNamespaceSchemaLocation=\"irrgo.xsd\">\n"
        << "    <rectangularP val=\"" << (rect ? "True" : "False") << "\"/>\n";
    if (rect) {
        out << "    <rows val=\"" << rect->rows() << "\"/>\n"
            << "    <cols val=\"" << rect->cols() << "\"/>\n";
    } else {
        out << "    <seed val=\"" << graph.seed() << "\"/>\n";
    }

    out << "    <coord_list>\n";
    for (const Node& nd : nodes) {
        out << "        <coord>\n"
            << "            <node id=\"" << xmlAttr(nd.label) << "\"/>\n"
            << "            <R val=\"" << nd.row << "\"/>\n"
            << "            <C val=\"" << nd.col << "\"/>\n"
            << "        </coord>\n";
    }
    out << "    </coord_list>\n";

    // Each undirected edge once, from the lower-id endpoint (as the GUI writes it).
    out << "    <edge_list>\n";
    for (const Node& nd : nodes) {
        for (const int nb : nd.neighbors) {
            if (nd.id < nb) {
                out << "        <edge>\n"
                    << "            <node id=\"" << xmlAttr(nd.label) << "\"/>\n"
                    << "            <node id=\"" << xmlAttr(nodes[nb].label) << "\"/>\n"
                    << "        </edge>\n";
            }
        }
    }
    out << "    </edge_list>\n";

    out << "    <Game>\n"
        << "        <sideToMove color=\""
        << (game.toMove() == Player::Black ? "Black" : "White") << "\"/>\n";

    // Stones placed before play began. Passes carry nodeId -1 and are not placements.
    out << "        <Position type=\"setup\">\n";
    for (int i = 0; i < setupCount; ++i) {
        const Move& mv = history[static_cast<std::size_t>(i)];
        if (mv.nodeId >= 0) {
            writeStone(out, "            ", nodes[mv.nodeId].label, mv.color);
        }
    }
    out << "        </Position>\n";

    out << "        <move_list>\n";
    for (std::size_t i = static_cast<std::size_t>(setupCount); i < history.size(); ++i) {
        const Move& mv = history[i];
        out << "            <move>\n"
            << "                <node id=\""
            << (mv.nodeId >= 0 ? xmlAttr(nodes[mv.nodeId].label) : std::string("PASS"))
            << "\"/>\n"
            << "                <color val=\""
            << (mv.color == Color::Black ? "Black" : "White") << "\"/>\n"
            << "                <movenum num=\"" << mv.turn << "\"/>\n"
            << "            </move>\n";
    }
    out << "        </move_list>\n";

    out << "        <Position type=\"after-play\">\n";
    for (const Node& nd : nodes) {
        const Color c = game.colorAt(nd.id);
        if (c != Color::Empty) {
            writeStone(out, "            ", nd.label, c);
        }
    }
    out << "        </Position>\n"
        << "    </Game>\n"
        << "</IrrGoBoard>\n";

    if (!out) {
        throw std::runtime_error("writeGameXml: the output stream failed while writing");
    }
}

}  // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.
