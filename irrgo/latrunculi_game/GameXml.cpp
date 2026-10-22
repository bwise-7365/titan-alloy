// Copyright Ben Paul Wise. All Rights Reserved.
#include "GameXml.h"

#include <cctype>
#include <iomanip>
#include <locale>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace Latrunculi {

namespace {

const char* cellName(Cell c) {
    switch (c) {
        case Cell::P0Free:  return "P0Free";
        case Cell::P0Bound: return "P0Bound";
        case Cell::P1Free:  return "P1Free";
        case Cell::P1Bound: return "P1Bound";
        case Cell::Empty:   return "Empty";
    }
    // Unreachable for a valid Cell. Refuse rather than name it "Empty": a board written
    // with a corrupted cell would reload as a different position.
    throw std::invalid_argument("writeGameXml: unrecognised cell state");
}

// doc/latrunculi.xsd restricts every colour to the pattern #[0-9a-fA-F]{6}.
void requireHexColor(const std::string& value, const char* what) {
    bool ok = (value.size() == 7) && (value[0] == '#');
    for (std::size_t i = 1; ok && (i < value.size()); ++i) {
        ok = (std::isxdigit(static_cast<unsigned char>(value[i])) != 0);
    }
    if (!ok) {
        throw std::invalid_argument(std::string("writeGameXml: ") + what +
                                    " must be #rrggbb, got \"" + value + "\"");
    }
}

// The move path as the schema's squareList: space-separated indices, possibly empty.
std::string pathText(const std::vector<int>& path) {
    std::string out;
    for (std::size_t i = 0; i < path.size(); ++i) {
        if (i > 0) {
            out += ' ';
        }
        out += std::to_string(path[i]);
    }
    return out;
}

// Komi as the schema's halfInteger ("[0-9]+\.5"), matching the GUI's
// QString::number(komi, 'f', 1).
std::string komiText(double komi) {
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os << std::fixed << std::setprecision(1) << komi;
    return os.str();
}

}  // namespace

void writeGameXml(std::ostream& out, const Game& game, const XmlColors& colors) {
    requireHexColor(colors.sideA, "sideA");
    requireHexColor(colors.sideB, "sideB");
    requireHexColor(colors.background, "background");

    out.imbue(std::locale::classic());

    const bool placing = (game.phase() == Phase::Placement);

    // placed_ is needed to resume a placement-phase game; in movement it equals perSide.
    // No captures occur during placement, so the board gives it directly. This mirrors
    // latrunculi_gui/MainWindow_save.cpp.
    const int placed0 = placing ? game.totalDiscs(0) : game.perSide();
    const int placed1 = placing ? game.totalDiscs(1) : game.perSide();

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<LatrunculiBoard xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
        << " xsi:noNamespaceSchemaLocation=\"latrunculi.xsd\">\n"
        << "    <dims rows=\"" << game.rows() << "\" cols=\"" << game.columns() << "\"/>\n"
        << "    <perSide val=\"" << game.perSide() << "\"/>\n"
        << "    <movement val=\""
        << (game.moveStyle() == MoveStyle::Slide ? "Slide" : "StepLeap") << "\"/>\n"
        << "    <komi val=\"" << komiText(game.komi()) << "\"/>\n"
        << "    <phase val=\"" << (placing ? "Placement" : "Movement") << "\"/>\n"
        << "    <sideToMove val=\"" << game.currentPlayer() << "\"/>\n"
        << "    <placed p0=\"" << placed0 << "\" p1=\"" << placed1 << "\"/>\n"
        << "    <colors sideA=\"" << colors.sideA << "\" sideB=\"" << colors.sideB
        << "\" background=\"" << colors.background << "\"/>\n";

    // Authoritative board state: non-empty squares only.
    out << "    <Position>\n";
    for (int s = 0; s < game.squareCount(); ++s) {
        const Cell c = game.cellAt(s);
        if (c == Cell::Empty) {
            continue;
        }
        out << "        <cell sq=\"" << s << "\" state=\"" << cellName(c) << "\"/>\n";
    }
    out << "    </Position>\n";

    // Move log (display only; the position above is what reconstruction uses).
    out << "    <move_list>\n";
    for (const Move& m : game.history()) {
        out << "        <move turn=\"" << m.turn << "\" player=\"" << m.player
            << "\" from=\"" << m.from << "\" to=\"" << m.to
            << "\" removed=\"" << m.removed
            << "\" path=\"" << pathText(m.path) << "\"/>\n";
    }
    out << "    </move_list>\n"
        << "</LatrunculiBoard>\n";

    if (!out) {
        throw std::runtime_error("writeGameXml: the output stream failed while writing");
    }
}

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.
