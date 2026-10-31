// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "GenerateDock.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>

#include "hexgen/Generate.h"

#include <exception>
#include <random>
#include <stdexcept>

namespace HexQt {

  namespace {

    constexpr std::array<HexGen::Role, 4> kPercentRoles{HexGen::Role::Low, HexGen::Role::High, HexGen::Role::Cover,
                                                        HexGen::Role::City};
    constexpr std::array<HexGen::Side, 4> kSides{HexGen::Side::Top, HexGen::Side::Bottom, HexGen::Side::Left,
                                                 HexGen::Side::Right};

    QString
    q(const std::string& s)
    {
      return QString::fromStdString(s);
    }

    QSpinBox*
    spin(QWidget* parent, int lo, int hi)
    {
      QSpinBox* box = new QSpinBox(parent);
      box->setRange(lo, hi);
      return box;
    }

  }  // namespace

  GenerateDock::GenerateDock(SheetHandler onSheet, QWidget* parent)
    : QWidget(parent),
      onSheet_(std::move(onSheet)),
      title_(new QLineEdit(this)),
      cols_(spin(this, 5, 300)),
      rows_(spin(this, 5, 300)),
      orientation_(new QComboBox(this)),
      offset_(new QComboBox(this)),
      seed_(new QLineEdit(this)),
      method_(new QComboBox(this)),
      roughness_(new QDoubleSpinBox(this)),
      rest_(new QLabel(this)),
      altitude_(new QCheckBox("Write altitudes (metres)", this)),
      generate_(new QPushButton("Generate", this)),
      message_(new QLabel(this))
  {
    orientation_->addItems({"flat", "pointy"});
    offset_->addItems({"odd", "even"});
    method_->addItems({"fractal", "perlin"});
    roughness_->setRange(0.0, 10.0);
    roughness_->setDecimals(4);
    roughness_->setSingleStep(0.05);
    roughness_->setToolTip("fractal: the perturbation scale; perlin: the amplitude kept from one octave to the next");
    QPushButton* random = new QPushButton("Random", this);
    connect(random, &QPushButton::clicked, this, [this] {
      std::random_device device;  // the GUI's own choice of a fresh seed; the generator never draws from it
      seed_->setText(QString::number(static_cast<qlonglong>(device() % 100000000u)));
    });
    QHBoxLayout* seedRow = new QHBoxLayout;
    seedRow->addWidget(seed_, 1);
    seedRow->addWidget(random);

    QFormLayout* form = new QFormLayout;
    form->addRow("Title", title_);
    form->addRow("Columns", cols_);
    form->addRow("Rows", rows_);
    form->addRow("Orientation", orientation_);
    form->addRow("Offset", offset_);
    form->addRow("Seed", seedRow);
    form->addRow("Heights", method_);
    form->addRow("Roughness", roughness_);

    QGroupBox* terrain = new QGroupBox("Terrain %", this);
    QFormLayout* terrainForm = new QFormLayout(terrain);
    for (std::size_t k = 0; k < kPercentRoles.size(); ++k) {
      percentLabels_[k] = new QLabel(terrain);
      percents_[k] = spin(terrain, 0, 100);
      terrainForm->addRow(percentLabels_[k], percents_[k]);
      connect(percents_[k], qOverload<int>(&QSpinBox::valueChanged), this, [this](int) { refreshLimits(); });
    }
    terrainForm->addRow(rest_);

    QGroupBox* edges = new QGroupBox("Edge roads", this);
    QGridLayout* grid = new QGridLayout(edges);
    grid->addWidget(new QLabel("Roads", edges), 0, 1);
    grid->addWidget(new QLabel("Cities", edges), 0, 2);
    for (std::size_t k = 0; k < kSides.size(); ++k) {
      const int row = static_cast<int>(k) + 1;
      const std::string_view name = HexGen::sideName(kSides[k]);
      grid->addWidget(new QLabel(QString::fromUtf8(name.data(), static_cast<int>(name.size())), edges), row, 0);
      edges_[k] = EdgeRow{spin(edges, 0, 300), spin(edges, 0, 300)};
      grid->addWidget(edges_[k].roads, row, 1);
      grid->addWidget(edges_[k].cities, row, 2);
      connect(edges_[k].roads, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) { refreshLimits(); });
    }

    QPushButton* load = new QPushButton("Load settings...", this);
    QPushButton* save = new QPushButton("Save settings...", this);
    connect(load, &QPushButton::clicked, this, [this] { loadSettings(); });
    connect(save, &QPushButton::clicked, this, [this] { saveSettings(); });
    connect(generate_, &QPushButton::clicked, this, [this] { generate(); });
    connect(&watcher_, &QFutureWatcher<Outcome>::finished, this, [this] { finished(); });
    QHBoxLayout* files = new QHBoxLayout;
    files->addWidget(load);
    files->addWidget(save);

    message_->setWordWrap(true);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(terrain);
    layout->addWidget(edges);
    layout->addWidget(altitude_);
    layout->addLayout(files);
    layout->addWidget(generate_);
    layout->addWidget(message_);
    layout->addStretch(1);
    setSettings(HexGen::Settings::hexkrieg());
  }

  GenerateDock::~GenerateDock()
  {
    watcher_.waitForFinished();  // the worker reads nothing of this widget, but its result lands here
  }

  bool
  GenerateDock::runningP() const
  {
    return watcher_.isRunning();
  }

  HexGen::Settings
  GenerateDock::settings() const
  {
    HexGen::Settings s = base_;
    s.title = title_->text().toStdString();
    s.cols = cols_->value();
    s.rows = rows_->value();
    s.orientation = 0 == orientation_->currentIndex() ? HexCoord::Orientation::Flat : HexCoord::Orientation::Pointy;
    s.offset = 0 == offset_->currentIndex() ? HexCoord::Parity::Odd : HexCoord::Parity::Even;
    bool okP = false;
    s.seed = seed_->text().trimmed().toLongLong(&okP);
    if (!okP) {
      throw std::invalid_argument("the seed '" + seed_->text().toStdString() + "' is not a whole number");
    }
    s.method = 0 == method_->currentIndex() ? HexGen::Method::Fractal : HexGen::Method::Perlin;
    s.roughness = roughness_->value();
    for (std::size_t k = 0; k < kPercentRoles.size(); ++k) {
      s.terrains[s.terrainIndex(kPercentRoles[k])].percent = percents_[k]->value();
    }
    if (!s.roads) {
      s.roads = HexGen::RoadSettings{};
    }
    s.roads->sides.clear();
    for (std::size_t k = 0; k < kSides.size(); ++k) {
      if (0 < edges_[k].roads->value()) {
        s.roads->sides.push_back({kSides[k], edges_[k].roads->value(), edges_[k].cities->value()});
      }
    }
    if (!altitude_->isChecked()) {
      s.altitude.reset();
    }
    else if (!s.altitude) {
      s.altitude = HexGen::AltitudeSettings{};
    }
    HexGen::validate(s, "the generator's settings");
    return s;
  }

  void
  GenerateDock::setSettings(const HexGen::Settings& s)
  {
    base_ = s;
    for (QSpinBox* box : percents_) {
      box->setMaximum(100);  // refreshLimits narrows them again once every value is in
    }
    for (const EdgeRow& row : edges_) {
      row.cities->setMaximum(300);
    }
    title_->setText(q(s.title));
    cols_->setValue(s.cols);
    rows_->setValue(s.rows);
    orientation_->setCurrentIndex(HexCoord::Orientation::Flat == s.orientation ? 0 : 1);
    offset_->setCurrentIndex(HexCoord::Parity::Odd == s.offset ? 0 : 1);
    seed_->setText(QString::number(static_cast<qlonglong>(s.seed)));
    method_->setCurrentIndex(HexGen::Method::Fractal == s.method ? 0 : 1);
    roughness_->setValue(s.roughness);
    for (std::size_t k = 0; k < kPercentRoles.size(); ++k) {
      const HexGen::TerrainSetting& t = s.terrain(kPercentRoles[k]);
      percentLabels_[k]->setText(q(t.name) + " (" + q(std::string(HexGen::roleName(t.role))) + ")");
      percents_[k]->blockSignals(true);
      percents_[k]->setValue(t.percent);
      percents_[k]->blockSignals(false);
    }
    for (std::size_t k = 0; k < kSides.size(); ++k) {
      int roads = 0;
      int cities = 0;
      if (s.roads) {
        for (const HexGen::SideSetting& side : s.roads->sides) {
          if (kSides[k] == side.at) {
            roads = side.roads;
            cities = side.cities;
          }
        }
      }
      edges_[k].roads->blockSignals(true);
      edges_[k].roads->setValue(roads);
      edges_[k].roads->blockSignals(false);
      edges_[k].cities->setValue(cities);
    }
    altitude_->setChecked(s.altitude.has_value());
    refreshLimits();
    message_->clear();
    return;
  }

  void
  GenerateDock::refreshLimits()
  {
    int total = 0;
    for (const QSpinBox* box : percents_) {
      total += box->value();
    }
    for (QSpinBox* box : percents_) {
      box->setMaximum(box->value() + 100 - total);
    }
    const HexGen::TerrainSetting& rest = base_.terrain(HexGen::Role::Rest);
    rest_->setText(q(rest.name) + " (rest): " + QString::number(100 - total) + "%");
    for (const EdgeRow& row : edges_) {
      row.cities->setMaximum(row.roads->value());
    }
    return;
  }

  void
  GenerateDock::generate()
  {
    if (runningP()) {
      return;
    }
    HexGen::Settings s;
    try {
      s = settings();
    } catch (const std::exception& e) {
      message_->setText(QString("Refused: ") + e.what());
      return;
    }
    generate_->setEnabled(false);
    message_->setText("Generating...");
    watcher_.setFuture(QtConcurrent::run([s] {
      Outcome out;
      try {
        out.sheet = HexGen::generate(s);
      } catch (const std::exception& e) {
        out.error = e.what();
      }
      return out;
    }));
    return;
  }

  void
  GenerateDock::finished()
  {
    generate_->setEnabled(true);
    Outcome out = watcher_.result();
    if (!out.sheet) {
      message_->setText(QString("Refused: ") + q(out.error));
      return;
    }
    message_->setText(QString("Generated %1 x %2, seed %3.").arg(cols_->value()).arg(rows_->value()).arg(seed_->text()));
    onSheet_(std::move(*out.sheet));
    return;
  }

  void
  GenerateDock::loadSettings()
  {
    const QString path = QFileDialog::getOpenFileName(this, "Load generator settings", QString(), "Settings (*.xml)");
    if (path.isEmpty()) {
      return;
    }
    try {
      setSettings(HexGen::Settings::load(path.toStdString()));
    } catch (const std::exception& e) {
      message_->setText(QString("Cannot load: ") + e.what());
    }
    return;
  }

  void
  GenerateDock::saveSettings()
  {
    HexGen::Settings s;
    try {
      s = settings();
    } catch (const std::exception& e) {
      message_->setText(QString("Refused: ") + e.what());
      return;
    }
    const QString path = QFileDialog::getSaveFileName(this, "Save generator settings", q(s.id) + ".xml", "Settings (*.xml)");
    if (path.isEmpty()) {
      return;
    }
    try {
      HexGen::writeSettings(path.toStdString(), s);
      message_->setText("Saved " + path);
    } catch (const std::exception& e) {
      message_->setText(QString("Cannot save: ") + e.what());
    }
    return;
  }

}  // namespace HexQt
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
