// Copyright Ben Paul Wise. All Rights Reserved.
#include <gtest/gtest.h>

#include <vector>

#include <QByteArray>
#include <QFile>
#include <QString>
#include <QTemporaryDir>

#include "PasswordStore.h"
#include "PasswordGenerator.h"
#include "SiteEntry.h"

using namespace pwstore;

namespace {

SiteEntry makeEntry(const QString& t, const QString& s, const QString& u,
                    const QString& p, const QString& c) {
    SiteEntry e;
    e.Title = t;
    e.Site = s;
    e.UserID = u;
    e.Password = p;
    e.Comment = c;
    return e;
}

bool sameEntry(const SiteEntry& a, const SiteEntry& b) {
    return a.Title == b.Title && a.Site == b.Site && a.UserID == b.UserID &&
           a.Password == b.Password && a.Comment == b.Comment;
}

std::vector<SiteEntry> sampleEntries() {
    std::vector<SiteEntry> v;
    v.push_back(makeEntry("Amazon", "http://amazon.com", "citizen <not cane>",
                          "ei@67~dF$aFDVS11", "we love it & more"));
    v.push_back(makeEntry(QString::fromUtf8("\xD0\x91\xD0\xB0\xD1\x85\xD1\x80\xD0\xB0\xD0\xBC V"),
                          QString::fromUtf8("\xD1\x81\xD0\xB2\xD0\xBE\xD0\xB1\xD0\xBE\xD0\xB4\xD0\xB0"),
                          "", "pw", ""));
    return v;
}

} // namespace

// Standard PBKDF2-HMAC-SHA256 known-answer vector (P="password", S="salt", c=1).
TEST(Store, Pbkdf2KnownAnswer) {
    const QByteArray dk = pbkdf2HmacSha256("password", "salt", 1, 32);
    EXPECT_EQ(dk.toHex(),
              QByteArray("120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b"));
}

TEST(Store, XmlRoundTrip) {
    const auto entries = sampleEntries();
    const QByteArray xml = serializeXml(entries);
    const auto parsed = parseXml(xml);
    ASSERT_EQ(parsed.size(), entries.size());
    for (size_t i = 0; i < entries.size(); ++i)
        EXPECT_TRUE(sameEntry(parsed[i], entries[i]));
}

// Legacy <SiteTable>-rooted XML (no <FpwdMan> wrapper) must also parse.
TEST(Store, ParsesLegacyRootedXml) {
    const QByteArray xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<SiteTable>\n<SiteEntry>\n<title>T</title>\n<site>S</site>\n"
        "<userid>U</userid>\n<password>P</password>\n<comments>C</comments>\n"
        "</SiteEntry>\n</SiteTable>\n";
    const auto parsed = parseXml(xml);
    ASSERT_EQ(parsed.size(), 1u);
    EXPECT_EQ(parsed[0].Title, QString("T"));
    EXPECT_EQ(parsed[0].Password, QString("P"));
}

TEST(Store, ModernRoundTrip) {
    const auto entries = sampleEntries();
    const QByteArray blob = encodeModern("correct horse", entries);
    EXPECT_EQ(detectFormat(blob), Format::Modern);

    const auto out = decodeModern(blob, "correct horse");
    ASSERT_EQ(out.size(), entries.size());
    for (size_t i = 0; i < entries.size(); ++i)
        EXPECT_TRUE(sameEntry(out[i], entries[i]));
}

TEST(Store, ModernWrongPassphrase) {
    const auto entries = sampleEntries();
    const QByteArray blob = encodeModern("correct horse", entries);
    EXPECT_THROW(decodeModern(blob, "battery staple"), WrongPassphrase);
}

TEST(Store, DetectLegacyFormat) {
    const QByteArray legacyish = "868 651\nyv5HgAIAAGxc\n";
    EXPECT_EQ(detectFormat(legacyish), Format::Legacy);
}

TEST(Store, GeneratorLengthAndAlphabet) {
    const QString pw = PasswordGenerator::generate(20, PasswordGenerator::Mode::UcLcDds);
    EXPECT_EQ(pw.size(), 20);
    const QString set = PasswordGenerator::alphabet(PasswordGenerator::Mode::UcLcDds);
    for (const QChar ch : pw)
        EXPECT_TRUE(set.contains(ch));
}

TEST(Store, SaveFileCreatesBackupAndAtomicFile) {
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    const QString path = tempDir.filePath("test_db.sbc");
    const QString bakPath = path + ".bak";
    const auto entries1 = sampleEntries();

    // First save: file created, no .bak yet
    saveFile(path, "pass1", entries1);
    EXPECT_TRUE(QFile::exists(path));
    EXPECT_FALSE(QFile::exists(bakPath));

    const auto loaded1 = openFile(path, "pass1");
    ASSERT_EQ(loaded1.size(), entries1.size());

    // Second save: update database and verify .bak is created holding pass1 data
    std::vector<SiteEntry> entries2 = entries1;
    entries2.push_back(makeEntry("GitHub", "https://github.com", "user", "secret", "note"));
    saveFile(path, "pass2", entries2);

    EXPECT_TRUE(QFile::exists(path));
    EXPECT_TRUE(QFile::exists(bakPath));

    // Verify main file has pass2 / entries2
    const auto loaded2 = openFile(path, "pass2");
    ASSERT_EQ(loaded2.size(), entries2.size());

    // Verify backup file has pass1 / entries1
    const auto loadedBak = openFile(bakPath, "pass1");
    ASSERT_EQ(loadedBak.size(), entries1.size());
}
// Copyright Ben Paul Wise. All Rights Reserved.
