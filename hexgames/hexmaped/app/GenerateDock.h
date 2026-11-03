// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// GenerateDock -- the map generator's form (hexgen; doc/2026-10-31-map-generator-plan.md): size,
// seed, height method and roughness, the four terrain percentages (the rest role takes what is
// left, as HexKrieg's Map tab shows it), roads and cities on each edge, altitude. The form edits a
// whole HexGen::Settings, so what it does not show (terrain ids, colours, costs) comes from the last
// settings loaded, HexKrieg's by default. Generate runs HexGen::generate on a worker thread and hands
// the sheet to the handler; a refusal is shown in the dock.
// ----------------------------------------------
#pragma once
// Qt headers first, for Qt5's moc on GCC: see the note in MapView.h.
#include <QFutureWatcher>
#include <QWidget>

#include "hexgen/Settings.h"
#include "hexxml/SheetDoc.h"

#include <array>
#include <functional>
#include <optional>
#include <string>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

namespace HexQt {

  class GenerateDock final : public QWidget {
    Q_OBJECT
  public:
    using SheetHandler = std::function<void(HexXml::SheetDoc)>;

    explicit GenerateDock(SheetHandler onSheet, QWidget* parent = nullptr);
    ~GenerateDock() override;

    // The form's settings; throws std::invalid_argument naming what is wrong.
    HexGen::Settings settings() const;
    void setSettings(const HexGen::Settings&);
    void generate();  // starts a generation unless one is running
    bool runningP() const;

  private:
    struct Outcome {
      std::optional<HexXml::SheetDoc> sheet;
      std::string error;
    };
    struct EdgeRow {
      QSpinBox* roads;
      QSpinBox* cities;
    };

    void loadSettings();
    void saveSettings();
    void refreshLimits();  // percentages never exceed 100; cities never exceed roads
    void finished();

    SheetHandler onSheet_;
    HexGen::Settings base_;
    QLineEdit* title_;
    QSpinBox* cols_;
    QSpinBox* rows_;
    QComboBox* orientation_;
    QComboBox* offset_;
    QLineEdit* seed_;
    QComboBox* method_;
    QDoubleSpinBox* roughness_;
    std::array<QLabel*, 4> percentLabels_{};
    std::array<QSpinBox*, 4> percents_{};  // low, high, cover, city
    QLabel* rest_;
    std::array<EdgeRow, 4> edges_{};  // top, bottom, left, right
    QCheckBox* altitude_;
    QPushButton* generate_;
    QLabel* message_;
    QFutureWatcher<Outcome> watcher_;
  };

}  // namespace HexQt
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
