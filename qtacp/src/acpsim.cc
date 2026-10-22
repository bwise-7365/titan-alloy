// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. See acpsim.h for the DESim
// adaptation rules (event ownership, lazy cancellation, the
// two-argument schedule).
// ------------------------------------------

#include "struct.h"
#include "components.h"
#include "acpsim.h"
#include "unit.h"

#include "runit.h"
#include "cunit.h"

#include <vector>
#include <algorithm>

extern ACPSim* theSim;

using std::vector;

// ------------------------------------------

bool ACPSim::traceOrders = false;
bool ACPSim::traceMoves = false;
bool ACPSim::traceShots = false;
bool ACPSim::tracePlanning = false;
bool ACPSim::traceGeometry = false;
bool ACPSim::traceFSM = false;
bool ACPSim::traceSensors = false;

// for debugging, the initial default is 'true'
// you can always change it at the GUI
bool ACPSim::RepeatableSeedP = true;

//unsigned long int ACPSim::RepeatableSeed = 93071;

unsigned long int ACPSim::RepeatableSeed = 17306;

// ------------------------------------------

ACPSim::ACPSim() : Simulation::DESim(panj::PRNG::msRandom()) {
  cout << "Creating blank ACPSim with random seed" << endl << flush;
  initialize();
  cout << "Created and initialized blank ACPSim" << endl << flush;
}

ACPSim::ACPSim(int s) : Simulation::DESim((uint64_t) s) {
  cout << "Creating blank ACPSim with fixed seed " << s  << endl << flush;
  initialize();
  cout << "Created and initialized blank ACPSim" << endl << flush;
}

void
ACPSim::initialize()  {
  rng = getPRNG();
  eventsProcessed = 0;

  tgrid = NULL;
  rUnits = new vector<ResUnit*>();
  tempCorridors  = new vector<TempCorridor*>();
  boxes  = new vector<Box*>();
  cUnits = new vector<CmndUnit*>();
  sensors = new vector<Sensor*>();
  jammers = new vector<Jammer*>();

  assert (NULL != rng);
  assert (NULL != rUnits);
  assert (NULL != tempCorridors);
  assert (NULL != boxes);
  assert (NULL != cUnits);
  assert (NULL != sensors);
  assert (NULL != jammers);

  theSim = this;
}

ACPSim::~ACPSim() {
  cout << "Deleting entire ACPSim" << endl;
  Box* bx = NULL;
  ResUnit* ru = NULL;
  CmndUnit* cu = NULL;
  TempCorridor* tc = NULL;

  delete tgrid;
  tgrid = NULL;

  delete sensors;
  sensors = NULL;

  delete jammers;
  jammers = NULL;

  cout << "There are " << rUnits->size() << " ResUnits " << endl << flush;
  while (rUnits->size() > 0) {
    ru = rUnits->back();
    rUnits->pop_back();
    delete ru;
    ru = NULL;
  }
  delete rUnits;
  rUnits = NULL;

  while (boxes->size() > 0) {
    bx = boxes->back();
    boxes->pop_back();
    delete bx;
    bx = NULL;
  }
  delete boxes;
  boxes = NULL;

  cout << "There are " << cUnits->size() << " CmndUnits " << endl << flush;
  while (cUnits->size() > 0) {
    cu = cUnits->back();
    cUnits->pop_back();
    delete cu;
    cu = NULL;
  }

  delete cUnits;
  cUnits = NULL;

  while (tempCorridors->size() > 0) {
    tc = tempCorridors->back();
    tempCorridors->pop_back();
    delete tc;
    tc = NULL;
  }
  delete tempCorridors;
  tempCorridors = NULL;

  cout << "Done deleting entire ACPSim" << endl << flush;
}

// ------------------------------------------
// the engine-stepping adapter (DESim keeps step() protected)

bool
ACPSim::eventsPending() const {
  return ((NULL != queue) && (false == queue->isNull()));
}

void
ACPSim::stepOnce() {
  if (false == eventsPending())
    return;
  step();
  eventsProcessed = eventsProcessed + 1;
  return;
}

void
ACPSim::stepN(int n) {
  int i = 0;
  for (i=0; i<n; i++) {
    if (false == eventsPending())
      break;
    stepOnce();
  }
  return;
}

void
ACPSim::schedule(double t, ACPSimEvent* ev) {
  assert (NULL != ev);
  if (t < getCurrTime()) {
    // the old engine tolerated stale times; DESim throws on them
    cout << "ACPSim::schedule clamped past time " << t
         << " to current time " << getCurrTime() << endl;
    t = getCurrTime();
  }
  ev->setSchedTime(t);
  Simulation::DESim::schedule(ev);
  return;
}

// ------------------------------------------


ACPSimEvent::ACPSimEvent(ACPSim* s, ACPSimEventType t)
  : Simulation::Event(s, s->getCurrTime()) {
  mySim = s;
  initialize();
  type = t;
};

ACPSimEvent::~ACPSimEvent() {
}

void
ACPSimEvent::initialize() {
  type = SSNullEvent;
  processFN = NULL;
  data = NULL;
  owner = NULL;
  cancelled = false;
  return;
}

void
ACPSimEvent::processEvent() {
  ResUnit* ru = NULL;
  CmndUnit* cu = NULL;
  double now = mySim->clock();

#ifndef NDEBUG
  // debugging tripwire (2026-07-16 access violation in Layered CU):
  // a unit-update event must reference a unit still registered with
  // the current simulation. this fires a clean assert at the FIRST
  // stale event, instead of an access violation inside update().
  if (((SSStateUpdate == type) || (SSCmndUnitUpdate == type))
      && (false == cancelled)) {
    assert (mySim == theSim);
    assert (NULL != data);
    if (SSStateUpdate == type) {
      assert (mySim->rUnits->end()
              != std::find(mySim->rUnits->begin(), mySim->rUnits->end(),
                           ((ResUnit*) data)));
    }
    else {
      assert (mySim->cUnits->end()
              != std::find(mySim->cUnits->begin(), mySim->cUnits->end(),
                           ((CmndUnit*) data)));
    }
  }
#endif

  // clear the owner's back-pointer before dispatch, so that an
  // update() scheduling a fresh event is not clobbered afterward
  if ((NULL != owner) && (this == owner->nextEvent)) {
    owner->nextEvent = NULL;
  }
  owner = NULL;

  // a cancelled event drains through the queue as a no-op;
  // the engine deletes it after this returns
  if (true == cancelled)
    return;

  switch (type) {
  case SSNullEvent:
    break;

  case SSSensorScan:
    assert (NULL != processFN);
    assert (NULL != data);
    if (true == ACPSim::traceSensors)
      cout << "Doing SSSensorScan at time " << now << endl << flush;
    processFN(data);
    break;

  case SSDetonation:
    assert (NULL != processFN);
    assert (NULL != data);

    if (true == ACPSim::traceShots)
      cout << "Doing SSDetonation at time " << now << endl << flush;

    processFN(data);
    break;

  case SSStateUpdate:
    assert (NULL != processFN); // but it is not used here
    assert (NULL != data);

    ru = ((ResUnit*) data);
    if (true == ACPSim::traceMoves) {
      cout << "Doing SSStateUpdate at time " << now;
      cout << " for "<<ru->side<<" ResUnit " << ru->getSimEntID();
      cout << endl << flush;
    }

    ru->update();
    ru = NULL;
    break;

  case SSCmndUnitUpdate:
    assert (NULL != processFN); // but it is not used here
    assert (NULL != data);
    cu = ((CmndUnit*) data);

    if (true == ACPSim::tracePlanning) {
      cout << "Doing SSCmndUnitUpdate at time " << now;
      cout << " for "<<cu->side<<" CmndUnit " << cu->getSimEntID();
      cout << endl << flush;
    }

    cu->update();
    cu = NULL;
    break;
  };

  return;
}



void
ACPSim::addResUnit(ResUnit* newRU) {
  assert (NULL != newRU);
  // do not double-add them
  assert (rUnits->end() == std::find(rUnits->begin(), rUnits->end(), newRU));
  rUnits->push_back(newRU);
  return;
}

void
ACPSim::addCmndUnit(CmndUnit* newCU) {
  assert (NULL != newCU);
  // do not double-add them
  assert (cUnits->end() == std::find(cUnits->begin(), cUnits->end(), newCU));
  cUnits->push_back(newCU);
  return;
}



void
ACPSim::removeResUnit(ResUnit* old_unit) {
  // how is this done now?
  return;
}

void
ACPSim::removeCmndUnit(CmndUnit* old_unit){
  // how is this done now?
  return;
}


// Euclidean distance between two vectors
double dist(AAA::GVector a, AAA::GVector b) {
  unsigned int i = 0;
  unsigned int n = a.getDim();
  assert (n == b.getDim());
  double rslt = 0.0;
  double dc = 0.0;
  for (i=0; i<n; i++) {
    dc = a.get(i) - b.get(i);
    rslt = rslt + (dc*dc);
  }

  return sqrt(rslt);
}

double ballisticFlightTime(double range) {
  return sqrt( (2.0 * range) / earthG);
}


double minBallisticTime(AAA::GVector pFrom, AAA::GVector pTo) {
const double xFrom = pFrom.get(0);
  const double yFrom = pFrom.get(1);
  const double zFrom = pFrom.get(2);

  const double xTo = pTo.get(0);
  const double yTo = pTo.get(1);
  const double zTo = pTo.get(2);

  const double dx = xTo - xFrom;
  const double dy = yTo - yFrom;
  const double dz = zTo - zFrom;

  const double dr2 = ( dx*dx  + dy*dy );
  const double dz2 = dz*dz;

  const double t4 = (4 * ( dr2  + dz2 )) / (earthG*earthG);

  return sqrt(sqrt(t4));

}


// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
