// Copyright Ben Paul Wise. All Rights Reserved.
#include <gtest/gtest.h>

#include "sbc_legacy.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef SBC_TEST_DATA_DIR
#define SBC_TEST_DATA_DIR "."
#endif

namespace {

std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string dataPath(const std::string& name) {
    return std::string(SBC_TEST_DATA_DIR) + "/" + name;
}

// Strip CR so a CRLF-checked-out fixture compares equal to the LF plaintext
// that FPwdMan actually stored.
std::string stripCr(std::string s) {
    s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
    return s;
}

} // namespace

// The primary golden vector: this real old FPwdMan file decrypts, under the
// documented passphrase "qwerty", to exactly example-good-UTF.xml.
TEST(SbcLegacy, GoldenVectorQwerty) {
    const std::string fileText = readFile(dataPath("example-good-UTF-xml.sbc"));
    ASSERT_FALSE(fileText.empty()) << "missing test data";
    ASSERT_TRUE(SBC::looksLikeLegacy(fileText));

    const std::vector<uint8_t> plain = SBC::legacyDecrypt(fileText, "qwerty");
    const std::string got(plain.begin(), plain.end());

    const std::string expected = readFile(dataPath("example-good-UTF.xml"));
    ASSERT_FALSE(expected.empty()) << "missing expected xml";

    // Exact match up to line-ending normalization (the fixture may be CRLF on
    // Windows; the stored plaintext is LF).
    EXPECT_EQ(stripCr(got), stripCr(expected));
    EXPECT_NE(got.find("<SiteTable>"), std::string::npos);
    EXPECT_NE(got.find("Amazon"), std::string::npos);
    EXPECT_NE(got.find("\xD0\x91\xD0\xB0\xD1\x85\xD1\x80\xD0\xB0\xD0\xBC"), std::string::npos)
        << "expected UTF-8 Cyrillic to survive round trip";
}

TEST(SbcLegacy, WrongPassphraseThrows) {
    const std::string fileText = readFile(dataPath("example-good-UTF-xml.sbc"));
    ASSERT_FALSE(fileText.empty());
    EXPECT_THROW(SBC::legacyDecrypt(fileText, "not-the-password"), SBC::WrongPassphrase);
}
// Copyright Ben Paul Wise. All Rights Reserved.
