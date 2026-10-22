// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include <QColor>
#include <QWidget>

// Display-only "what you selected" preview: a 3:2 board rectangle painted with
// the background color, holding two unit circles painted with the two piece
// colors. Holds no business logic.
namespace palette_widgets {

class BoardPreview : public QWidget {
    Q_OBJECT
public:
    explicit BoardPreview(QWidget* parent = nullptr);

    void setColors(const QColor& background, const QColor& piece1,
                   const QColor& piece2);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor background_{Qt::gray};
    QColor piece1_{Qt::black};
    QColor piece2_{Qt::white};
};

} // namespace palette_widgets
// Copyright Ben Paul Wise. All Rights Reserved.
