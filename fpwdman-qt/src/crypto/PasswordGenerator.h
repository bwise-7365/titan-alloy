// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef PASSWORDGENERATOR_H
#define PASSWORDGENERATOR_H

// -----------------------------------------------------------------------------
// Password suggestion, ported from EGApp::suggestPassword. The four character
// sets match the old app; the digit-doubled default (UcLcDd) makes digits ~28%
// of the alphabet. Randomness comes from the OS CSPRNG (QRandomGenerator::
// system()), replacing the old mouse-drawing entropy gatherer.
// -----------------------------------------------------------------------------

#include <QString>

namespace PasswordGenerator {

enum class Mode {
    LcDigits,   // 36: lower + digits
    UcLcDigits, // 62: upper + lower + digits
    UcLcDd,     // 72: upper + digits + lower + digits  (default)
    UcLcDds     // 84: symbols + upper + digits + lower + digits
};

// Returns the alphabet used for a mode (digits may appear twice by design).
QString alphabet(Mode mode);

// Generate a password of `length` characters, sampled uniformly (CSPRNG) from
// the mode's alphabet.
QString generate(int length, Mode mode = Mode::UcLcDd);

} // namespace PasswordGenerator

#endif // PASSWORDGENERATOR_H
// Copyright Ben Paul Wise. All Rights Reserved.
