// Copyright Ben Paul Wise. All Rights Reserved.
#include "PasswordGenerator.h"

#include <QRandomGenerator>

namespace PasswordGenerator {

namespace {
const QString kLower = QStringLiteral("abcdefghijklmnopqrstuvwxyz");
const QString kUpper = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
const QString kDigits = QStringLiteral("0123456789");
const QString kSymbols = QStringLiteral("!#$%*+=?@^_~");
} // namespace

QString alphabet(Mode mode) {
    switch (mode) {
    case Mode::LcDigits:
        return kLower + kDigits;
    case Mode::UcLcDigits:
        return kUpper + kLower + kDigits;
    case Mode::UcLcDd:
        return kUpper + kDigits + kLower + kDigits;
    case Mode::UcLcDds:
        return kSymbols + kUpper + kDigits + kLower + kDigits;
    }
    return kUpper + kDigits + kLower + kDigits;
}

QString generate(int length, Mode mode) {
    if (length <= 0)
        return QString();
    const QString set = alphabet(mode);
    const quint32 n = static_cast<quint32>(set.size());
    QString out;
    out.reserve(length);
    QRandomGenerator* rng = QRandomGenerator::system();
    for (int i = 0; i < length; ++i)
        out.append(set.at(static_cast<int>(rng->bounded(n))));
    return out;
}

} // namespace PasswordGenerator
// Copyright Ben Paul Wise. All Rights Reserved.
