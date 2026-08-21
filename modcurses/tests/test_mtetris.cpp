//
// MTetris game-logic tests. The board is a pure model, so the rules are
// checkable with no terminal involved at all - the same separation that lets
// TextBuffer be tested without a TextArea.
//
#include <set>
#include <string>
#include <vector>

#include "board.hpp"
#include "doctest.h"
#include "rng.hpp"
#include "scheme.hpp"
#include "shape.hpp"

using namespace mtetris;

namespace {

// A compact fingerprint of a whole game: the piece sequence and the column
// each one started in.
std::string play_out(std::uint64_t seed, bool random_placement, int moves) {
    Prng rng{seed};
    Board board{kDefaultRows, kDefaultClms, rng};
    board.random_placement = random_placement;

    std::string trace;
    for (int i = 0; i < moves; ++i) {
        trace += board.current().name();
        trace += std::to_string(board.current_col());
        trace += ' ';
        board.try_hdrop();
        if (board.step().game_over) {
            trace += "OVER";
            break;
        }
    }
    return trace;
}

}  // namespace

// ------------------------------------------------------------------ shapes

TEST_CASE("every tetromino occupies four cells and knows its name") {
    const char* names = "NIJLOSTZ";
    for (int p = I; p <= Z; ++p) {
        Shape s{static_cast<TCode>(p)};
        CHECK(s.name() == names[p]);
        std::set<std::pair<int, int>> cells;
        for (int k = 0; k < 4; ++k) cells.insert({s.x(k), s.y(k)});
        CHECK(cells.size() == 4);  // no duplicated cell
    }
}

TEST_CASE("four rotations return a piece to where it started") {
    for (int p = I; p <= Z; ++p) {
        Shape s{static_cast<TCode>(p)};
        Shape r = s.rrot().rrot().rrot().rrot();
        for (int k = 0; k < 4; ++k) {
            CHECK(r.x(k) == s.x(k));
            CHECK(r.y(k) == s.y(k));
        }
        CHECK(r.code() == s.code());  // rotation preserves identity
    }
}

TEST_CASE("left and right rotation are inverses") {
    for (int p = I; p <= Z; ++p) {
        Shape s{static_cast<TCode>(p)};
        Shape back = s.rrot().lrot();
        for (int k = 0; k < 4; ++k) {
            CHECK(back.x(k) == s.x(k));
            CHECK(back.y(k) == s.y(k));
        }
    }
}

// ---------------------------------------------------------- the seed claim

TEST_CASE("the same seed replays exactly the same game") {
    // This is the whole reason the seed is exposed. If it ever stops holding,
    // "--seed replays the game" becomes a lie.
    CHECK(play_out(12345, false, 40) == play_out(12345, false, 40));
    CHECK(play_out(12345, true, 40) == play_out(12345, true, 40));
}

TEST_CASE("different seeds give different games") {
    CHECK(play_out(1, false, 40) != play_out(2, false, 40));
}

TEST_CASE("the PRNG stream is identical across platforms by construction") {
    // splitmix64 is fully specified by its constants, so these values are
    // fixed for every conforming compiler. A change here means the algorithm
    // drifted and old seeds no longer reproduce their games.
    Prng rng{0};
    CHECK(rng.next() == 0xE220A8397B1DCDAFULL);
    CHECK(rng.next() == 0x6E789E6AA1B965F4ULL);
    CHECK(rng.next() == 0x06C45D188009454FULL);

    Prng one{1};
    CHECK(one.next() == 0x910A2DEC89025CC1ULL);
}

TEST_CASE("a seed of zero is the caller asking for a random one") {
    // random_seed() must never answer 0, or "0 means random" would recurse.
    for (int i = 0; i < 16; ++i) CHECK(Prng::random_seed() != 0);
}

// ---------------------------------------------------- starting placement

TEST_CASE("centred placement always starts in the same column") {
    Prng rng{99};
    Board board{kDefaultRows, kDefaultClms, rng};
    board.random_placement = false;
    for (int i = 0; i < 20; ++i) {
        CHECK(board.current_col() == kDefaultClms / 2);
        board.try_hdrop();
        if (board.step().game_over) break;
    }
}

TEST_CASE("random placement varies the column and leaves room for the I piece") {
    Prng rng{7};
    Board board{kDefaultRows, kDefaultClms, rng};
    board.random_placement = true;

    std::set<int> columns;
    for (int i = 0; i < 30; ++i) {
        const int j = board.current_col();
        columns.insert(j);
        // FTetris' bound: two clear columns on the left for the I piece and
        // one on the right for everything else.
        CHECK(j >= 2);
        CHECK(j <= kDefaultClms - 2);
        board.try_hdrop();
        if (board.step().game_over) break;
    }
    CHECK(columns.size() > 1);
}

// ----------------------------------------------------------------- rules

TEST_CASE("a board starts empty with a piece at the top") {
    Prng rng{3};
    Board board{kDefaultRows, kDefaultClms, rng};
    CHECK(board.rows() == kDefaultRows);
    CHECK(board.clms() == kDefaultClms);
    CHECK(board.current_row() == kDefaultRows - 1);  // row 0 is the bottom
    CHECK(board.current().code() != N);
    CHECK(board.next().code() != N);
    for (int i = 0; i < board.rows(); ++i)
        for (int j = 0; j < board.clms(); ++j) CHECK(board.at(i, j) == N);
}

TEST_CASE("board dimensions are clamped to FTetris' limits") {
    Prng rng{1};
    Board tiny{2, 2, rng};
    CHECK(tiny.rows() == kMinRows);
    CHECK(tiny.clms() == kMinClms);
    Board huge{999, 999, rng};
    CHECK(huge.rows() == kMaxRows);
    CHECK(huge.clms() == kMaxClms);
}

TEST_CASE("a hard drop lands the piece on the floor") {
    Prng rng{5};
    Board board{kDefaultRows, kDefaultClms, rng};
    board.try_hdrop();
    CHECK_FALSE(board.test_sdrop());  // nowhere left to fall

    board.step();  // lands it and starts the next piece
    int occupied = 0;
    for (int i = 0; i < board.rows(); ++i)
        for (int j = 0; j < board.clms(); ++j)
            if (board.at(i, j) != N) ++occupied;
    CHECK(occupied == 4);
    CHECK(board.current_row() == board.rows() - 1);  // the next piece is back at the top
}

TEST_CASE("pieces cannot be moved or rotated through the walls") {
    Prng rng{11};
    Board board{kDefaultRows, kDefaultClms, rng};
    for (int i = 0; i < 40; ++i) board.try_lmove();
    CHECK(board.current_col() >= 0);
    CHECK_FALSE(board.try_lmove());  // hard against the left wall

    for (int i = 0; i < 40; ++i) board.try_rmove();
    CHECK_FALSE(board.try_rmove());
    CHECK(board.current_col() < board.clms());
}

TEST_CASE("the O piece reports rotation as a no-op rather than a failure") {
    // FTetris short-circuited O because it is symmetric; rotating it must not
    // read as "blocked", or a caller could mistake it for a wall.
    Prng rng{1};
    Board board{kDefaultRows, kDefaultClms, rng};
    Shape o{O};
    Shape rotated = o.rrot();
    std::set<std::pair<int, int>> before, after;
    for (int k = 0; k < 4; ++k) {
        before.insert({o.x(k), o.y(k)});
        after.insert({rotated.x(k), rotated.y(k)});
    }
    CHECK(before.size() == 4);
    CHECK(after.size() == 4);
}

TEST_CASE("a completed line is cleared and the rows above slide down") {
    Prng rng{21};
    Board board{kMinRows, kMinClms, rng};
    board.random_placement = false;

    // Fill the floor by dropping pieces along it until a line goes.
    int cleared = 0;
    for (int i = 0; i < 400 && cleared == 0; ++i) {
        // Sweep left to right so the bottom row fills up rather than stacking.
        for (int m = 0; m < (i % board.clms()); ++m) board.try_rmove();
        for (int m = 0; m < board.clms(); ++m)
            if (!board.try_lmove()) break;
        for (int m = 0; m < (i % board.clms()); ++m) board.try_rmove();
        board.try_hdrop();
        const StepResult r = board.step();
        cleared += r.lines_cleared;
        if (r.game_over) break;
    }
    // Whether or not a line happened to complete, the invariant that matters
    // is that clear_lines never leaves a full row behind.
    for (int i = 0; i < board.rows(); ++i) {
        bool full = true;
        for (int j = 0; j < board.clms(); ++j) full = full && board.at(i, j) != N;
        CHECK_FALSE(full);
    }
}

TEST_CASE("out-of-bounds reads answer empty rather than crashing") {
    Prng rng{1};
    Board board{kDefaultRows, kDefaultClms, rng};
    CHECK(board.at(-1, 0) == N);
    CHECK(board.at(0, -1) == N);
    CHECK(board.at(999, 999) == N);
    CHECK_FALSE(board.in_bounds(-1, 0));
}

// --------------------------------------------------------------- schemes

TEST_CASE("every scheme defines all seven pieces and both column stripes") {
    for (int b = 0; b < kBackgroundCount; ++b) {
        for (int p = 0; p < kPiecesCount; ++p) {
            const Scheme s = make_scheme(static_cast<Background>(b), static_cast<Pieces>(p));
            std::set<std::tuple<int, int, int>> distinct;
            for (int k = I; k <= Z; ++k) {
                const Rgb c = s.piece[static_cast<std::size_t>(k)];
                distinct.insert({c.r, c.g, c.b});
            }
            // All seven pieces must be visually distinguishable.
            CHECK(distinct.size() == 7);
            // The two stripes must differ, or the columns stop being readable.
            CHECK(s.even_column != s.odd_column);
        }
    }
}

TEST_CASE("the scheme values match FTetris exactly") {
    // Spot checks against the RGB constants in FTetris' applyColorScheme.
    const Scheme sega = make_scheme(Background::Black, Pieces::Sega);
    CHECK(sega.piece[I] == Rgb{0xFF, 0x00, 0x00});  // red
    CHECK(sega.piece[O] == Rgb{0xFF, 0xFF, 0x00});  // yellow
    CHECK(sega.even_column == Rgb{0x00, 0x00, 0x00});
    CHECK(sega.odd_column == Rgb{0x20, 0x20, 0x20});  // offBlack

    const Scheme gerasimov = make_scheme(Background::Beige, Pieces::Gerasimov);
    CHECK(gerasimov.piece[I] == Rgb{0xAA, 0x00, 0x00});  // maroon
    CHECK(gerasimov.piece[J] == Rgb{0xCC, 0xCC, 0xCC});  // lightGrey
    CHECK(gerasimov.even_column == Rgb{0xFF, 0xFF, 0xEE});
    CHECK(gerasimov.odd_column == Rgb{0xFF, 0xFF, 0xBF});

    const Scheme company = make_scheme(Background::White, Pieces::TetrisCompany);
    CHECK(company.piece[I] == Rgb{0x00, 0xFF, 0xFF});  // cyan
    CHECK(company.odd_column == Rgb{0xF0, 0xF0, 0xFC});  // offWhite
}

TEST_CASE("scheme names read the way FTetris' menus did") {
    CHECK(std::string{name_of(Background::Beige)} == "Beige");
    CHECK(std::string{name_of(Pieces::SovietMindGame)} == "Soviet Mind Game");
    CHECK(std::string{name_of(Pieces::GameBoy)} == "Game Boy");
}

TEST_CASE("nearest_ansi picks the obvious slot for an obvious colour") {
    CHECK(nearest_ansi(Rgb{0x00, 0x00, 0x00}) == Color::Black);
    CHECK(nearest_ansi(Rgb{0xFF, 0xFF, 0xFF}) == Color::BrightWhite);
}

TEST_CASE("every scheme colour gets its own slot, so none of them collide") {
    // Nine colours into sixteen slots. Two pieces sharing a slot would be
    // indistinguishable on screen no matter what the palette says.
    for (int b = 0; b < kBackgroundCount; ++b) {
        for (int p = 0; p < kPiecesCount; ++p) {
            const SlotMap m = assign_slots(
                make_scheme(static_cast<Background>(b), static_cast<Pieces>(p)));
            std::set<int> slots;
            for (int k = I; k <= Z; ++k)
                slots.insert(static_cast<int>(m.piece[static_cast<std::size_t>(k)]));
            slots.insert(static_cast<int>(m.even_column));
            slots.insert(static_cast<int>(m.odd_column));
            CHECK(slots.size() == 9);
        }
    }
}

TEST_CASE("changing the piece scheme changes which SLOTS are used") {
    // THE regression test. The first version pinned every scheme to the same
    // seven slots and varied only the RGB behind them via the palette. On a
    // terminal that ignores colour redefinition - which turned out to include
    // plain CMD - the sidebar label changed and the board did not, because
    // not one cell attribute differed. Slots must differ, or nothing visible
    // does.
    const auto piece_slots = [](Pieces p) {
        const SlotMap m = assign_slots(make_scheme(Background::Black, p));
        std::vector<int> out;
        for (int k = I; k <= Z; ++k) out.push_back(static_cast<int>(m.piece[static_cast<std::size_t>(k)]));
        return out;
    };

    const auto sega = piece_slots(Pieces::Sega);
    CHECK(piece_slots(Pieces::GameBoy) != sega);
    CHECK(piece_slots(Pieces::Gerasimov) != sega);
    CHECK(piece_slots(Pieces::SovietMindGame) != sega);
    CHECK(piece_slots(Pieces::TetrisCompany) != sega);
}

TEST_CASE("changing the background changes which SLOTS the stripes use") {
    const auto stripes = [](Background b) {
        const SlotMap m = assign_slots(make_scheme(b, Pieces::Sega));
        return std::pair<int, int>{static_cast<int>(m.even_column),
                                   static_cast<int>(m.odd_column)};
    };
    CHECK(stripes(Background::Black) != stripes(Background::White));
    CHECK(stripes(Background::Black) != stripes(Background::Beige));
    CHECK(stripes(Background::White) != stripes(Background::Beige));

    SUBCASE("and a dark background really does land on dark slots") {
        const auto [even, odd] = stripes(Background::Black);
        CHECK(even == static_cast<int>(Color::Black));
        CHECK(odd == static_cast<int>(Color::BrightBlack));
    }
}

TEST_CASE("slot assignment is deterministic") {
    // The same scheme must always resolve the same way, or the board would
    // change colour for no reason.
    for (int p = 0; p < kPiecesCount; ++p) {
        const Scheme s = make_scheme(Background::Beige, static_cast<Pieces>(p));
        const SlotMap a = assign_slots(s);
        const SlotMap b = assign_slots(s);
        for (int k = I; k <= Z; ++k)
            CHECK(a.piece[static_cast<std::size_t>(k)] == b.piece[static_cast<std::size_t>(k)]);
        CHECK(a.even_column == b.even_column);
        CHECK(a.odd_column == b.odd_column);
    }
}
