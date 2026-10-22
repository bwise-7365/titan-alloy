// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef VIEWSITEENTRY_H
#define VIEWSITEENTRY_H

#include <QDialog>
#include "SiteEntry.h"

class ViewSiteEntry : public QDialog {
    Q_OBJECT

public:
    explicit ViewSiteEntry(const SiteEntry *entry, int clipboardClearMs, QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;
};

#endif // VIEWSITEENTRY_H
// Copyright Ben Paul Wise. All Rights Reserved.
