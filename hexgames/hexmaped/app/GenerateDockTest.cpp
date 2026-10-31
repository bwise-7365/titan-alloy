// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The Generate dock, offscreen: its form gives back the settings it was given, refuses what the
// generator would, and a Generate hands over the generated sheet.
// ----------------------------------------------
#include <QApplication>
#include <QElapsedTimer>

#include "GenerateDock.h"

#include "hexgen/Generate.h"

#include <gtest/gtest.h>

#include <optional>

namespace {

  QApplication&
  app()
  {
    static int argc = 1;
    static char name[] = "hexmaped_generate_dock_test";
    static char* argv[] = {name, nullptr};
    static QApplication instance(argc, argv);
    return instance;
  }

  TEST(GenerateDock, TheFormGivesBackItsSettings)
  {
    app();
    HexQt::GenerateDock dock([](HexXml::SheetDoc) {});
    HexGen::Settings s = HexGen::Settings::hexkrieg();
    s.cols = 20;
    s.rows = 12;
    s.seed = -5;
    s.method = HexGen::Method::Fractal;
    s.altitude = HexGen::AltitudeSettings{};
    s.roads->sides = {{HexGen::Side::Top, 3, 1}, {HexGen::Side::Left, 2, 2}};
    dock.setSettings(s);
    EXPECT_EQ(HexGen::writeSettings(s), HexGen::writeSettings(dock.settings()));
    return;
  }

  TEST(GenerateDock, GenerateHandsOverTheSheet)
  {
    app();
    std::optional<HexXml::SheetDoc> got;
    HexQt::GenerateDock dock([&](HexXml::SheetDoc sheet) { got = std::move(sheet); });
    HexGen::Settings s = HexGen::Settings::hexkrieg();
    s.cols = 20;
    s.rows = 12;
    dock.setSettings(s);
    dock.generate();
    QElapsedTimer timer;
    timer.start();
    while (!got && 30000 > timer.elapsed()) {
      QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
    ASSERT_TRUE(got.has_value());
    ASSERT_EQ(1u, got->grids.size());
    EXPECT_EQ(20, got->grids[0].cols);
    EXPECT_EQ(12, got->grids[0].rows);
    EXPECT_EQ(s.seed, got->seed);
    EXPECT_FALSE(got->links.empty());
    return;
  }

}  // namespace
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
