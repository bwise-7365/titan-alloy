// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexview/Style.h"

#include <stdexcept>

namespace HexView {

  namespace {

    int
    hexDigit(char c)
    {
      if ('0' <= c && '9' >= c) {
        return c - '0';
      }
      if ('a' <= c && 'f' >= c) {
        return 10 + (c - 'a');
      }
      if ('A' <= c && 'F' >= c) {
        return 10 + (c - 'A');
      }
      return -1;
    }

  }  // namespace

  Color
  Color::parse(std::string_view text, std::string_view what)
  {
    const auto refuse = [&]() {
      return std::invalid_argument(std::string(what) + ": colour '" + std::string(text) +
                                   "' is not #rrggbb");
    };
    if (7 != text.size() || '#' != text[0]) {
      throw refuse();
    }
    std::uint8_t channel[3] = {0, 0, 0};
    for (int k = 0; k < 3; ++k) {
      const int hi = hexDigit(text[static_cast<std::size_t>(1 + 2 * k)]);
      const int lo = hexDigit(text[static_cast<std::size_t>(2 + 2 * k)]);
      if (0 > hi || 0 > lo) {
        throw refuse();
      }
      channel[k] = static_cast<std::uint8_t>(16 * hi + lo);
    }
    return Color{channel[0], channel[1], channel[2], 255};
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
