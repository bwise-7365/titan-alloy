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

namespace {

// The widget scaffolding every submenu shares: a checkable action in the exclusive group
// carrying a pop-out QMenu whose single widget holds a form (the caller fills its rows), a
// horizontal rule, and a Go! button. Returns the form to populate and the Go! button to
// wire; `widget` parents any controls the caller adds to the form.
struct MenuShell {
    QFormLayout* form   = nullptr;
    QPushButton* goBtn  = nullptr;
    QWidget*     widget = nullptr;
};

MenuShell buildMenuShell(const char* title, QWidget* owner, QMenu* parent,
                         QActionGroup* group) {
    auto* action = new QAction(title, owner);
    action->setCheckable(true);
    group->addAction(action);
    parent->addAction(action);

    auto* budgetMenu = new QMenu(owner);
    auto* widget = new QWidget;
    auto* vbox   = new QVBoxLayout(widget);
    vbox->setContentsMargins(8, 6, 8, 6);
    auto* form   = new QFormLayout;
    form->setSpacing(6);
    vbox->addLayout(form);  // the form keeps its slot above the rule/Go! as rows are added

    auto* sep = new QFrame(widget);
    sep->setFrameShape(QFrame::HLine);
    vbox->addWidget(sep);

    auto* goBtn = new QPushButton("Go!", widget);
    vbox->addWidget(goBtn);
    QObject::connect(goBtn, &QPushButton::clicked, budgetMenu, &QMenu::hide);

    auto* wa = new QWidgetAction(budgetMenu);
    wa->setDefaultWidget(widget);
    budgetMenu->addAction(wa);
    action->setMenu(budgetMenu);

    QObject::connect(budgetMenu, &QMenu::aboutToShow, action, [action]() {
        action->setChecked(true);
    });
    return {form, goBtn, widget};
}

// Shared body of the time-budgeted submenus. The MCTS and NegaMax menus are the same
// widget set (Time [+ Turns] + Go!) and differ only in their title, so they share one
// implementation rather than a copy each.
QPushButton* buildTimeMenu(const char* title, QWidget* owner, QMenu* parent,
                           QActionGroup* group, const TimeMenuConfig& cfg,
                           QComboBox*& secOut, QSpinBox*& turnsOut) {
    MenuShell shell = buildMenuShell(title, owner, parent, group);

    secOut = new QComboBox(shell.widget);
    for (std::size_t i = 0; i < cfg.optionCount; ++i) {
        secOut->addItem(cfg.options[i].label, cfg.options[i].secs);
    }
    shell.form->addRow("Time:", secOut);

    if (cfg.withTurns) {
        turnsOut = new QSpinBox(shell.widget);
        turnsOut->setRange(cfg.turnsMin, cfg.turnsMax);
        turnsOut->setValue(cfg.turnsDefault);
        shell.form->addRow("Turns:", turnsOut);
    } else {
        turnsOut = nullptr;
    }
    return shell.goBtn;
}

}  // anonymous namespace

QPushButton* buildMctsMenu(QWidget* owner, QMenu* parent, QActionGroup* group,
                           const TimeMenuConfig& cfg,
                           QComboBox*& secOut, QSpinBox*& turnsOut) {
    return buildTimeMenu("MCTS", owner, parent, group, cfg, secOut, turnsOut);
}

QPushButton* buildNegaMaxTimeMenu(QWidget* owner, QMenu* parent, QActionGroup* group,
                                  const TimeMenuConfig& cfg,
                                  QComboBox*& secOut, QSpinBox*& turnsOut) {
    return buildTimeMenu("NegaMax", owner, parent, group, cfg, secOut, turnsOut);
}

QPushButton* buildComputerMenu(QWidget* owner, QMenu* parent, QActionGroup* group,
                               const ComputerMenuConfig& cfg,
                               QComboBox*& secOut, QComboBox*& sideOut) {
    MenuShell shell = buildMenuShell("Computer", owner, parent, group);

    secOut = new QComboBox(shell.widget);
    for (std::size_t i = 0; i < cfg.optionCount; ++i) {
        secOut->addItem(cfg.options[i].label, cfg.options[i].secs);
    }
    shell.form->addRow("Think:", secOut);

    // The human's side. Combo data is the player index the human plays; the computer takes
    // the other one.
    sideOut = new QComboBox(shell.widget);
    sideOut->addItem(cfg.side0Label, 0);
    sideOut->addItem(cfg.side1Label, 1);
    sideOut->setCurrentIndex((cfg.defaultHumanSide == 1) ? 1 : 0);
    shell.form->addRow("You play:", sideOut);

    return shell.goBtn;
}

}  // namespace guicommon
// Copyright Ben Paul Wise. All Rights Reserved.
