// Copyright Ben Paul Wise. All Rights Reserved.
#include <gtest/gtest.h>
#include "../src/sbc.h"

using namespace SBC;

TEST(SbcTest, QTransW128MatchesW64) {
    W128 v{};
    v.first = 1234567890ULL;
    v.second = 9876543210ULL;

    W128 result = qTrans(v);


    const W64 A = v.first;
    const W64 B = v.second;
    const W64 X = qTrans(A);
    const W64 Y = qTrans(B)^X;

    EXPECT_EQ(result.first, X);
    EXPECT_EQ(result.second, Y);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
// Copyright Ben Paul Wise. All Rights Reserved.
