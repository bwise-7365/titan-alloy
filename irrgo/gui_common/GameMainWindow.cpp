// Copyright Ben Paul Wise. All Rights Reserved.
#include "GameMainWindow.h"

namespace guicommon {

GameMainWindow::GameMainWindow(QWidget* parent) : QMainWindow(parent) {
    search_ = new SearchController(this);
    connect(search_, &SearchController::searchingChanged, this, [this](bool) {
        updateControls();
    });
}

void GameMainWindow::startPlay(SearchController::Params params, int turns) {
    if (!currentGame() || currentGame()->isTerminal()
        || search().isSearching() || extraSearchBlock()) {
        return;
    }
    search().beginTurnRun(turns);
    search().launch(currentGame()->clone(), params,
                    [this, params, turns](AbsGame::MoveId mv, unsigned) {
        applyComputedMove(mv);
        search().advanceTurn();
        if (currentGame() && !currentGame()->isTerminal() && search().turnRunActive()) {
            startPlay(params, turns);
        } else {
            search().endTurnRun();
        }
    });
}

}  // namespace guicommon
// Copyright Ben Paul Wise. All Rights Reserved.
