// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. See unit.h for the changes.
// ------------------------------------------

#include "unit.h"
#include "cunit.h"
#include "components.h"
#include "tdv.h"
#include "mcontrol.h"
#include "subsa.h"

#include <vector>

// ------------------------------------------

extern ACPSim* theSim;

using AAA::Logical;
using AAA::LTrue;
using AAA::LFalse;
using AAA::LUnknown;

using AAA::GVector;
using AAA::FSM;

using std::vector;

// ------------------------------------------

Unit::Unit(ACPSim* s) : Simulation::Entity(s) {
  sim = s;
  simEntID = getID();
  initialize();
  assert (NULL != sim);
}

Unit::~Unit() {
  // this cleans up the FSM, but it may still leave behind a bunch
  // of shared bits of data (e.g. planning routes) that no one
  // destructor can get, because they were shared
  if (NULL != current_fsm) {
    delete current_fsm;
    current_fsm = NULL;
  }

  delete ssa;
  ssa = NULL;

  if (NULL != moveController) {
    delete moveController;
    moveController = NULL;
  }

  // a pending event stays queued (the engine owns it); make sure
  // it does not fire on, or point back to, a deleted unit
  descheduleNextEvent();
}

void
Unit::initialize() {
  // for general units, CmndUnit or ResUnit,
  // these values are unknown. So I give them
  // values that are sure to fail 'assert' later
  // unless properly initialized
  maxStepDist = 0.0; // meters maximum step
  maxUpdateInterval = 0.0;  // seconds
  posTolerance = 0.0;
  maxSpeed = 0.0;
  sensorRange = 0.0;
  weaponRange = 0.0;

  moveController = NULL;

  ssa = new SubSA1(this);

  aliveP = LTrue;
  allowSubPlanning = LTrue;
  allowManeuverPlanning = LFalse; // test old stuff first!
  superior = NULL;
  brokenP = LFalse;
  p0 = GVector(0,0,0);
  v0 = GVector(0,0,0);
  t0  = 0.0;
  birthTime = 0.0;
  deathTime = 0.0;
  side = GreySide; // unknown
  assert (NULL != sim);
  tgrid = NULL;
  current_fsm = NULL;
  nextEvent = NULL;
}


CmndUnit* Unit::nthSuperior(unsigned long int n) {
  CmndUnit* cu = NULL;

  if (0 == n)
    cu = NULL;
  else if (1 == n) {
    cu = superior; // NULL or not, that is the one
      }
  else { // n > 1
    if (NULL == superior)
      cu = NULL;
    else
      cu = superior->nthSuperior(n-1);
    }
  return cu;
}


void
Unit::reportPV() {
  double t = sim->clock();
  GVector p = currentPos();
  GVector v = currentVel();
  if (true == ACPSim::traceMoves) {
    cout << "Unit " << simEntID << " is at pos" << p ;
    cout << " moving " << v;
    cout << " at time " << t << endl << flush;
  }
  return;
}



Logical
Unit::centerInAreaP(Box* b)
{
    GVector my_pos;
    Logical rslt;
    my_pos = this->currentPos();
    rslt = b->insideP(my_pos);
    return rslt;
}


void
Unit::enemyStrengthInArea(Box* b,
		      float& strength,
		      GVector& center_of_gravity) {
  vector<ResUnit*> *units_in_box = NULL;
  vector<ResUnit*> *units_opposed = new vector<ResUnit*>();

  ResUnit *u;
  vector<GVector*>*  unit_ctrs = new vector<GVector*>();
  vector<float>* unit_wght = new vector<float>();


  GVector pt = GVector(3);
  GVector mean_u = GVector(3);
  GVector *gvptr = NULL;

  float opp_strength, wt;


  assert (NULL != unit_ctrs);
  assert (NULL != unit_wght);
  assert (NULL != units_opposed);

  unsigned int i = 0;
  unsigned int numUnits = 0;

  opp_strength = 0.0;
  mean_u = GVector(0,0,0);

  if (b != NULL) {
    units_in_box = theSim->tgrid->units_in_area(b);
    numUnits = units_in_box->size();
    if (numUnits > 0) {
      for (i=0; i<numUnits; i++) {
	u = (*units_in_box)[i];
	if ((LTrue == u->aliveP) && (opposedByP(u->side, this->side) == LTrue)) {
	  units_opposed->push_back(u);
	  wt = u->currentStrength();
	  opp_strength = opp_strength + wt;
	  gvptr = new GVector(u->currentPos());
	  unit_ctrs->push_back(gvptr);
	  unit_wght->push_back(wt);
	}
	else {
	  // found friendly, neutral or dead unit
	}
      }
    }
    if (opp_strength > 0) {
      mean_u = weighted_mean_pt(unit_ctrs, unit_wght);
    }
  }

  strength = opp_strength;       // possibly zero
  center_of_gravity = mean_u;    // possibly a meaningless (0,0)!

  // -  -  -  -  -  -  -  -  -  -  -  -  -
  // clean up
  // delete list, but not contents!
  delete units_in_box;
  units_in_box = NULL;
  delete units_opposed;
  units_opposed = NULL;

  // delete lists and contents
  while (unit_ctrs->size() > 0) {
    gvptr = unit_ctrs->back();
    unit_ctrs->pop_back();
    delete gvptr;
    gvptr = NULL;
  }
  delete unit_ctrs;
  unit_ctrs = NULL;

  delete unit_wght;
  unit_wght = NULL;

  return;
}


void
Unit::strengthsInArea(double x0, double y0, double x1, double y1, float& fS, float& eS) {
  fS = 0.0;
  eS = 0.0;
  vector<ResUnit*>* found = tgrid->units_in_ranges(x0, y0, x1, y1);
  unsigned long int i = 0;
  unsigned long int n = found->size();
  double wt = 0.0;
  ResUnit* u = NULL;

  for (i=0; i<n; i++) {
    u = (*found)[i];
    wt = u->currentStrength();
    if (LTrue == u->aliveP) {
      if (side == u->side)
	fS = fS + wt;
      else if (opposedByP(u->side, this->side) == LTrue)
	eS = eS + wt;
    }
  }

  delete found;
  found = NULL;

  return;
}


void
Unit::friendlyStrengthInArea(Box* b,
		      float& strength,
		      GVector& center_of_gravity) {
  assert (NULL != b);
  ResUnit *u = NULL;
  vector<ResUnit*>  *units_allied = friendlyRUInArea(b);
  vector<GVector*> *unit_ctrs = new vector<GVector*>();
  vector<float> *unit_wght = new vector<float>();


  GVector* gvptr = NULL;
  GVector pt = GVector(3);
  GVector mean_u = GVector(3);
  float opp_strength = 0.0;
  float wt = 0.0;



  assert (NULL != unit_ctrs);
  assert (NULL != unit_wght);
  assert (NULL != units_allied);

  unsigned int i = 0;
  unsigned int numUnits = units_allied->size();

  opp_strength = 0.0;
  mean_u = GVector(0,0,0);

  for (i=0; i<numUnits; i++) {
    u = (*units_allied)[i];
    wt = u->currentStrength();
    opp_strength = opp_strength + wt;
    gvptr = new GVector(u->currentPos());
    unit_ctrs->push_back(gvptr);
    unit_wght->push_back(wt);
    }

    if (opp_strength > 0) {
      mean_u = weighted_mean_pt(unit_ctrs, unit_wght);
    }

  strength = opp_strength;       // possibly zero
  center_of_gravity = mean_u;    // possibly a meaningless (0,0)!

  // -  -  -  -  -  -  -  -  -  -  -  -  -
  // clean up
  // delete list, but not contents!
  delete units_allied;
  units_allied = NULL;

  // delete lists and contents
  while (unit_ctrs->size() > 0) {
    gvptr = unit_ctrs->back();
    unit_ctrs->pop_back();
    delete gvptr;
    gvptr = NULL;
  }
  delete unit_ctrs;
  unit_ctrs = NULL;

  delete unit_wght;
  unit_wght = NULL;

  // -  -  -  -  -  -  -  -  -  -  -  -  -

  return;
}

vector<ResUnit*>*
Unit::friendlyRUInArea(Box* b) {
  assert (NULL != b);
  ResUnit *u = NULL;

  vector<ResUnit*>  *units_in_box = NULL;
  vector<ResUnit*>  *units_allied = new vector<ResUnit*>();

  assert (NULL != units_allied);

  unsigned int i = 0;
  unsigned int numUnits = 0;

  if (b != NULL) {
    units_in_box = theSim->tgrid->units_in_area(b);
    numUnits = units_in_box->size();
    if (numUnits > 0) {
      for (i=0; i<numUnits; i++) {
	u = (*units_in_box)[i];
	if ((LTrue == u->aliveP) && (u->side == this->side)) {
	  units_allied->push_back(u);
	}
	else {
	  // found enemy, neutral or dead unit
	}
      }
    }
  }

  // delete list, but not contents!
  if (NULL != units_in_box) {
    delete units_in_box;
    units_in_box = NULL;
  }
 return units_allied;
}

void
Unit::die() {
  assert (LFalse != aliveP); // you can not die twice
  aliveP = LFalse;

  // remove self from superior's subordinate list, if any
  if (NULL != superior) {
    superior->remove_sub(this);
    assert (0 == superior->hasSubP(this));
    // if the superior now has Zero living subs
    // (not even his own command vehicle!), then
    // the superior command unit is also dead.
    if (0 == superior->numLiveSubs()) {
      cout << "CmndUnit " << superior->simEntID <<" just lost last subordinate"<<endl;
      cout << "CmndUnit, side="<<superior->side <<", gone at time "<<sim->clock()<<endl;
      cout << flush;
      // XXXX just a hook for placing breakpoints
      cout << flush;
      superior->die();
    }
  }
  return;
}

void
Unit::setFSM(FSM *fsm) {
  if (1 == FSM::debugFSM) {
    if (NULL == fsm)
      cout << "Unit " << simEntID << " sets fsm to NULL" << endl << flush;
    else
      cout << "Unit " << simEntID << " sets fsm to " << fsm->getID() << endl << flush;
  }

  if (current_fsm != NULL) {
    delete current_fsm;
    current_fsm = NULL;
  }

  current_fsm = fsm;
  return;
}

void
Unit::scheduleNextEvent(double scheduled_time, ACPSimEvent *event) {
  assert (NULL != event);
  event->owner = this;
  nextEvent = event;
  sim->schedule(scheduled_time, event);
  return;
}

void
Unit::descheduleNextEvent() {
  // lazy cancellation: the event stays queued and the engine
  // deletes it at its scheduled time; processing is a no-op
  if (NULL != nextEvent) {
    nextEvent->owner = NULL;
    nextEvent->cancel();
    nextEvent = NULL;
  }
  return;
}


// ------------------------------------------
// I've arbitrarily hard coded in the following structure of opposition:
// Blue  <--> Red (of course)
// Blue  <--> Purple
// Red   <--> Purple
// Green <--> Orange
// so that
// Red, Blue, and Purple all fight each other
// Green and Orange fight each other
// The first three don't attack the last
// two, and vice versa.
// No one attacks the Whites.
// NOTICE that this need not by commutative!
// It is entirely possible to say "UN" opposes no one,
// but (e.g.) many sides oppose "UN"!

// would a2 want to attack a1? (not vice versa)
Logical
opposedByP(Alignment a1, Alignment a2) {
  Logical result = LFalse;
  switch (a1) {
    case BlueSide:
      if ((a2 == RedSide) || (a2 == PurpleSide))
	result = LTrue;
      break;
    case RedSide:
      if ((a2 == BlueSide) || (a2 == PurpleSide))
	result = LTrue;
      break;
    case PurpleSide:
      if ((a2 == RedSide) || (a2 == BlueSide))
	result = LTrue;
      break;
    case OrangeSide:
      if (a2 == GreenSide)
	result = LTrue;
      break;
    case GreenSide:
      if (a2 == OrangeSide)
	result = LTrue;
      break;
    case WhiteSide:
      result = LFalse; // no one opposed to White
      break;
    case GreySide:
      result = LUnknown; // Grey represents unknown
      break;
    default:
      cout << "opposed  Unrecognized Alignment: " << a1<< endl;
      break;
    }
  return result;
}



void
updatePV(void* ruPtr) { // really, a ResUnit*
  ResUnit* ru = ((ResUnit*) ruPtr);
  if (LTrue == ru->aliveP)
    ru->update(); // potentially does a bunch of stuff
  return;
}



GVector
perturbGVector(GVector p0, double amount, panj::PRNG* rng) { // in meters
  double nx, ny, nz;
  GVector p1 = p0;
  if (amount > 0.10) // if more than 10 cm
    {
      nx = rng->uniform(-amount, amount);
      ny = rng->uniform(-amount, amount);
      nz = rng->uniform(-amount, amount);
      p1 = p0 + GVector(nx, ny, nz);
    }
  return p1;
}
// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
