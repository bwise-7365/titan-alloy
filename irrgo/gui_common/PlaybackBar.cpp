// Copyright Ben Paul Wise. All Rights Reserved.
#include "PlaybackBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QShortcut>
#include <QSlider>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <algorithm>

namespace guicommon {

namespace {
constexpr int kAutoplayMs = 700;  // autoplay step interval
}

PlaybackBar::PlaybackBar(QWidget* parent) : QWidget(parent) {
    auto iconButton = [this](QStyle::StandardPixmap sp, const QString& tip) {
        auto* b = new QToolButton(this);
        b->setIcon(style()->standardIcon(sp));
        b->setAutoRaise(true);
        b->setToolTip(tip);
        return b;
    };
    firstBtn_ = iconButton(QStyle::SP_MediaSkipBackward, "First (Home)");
    prevBtn_  = iconButton(QStyle::SP_MediaSeekBackward, "Previous (Left)");
    playBtn_  = iconButton(QStyle::SP_MediaPlay,         "Play / Pause");
    nextBtn_  = iconButton(QStyle::SP_MediaSeekForward,  "Next (Right)");
    lastBtn_  = iconButton(QStyle::SP_MediaSkipForward,  "Last (End)");

    slider_ = new QSlider(Qt::Horizontal, this);
    slider_->setMinimum(0);
    slider_->setMaximum(0);

    label_ = new QLabel("0 / 0", this);
    label_->setMinimumWidth(56);
    label_->setAlignment(Qt::AlignCenter);

    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(2);
    row->addWidget(firstBtn_);
    row->addWidget(prevBtn_);
    row->addWidget(playBtn_);
    row->addWidget(nextBtn_);
    row->addWidget(lastBtn_);
    row->addWidget(slider_, 1);
    row->addWidget(label_);

    connect(firstBtn_, &QToolButton::clicked, this, &PlaybackBar::goFirst);
    connect(prevBtn_,  &QToolButton::clicked, this, &PlaybackBar::goPrev);
    connect(playBtn_,  &QToolButton::clicked, this, &PlaybackBar::togglePlay);
    connect(nextBtn_,  &QToolButton::clicked, this, &PlaybackBar::goNext);
    connect(lastBtn_,  &QToolButton::clicked, this, &PlaybackBar::goLast);
    connect(slider_,   &QSlider::valueChanged, this, [this](int v) {
        if (!settingSlider_) {
            emitSeek(v);
        }
    });

    autoTimer_ = new QTimer(this);
    autoTimer_->setInterval(kAutoplayMs);
    connect(autoTimer_, &QTimer::timeout, this, [this]() {
        if (cursor_ < plies_) {
            goNext();
        } else {
            stopPlaying();
        }
    });

    // Keyboard navigation, active whenever this widget's window is focused (a
    // focused spinbox/slider consumes the arrow first, so there is no conflict).
    auto key = [this](QKeySequence::StandardKey sk, void (PlaybackBar::*slot)()) {
        auto* s = new QShortcut(sk, this);
        s->setContext(Qt::WindowShortcut);
        connect(s, &QShortcut::activated, this, slot);
    };
    key(QKeySequence::MoveToPreviousChar, &PlaybackBar::goPrev);   // Left
    key(QKeySequence::MoveToNextChar,     &PlaybackBar::goNext);   // Right
    key(QKeySequence::MoveToStartOfLine,  &PlaybackBar::goFirst);  // Home
    key(QKeySequence::MoveToEndOfLine,    &PlaybackBar::goLast);   // End

    refreshControls();
}

void PlaybackBar::setPlyCount(int plies) {
    plies_ = std::max(0, plies);
    cursor_ = std::min(cursor_, plies_);
    refreshControls();
}

void PlaybackBar::setCursor(int ply) {
    cursor_ = std::clamp(ply, 0, plies_);
    if (playing_ && cursor_ >= plies_) {
        stopPlaying();
    }
    refreshControls();
}

void PlaybackBar::goFirst() { emitSeek(0); }
void PlaybackBar::goPrev()  { emitSeek(cursor_ - 1); }
void PlaybackBar::goNext()  { emitSeek(cursor_ + 1); }
void PlaybackBar::goLast()  { emitSeek(plies_); }

void PlaybackBar::togglePlay() {
    if (playing_) {
        stopPlaying();
    } else if (cursor_ < plies_) {
        playing_ = true;
        autoTimer_->start();
        refreshControls();
    }
}

void PlaybackBar::stopPlaying() {
    playing_ = false;
    autoTimer_->stop();
    refreshControls();
}

void PlaybackBar::emitSeek(int ply) {
    const int k = std::clamp(ply, 0, plies_);
    emit seek(k);  // the owner reconstructs and calls setCursor() back
}

void PlaybackBar::refreshControls() {
    firstBtn_->setEnabled(cursor_ > 0);
    prevBtn_->setEnabled(cursor_ > 0);
    nextBtn_->setEnabled(cursor_ < plies_);
    lastBtn_->setEnabled(cursor_ < plies_);
    playBtn_->setEnabled(plies_ > 0);
    playBtn_->setIcon(style()->standardIcon(
        playing_ ? QStyle::SP_MediaPause : QStyle::SP_MediaPlay));

    settingSlider_ = true;
    slider_->setMaximum(plies_);
    slider_->setValue(cursor_);
    settingSlider_ = false;

    label_->setText(QString("%1 / %2").arg(cursor_).arg(plies_));
}

}  // namespace guicommon
// Copyright Ben Paul Wise. All Rights Reserved.
