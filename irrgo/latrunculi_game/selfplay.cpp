// Copyright Ben Paul Wise. All Rights Reserved.
//
// Self-play validation driver for the Milestone-1 Latrunculi engine. Plays a full
// game (placement is filled deterministically; movement uses NegaMax) and prints
// the move log in chess-like notation, then the result. A movement-ply cap stands
// in for the not-yet-implemented super-ko / draw rule so the game always ends.

#include "Game.h"
#include "Searcher.h"
#include "utils.h"  // AbsGame::dSeed

#include "draw_params.h"
#include "irregular_grid.h"  // generate_position_svg, BoardSpec, PlacedPiece

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
    if (num.size() < 2) {
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
// (A = side_a colour, B = side_b), an X on each Bound (immobilised) disc.
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
            gb::PlacedPiece{s, (owner == 0) ? colors.side_a : colors.side_b, bound});
    }
    render_svg_to_png(gb::generate_position_svg(look, pieces, config, style), frame_name(frame));
}

// True if any orthogonal neighbour of `square` is occupied by `opponent`. Used to
// keep the random opening capture-free (no piece placed next to an opponent).
bool adjacentToOpponent(const Latrunculi::Game& game, int square, int opponent,
                        int rows, int columns) {
    const int r = square / columns;
    const int c = square % columns;
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};
    for (int k = 0; k < 4; ++k) {
        const int nr = r + dr[k];
        const int nc = c + dc[k];
        if (nr < 0 || nr >= rows || nc < 0 || nc >= columns) {
            continue;
        }
        if (game.ownerAt(nr * columns + nc) == opponent) {
            return true;
        }
    }
    return false;
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
        out += notate(m.from, rows, columns) + "-" + notate(m.to, rows, columns);
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

    const int rows = 6;
    const int columns = 6;
    const int perSide = 9;               // 18 discs on 36 squares, placed apart
    const int searchDepth = 3;
    const int searchMs = 1000;
    const int maxMovementPlies = 2 * rows * columns;  // safety cap (no super-ko yet)

    Game game(rows, columns, perSide);

    std::cout << "Latrunculi self-play: " << rows << "x" << columns << ", "
              << perSide << " discs per side.\n\n";

    // Position rendering (irregular_grids): a hand-scratched board the same size.
    gb::BoardSpec look;
    look.grid.rows = rows;
    look.grid.columns = columns;
    gb::apply_draw_defaults(look);  // disc shape, outline, outer box, X-mark
    const gb::RenderConfig renderCfg = gb::default_render_config();
    const gb::SvgStyle svgStyle = gb::default_svg_style();
    const gb::PieceColors pieceColors = gb::piece_colors();
    write_position_png(game, look, renderCfg, svgStyle, pieceColors, 0);  // empty board

    bool announcedMovement = false;
    int movementPlies = 0;
    std::mt19937_64 rng(AbsGame::dSeed);

    std::cout << "-- placement phase (" << 2 * perSide << " placements) --\n";
    while (!game.isTerminal()) {
        const std::vector<AbsGame::MoveId> moves = game.getLegalMoves();
        if (moves.empty()) {
            break;  // guard; isTerminal() should already be true
        }

        if (game.phase() == Phase::Movement && !announcedMovement) {
            std::cout << "\n-- movement phase --\n";
            announcedMovement = true;
        }

        AbsGame::MoveId mv = moves.front();
        if (game.phase() == Phase::Placement) {
            // Random capture-free opening: a random empty square not orthogonally
            // adjacent to an opponent disc, so play starts without forced captures.
            // Fall back to any legal placement if the constraint cannot be met.
            const int opponent = 1 - game.currentPlayer();
            std::vector<AbsGame::MoveId> candidates;
            candidates.reserve(moves.size());
            for (AbsGame::MoveId s : moves) {
                if (!adjacentToOpponent(game, s, opponent, rows, columns)) {
                    candidates.push_back(s);
                }
            }
            const std::vector<AbsGame::MoveId>& pool = candidates.empty() ? moves : candidates;
            mv = pool[rng() % pool.size()];
        } else {
            const AbsGame::MoveId best =
                AbsGame::Searcher::bestMove(game, searchDepth, searchMs);
            if (best != AbsGame::kPass && game.isLegalMove(best)) {
                mv = best;
            }
            ++movementPlies;
        }

        game.applyMove(mv);
        std::cout << format_move(game.history().back(), rows, columns) << '\n';
        write_position_png(game, look, renderCfg, svgStyle, pieceColors,
                           static_cast<int>(game.history().size()));

        if (movementPlies >= maxMovementPlies) {
            std::cout << "\n[movement-ply cap reached -> treat as a draw]\n";
            break;
        }
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
        std::cout << "Draw: A " << game.totalDiscs(0) << " discs vs B "
                  << game.totalDiscs(1) << ".\n";
    }
    std::cout << "Total plies: " << game.history().size()
              << " (movement plies: " << movementPlies << ").\n";
    return 0;
}
// Copyright Ben Paul Wise. All Rights Reserved.
