// Copyright Ben Paul Wise. All Rights Reserved.
//
// Self-play validation driver for the Milestone-1 Latrunculi engine. Plays a full
// game (placement is filled deterministically; movement uses NegaMax) and prints
// the move log in chess-like notation, then the result. The game plays to its end:
// the engine guarantees termination via super-ko and the Pacific quiet-game rule.

#include "Game.h"
#include "PlacementPolicy.h"  // Latrunculi::PlacementPolicy (shared with the GUI)
#include "Searcher.h"  // AbsGame::Searcher, AbsGame::makeSeed (via AbsGame.h)

#include "draw_params.h"
#include "irregular_grid.h"  // generate_position_svg, BoardSpec, PlacedPiece

#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <QByteArray>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QString>
#include <QSvgRenderer>

namespace gb = games::board;

namespace {

// Zero-padded (>=2 digit) frame file name, e.g. pos_00.png, pos_07.png, pos_42.png.
std::string frame_name(int n) {
    std::string num = std::to_string(n);
    if (1 == num.size()) {
        num = "00" + num;
    }
    if (2 == num.size()) {
        num = "0" + num;
    }
    return "pos_" + num + ".png";
}

// Rasterises an SVG document to a PNG file using Qt's native SVG renderer.
void render_svg_to_png(const std::string& svg, const std::string& path) {
    QSvgRenderer renderer(QByteArray::fromStdString(svg));
    QImage image(renderer.defaultSize(), QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    renderer.render(&painter);
    painter.end();
    image.save(QString::fromStdString(path), "PNG");
}

// Renders the current position to pos_NN.png: a disc per occupied square
// (A = side_a color, B = side_b), an X on each Bound (immobilised) disc.
void write_position_png(const Latrunculi::Game& game, const gb::BoardSpec& look,
                        const gb::RenderConfig& config, const gb::SvgStyle& style,
                        const gb::PieceColors& colors, int frame) {
    std::vector<gb::PlacedPiece> pieces;
    for (int s = 0; s < game.squareCount(); ++s) {
        const int owner = game.ownerAt(s);
        if (owner < 0) {
            continue;
        }
        const Latrunculi::Cell c = game.cellAt(s);
        const bool bound = (c == Latrunculi::Cell::P0Bound || c == Latrunculi::Cell::P1Bound);
        pieces.push_back(
            gb::PlacedPiece{s, (owner == 0) ? colors.side_a : colors.side_b, bound, owner});
    }
    render_svg_to_png(gb::generate_position_svg(look, pieces, config, style), frame_name(frame));
}

// Chess-like notation matching the SVG edge labels: column letter 'A'+col, row
// number with 1 at the bottom. (Mirrors games::board::square_to_notation.)
std::string notate(int square, int rows, int columns) {
    const int column = square % columns;
    const int row = square / columns;
    std::string out(1, static_cast<char>('A' + column));
    out += std::to_string(rows - row);
    return out;
}

std::string format_move(const Latrunculi::Move& m, int rows, int columns) {
    const char side = (m.player == 0) ? 'A' : 'B';  // A = side_a (player 0), B = side_b
    std::string out = std::to_string(m.turn) + ". " + side + ": ";
    if (m.from < 0) {
        out += "place " + notate(m.to, rows, columns);
    } else {
        // from -> landing -> ... -> to (a slide is two squares; a multi-leap lists
        // each landing so the leapt-over squares are implied between them).
        if (m.path.empty()) {
            out += notate(m.from, rows, columns) + " -> " + notate(m.to, rows, columns);
        } else {
            bool first = true;
            for (int sq : m.path) {
                if (!first) {
                    out += " -> ";
                }
                first = false;
                out += notate(sq, rows, columns);
            }
        }
        if (m.removed >= 0) {
            out += "  (remove " + notate(m.removed, rows, columns) + ")";
        }
    }
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);  // initialises Qt (fonts, image backend) for rendering
    using namespace Latrunculi;

    const int rows = 8;
    const int columns = 10;
    const int perSide = 20;              // 40 discs on 80 squares
    const int searchDepth = 6;
    const int searchMs = 1000;
    // Which movement rule to play under. Switch to MoveStyle::StepLeap to run the same
    // driver over the older Seneca rules; everything else is identical, so the two runs
    // are directly comparable (see MoveStyle in Game.h).
    const MoveStyle moveStyle = MoveStyle::Slide;

    Game game(rows, columns, perSide, moveStyle);

    std::cout << "Latrunculi self-play: " << rows << "x" << columns << ", "
              << perSide << " discs per side, movement = "
              << (moveStyle == MoveStyle::Slide ? "Kharebga slide" : "Seneca step/leap")
              << ".\n\n";

    // Position rendering (irregular_grids): a hand-scratched board the same size.
    gb::BoardSpec look;
    look.grid.rows = rows;
    look.grid.columns = columns;
    gb::apply_draw_defaults(look);  // disc shape, outline, outer box, X-mark
    const gb::RenderConfig renderCfg = gb::default_render_config();
    const gb::SvgStyle svgStyle = gb::default_svg_style();
    const gb::PieceColors pieceColors = gb::piece_colors();
    // Immobilisation "X" is drawn in the opponent's color (shows who immobilised it).
    look.mark_color_p0 = pieceColors.side_b;  // X on an A (player 0) disc = B's color
    look.mark_color_p1 = pieceColors.side_a;  // X on a  B (player 1) disc = A's color
    write_position_png(game, look, renderCfg, svgStyle, pieceColors, 0);  // empty board

    bool announcedMovement = false;
    int movementPlies = 0;
    // Placement RNG seed: 0 => clock-derived (a different game each run); set to a
    // printed value to reproduce a particular game.
    // 5824362463858583254
    // 9659779445541695208
    const std::uint64_t seedInput = 0;
    const std::uint64_t placementSeed = AbsGame::makeSeed(seedInput);
    // Opening variety: one random placement, then a run of searched ones, per side.
    // Shared with the GUI so both drivers open the same way (see PlacementPolicy.h).
    PlacementPolicy placement(placementSeed);
    std::cout << "placement seed: " << placementSeed << '\n';

    std::cout << "-- placement phase (" << 2 * perSide << " placements, "
              << "random then runs of " << PlacementPolicy::kRunMin << "-"
              << PlacementPolicy::kRunMin + 1 << " searched) --\n";
    while (!game.isTerminal()) {
        const std::vector<AbsGame::MoveId> moves = game.getLegalMoves();
        if (moves.empty()) {
            // The engine ends any position with no legal move (Game::
            // checkImmobilizationTerminal, both phases), so isTerminal() must already
            // have been true and the loop must not have reached here. Reaching it means
            // the engine disagrees with itself; say so rather than breaking out and
            // reporting whatever the score happens to be.
            std::cerr << "error: no legal moves at ply " << game.history().size() + 1
                      << " but the game is not terminal\n";
            return 1;
        }

        if (game.phase() == Phase::Movement && !announcedMovement) {
            std::cout << "\n-- movement phase --\n";
            announcedMovement = true;
        }

        // Decide whether this ply is searched or random. Movement is always searched;
        // placement follows the alternating policy described above. Random placements
        // range over every legal placement, adjacent enemies included -- the placement
        // rule already forbids the only thing that must not happen, a placement that
        // completes a custodial capture (Game::isLegalPlacement).
        AbsGame::MoveId mv = moves.front();
        bool randomPlacement = false;

        if (game.phase() == Phase::Placement) {
            randomPlacement = placement.nextIsRandom(game.currentPlayer());
            if (randomPlacement) {
                mv = placement.pickRandomPlacement(game, moves);
            }
        } else {
            ++movementPlies;
        }

        if (!randomPlacement) {
            const AbsGame::MoveId best =
                AbsGame::Searcher::bestMove(game, searchDepth, searchMs);
            if (best != AbsGame::kPass && game.isLegalMove(best)) {
                mv = best;
            } else {
                // The searcher returned nothing usable although legal moves exist. That
                // is an engine fault, not a position to shrug at: say so rather than
                // quietly playing moves.front() as if it had been chosen.
                std::cerr << "warning: search returned no usable move at ply "
                          << game.history().size() + 1 << "; playing the first legal move\n";
            }
        }

        game.applyMove(mv);
        std::cout << format_move(game.history().back(), rows, columns)
                  << (randomPlacement ? "  [random]" : "") << '\n';
        write_position_png(game, look, renderCfg, svgStyle, pieceColors,
                           static_cast<int>(game.history().size()));
    }

    std::cout << "\n--- Result ---\n";
    if (game.isOver()) {
        const int w = game.winner();
        const int l = 1 - w;
        const char winner = (w == 0) ? 'A' : 'B';
        const char loser = (l == 0) ? 'A' : 'B';
        std::cout << winner << " wins: " << game.totalDiscs(w) << " discs ("
                  << game.freeDiscs(w) << " free, " << game.boundDiscs(w) << " bound) vs "
                  << game.totalDiscs(l) << " for " << loser << ".\n";
    } else {
        // Komi makes an exact material tie impossible and every terminal names a winner,
        // so this branch is unreachable. It used to print "Draw", which invented a result
        // the rules do not contain and hid a real engine fault behind it.
        std::cerr << "error: the game loop ended with a game that is not over (A "
                  << game.totalDiscs(0) << " discs vs B " << game.totalDiscs(1) << ")\n";
        return 1;
    }
    std::cout << "Total plies: " << game.history().size()
              << " (movement plies: " << movementPlies << ").\n";
    return 0;
}
// Copyright Ben Paul Wise. All Rights Reserved.
