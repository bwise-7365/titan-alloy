// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The small LCS-based line diff used by the golden report.
// ----------------------------------------------
#include "hexrecord/GoldenCompare.h"

#include <gtest/gtest.h>

TEST(DiffTest, MarksAddedRemovedAndCommonLines)
{
  const std::string expected = "a\nb\nc\n";
  const std::string actual = "a\nx\nc\n";
  const std::string diff = HexRecord::unifiedDiff(expected, actual);

  EXPECT_NE(std::string::npos, diff.find("  a\n"));
  EXPECT_NE(std::string::npos, diff.find("- b\n"));
  EXPECT_NE(std::string::npos, diff.find("+ x\n"));
  EXPECT_NE(std::string::npos, diff.find("  c\n"));
}

TEST(DiffTest, EqualTextsHaveNoAddedOrRemovedLines)
{
  const std::string text = "same\nlines\n";
  const std::string diff = HexRecord::unifiedDiff(text, text);
  EXPECT_EQ(std::string::npos, diff.find("- "));
  EXPECT_EQ(std::string::npos, diff.find("+ "));
}

TEST(DiffTest, TruncatesPastMaxLines)
{
  std::string expected;
  std::string actual;
  for (int i = 0; i < 100; ++i) {
    expected += "line" + std::to_string(i) + "\n";
    actual += "line" + std::to_string(i) + "x\n";
  }
  const std::string diff = HexRecord::unifiedDiff(expected, actual, 10);

  int lineCount = 0;
  for (char c : diff) {
    if ('\n' == c) {
      ++lineCount;
    }
  }
  EXPECT_LE(lineCount, 11);  // at most 10 diff lines plus the truncation marker
  EXPECT_NE(std::string::npos, diff.find("truncated"));
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
