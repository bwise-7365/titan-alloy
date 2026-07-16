// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. See xtsim.h for the yr-to-Qt
// mapping. Changes beyond that mapping: the scenario chooser uses
// QComboBox::activated so re-selecting the current scenario still
// rebuilds it (the old pop-up choice fired on every selection);
// the mouse handler checks tgrid before dereferencing it (the
// blank pre-scenario sim has none, and the old code would have
// crashed there); lowerSimWindow clears the global theSim, which
// the old SimWindowLower left dangling.
// ------------------------------------------

#include "xtdemo.h"
#include "xtsim.h"
#include "cpanel.h"

#include "frwrdec.h"
#include "mat.h"
#include "runit.h"
#include "cunit.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QVBoxLayout>

ACPSim *theSim = NULL;

using AAA::imin;
using AAA::imax;
using AAA::expt;

using AAA::LTrue;
using AAA::LFalse;

// ----------------------------------

QColor SimGUIModule::defaultMapColor = GraphicManager::grey90Color;

// ----------------------------------
// the drawing canvas

SimCanvas::SimCanvas(int w, int h, QWidget* parent)
  : QWidget(parent), back(w, h) {
  module = NULL;
  painting = false;
  setFixedSize(w, h);
  back.fill(SimGUIModule::defaultMapColor);
}

void
SimCanvas::paintEvent(QPaintEvent*) {
  QPainter p(this);
  p.drawPixmap(0, 0, back);
}

void
SimCanvas::beginPaint() {
  if (false == painting) {
    painter.begin(&back);
    painting = true;
  }
}

void
SimCanvas::endPaint() {
  if (true == painting) {
    painter.end();
    painting = false;
  }
}

void
SimCanvas::clearBack(const QColor& c) {
  assert (false == painting); // QPixmap::fill under an open painter is illegal
  back.fill(c);
}

void
SimCanvas::drawColorLine(int x1, int y1, int x2, int y2, const QColor& fg) {
  if (true == painting) {
    painter.setPen(fg);
    painter.drawLine(x1, y1, x2, y2);
  }
  else {
    QPainter p(&back);
    p.setPen(fg);
    p.drawLine(x1, y1, x2, y2);
  }
}

void
SimCanvas::drawColorDot(int x, int y, int w, int h, const QColor& fg) {
  if (true == painting) {
    painter.setPen(fg);
    painter.setBrush(fg);
    painter.drawEllipse(x, y, w, h);
  }
  else {
    QPainter p(&back);
    p.setPen(fg);
    p.setBrush(fg);
    p.drawEllipse(x, y, w, h);
  }
}

void
SimCanvas::drawColorBox(int x, int y, int w, int h, const QColor& fg) {
  if (true == painting) {
    painter.fillRect(x, y, w, h, fg);
  }
  else {
    QPainter p(&back);
    p.fillRect(x, y, w, h, fg);
  }
}

// ----------------------------------
// the old handleDrawButtonPress, as a mouse handler:
// left = zoom in, right = zoom out, middle = recenter only,
// control-click = identify the nearest unit

void
SimCanvas::mousePressEvent(QMouseEvent* ev) {
  SimGUIModule* sgm = module;
  if (NULL == sgm)
    return;

  int dx = ((int) ev->position().x()); // draw window coords
  int dy = ((int) ev->position().y()); // draw window coords

  int maxScale = 25;
  float mx = 0.0;
  float my = 0.0;

  int w = 10;

  if ((NULL != sgm->mySim) && (NULL != sgm->mySim->tgrid)) {
    float maxX = sgm->mySim->tgrid->max_X;
    float maxY = sgm->mySim->tgrid->max_Y;

    sgm->windowToMap(dx, dy, mx, my);

    drawColorDot(dx-w/2, dy-w/2, w, w, GraphicManager::redColor);
    update();

    if (ev->modifiers() & Qt::ControlModifier) {
      cout << "ID the nearest unit" << endl;
    }
    else {
      switch (ev->button()) {
      case Qt::LeftButton:
	//	cout << "zoom in and recenter" << endl;
	sgm->innerSetScaleFactor( imin(maxScale, sgm->displayScale + 1));
	break;
      case Qt::MiddleButton:
	//	cout << "just recenter to " <<mx <<", "<<my<< endl;
	break;
      case Qt::RightButton:
	//	cout << "zoom out and recenter" << endl;
	sgm->innerSetScaleFactor( imax( 0, sgm->displayScale - 1));
	break;

      default:
	cout << "unrecognized button press ?!" << endl;
      }
      if (mx < 0.0)  mx = 0.0;
      if (mx > maxX) mx = maxX;
      if (my < 0.0)  my = 0.0;
      if (my > maxY) my = maxY;

      if (0 == sgm->displayScale) {
	mx = maxX / 2.0;
	my = maxY / 2.0;
      }

      sgm->centerX = mx;
      sgm->centerY = my;
      sgm->setAspectRatio();
      sgm->displaySim();

    }
  }
  return;
}

// ----------------------------------

void
XtTestApp::launchSimDemo() {
  if (NULL == xtTestApp->ymw5)
    xtTestApp->setupSimDemo();

  xtTestApp->ymw5->show();
  xtTestApp->ymw5->raise();

  return;
}

void
XtTestApp::setupSimDemo() {
  assert (NULL == ymw5);  // need to set it up
  assert (NULL == simGUImodule);


  int drawHeight = 450;

  ymw5 = new QWidget(this, Qt::Window);
  ymw5->setWindowTitle("Feldspar");
  QVBoxLayout* yrf = new QVBoxLayout(ymw5);

  // --------------------------------
  // title bar: label plus Dismiss
  QHBoxLayout* titleBar = new QHBoxLayout();
  yrf->addLayout(titleBar);
  titleBar->addWidget(new QLabel("ACP Sim", ymw5));

  QPushButton* dismissButton = new QPushButton("Dismiss", ymw5);
  dismissButton->setToolTip("Hide this simulation window");
  titleBar->addWidget(dismissButton);

  // --------------------------------
  // the scenario chooser. if there get to be too many scenarios,
  // it might make sense to go to a menu with submenus.
  QComboBox* scenarioCombo = new QComboBox(ymw5);
  scenarioCombo->addItem("NULL Scenario");
  scenarioCombo->addItem("One RU");
  scenarioCombo->addItem("One CU");
  scenarioCombo->addItem("Layered CU");
  scenarioCombo->addItem("Random Corr");
  scenarioCombo->addItem("Fix and Flank");
  scenarioCombo->setCurrentIndex(0); // have scenario NONE

  // --------------------------------
  // this is a totally generic discrete event simulation, with
  // no events scheduled

  ACPSim* sim = new ACPSim( ((int) ACPSim::RepeatableSeed) );

  theSim = sim;

  SimCanvas* drawArea
    = new SimCanvas(((int) (0.5 + (1.618 * drawHeight))), drawHeight, ymw5);
  drawArea->setToolTip("This is the main simulation display area.\nClick to resize and shift\nControl-click to ID units");
  yrf->addWidget(drawArea);

  QWidget* commandBar = new QWidget(ymw5);
  QHBoxLayout* commandBarLayout = new QHBoxLayout(commandBar);
  yrf->addWidget(commandBar);

  // the chooser is the first control on the command bar
  commandBarLayout->addWidget(scenarioCombo);

  // setup sim gui module
  simGUImodule = new SimGUIModule(sim, drawArea, commandBar);
  drawArea->module = simGUImodule;

  // activated (not currentIndexChanged) so that re-selecting the
  // same scenario rebuilds it, as the old choice control did
  QObject::connect(scenarioCombo, &QComboBox::activated,
                   [this](int s) { simGUImodule->innerSetupSim(s); });

  QObject::connect(dismissButton, &QPushButton::clicked,
                   [this]() {
                     simGUImodule->lowerSimWindow();
                     ymw5->hide();
                   });

  return;
}

// ----------------------------------

void
XtTestApp::launchControlPanel() {
  if (NULL == xtTestApp->ymw2)
    xtTestApp->setupControlPanel();

  xtTestApp->ymw2->show();
  xtTestApp->ymw2->raise();

  return;
}

void
XtTestApp::setupControlPanel() {
  ymw2 = new ControlPanel("Granite", this);
  ymw2->initialize();

  return;
}

// ----------------------------------


SimGUIModule::SimGUIModule() {
  initialize();

}


SimGUIModule::SimGUIModule(ACPSim* sim, SimCanvas* canvas, QWidget* cmdBar) {
  assert (NULL != sim);
  assert (NULL != canvas);
  assert (NULL != cmdBar);

  initialize();

  mySim = sim;
  drawArea = canvas;
  commandBar = cmdBar;


  float initialFreq = 3.0; // steps per second, initially
  float initialCount = 1.0; // events per step, initially

  QHBoxLayout* bar = ((QHBoxLayout*) cmdBar->layout());
  assert (NULL != bar);

  // this must be instantiated before the associated
  // run/stop and step controls are setup
  simTimer = new QTimer();
  simTimer->setInterval((int) (0.5 + (1000.0 / initialFreq)));
  QObject::connect(simTimer, &QTimer::timeout,
                   [this]() { innerStepSimulation(); });

  // add time / event pair
  QVBoxLayout* teFrame = new QVBoxLayout();
  bar->addLayout(teFrame);

  QHBoxLayout* tFrame = new QHBoxLayout();
  teFrame->addLayout(tFrame);
  tFrame->addWidget(new QLabel(" Time", cmdBar));
  timeOutputLabel = new QLabel("0000.0000", cmdBar);
  timeOutputLabel->setToolTip("Displays simulation time in seconds");
  tFrame->addWidget(timeOutputLabel);

  QHBoxLayout* eFrame = new QHBoxLayout();
  teFrame->addLayout(eFrame);
  eFrame->addWidget(new QLabel("Event", cmdBar));
  eventOutputLabel = new QLabel("0000.0000", cmdBar);
  eventOutputLabel->setToolTip("Displays count of events completed");
  eFrame->addWidget(eventOutputLabel);

  // add runstop / step pair
  QVBoxLayout* rssFrame = new QVBoxLayout();
  bar->addLayout(rssFrame);

  runStopButton = new QPushButton("Run", cmdBar);
  runStopButton->setCheckable(true);
  runStopButton->setToolTip("Run or stop the simulation");
  QObject::connect(runStopButton, &QPushButton::toggled,
                   [this](bool running) {
                     if (running) {
                       runStopButton->setText("Stop");
                       simTimer->start();
                     }
                     else {
                       runStopButton->setText("Run");
                       simTimer->stop();
                     }
                   });
  rssFrame->addWidget(runStopButton);

  stepButton = new QPushButton("Step", cmdBar);
  stepButton->setToolTip("Do exactly one step\nof the simulation");
  QObject::connect(stepButton, &QPushButton::clicked,
                   [this]() { innerStepSimulation(); });
  rssFrame->addWidget(stepButton);

  // initially, we have no scenario, so they start inactivated
  runStopButton->setEnabled(false);
  stepButton->setEnabled(false);


  // add steps/sec slider

  QVBoxLayout* stepSliderFrame = new QVBoxLayout();
  bar->addLayout(stepSliderFrame);

  stepSlider = new QSlider(Qt::Horizontal, cmdBar);
  stepSlider->setRange(1, 30);
  stepSlider->setValue((int) (0.5 + initialFreq));
  stepSlider->setToolTip("Drag this to control speed via simulation\nsteps per second");
  stepSliderFrame->addWidget(stepSlider);

  QHBoxLayout* stepSliderOutputFrame = new QHBoxLayout();
  stepSliderFrame->addLayout(stepSliderOutputFrame);
  stepSliderOutputFrame->addWidget(new QLabel("Steps/sec:", cmdBar));
  stepSliderValueLabel = new QLabel("---", cmdBar);
  stepSliderOutputFrame->addWidget(stepSliderValueLabel);

  QObject::connect(stepSlider, &QSlider::valueChanged,
                   [this](int v) { innerSetStepInterval(v); });
  innerSetStepInterval((int) (0.5 + initialFreq));

  // add events / step slider
  QVBoxLayout* countSliderFrame = new QVBoxLayout();
  bar->addLayout(countSliderFrame);

  countSlider = new QSlider(Qt::Horizontal, cmdBar);
  countSlider->setRange(1, 100);
  countSlider->setValue((int) (0.5 + initialCount));
  countSlider->setToolTip("Drag this to control speed via simulation\nevents per step");
  countSliderFrame->addWidget(countSlider);

  QHBoxLayout* countSliderOutputFrame = new QHBoxLayout();
  countSliderFrame->addLayout(countSliderOutputFrame);
  countSliderOutputFrame->addWidget(new QLabel("Events/step:", cmdBar));
  countSliderValueLabel = new QLabel("---", cmdBar);
  countSliderOutputFrame->addWidget(countSliderValueLabel);

  QObject::connect(countSlider, &QSlider::valueChanged,
                   [this](int v) { innerSetEventsPerStep(v); });
  innerSetEventsPerStep((int) (0.5 + initialCount));

  // add scale display
  QHBoxLayout* scaleOutputFrame = new QHBoxLayout();
  bar->addLayout(scaleOutputFrame);
  scaleOutputFrame->addWidget(new QLabel("Scale:", cmdBar));
  scaleValueLabel = new QLabel("---", cmdBar);
  scaleOutputFrame->addWidget(scaleValueLabel);

  innerSetScaleFactor(1);
  innerSetScaleFactor(0);

}

void
SimGUIModule::initialize() {
  mySim = NULL;
  simTimer = NULL;
  drawArea = NULL;
  commandBar = NULL;
  timeOutputLabel = NULL;
  eventOutputLabel = NULL;

  runStopButton = NULL;
  stepButton = NULL;

  stepSlider = NULL;
  stepSliderValueLabel = NULL;

  countSlider = NULL;
  countSliderValueLabel = NULL;

  scaleValueLabel = NULL;

  oldStepFreq = 0;
  oldEventsPerStep = 0;
  displayScale = 0;
  oldDisplayScale = 0;
  centerX = 0.0;
  centerY = 0.0;
  xAspect = 1.0;
  yAspect = 1.0;

  margin = 0.02; // 2% of screen width
  scaleFactor = 1.25;
  displayGridP = 0;
  displayReliefP = 0;
  displayBoxesP = 1;
  displayUnitsP = 1;
  displayUnitsLargeP = 0;
}

SimGUIModule::~SimGUIModule() {
  assert (NULL != simTimer);

  // be sure to stop it!
  if (NULL != mySim) {
    mySim->stop();
    delete mySim;
    mySim = NULL;
    theSim = NULL;
  }

  // be sure to stop it!
  simTimer->stop();
  delete simTimer;
  simTimer = NULL;

  // the labels, buttons, sliders and canvas are children of the
  // sim window, so Qt deletes them with it

}

void
SimGUIModule::innerSetupSim(int s) {
  cout <<endl<< "Resetting scenario to "<<s<<endl<<flush;
  if (NULL != mySim) {
    delete mySim;
    mySim = NULL;
  }

  innerSetScaleFactor(0);

  switch (s) {
  case 0:
    scenarioNull(); // run test functions, if desired
    assert (NULL == mySim);
    break;

  case 1:
    setupScenarioOneRU();
    assert (NULL != mySim);
    break;

  case 2:
    setupScenarioOneCU();
    assert (NULL != mySim);
    break;

  case 3:
    setupScenarioLayeredCU();
    assert (NULL != mySim);
    break;

  case 4:
    setupScenarioRandomCorr();
    assert (NULL != mySim);
    break;

  case 5:
    setupScenarioRecursiveCorr();
    assert (NULL != mySim);
    break;

  default:
    mySim = NULL;
    break; // end of defaultcase
  }

  if (NULL == mySim) {
    // disable run/stop/step stuff
    if (runStopButton->isChecked())
      runStopButton->setChecked(false); // stops the timer
    runStopButton->setEnabled(false);
    stepButton->setEnabled(false);

    // black out the sim window
    drawArea->clearBack(GraphicManager::blackColor);
    drawArea->update();

  }
  else {
    // enable run/stop/step stuff
    runStopButton->setEnabled(true);
    stepButton->setEnabled(true);

    setAspectRatio();
    displaySim();
  }

  theSim = mySim;
  return;
}


// i = steps per second
void
SimGUIModule::innerSetStepInterval(int i) {
  assert (i > 0);
  assert (NULL != stepSliderValueLabel);
  int nuInterval =  ((int) (0.5 + ( 1000.0 / i )));
  int nuFreq = i;

  // if integer value changed, update it
  if (nuFreq != oldStepFreq) {
    simTimer->setInterval(nuInterval);
    stepSliderValueLabel->setNum(nuFreq);
    oldStepFreq = nuFreq;
  }
  return;
}

void
SimGUIModule::innerSetEventsPerStep(int i) {
  assert (NULL != countSliderValueLabel);
  assert (i > 0);
  int nuEvents = i;
  // if integer value changed, update it
  if (nuEvents != oldEventsPerStep) {
    countSliderValueLabel->setNum(nuEvents);
    oldEventsPerStep = nuEvents;
  }
  return;
}

void
SimGUIModule::innerSetScaleFactor(int i) {
  assert (NULL != scaleValueLabel);
  assert (i >= 0);
  oldDisplayScale = displayScale;
  displayScale = i;
  if (displayScale != oldDisplayScale)
    scaleValueLabel->setNum(displayScale);

  return;
}

void
SimGUIModule::innerStepSimulation() {

  if (NULL != mySim) {
    mySim->stepN(oldEventsPerStep);

    displaySim();
    timeOutputLabel->setText(QString::number(mySim->clock(), 'f', 2));
    eventOutputLabel->setNum((int) mySim->eventNum());
  }

  else {
    cout << "There is no scenario; stepping will stop"<<endl<<flush;
    // be sure to stop the timer
    simTimer->stop();
    if (runStopButton->isChecked())
      runStopButton->setChecked(false);

  }

  return;
}

// the Dismiss semantics of the old SimWindowLower command
void
SimGUIModule::lowerSimWindow() {
  // be sure to stop the simTimer, if running
  if (runStopButton->isChecked())
    runStopButton->setChecked(false); // stops the timer

  if (NULL != mySim) {
    delete mySim;
    mySim = NULL;
    theSim = NULL;
  }

  runStopButton->setEnabled(false);
  stepButton->setEnabled(false);

  // black out the sim window
  drawArea->clearBack(GraphicManager::blackColor);
  drawArea->update();

  return;
}

void SimGUIModule::displaySim() {
  if (NULL != mySim) {
    drawArea->clearBack(GraphicManager::grey90Color);

    drawArea->beginPaint();
    displayRelief();
    displayGrid();
    displayMapFrame();
    displayBoxes();
    displayUnits();
    drawArea->endPaint();

    drawArea->update();
  }
  else {
    appBell();
    cout << "No sim to display" << endl << flush;
  }
  return;
}


void
SimGUIModule::setAspectRatio() {

  float correction;
  float max_X;
  float max_Y;
  int height = drawArea->getHeight();
  int width = drawArea->getWidth();

  xAspect = 1.0;
  yAspect = 1.0;

  if (NULL != mySim) {
    max_X = mySim->tgrid->max_X;
    max_Y = mySim->tgrid->max_Y;

    correction = (height * max_X) / (width * max_Y);

    if (correction > 1) {
      xAspect = 1.0;
      yAspect = 1.0 / correction;
    }

    if (correction < 1) {
      xAspect = correction;
      yAspect = 1.0;
    }
  }

  return;
}

void
SimGUIModule::displayMapFrame() {
  TGrid *tgrid;
  double max_X;
  double max_Y;

  int lowX, lowY, highX, highY;

  QColor bPixel = GraphicManager::blackColor;

  assert (NULL != mySim);
  tgrid = mySim->tgrid;
  if (NULL != tgrid) {
    max_X = tgrid->max_X;
    max_Y = tgrid->max_Y;

    mapToWindow(  0.0,   0.0,  lowX,  lowY);
    mapToWindow(max_X, max_Y, highX, highY);

    drawArea->drawColorLine( lowX,  lowY,  highX,  lowY,  bPixel);
    drawArea->drawColorLine( lowX,  highY, highX,  highY, bPixel);
    drawArea->drawColorLine( lowX,  lowY,  lowX,   highY, bPixel);
    drawArea->drawColorLine( highX, lowY,  highX,  highY, bPixel);

  }
  return;
}


void
SimGUIModule::displayRelief() {
  TGrid *tgrid = NULL;
  TCell *tc = NULL;
  int rows;
  int clms;
  double max_X;
  double max_Y;
  double min_Z;
  double max_Z;
  double tx = 0.0; // terrain x-coordinate at center of cell
  double ty = 0.0; // terrain y-coordinate at center of cell
  int i = 0;
  int j = 0;
  int lowX, lowY, highX, highY;
  int x1,y1,x2, y2;
  int w, h;
  double f = 0.0;


  // this will vary from white, to grey90 (nearly white), to grey20 (nearly black)
  QColor greyPixel = GraphicManager::grey50Color;

  if (displayReliefP) {

    assert (NULL != mySim);
    tgrid = mySim->tgrid;
    if  (NULL != tgrid) {
      max_X = tgrid->max_X;
      max_Y = tgrid->max_Y;
      min_Z = tgrid->min_Z;
      max_Z = tgrid->max_Z;
      rows = tgrid->rows;
      clms = tgrid->clms;

      mapToWindow(  0.0,   0.0,  lowX,  lowY);
      mapToWindow(max_X, max_Y, highX, highY);

      for (i=0 ; i<rows; i++) {
	ty = ((i+0.5)*max_Y) / rows;
	for (j=0 ; j<clms; j++) {
	  tx = ((j+0.5)*max_X) / clms;

	  tc = tgrid->getTCell(tx,ty);
	  assert (NULL != tc);

	  x1 = (((clms -   j   ) *lowX) +  (  j   * highX)) / clms;
	  y1 = (((rows -   i   ) *lowY) +  (  i   * highY)) / rows;
	  x2 = (((clms - (j+1) ) *lowX) +  ((j+1) * highX)) / clms;
	  y2 = (((rows - (i+1) ) *lowY) +  ((i+1) * highY)) / rows;
	  w = x2 - x1;
	  h = y2 - y1;

	  // f will be 0.0 for the lowest cell, and 9.0 for the highest
	  f = 9.0 * (tc->height - min_Z)/(max_Z - min_Z);
	  if ( f < 1.0)
	    greyPixel = GraphicManager::grey20Color;
	  else if (f < 2.0)
	    greyPixel = GraphicManager::grey30Color;
	  else if (f < 3.0)
	    greyPixel = GraphicManager::grey40Color;
	  else if (f < 4.0)
	    greyPixel = GraphicManager::grey50Color;
	  else if (f < 5.0)
	    greyPixel = GraphicManager::grey60Color;
	  else if (f < 6.0)
	    greyPixel = GraphicManager::grey70Color;
	  else if (f < 7.0)
	    greyPixel = GraphicManager::grey80Color;
	  else if (f < 8.0)
	    greyPixel = GraphicManager::grey90Color;
	  else
	    greyPixel = GraphicManager::whiteColor;

	  drawArea->drawColorBox(x1, y1, w, h, greyPixel);

	}
      }
    }
  }

  return;
}

void
SimGUIModule::displayGrid() {
  TGrid *tgrid;
  int rows;
  int clms;
  double max_X;
  double max_Y;
  int i;
  int lowX, lowY, highX, highY;
  int x,y;

  QColor tPixel = GraphicManager::tanColor;

  if (displayGridP) {
    assert (NULL != mySim);
    tgrid = mySim->tgrid;
    if  (NULL != tgrid) {
      max_X = tgrid->max_X;
      max_Y = tgrid->max_Y;
      rows = tgrid->rows;
      clms = tgrid->clms;

      mapToWindow(  0.0,   0.0,  lowX,  lowY);
      mapToWindow(max_X, max_Y, highX, highY);

      for (i=0 ; i<=rows; i++) {
	y = (((rows - i ) *lowY) +  (i * highY)) / rows;
	drawArea->drawColorLine( lowX,  y,
				 highX, y,
				 tPixel);
      }

      for (i=0 ; i<=clms; i++) {
	x = (((clms - i ) *lowX) +  (i * highX)) / clms;
	drawArea->drawColorLine( x,  lowY,
				 x, highY,
				 tPixel);
      }
    }
  }
  return;
}


// ----------------------------------
// notice that we FOLLOW the X-convention:
// x-coord increases from left to right,
// y-coord increases from top to bottom
void
SimGUIModule::mapToWindow(float mx, float my, int &wx, int &wy) {

  TGrid *tgrid = mySim->tgrid;
  double maxX = tgrid->max_X;
  double maxY = tgrid->max_Y;
  double maxW = drawArea->getWidth(); // in pixels
  double maxH = drawArea->getHeight(); // in pixels
  double centerW = maxW / 2.0;
  double centerH = maxH / 2.0;

  double scale = expt (scaleFactor, displayScale);

  // this is the actual mapping
  wx = ((int) (0.5 + centerW + ((1.0 - margin) * (mx - centerX) * maxW * scale* xAspect) / (maxX)));
  wy = ((int) (0.5 + centerH + ((1.0 - margin) * (my - centerY) * maxH * scale * yAspect) / (maxY)));

  return;
}


void
SimGUIModule::windowToMap(int wx, int wy, float &mx, float &my) {

  TGrid *tgrid = mySim->tgrid;
  double maxX = tgrid->max_X;
  double maxY = tgrid->max_Y;
  double maxW = drawArea->getWidth(); // in pixels
  double maxH = drawArea->getHeight(); // in pixels
  double centerW = maxW / 2.0;
  double centerH = maxH / 2.0;

  double scale = expt (scaleFactor, displayScale);

  // this is the inverse of the mapping above

  mx = centerX + (((wx - centerW) * maxX) / ((1.0 - margin) * maxW * scale * xAspect));
  my = centerY + (((wy - centerH) * maxY) / ((1.0 - margin) * maxH * scale * yAspect));

  return;
}


// ----------------------------------

void
SimGUIModule::displayBoxes() {

  Box* bx1 = NULL;
  unsigned long int i = 0;
  unsigned long int numB = mySim->boxes->size();

  for (i=0; i<numB; i++) {
    bx1 = (*(mySim->boxes))[i];
    displayBox(bx1);
  }

  return;
}

void
SimGUIModule::displayBox(Box* bx1) {
  assert (NULL != bx1);
  QColor cPixel = GraphicManager::cyanColor;

  double ax = 0.0;
  double ay = 0.0;
  double bx = 0.0;
  double by = 0.0;
  double cx = 0.0;
  double cy = 0.0;
  double dx = 0.0;
  double dy = 0.0;

  int wx1 = 0;
  int wy1 = 0;
  int wx2 = 0;
  int wy2 = 0;

    ax = bx1->get_A().get(0);
    ay = bx1->get_A().get(1);

    bx = bx1->get_B().get(0);
    by = bx1->get_B().get(1);

    cx = bx1->get_C().get(0);
    cy = bx1->get_C().get(1);

    dx = bx1->get_D().get(0);
    dy = bx1->get_D().get(1);

    mapToWindow(ax, ay, wx1, wy1);
    mapToWindow(bx, by, wx2, wy2);
    drawArea->drawColorLine( wx1,  wy1,  wx2,  wy2,  cPixel);

    mapToWindow(bx, by, wx1, wy1);
    mapToWindow(cx, cy, wx2, wy2);
    drawArea->drawColorLine( wx1,  wy1,  wx2,  wy2,  cPixel);

    mapToWindow(cx, cy, wx1, wy1);
    mapToWindow(dx, dy, wx2, wy2);
    drawArea->drawColorLine( wx1,  wy1,  wx2,  wy2,  cPixel);

    mapToWindow(dx, dy, wx1, wy1);
    mapToWindow(ax, ay, wx2, wy2);
    drawArea->drawColorLine( wx1,  wy1,  wx2,  wy2,  cPixel);

  return;
}


void
SimGUIModule::displayUnits() {
  double unitMapX = 0.0;
  double unitMapY = 0.0;
  int unitWindowX = 0;
  int unitWindowY = 0;

  int dotWS = 5;
  int dotHS = 5;
  int dotWL = 11;
  int dotHL = 11;
  int offSet = (dotWL - dotWS)/2;
  ResUnit* ru = NULL;
  CmndUnit* cu = NULL;
  AAA::GVector cp;
  unsigned long int i = 0;
  unsigned long int numRU = mySim->rUnits->size();
  unsigned long int numCU = mySim->cUnits->size();
  QColor bPixel = GraphicManager::blackColor;
  QColor sPixel = GraphicManager::blackColor;


  for (i=0; i<numRU; i++) {
    ru = (*(mySim->rUnits))[i];
    assert (ru != NULL);
    cp = ru->currentPos();
    unitMapX = cp.get(0);
    unitMapY = cp.get(1);
    mapToWindow(unitMapX, unitMapY, unitWindowX, unitWindowY);

    switch (ru->side) {
    case BlueSide:
      sPixel =  GraphicManager::blueColor;
      break;
    case RedSide:
      sPixel = GraphicManager::redColor;
      break;
    case GreenSide:
      sPixel = GraphicManager::greenColor;
      break;
    case OrangeSide:
      sPixel = GraphicManager::orangeColor;
      break;

    case PurpleSide:
      sPixel = GraphicManager::purpleColor;
      break;
    case BlackSide:
      // NOT a good choice!
      sPixel = GraphicManager::blackColor;
      break;
    case WhiteSide:
      // NOT a good choice!
      sPixel = GraphicManager::whiteColor;
      break;
    case GreySide:
      sPixel = GraphicManager::greyColor;
      break;
    case GoldSide:
      sPixel = GraphicManager::goldColor;
      break;
    case CrimsonSide:
      sPixel = GraphicManager::crimsonColor;
      break;

    default:
      sPixel = GraphicManager::blackColor;
      break;
    }


    if (displayUnitsLargeP) {
    // draw a large colored dot
    drawArea->drawColorDot(unitWindowX-offSet, unitWindowY-offSet,
			   dotWL,  dotHL,
			   sPixel);
    if (LFalse == ru->aliveP)
      drawArea->drawColorDot(unitWindowX, unitWindowY,
			     dotWS,  dotHS,
			     bPixel);

    }
    else {
    // draw a simple little colored dot
      if (LTrue == ru->aliveP)
	drawArea->drawColorDot(unitWindowX, unitWindowY,
			       dotWS,  dotHS,
			       sPixel);
      else
	drawArea->drawColorDot(unitWindowX, unitWindowY,
			       dotWS,  dotHS,
			       bPixel);

    }


  } // end of loop over ResUnits
  ru = NULL;

  for (i=0; i<numCU; i++) {
    cu = (*(mySim->cUnits))[i];
    assert (cu != NULL);
    cp = cu->currentPos();
    unitMapX = cp.get(0);
    unitMapY = cp.get(1);
    mapToWindow(unitMapX, unitMapY, unitWindowX, unitWindowY);

    bPixel = GraphicManager::blackColor;
    if (LTrue == cu->aliveP) {
      switch (cu->side) {
      case BlueSide:
	bPixel =  GraphicManager::blueColor;
	break;
      case RedSide:
	bPixel = GraphicManager::redColor;
	break;
      case GreenSide:
	bPixel = GraphicManager::greenColor;
	break;
      case OrangeSide:
	bPixel = GraphicManager::orangeColor;
	break;

      case PurpleSide:
	bPixel = GraphicManager::purpleColor;
	break;
      case BlackSide:
	// NOT a good choice!
	bPixel = GraphicManager::blackColor;
	break;
      case WhiteSide:
	// NOT a good choice!
	bPixel = GraphicManager::whiteColor;
	break;
      case GreySide:
	bPixel = GraphicManager::greyColor;
	break;
      case GoldSide:
	bPixel = GraphicManager::goldColor;
	break;
      case CrimsonSide:
	bPixel = GraphicManager::crimsonColor;
	break;

      default:
	bPixel = GraphicManager::blackColor;
	break;
      }
    }



    drawArea->drawColorDot(unitWindowX, unitWindowY,
			   dotWS,  dotHS,
			   bPixel);

  } // end of loop over CmndUnits
  cu = NULL;

  return;
}


// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
