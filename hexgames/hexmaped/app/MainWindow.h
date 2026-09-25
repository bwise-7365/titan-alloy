// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#pragma once
// MainWindow -- HexMapEd: a Document, the MapView, a mode (select, terrain, hexside, link, clip,
// glyph, name) with the kind the mode paints, the doubts list, the inspector, and File/Edit menus.
// Every click becomes one Document edit; the Document refuses what the schema would; the view
// redraws from the Document. Save writes the sheet and runs tools/validate-xml.py on the result.

#include "MapView.h"
#include "hexmaped/Document.h"

#include <QMainWindow>
#include <QString>

#include <memory>
#include <optional>
#include <string>

class QComboBox;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QSpinBox;
class QTextEdit;

namespace HexQt {

  class MainWindow final : public QMainWindow {
    Q_OBJECT
  public:
    explicit MainWindow(QWidget* parent = nullptr);

    void open(const QString& path);

  protected:
    void closeEvent(QCloseEvent*) override;

  private:
    enum class Mode { Select, Terrain, Hexside, Link, Clip, Glyph, Name };

    void buildMenus();
    void buildToolbar();
    void buildDocks();
    void refreshKinds();
    void refreshActions();
    void loadDoubtsBeside(const QString& sheetPath);
    void onHovered(QPointF p);
    void onClicked(QPointF p, Qt::MouseButton button, Qt::KeyboardModifiers modifiers);
    void onDoubtChosen(QListWidgetItem*);
    void save();
    void saveAs();
    void validateSaved();
    void applyEdit(const std::function<void()>& edit);  // runs it, reports a refusal, redraws
    void describe(const std::string& hex);
    Mode mode() const;

    std::unique_ptr<HexMapEd::Document> doc_;
    MapView* view_;
    QComboBox* modeBox_;
    QComboBox* kindBox_;
    QComboBox* slotBox_;
    QComboBox* linkKindBox_;
    QSpinBox* wavinessBox_;  // presentation: hand-scratched roads, railways, rivers (0 = straight)
    QLabel* status_;
    QListWidget* doubts_;
    QTextEdit* inspector_;
    QAction* saveAction_ = nullptr;
    QAction* undoAction_ = nullptr;
    QAction* redoAction_ = nullptr;
    std::optional<std::string> linkStart_;  // Link mode: the first hex of the step being added
  };

}  // namespace HexQt
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
