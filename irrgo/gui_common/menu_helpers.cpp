// Copyright Ben Paul Wise. All Rights Reserved.
#include "menu_helpers.h"

#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QMenu>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>
#include <QWidgetAction>

namespace guicommon {

void retainSizeWhenHidden(QWidget* w) {
    auto sp = w->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    w->setSizePolicy(sp);
}

QProgressBar* makeStatusBar(QWidget* parent, int heightPx) {
    auto* bar = new QProgressBar(parent);
    bar->setRange(0, 100);
    bar->setFixedHeight(heightPx);
    bar->setTextVisible(false);
    retainSizeWhenHidden(bar);
    bar->hide();
    return bar;
}

QPushButton* buildNegaMaxMenu(QWidget* owner, QMenu* parent, QActionGroup* group,
                              const NegaMaxMenuConfig& cfg,
                              QSpinBox*& depthOut, QSpinBox*& turnsOut) {
    auto* action = new QAction("NegaMax", owner);
    action->setCheckable(true);
    group->addAction(action);
    parent->addAction(action);

    auto* nmMenu = new QMenu(owner);
    auto* widget = new QWidget;
    auto* vbox   = new QVBoxLayout(widget);
    vbox->setContentsMargins(8, 6, 8, 6);
    auto* form   = new QFormLayout;
    form->setSpacing(6);
    vbox->addLayout(form);

    depthOut = new QSpinBox(widget);
    depthOut->setRange(cfg.depthMin, cfg.depthMax);
    depthOut->setValue(cfg.depthDefault);
    form->addRow("Depth:", depthOut);

    if (cfg.withTurns) {
        turnsOut = new QSpinBox(widget);
        turnsOut->setRange(cfg.turnsMin, cfg.turnsMax);
        turnsOut->setValue(cfg.turnsDefault);
        form->addRow("Turns:", turnsOut);
    } else {
        turnsOut = nullptr;
    }

    auto* sep = new QFrame(widget);
    sep->setFrameShape(QFrame::HLine);
    vbox->addWidget(sep);

    auto* goBtn = new QPushButton("Go!", widget);
    vbox->addWidget(goBtn);
    QObject::connect(goBtn, &QPushButton::clicked, nmMenu, &QMenu::hide);

    auto* wa = new QWidgetAction(nmMenu);
    wa->setDefaultWidget(widget);
    nmMenu->addAction(wa);
    action->setMenu(nmMenu);

    QObject::connect(nmMenu, &QMenu::aboutToShow, action, [action]() {
        action->setChecked(true);
    });
    return goBtn;
}

QPushButton* buildMctsMenu(QWidget* owner, QMenu* parent, QActionGroup* group,
                           const MctsMenuConfig& cfg,
                           QComboBox*& secOut, QSpinBox*& turnsOut) {
    auto* action = new QAction("MCTS", owner);
    action->setCheckable(true);
    group->addAction(action);
    parent->addAction(action);

    auto* mctsMenu = new QMenu(owner);
    auto* widget   = new QWidget;
    auto* vbox     = new QVBoxLayout(widget);
    vbox->setContentsMargins(8, 6, 8, 6);
    auto* form     = new QFormLayout;
    form->setSpacing(6);
    vbox->addLayout(form);

    secOut = new QComboBox(widget);
    for (std::size_t i = 0; i < cfg.optionCount; ++i) {
        secOut->addItem(cfg.options[i].label, cfg.options[i].secs);
    }
    form->addRow("Time:", secOut);

    if (cfg.withTurns) {
        turnsOut = new QSpinBox(widget);
        turnsOut->setRange(cfg.turnsMin, cfg.turnsMax);
        turnsOut->setValue(cfg.turnsDefault);
        form->addRow("Turns:", turnsOut);
    } else {
        turnsOut = nullptr;
    }

    auto* sep = new QFrame(widget);
    sep->setFrameShape(QFrame::HLine);
    vbox->addWidget(sep);

    auto* goBtn = new QPushButton("Go!", widget);
    vbox->addWidget(goBtn);
    QObject::connect(goBtn, &QPushButton::clicked, mctsMenu, &QMenu::hide);

    auto* wa = new QWidgetAction(mctsMenu);
    wa->setDefaultWidget(widget);
    mctsMenu->addAction(wa);
    action->setMenu(mctsMenu);

    QObject::connect(mctsMenu, &QMenu::aboutToShow, action, [action]() {
        action->setChecked(true);
    });
    return goBtn;
}

}  // namespace guicommon
// Copyright Ben Paul Wise. All Rights Reserved.
