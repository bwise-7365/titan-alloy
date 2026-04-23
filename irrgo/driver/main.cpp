// Copyright Ben Paul Wise. All Rights Reserved.
#include "Game.h"
#include "IrregularGraph.h"
#include "RectangularGraph.h"
#include <iostream>

using namespace IrrGo;

int main() {

    std::cout << "=== Rectangular 9x9 ===\n";
    RectangularGraph r_9_9(9, 9);
    std::cout << r_9_9.asciiRepresentation() << '\n';

    std::cout << "=== Rectangular 9x13 ===\n";
    RectangularGraph r_9_13(9, 13);
    std::cout << r_9_13.asciiRepresentation() << '\n';

    std::cout << "=== Irregular 5x5, maxDegree=4, seed=42 ===\n";
    IrregularGraph irr5(5, 5, 4, 42ULL);
    std::cout << irr5.asciiRepresentation() << '\n';

    std::cout << "=== Irregular 9x9, maxDegree=4, seed=42 ===\n";
    IrregularGraph irr(9, 9, 4, 42ULL);
    std::cout << "Nodes: " << irr.nodeCount() << '\n';
    std::cout << irr.asciiRepresentation() << '\n';

    std::cout << "=== Sample game on 9x9 rectangular ===\n";
    RectangularGraph g(9, 9);
    Game game(g, 1.5);
    game.placeStone(12);  // Black center
    game.placeStone(6);   // White near center
    game.placeStone(7);   // Black
    game.placeStone(11);  // White
    std::cout << "Current player: "
              << (game.toMove() == Player::Black ? "Black" : "White") << '\n';
    game.pass();
    game.pass();
    auto result = game.score();
    std::cout << "Black: " << result.blackScore
              << "  White: " << result.whiteScore
              << "  Winner: " << (result.winner == Player::Black ? "Black" : "White") << '\n';

    return 0;
}
// Copyright Ben Paul Wise. All Rights Reserved.
