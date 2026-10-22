#include "scheme.hpp"

#include <algorithm>
#include <span>

namespace mtetris {
namespace {

// FTetris' private palette, transcribed value for value. The comment there is
// worth keeping: offBlack and offWhite are chosen to give enough contrast that
// you can see at a glance which column a piece will fall into, while still
// looking reasonable with at least one scheme.
constexpr Rgb kAmber{0xFF, 0xC0, 0x00};
constexpr Rgb kBlue{0x00, 0x00, 0xFF};
constexpr Rgb kBrown{0xAA, 0x55, 0x00};
constexpr Rgb kCyan{0x00, 0xFF, 0xFF};
constexpr Rgb kDarkBeige{0xFF, 0xFF, 0xBF};
constexpr Rgb kDarkGreen{0x00, 0xAA, 0x00};
constexpr Rgb kLightBeige{0xFF, 0xFF, 0xEE};
constexpr Rgb kLightGrey{0xCC, 0xCC, 0xCC};
constexpr Rgb kLime{0x80, 0xFF, 0x00};
constexpr Rgb kMagenta{0xFF, 0x00, 0xFF};
constexpr Rgb kMaroon{0xAA, 0x00, 0x00};
constexpr Rgb kNavyBlue{0x00, 0x00, 0xAA};
constexpr Rgb kOffBlack{0x20, 0x20, 0x20};
constexpr Rgb kOffWhite{0xF0, 0xF0, 0xFC};
constexpr Rgb kOlive{0x80, 0x80, 0x00};
constexpr Rgb kOrange{0xFF, 0xA5, 0x00};
constexpr Rgb kPureBlack{0x00, 0x00, 0x00};
constexpr Rgb kPureWhite{0xFF, 0xFF, 0xFF};
constexpr Rgb kPurple{0xAA, 0x00, 0xAA};
constexpr Rgb kRed{0xFF, 0x00, 0x00};
constexpr Rgb kTeal{0x00, 0xAA, 0xAA};
constexpr Rgb kYellow{0xFF, 0xFF, 0x00};

// Canonical RGB for the sixteen ANSI slots, used only by the fallback.
struct AnsiEntry {
    Color color;
    Rgb rgb;
};
constexpr AnsiEntry kAnsi[] = {
    {Color::Black, {0x00, 0x00, 0x00}},         {Color::Red, {0xAA, 0x00, 0x00}},
    {Color::Green, {0x00, 0xAA, 0x00}},         {Color::Yellow, {0xAA, 0x55, 0x00}},
    {Color::Blue, {0x00, 0x00, 0xAA}},          {Color::Magenta, {0xAA, 0x00, 0xAA}},
    {Color::Cyan, {0x00, 0xAA, 0xAA}},          {Color::White, {0xAA, 0xAA, 0xAA}},
    {Color::BrightBlack, {0x55, 0x55, 0x55}},   {Color::BrightRed, {0xFF, 0x55, 0x55}},
    {Color::BrightGreen, {0x55, 0xFF, 0x55}},   {Color::BrightYellow, {0xFF, 0xFF, 0x55}},
    {Color::BrightBlue, {0x55, 0x55, 0xFF}},    {Color::BrightMagenta, {0xFF, 0x55, 0xFF}},
    {Color::BrightCyan, {0x55, 0xFF, 0xFF}},    {Color::BrightWhite, {0xFF, 0xFF, 0xFF}},
};

}  // namespace

const char* name_of(Background b) {
    switch (b) {
        case Background::Beige: return "Beige";
        case Background::Black: return "Black";
        case Background::White: return "White";
    }
    return "?";
}

const char* name_of(Pieces p) {
    switch (p) {
        case Pieces::GameBoy: return "Game Boy";
        case Pieces::Gerasimov: return "Gerasimov";
        case Pieces::Sega: return "Sega";
        case Pieces::SovietMindGame: return "Soviet Mind Game";
        case Pieces::TetrisCompany: return "Tetris Company";
    }
    return "?";
}

Scheme make_scheme(Background bg, Pieces pc) {
    Scheme s;
    s.piece.fill(kPureWhite);

    switch (bg) {
        case Background::Beige:
            s.even_column = kLightBeige;
            s.odd_column = kDarkBeige;
            break;
        case Background::White:
            s.even_column = kPureWhite;
            s.odd_column = kOffWhite;
            break;
        case Background::Black:
        default:
            s.even_column = kPureBlack;
            s.odd_column = kOffBlack;
            break;
    }

    // Roughly the schemes catalogued at https://en.wikipedia.org/wiki/Tetris,
    // in TCode order: I J L O S T Z.
    switch (pc) {
        case Pieces::GameBoy:
            s.piece[I] = kOrange;
            s.piece[J] = kCyan;
            s.piece[L] = kRed;
            s.piece[O] = kYellow;
            s.piece[S] = kMagenta;
            s.piece[T] = kLime;
            s.piece[Z] = kAmber;
            break;
        case Pieces::Gerasimov:  // Tetris 3.12
            s.piece[I] = kMaroon;
            s.piece[J] = kLightGrey;
            s.piece[L] = kPurple;
            s.piece[O] = kNavyBlue;
            s.piece[S] = kDarkGreen;
            s.piece[T] = kBrown;
            s.piece[Z] = kTeal;
            break;
        case Pieces::SovietMindGame:
            s.piece[I] = kRed;
            s.piece[J] = kOrange;
            s.piece[L] = kMagenta;
            s.piece[O] = kBlue;
            s.piece[S] = kLime;
            s.piece[T] = kOlive;
            s.piece[Z] = kCyan;
            break;
        case Pieces::TetrisCompany:
            s.piece[I] = kCyan;
            s.piece[J] = kBlue;
            s.piece[L] = kOrange;
            s.piece[O] = kYellow;
            s.piece[S] = kLime;
            s.piece[T] = kPurple;
            s.piece[Z] = kRed;
            break;
        case Pieces::Sega:
        default:
            s.piece[I] = kRed;
            s.piece[J] = kBlue;
            s.piece[L] = kOrange;
            s.piece[O] = kYellow;
            s.piece[S] = kMagenta;
            s.piece[T] = kCyan;
            s.piece[Z] = kLime;
            break;
    }
    return s;
}

namespace {

// Plain squared distance in RGB. A perceptual metric would be better in
// general and makes no difference here: the scheme colours are far apart.
long distance(Rgb a, Rgb b) {
    const long dr = static_cast<long>(a.r) - b.r;
    const long dg = static_cast<long>(a.g) - b.g;
    const long db = static_cast<long>(a.b) - b.b;
    return dr * dr + dg * dg + db * db;
}

}  // namespace

SlotMap assign_slots(const Scheme& s) {
    // Nine colours into sixteen slots, so this always succeeds. Each takes the
    // nearest slot still free; one whose first choice is gone falls to its
    // next best rather than colliding.
    bool taken[16] = {};
    const auto take = [&taken](Rgb want, std::span<const Color> allowed) {
        Color best = Color::White;
        long best_distance = -1;
        for (const AnsiEntry& e : kAnsi) {
            const int index = static_cast<int>(e.color) - 1;
            if (taken[index]) continue;
            if (!allowed.empty() &&
                std::find(allowed.begin(), allowed.end(), e.color) == allowed.end())
                continue;
            const long d = distance(want, e.rgb);
            if (best_distance < 0 || d < best_distance) {
                best_distance = d;
                best = e.color;
            }
        }
        taken[static_cast<int>(best) - 1] = true;
        return best;
    };

    // The two column stripes go FIRST, and only from the neutral slots.
    //
    // They cover most of the screen, so they are what a background change has
    // to show. Letting them take pot luck after the pieces made White and
    // Beige resolve identically - there is no "beige" among sixteen colours,
    // so both landed on the same pale pair and cycling backgrounds appeared
    // to do nothing. Restricting them to the greys plus the yellows keeps all
    // three backgrounds distinct: black/grey, white/grey, white/yellow.
    static constexpr Color kNeutral[] = {
        Color::Black,  Color::BrightBlack,  Color::White,
        Color::BrightWhite, Color::Yellow, Color::BrightYellow,
    };

    SlotMap map;
    map.even_column = take(s.even_column, kNeutral);
    map.odd_column = take(s.odd_column, kNeutral);
    // The pieces then take the nearest of everything still free.
    for (int p = I; p <= Z; ++p)
        map.piece[static_cast<std::size_t>(p)] =
            take(s.piece[static_cast<std::size_t>(p)], {});
    return map;
}

Color nearest_ansi(Rgb value) {
    Color best = Color::White;
    long best_distance = -1;
    for (const AnsiEntry& e : kAnsi) {
        const long d = distance(value, e.rgb);
        if (best_distance < 0 || d < best_distance) {
            best_distance = d;
            best = e.color;
        }
    }
    return best;
}

}  // namespace mtetris
