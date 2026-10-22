// Copyright Ben Paul Wise. All Rights Reserved.
#include <gtest/gtest.h>

#include <vector>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QList>
#include <QString>
#include <QTemporaryDir>

#include "PasswordStore.h"
#include "PasswordGenerator.h"
#include "SiteEntry.h"

#ifndef SBC_TEST_DATA_DIR
#define SBC_TEST_DATA_DIR "."
#endif

using namespace pwstore;

namespace {

QString dataPath(const char* name) {
    return QString::fromUtf8(SBC_TEST_DATA_DIR) + "/" + QString::fromUtf8(name);
}

QByteArray readAll(const QString& path) {
    QFile f(path);
    EXPECT_TRUE(f.open(QIODevice::ReadOnly)) << path.toStdString();
    return f.readAll();
}

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

// One real FPMQ1 file, encoded once: every encode pays a full key derivation.
const QByteArray& modernBlob() {
    static const QByteArray blob = encodeModern("correct horse", sampleEntries());
    return blob;
}

// One FPMQ1 file, likewise encoded once.
const QByteArray& fpmq1Blob() {
    static const QByteArray blob = encodeFpmq1("correct horse", sampleEntries());
    return blob;
}

// `blob` with its iteration count replaced by `value`.
QByteArray withIters(const QByteArray& blob, const QByteArray& value) {
    QByteArray b = blob;
    b.replace("iters=600000;", "iters=" + value + ";");
    EXPECT_TRUE(b.contains("iters=" + value + ";")); // the substitution happened
    return b;
}

QByteArray withIters(const QByteArray& value) {
    return withIters(modernBlob(), value);
}

// `blob` with one bit flipped in the base64 value that follows `key` (e.g. the
// salt or the tag). Re-encoding keeps the field the same length.
QByteArray withFlippedField(const QByteArray& blob, const char* key) {
    const int start = blob.indexOf(key) + static_cast<int>(qstrlen(key));
    int end = start;
    while (end < blob.size() && blob[end] != ';' && blob[end] != '\n')
        ++end;
    QByteArray raw = QByteArray::fromBase64(blob.mid(start, end - start));
    raw[0] = static_cast<char>(raw[0] ^ 1);
    QByteArray b = blob;
    b.replace(start, end - start, raw.toBase64());
    EXPECT_NE(b, blob);
    return b;
}

// Line `index` of `blob` (split on '\n') replaced by `line`.
QByteArray withLine(const QByteArray& blob, int index, const QByteArray& line) {
    QList<QByteArray> lines = blob.split('\n');
    lines[index] = line;
    return lines.join('\n');
}

void expectSameEntries(const std::vector<SiteEntry>& a, const std::vector<SiteEntry>& b) {
    ASSERT_EQ(a.size(), b.size());
    for (size_t i = 0; i < a.size(); ++i) {
        EXPECT_EQ(a[i].Title, b[i].Title) << "entry " << i;
        EXPECT_EQ(a[i].Site, b[i].Site) << "entry " << i;
        EXPECT_EQ(a[i].UserID, b[i].UserID) << "entry " << i;
        EXPECT_EQ(a[i].Password, b[i].Password) << "entry " << i;
        EXPECT_EQ(a[i].Comment, b[i].Comment) << "entry " << i;
    }
}

// The plaintext behind both fixture files.
std::vector<SiteEntry> fixtureEntries() {
    return parseXml(readAll(dataPath("example-good-UTF.xml")));
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
    EXPECT_TRUE(blob.startsWith("FPMQ2\n"));
    EXPECT_EQ(detectFormat(blob), Format::Fpmq2);

    const auto out = decodeModern(blob, "correct horse");
    ASSERT_EQ(out.size(), entries.size());
    for (size_t i = 0; i < entries.size(); ++i)
        EXPECT_TRUE(sameEntry(out[i], entries[i]));
}

// FPMQ1 is no longer written, but must still be read.
TEST(Store, Fpmq1StillDecodes) {
    EXPECT_TRUE(fpmq1Blob().startsWith("FPMQ1\n"));
    EXPECT_EQ(detectFormat(fpmq1Blob()), Format::Fpmq1);
    expectSameEntries(decodeModern(fpmq1Blob(), "correct horse"), sampleEntries());
}

// --- the fixture files and migration ------------------------------------------
// Under the passphrase "qwerty", test/data holds example-good-UTF.xml in the
// legacy format (example-good-UTF-xml.sbc) and, with one comment added to the
// fifth entry, in FPMQ1 (example-good-UTF-xml-v2.sbc).

TEST(Store, OpensLegacyFixture) {
    expectSameEntries(openFile(dataPath("example-good-UTF-xml.sbc"), "qwerty"),
                      fixtureEntries());
}

TEST(Store, OpensFpmq1Fixture) {
    const QString path = dataPath("example-good-UTF-xml-v2.sbc");
    EXPECT_EQ(detectFormat(readAll(path)), Format::Fpmq1);
    const auto got = openFile(path, "qwerty");
    const auto want = fixtureEntries();
    ASSERT_EQ(got.size(), want.size());
    for (size_t i = 0; i < got.size(); ++i) { // comments differ: see above
        EXPECT_EQ(got[i].Title, want[i].Title) << "entry " << i;
        EXPECT_EQ(got[i].Site, want[i].Site) << "entry " << i;
        EXPECT_EQ(got[i].UserID, want[i].UserID) << "entry " << i;
        EXPECT_EQ(got[i].Password, want[i].Password) << "entry " << i;
    }
}

// Opening an older file and saving it writes FPMQ2 holding the same entries,
// and leaves no copy of the original (in its weaker format) beside it.
TEST(Store, SavingMigratesOlderFormatsToFpmq2) {
    for (const char* fixture : {"example-good-UTF-xml.sbc", "example-good-UTF-xml-v2.sbc"}) {
        QTemporaryDir tempDir;
        ASSERT_TRUE(tempDir.isValid());
        const QString path = tempDir.filePath("db.sbc");
        ASSERT_TRUE(QFile::copy(dataPath(fixture), path)) << fixture;

        const auto entries = openFile(path, "qwerty");
        saveFile(path, "qwerty", entries);

        EXPECT_EQ(detectFormat(readAll(path)), Format::Fpmq2) << fixture;
        expectSameEntries(openFile(path, "qwerty"), entries);
        EXPECT_EQ(QDir(tempDir.path()).entryList(QDir::Files | QDir::Hidden),
                  QStringList{"db.sbc"}) << fixture;
    }
}

// --- FPMQ2 authenticates its header -------------------------------------------
// Each change below is caught. CorruptFile means it was refused before key
// derivation; WrongPassphrase means the MAC check caught it.

TEST(Store, ModernRejectsChangedKdfName) {
    const QByteArray b = withLine(modernBlob(), 1,
        modernBlob().split('\n')[1].replace("pbkdf2-sha256", "pbkdf2-sha512"));
    EXPECT_THROW(decodeModern(b, "correct horse"), CorruptFile);
}

TEST(Store, ModernRejectsStrayHeaderField) {
    const QByteArray b = withLine(modernBlob(), 1, modernBlob().split('\n')[1] + "; extra=1");
    EXPECT_THROW(decodeModern(b, "correct horse"), CorruptFile);
}

// Relabelling an FPMQ2 file as FPMQ1 must not let it be read the weaker way.
TEST(Store, ModernRejectsRelabelledVersion) {
    QByteArray b = modernBlob();
    b.replace(0, 5, "FPMQ1");
    EXPECT_THROW(decodeModern(b, "correct horse"), WrongPassphrase);
}

TEST(Store, ModernRejectsChangedSalt) {
    EXPECT_THROW(decodeModern(withFlippedField(modernBlob(), "salt="), "correct horse"),
                 WrongPassphrase);
}

TEST(Store, ModernRejectsChangedTag) {
    EXPECT_THROW(decodeModern(withFlippedField(modernBlob(), "tag="), "correct horse"),
                 WrongPassphrase);
}

TEST(Store, ModernRejectsChangedCiphertext) {
    QList<QByteArray> lines = modernBlob().split('\n');
    QByteArray body = lines[3]; // first line of base64 ciphertext
    body[10] = (body[10] == 'A') ? 'B' : 'A';
    EXPECT_THROW(decodeModern(withLine(modernBlob(), 3, body), "correct horse"),
                 WrongPassphrase);
}

TEST(Store, ModernWrongPassphrase) {
    const auto entries = sampleEntries();
    const QByteArray blob = encodeModern("correct horse", entries);
    EXPECT_THROW(decodeModern(blob, "battery staple"), WrongPassphrase);
}

// The iteration count is read from the unauthenticated header, so anything but
// FPMQ1's fixed 600000 must be refused before key derivation. CorruptFile (not
// WrongPassphrase) shows the refusal came first: had PBKDF2 run, the MAC check
// would have failed and thrown WrongPassphrase instead.
TEST(Store, ModernRejectsWrongIterationCount) {
    for (const char* v : {"1", "599999", "600001", "1200000"})
        EXPECT_THROW(decodeModern(withIters(v), "correct horse"), CorruptFile) << v;
}

// Without the check this would run about 6.4 billion HMACs (tens of minutes).
TEST(Store, ModernRejectsHugeIterationCount) {
    EXPECT_THROW(decodeModern(withIters("2147483647"), "correct horse"), CorruptFile);
}

TEST(Store, ModernRejectsNegativeIterationCount) {
    for (const char* v : {"-1", "-600000", "-2147483648"})
        EXPECT_THROW(decodeModern(withIters(v), "correct horse"), CorruptFile) << v;
}

TEST(Store, ModernRejectsMalformedIterationCount) {
    // trailing junk, exponent, hex, past INT_MAX, past 2^32, empty
    for (const char* v : {"600000x", "6e5", "0x927C0", "99999999999", "4295567296", ""})
        EXPECT_THROW(decodeModern(withIters(v), "correct horse"), CorruptFile) << v;
}

// The same refusal applies to FPMQ1 files.
TEST(Store, Fpmq1RejectsWrongIterationCount) {
    for (const char* v : {"1", "600001", "2147483647", "-600000", "600000x"})
        EXPECT_THROW(decodeModern(withIters(fpmq1Blob(), v), "correct horse"), CorruptFile) << v;
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

// Saving replaces the file and keeps no backup: after a passphrase change no
// copy remains that the old passphrase would open.
TEST(Store, SaveFileKeepsNoBackup) {
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    const QString path = tempDir.filePath("test_db.sbc");
    const QStringList onlyTheFile{"test_db.sbc"};
    const auto entries1 = sampleEntries();

    // First save: the file is created, and nothing else.
    saveFile(path, "pass1", entries1);
    EXPECT_EQ(QDir(tempDir.path()).entryList(QDir::Files | QDir::Hidden), onlyTheFile);
    expectSameEntries(openFile(path, "pass1"), entries1);

    // Second save, with new entries and a new passphrase: still one file.
    std::vector<SiteEntry> entries2 = entries1;
    entries2.push_back(makeEntry("GitHub", "https://github.com", "user", "secret", "note"));
    saveFile(path, "pass2", entries2);

    EXPECT_EQ(QDir(tempDir.path()).entryList(QDir::Files | QDir::Hidden), onlyTheFile);
    expectSameEntries(openFile(path, "pass2"), entries2);
    EXPECT_THROW(openFile(path, "pass1"), WrongPassphrase);
}
// Copyright Ben Paul Wise. All Rights Reserved.
