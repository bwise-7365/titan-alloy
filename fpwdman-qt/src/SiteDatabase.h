// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef SITEDATABASE_H
#define SITEDATABASE_H

// -----------------------------------------------------------------------------
// SiteDatabase: the in-memory password document -- the entries plus the state
// the old FLTK controller tracked (current file, master passphrase, and the two
// dirty flags that drive the status lights and the unsaved-changes guards).
// -----------------------------------------------------------------------------

#include <vector>

#include <QString>

#include "SiteEntry.h"

class SiteDatabase {
public:
    std::vector<SiteEntry> entries;
    QString filePath;            // empty until saved/opened
    QString passphrase;          // empty until set by the user
    bool unsavedChanges = false; // site edits not yet written
    bool unsavedMpChange = false; // passphrase changed but not yet written

    bool passphraseSet() const { return !passphrase.isEmpty(); }

    void reset() {
        entries.clear();
        filePath.clear();
        passphrase.clear();
        unsavedChanges = false;
        unsavedMpChange = false;
    }
};

#endif // SITEDATABASE_H
// Copyright Ben Paul Wise. All Rights Reserved.
