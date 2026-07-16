// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. The yr constructs map to Qt as
// follows: YR::Draw (double-buffered Xlib canvas) becomes
// SimCanvas, a QWidget painting from a QPixmap backing store,
// which also absorbs the raw Xt ButtonPress handler as
// mousePressEvent; SimGUITimer (XtAppAddTimeOut) becomes a
// QTimer; the RunStop/Step command objects become buttons; the
// sliders become QSlider; Pixel becomes QColor, with the old
// GraphicManager color names kept so the display code reads as
// before; the static trampolines through theApp (setupSim,
// stepSimulation, setStepInterval, setEventsPerStep) are gone --
// the widgets connect straight to the inner* member functions.
//
// supporting data structures for the simulation demo
// ------------------------------------------

#ifndef XT_TESTAPP_SIMGUI_H
#define XT_TESTAPP_SIMGUI_H

// ------------------------------------------

#include "des.h"
#include "xtdemo.h"
#include "acpsim.h"

#include <QColor>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QWidget>

class SimGUIModule;

// ------------------------------------------
// the drawing area. display code paints into the backing pixmap;
// paintEvent copies the pixmap to the screen; update() plays the
// role of the old copyBackToFront/copyFrontToWindow pair.

class SimCanvas : public QWidget {
public:
  SimCanvas(int w, int h, QWidget* parent);

  // batch painting: displaySim brackets its draw calls with
  // beginPaint/endPaint so one QPainter serves the whole frame.
  // outside a batch, each draw call opens its own painter.
  void beginPaint();
  void endPaint();

  void clearBack(const QColor& c); // must not be inside a batch

  void drawColorLine(int x1, int y1, int x2, int y2, const QColor& fg);
  void drawColorDot(int x, int y, int w, int h, const QColor& fg);
  void drawColorBox(int x, int y, int w, int h, const QColor& fg);

  int getWidth() const { return width(); }
  int getHeight() const { return height(); }

  // back-pointer for the mouse handler (zoom/recenter/ID)
  SimGUIModule* module;

protected:
  void paintEvent(QPaintEvent*) override;
  void mousePressEvent(QMouseEvent*) override;

  QPixmap back;
  QPainter painter;
  bool painting;

private:
};

// ------------------------------------------
// the old yr GraphicManager pixel names, as QColor values,
// so the display code keeps its original spellings.
// grey values follow X11 (greyNN = NN% of 255).

namespace GraphicManager {
  const QColor blackColor  = QColor(  0,   0,   0);
  const QColor whiteColor  = QColor(255, 255, 255);
  const QColor redColor    = QColor(255,   0,   0);
  const QColor greenColor  = QColor(  0, 255,   0);
  const QColor blueColor   = QColor(  0,   0, 255);
  const QColor cyanColor   = QColor(  0, 255, 255);
  const QColor yellowColor = QColor(255, 255,   0);
  const QColor orangeColor = QColor(255, 165,   0);
  const QColor purpleColor = QColor(160,  32, 240);
  const QColor greyColor   = QColor(190, 190, 190);
  const QColor goldColor   = QColor(255, 215,   0);
  const QColor crimsonColor= QColor(220,  20,  60);
  const QColor tanColor    = QColor(210, 180, 140);
  const QColor grey20Color = QColor( 51,  51,  51);
  const QColor grey30Color = QColor( 77,  77,  77);
  const QColor grey40Color = QColor(102, 102, 102);
  const QColor grey50Color = QColor(127, 127, 127);
  const QColor grey60Color = QColor(153, 153, 153);
  const QColor grey70Color = QColor(179, 179, 179);
  const QColor grey80Color = QColor(204, 204, 204);
  const QColor grey90Color = QColor(229, 229, 229);
}

// ------------------------------------------

class SimGUIModule {

 public:
  SimGUIModule();
  SimGUIModule(ACPSim* sim, SimCanvas* canvas, QWidget* cmdBar);
  virtual ~SimGUIModule();


  ACPSim* mySim;
  QTimer* simTimer;

  // output is piped to this window
  SimCanvas* drawArea;
  double margin;

  // various sim-controlling commands go here
  QWidget* commandBar;

  QLabel* timeOutputLabel;
  QLabel* eventOutputLabel;

  QPushButton* runStopButton; // checkable: checked = running
  QPushButton* stepButton;

  // control and display of steps/sec
  QSlider* stepSlider;
  QLabel* stepSliderValueLabel;

  // control and display of events/step
  QSlider* countSlider;
  QLabel* countSliderValueLabel;

  QLabel* scaleValueLabel;

  static QColor defaultMapColor;

  void innerSetupSim(int s);
  void innerStepSimulation();

  // the Dismiss semantics of the old SimWindowLower command:
  // stop the timer, delete the sim, black out the canvas
  void lowerSimWindow();

  void displaySim();

  // notice that these assume:
  // mx and my are in meters, measured on the simulate terrain
  // wx and wy are in pixels, measured on the screen
  void mapToWindow(float mx, float my, int &wx, int &wy);
  void windowToMap(int wx, int wy, float &mx, float &my);

  float centerX; // the x-coord, in meters, of the tgrid which is centered in the screen
  float centerY; // the y-coord, in meters,  of the tgrid which is centered in the screen
  void setAspectRatio();
  int displayScale;
  int oldDisplayScale;

  void innerSetScaleFactor(int i);


  int displayGridP;
  int displayReliefP;
  int displayBoxesP;
  int displayUnitsP;
  int displayUnitsLargeP;

  // i = events per step
  void innerSetEventsPerStep(int i);

  // i = steps per second
  void innerSetStepInterval(int i);

 protected:

  float xAspect;
  float yAspect;

  int oldStepFreq;
  int oldEventsPerStep;

  float scaleFactor;

  void scenarioNull(); // run test functions, if desired
  void setupScenarioOneRU();
  void setupScenarioOneCU();
  void setupScenarioLayeredCU();
  void setupScenarioRandomCorr();
  void setupScenarioRecursiveCorr();

  void displayCorridors();
  void displayCorridor();
  void displayBox(Box*, QColor p);

  void displayMapFrame();
  void displayRelief();
  void displayGrid();

  void displayBoxes();
  void displayBox(Box*);
  void displayUnits();
  void  idNearestUnit(float mx, float my);

 private:
  void initialize();

  friend class SimCanvas; // the mouse handler drives zoom/recenter

};

// ------------------------------------------

#endif

// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
