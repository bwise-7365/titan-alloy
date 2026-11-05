// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// pgg_trc_viewer's one window: the map from HexMapEd's MapView with the units over it, what the current step
// did down the side, and a slider along the bottom to drag through the game. File > Open takes a hexsave script
// or a run file; from a run file you choose a game, and it is played again on a worker thread. A fingerprint
// that differs from the run file's raises a warning.
// ----------------------------------------------
#pragma once
// Qt headers first, for Qt5's moc on GCC: see the note in MapView.h.
#include <QFutureWatcher>
#include <QMainWindow>

#include "ViewerGame.h"

#include <functional>
#include <memory>
#include <string>

class QAction;
class QLabel;
class QPlainTextEdit;
class QSlider;

namespace HexQt {

  class MapView;

  class ViewerWindow final : public QMainWindow {
    Q_OBJECT
  public:
    using Warning = std::function<void(const std::string& title, const std::string& text)>;

    explicit ViewerWindow(QWidget* parent = nullptr);
    ~ViewerWindow() override;

    void openFile(const QString& path);  // a script opens at once; a run file asks which game
    void openScript(const QString& path);
    void openRunGame(const RunFile&, const RunLine&);
    bool loadingP() const;
    void setWarning(Warning);            // by default a message box; a test listens instead
    void showMove(int move);

    const ViewerGame* game() const { return game_.get(); }
    QSlider* slider() const { return slider_; }

  private:
    struct Loaded {
      std::shared_ptr<ViewerGame> game;
      std::string error;
    };
    void load(std::function<std::unique_ptr<ViewerGame>()> make, const QString& what);
    void finished();
    void stepTurn(int by);
    void chooseFile();

    MapView* map_;
    QSlider* slider_;
    QLabel* status_;
    QLabel* fingerprint_;
    QPlainTextEdit* events_;
    QAction* reveal_;
    Warning warning_;
    QFutureWatcher<Loaded> watcher_;
    std::shared_ptr<ViewerGame> game_;
  };

}  // namespace HexQt
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
