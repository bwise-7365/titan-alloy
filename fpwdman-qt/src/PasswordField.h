// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef PASSWORDFIELD_H
#define PASSWORDFIELD_H

#include <vector>

#include <QCheckBox>
#include <QLineEdit>

// Shared "show/hide" wiring for secret fields. Every dialog that displays a
// password or passphrase pairs a masked QLineEdit with a "Show ..." checkbox
// that toggles the field(s) between hidden and clear text; this collects that
// one behavior so the dialogs don't each re-implement it.
namespace ui {

// Mask the given secret fields and wire `box` to reveal/hide them together.
// The fields are passed by value (a braced list converts to the vector at the
// call boundary) so the captured pointers can't dangle the way a captured
// std::initializer_list would. Using `box` as the connection context means the
// connection is dropped automatically when the checkbox is destroyed.
inline void wireRevealToggle(QCheckBox* box, std::vector<QLineEdit*> fields) {
    for (QLineEdit* field : fields)
        if (field)
            field->setEchoMode(QLineEdit::Password);
    QObject::connect(box, &QCheckBox::toggled, box, [fields = std::move(fields)](bool on) {
        const QLineEdit::EchoMode mode = on ? QLineEdit::Normal : QLineEdit::Password;
        for (QLineEdit* field : fields)
            if (field)
                field->setEchoMode(mode);
    });
}

} // namespace ui

#endif // PASSWORDFIELD_H
// Copyright Ben Paul Wise. All Rights Reserved.
