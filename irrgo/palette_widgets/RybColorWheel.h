// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include <QColor>
#include <QPixmap>
#include <QWidget>

// Display-only RYB color wheel (DESIGN.md §6). Paints the wheel and overlays
// markers at the hue angles of the current palette's three colors. It captures
// no input and holds no business logic; it only visualizes a result.
namespace palette_widgets {

class RybColorWheel : public QWidget {
    Q_OBJECT
public:
    explicit RybColorWheel(QWidget* parent = nullptr);

    void setPalette(const QColor& background, const QColor& piece1,
                    const QColor& piece2);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor background_{Qt::gray};
    QColor piece1_{Qt::black};
    QColor piece2_{Qt::white};
    // The coloured wheel depends only on widget geometry, not the palette, so it
    // is cached here and regenerated only when the device-pixel size changes.
    QPixmap wheelCache_;
};

} // namespace palette_widgets
// Copyright Ben Paul Wise. All Rights Reserved.
