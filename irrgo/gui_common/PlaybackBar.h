// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include <QWidget>

class QLabel;
class QSlider;
class QTimer;
class QToolButton;

namespace guicommon {

// A media-player-style transport for stepping through a recorded game: First /
// Prev / Play / Next / Last buttons, a position slider, and a "k / M" label, plus
// Left/Right/Home/End keyboard shortcuts. It is game-agnostic: it knows only a ply
// count M and a current ply k (0..M), and emits seek(k) when the user navigates.
// The owner reconstructs the position and calls setCursor() back.
class PlaybackBar : public QWidget {
    Q_OBJECT
public:
    explicit PlaybackBar(QWidget* parent = nullptr);

    void setPlyCount(int plies);  // total plies M (>= 0); clamps the cursor
    void setCursor(int ply);      // current ply k in [0, M]; does NOT emit seek()
    int  cursor() const { return cursor_; }
    int  plyCount() const { return plies_; }

public slots:
    void goFirst();
    void goPrev();
    void goNext();
    void goLast();
    void togglePlay();   // start/stop autoplay (steps forward on a timer)

signals:
    void seek(int ply);  // user asked to view this ply

private:
    void emitSeek(int ply);     // clamp to [0, M] and emit seek()
    void refreshControls();     // button enable-state, slider, label, play icon
    void stopPlaying();

    int cursor_ = 0;
    int plies_  = 0;

    QToolButton* firstBtn_ = nullptr;
    QToolButton* prevBtn_  = nullptr;
    QToolButton* playBtn_  = nullptr;
    QToolButton* nextBtn_  = nullptr;
    QToolButton* lastBtn_  = nullptr;
    QSlider*     slider_   = nullptr;
    QLabel*      label_    = nullptr;
    QTimer*      autoTimer_= nullptr;
    bool playing_         = false;
    bool settingSlider_   = false;  // guard against the programmatic slider feedback
};

}  // namespace guicommon
// Copyright Ben Paul Wise. All Rights Reserved.
