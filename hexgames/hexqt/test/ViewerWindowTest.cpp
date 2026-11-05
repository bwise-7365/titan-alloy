// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The viewer's window, offscreen: it opens a script, follows the slider, plays a game of a run file again and
// warns when the fingerprint is not the file's.
// ----------------------------------------------
#include <QApplication>
#include <QElapsedTimer>
#include <QSlider>

#include "ViewerWindow.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

  const std::filesystem::path kRoot = HEXGAMES_SOURCE_DIR;

  QApplication&
  app()
  {
    static int argc = 1;
    static char name[] = "hexqt_viewer_window_test";
    static char* argv[] = {name, nullptr};
    static QApplication instance(argc, argv);
    return instance;
  }

  void
  waitFor(HexQt::ViewerWindow& window)
  {
    QElapsedTimer timer;
    timer.start();
    do {
      QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    } while ((window.loadingP() || nullptr == window.game()) && 300000 > timer.elapsed());
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    return;
  }

}  // namespace

TEST(ViewerWindowTest, ItOpensAScriptAndFollowsTheSlider)
{
  app();
  HexQt::ViewerWindow window;
  std::vector<std::string> warnings;
  window.setWarning([&](const std::string& title, const std::string&) { warnings.push_back(title); });
  window.openScript(QString::fromStdString((kRoot / "games" / "trc" / "golden" / "combat-crt.script.xml").string()));
  waitFor(window);
  ASSERT_NE(nullptr, window.game());
  EXPECT_TRUE(warnings.empty());
  EXPECT_EQ(static_cast<int>(window.game()->moveCount()), window.slider()->maximum());
  window.slider()->setValue(window.slider()->maximum());
  window.slider()->setValue(0);
  window.slider()->setValue(1);
  EXPECT_EQ(1, window.slider()->value());
}

TEST(ViewerWindowTest, AFingerprintThatDiffersRaisesAWarning)
{
  app();
  HexQt::ViewerWindow window;
  std::vector<std::string> warnings;
  window.setWarning([&](const std::string& title, const std::string&) { warnings.push_back(title); });
  HexQt::RunFile run = HexQt::readRunFile(kRoot / "doc" / "pgg-runs" / "2026-11-04-drawn-seize-seize.txt");
  HexQt::RunLine line = run.lines.front();
  line.fingerprint += 1;
  window.openRunGame(run, line);
  waitFor(window);
  ASSERT_NE(nullptr, window.game());
  EXPECT_TRUE(window.game()->fingerprintMismatchP());
  ASSERT_EQ(1u, warnings.size());
  EXPECT_EQ("Fingerprint mismatch", warnings.front());
}

TEST(ViewerWindowTest, AGameOfARunFileMatchesItsFingerprint)
{
  app();
  HexQt::ViewerWindow window;
  std::vector<std::string> warnings;
  window.setWarning([&](const std::string& title, const std::string&) { warnings.push_back(title); });
  const HexQt::RunFile run = HexQt::readRunFile(kRoot / "doc" / "pgg-runs" / "2026-11-04-drawn-seize-seize.txt");
  window.openRunGame(run, run.lines.front());
  waitFor(window);
  ASSERT_NE(nullptr, window.game());
  EXPECT_FALSE(window.game()->fingerprintMismatchP());
  EXPECT_EQ(run.lines.front().fingerprint, window.game()->fingerprint());
  EXPECT_TRUE(warnings.empty());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
