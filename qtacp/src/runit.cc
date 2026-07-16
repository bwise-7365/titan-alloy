// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: include list, evector -> std::vector.
// ------------------------------------------

#include "aaa.h"
#include "fsm.h"
#include "runit.h"
#include "tthread.h"
#include "orders.h"

#include "acpsim.h"
#include "mcontrol.h"
#include "subsa.h"

#include <vector>

extern ACPSim* theSim;

using AAA::Logical;
using AAA::LTrue;
using AAA::LFalse;
using AAA::LUnknown;

using AAA::GVector;
using std::vector;

using AAA::FSM;
using AAA::State;
using AAA::Predicate;
using AAA::Action;
using AAA::Conjunction;
using AAA::AlwaysTrue;

using AAA::dmin;

// ------------------------------------------




ResUnit::ResUnit(ACPSim* sm, Alignment a, GVector initPos, GVector initVel)
  : Unit(sm) {
  if (true == ACPSim::traceMoves)
    cout <<endl << "trying to create res unit " << simEntID << endl;

  assert (NULL != theSim); 
  assert (NULL != sm);

  tgrid = theSim->tgrid;
  initialize();   // this need tgrid and sim to be set correctly
  side = a;
  theSim->addResUnit( this);

  // be careful to create it at the correct time,
  // or reset it.
  birthTime = sim->clock();

  if (true == ACPSim::traceMoves)
    cout<< "Instantiating ResUnit " << simEntID << " at " << initPos << endl;

  assert(NULL != tgrid);
  resetPV(initPos, initVel);
}

ResUnit::~ResUnit()
{
  if (NULL != moveController) {
    delete moveController;
    moveController = NULL;
  }

}

void
ResUnit::initialize()
{
  if (true == ACPSim::traceMoves)
    cout << "trying to initialize res unit " << simEntID << endl<< flush;

  maxUpdateInterval = 30;  // seconds

  // 13 meter / second is about 30 MPH, so this is
  // a reasonable speed for a surface vehicle

  maxSpeed = 10.0; // say 10 meters/second for a tank

  sensorRange = 5000.0; // say 5000m for  a tank

  weaponRange = 1500.0; // say 1500m for  a tank

  posTolerance = 2.0; // say 1 meter for a tank

  // 10 second position time constant
  // V* = (P* - P) / posTC
  posTC = 10.0; 

  // 2 second velocity time constant
  // A* = (V* - V) / velTC
  velTC = 10.0; 

  maxAccel = 2.0; // say 2.0 meters per sec per sec for a tank
  // note that maxSpeed / maxAccel = time to reach max speed = 5 seconds for a tank

  moveController = NULL;

  tcell = NULL;

  crossSection = 10.0; // 10 meter^2

  environment = LandPE;

  maxStepDist = standardRUmaxStepDist; // meters maximum step


  dt = 0.0;
  
  if (true == ACPSim::traceMoves)
    cout << "Creating unit " << simEntID << " with default location and velocity"<<endl<<flush;

  p0 = GVector(0.0, 0.0, 0.0);
  v0 = GVector(0.0, 0.0, 0.0);
  assert (NULL != sim);
  t0 = sim->clock();

  resetPV(GVector(0,0,0), GVector(0,0,0));

  return;
}

void
ResUnit::die() {
  GVector p = currentPos();
  GVector v = GVector(0,0,0);
  deathTime = sim->clock();
  assert (birthTime >= 0);
  assert (deathTime > birthTime);
  resetPV( p, v);

  if (true == ACPSim::traceShots) {
    appBell();
    cout << side << " ResUnit " << simEntID << " dying at time " << sim->clock() << endl;
  }

  assert (norm(v0) < essentiallyStationary);

  Unit::die();
  return;
}

void
ResUnit::update() {
  double now = sim->clock();
  double timeStep = maxUpdateInterval;
  if (true == ACPSim::traceMoves) {
    cout << "-------" << endl;
    cout << side << " ResUnit " << simEntID << " update() at time " << now << endl << flush;
  }
  // this just does basic PV update, if alive
  // note that if it is dead, it will probably schedule no
  // more events, ever.
  assert (NULL != sim);
  if (LTrue == aliveP) {

    if (true == ACPSim::traceSensors) {
      cout << side << " ResUnit " << simEntID << " updating friendly SSA at "<< now;
      cout << endl << flush;    
    }
    updateFriendlySSA(false);

    if (true == ACPSim::traceSensors) {
      cout << side << " ResUnit " << simEntID << " updating enemy SSA at "<<now;
      cout << endl << flush;    
    }
    updateEnemySSA(false);

    if (current_fsm != NULL) {

      if (1 == FSM::debugFSM) {
	cout << side << " ResUnit " << simEntID << " executing fsm ";
	cout << current_fsm->getID() << endl << flush;
      }

      current_fsm->execute();
    }
    else {
      if (true == ACPSim::traceFSM) {
	cout << "ResUnit " << simEntID << " has no fsm ";
	cout << endl << flush;
      }
    }

    if (NULL != moveController) {

      GVector pCurr = currentPos();
      GVector vDsrd;
      timeStep = 0.0;
      moveController->desiredVelocity(vDsrd, timeStep);

      if (true == ACPSim::traceMoves) {
	cout << "Resetting pv to: " << endl;
	cout << "        " << pCurr << endl;
	cout << "        " << vDsrd << endl;
	cout << flush;
      }
      resetPV (pCurr, vDsrd);
      perturbPosition (posTolerance/3.0); // shake things up a little
    }


    ACPSimEvent*  sse = new ACPSimEvent(theSim, SSStateUpdate);
    assert (NULL != sse);
    sse->data = (void*)(this);
    sse->processFN = updatePV;

    if (true == ACPSim::traceMoves) {
      cout << "ResUnit " << simEntID << " scheduling next event for " << now+timeStep << endl;
      cout << flush;
    }
    scheduleNextEvent(now + timeStep, sse);
    doShootEvents();
  }
  return;
}

void
ResUnit::doShootEvents() {
  Logical firedP = LFalse;
  //  NodeList* targetableUnits = NULL;
  vector<ResUnit*>* targetableUnits = NULL;
  //  Node* nd = NULL;
  ResUnit* ru = NULL;
  float effPd = 0.8; // prob of at least damage
  float effPk = 0.5;  // prob of death, given damage
  float rd = 0.0;
  float rk = 0.0;
  unsigned int i = 0;
  unsigned int n = 0;
  float effectiveRange = 0.0;

  bool visibleP = false;
  //  double visDist = 0.0;

  double now = sim->clock();
  GVector myPos = currentPos();
  GVector trgtPos;
  if (LTrue == aliveP) {
    effectiveRange = dmin(sensorRange, weaponRange);
    targetableUnits = theSim->tgrid->resUnitsNearLoc(myPos.get(0), 
						     myPos.get(1), 
						     effectiveRange,
						     false); // not sorted
    n = targetableUnits->size();

    // we take 1 shot at the nearest live enemy
    for (i=0; ((i<n) && (LFalse == firedP)); i++) {
      ru = (*targetableUnits)[i];
      trgtPos = ru->currentPos();
      // if you want to ignore earth's curvature, and get precise LOS length,
      // make the following call
      //      tgrid->lineOfSight(myPos, trgtPos, visibleP, visDist);
      // to include both local terrain and earth's curvature,
      // make the following call
      visibleP = tgrid->terrainVisibleP(myPos, trgtPos);
      if ((true == ACPSim::traceSensors) && (ru->simEntID != simEntID)) {

	if (false == visibleP) {
	  cout << "ResUnit " << simEntID << " has blocked visibility to ResUnit ";
	  cout << ru->simEntID << " at time " << now << endl;
	  cout << flush;
	}
	else {	 
	  cout << "ResUnit " << simEntID << " has clear visibility to ResUnit ";
	  cout << ru->simEntID << " at time " << now << endl;
	  cout << endl << flush;
	}

      }
      if ((true == visibleP)
	  && (LTrue == opposedByP(ru->side, side))
	  && (LTrue == ru->aliveP)) {
	firedP = LTrue;
	rd = sim->rng->uniform(0,1);
	rk = sim->rng->uniform(0,1);
	if (true == ACPSim::traceShots) {
	  cout << endl;
	  cout << side << " ResUnit " << simEntID << " shooting at ";
	  cout << ru->side << " ResUnit " << ru->simEntID << endl;
	}
	if (rd < effPd) {
	  ru->brokenP = LTrue;
	  if (true == ACPSim::traceShots) {
	    cout << ru->side << " ResUnit " << ru->simEntID << " is broken" << endl;
	  }


	  if (rd < effPk) {
	    // notice that the target, ru, need not even be processing
	    // events. we assess damage, not it.
	    if (true == ACPSim::traceShots) {
	      cout << ru->side << " ResUnit " << ru->simEntID << " is killed" << endl;
	    }
	    ru->die();
	  }
	}
      }
    }

    if (NULL != targetableUnits) {
      delete targetableUnits;
      targetableUnits = NULL;
    }
  }

  return;
}


float 
ResUnit::currentStrength()
{
  float rslt = 0.0;
  if (LTrue == aliveP)
    rslt = 1.0;
  else
    rslt = 0.0;
  return rslt;
}

Logical
ResUnit::inAreaP(Box* b)
{
  GVector my_pos;
  Logical rslt;
  my_pos = currentPos();

  if (dist(my_pos, b->center) < 2 * posNoise)
    rslt = LTrue;
  else
    rslt = b->insideP(my_pos);

  return rslt;
}


GVector
ResUnit::currentPos()
{
  double t1 = sim->clock();
  GVector p1;
  if (t1 < t0) {
    cout <<"inconsistent times in ResUnit::currentPos()"<<endl;
    cout <<"       t0: " << t0 << endl;
    cout <<"       t1: " << t1 << endl;
    cout << flush;
    assert (t1 >= t0);
  }
  p1 = p0 + v0*(t1 - t0);

  //   cout << "   currentPos of ResUnit " << simEntID << " is " << p1 << endl;

  if (p1.get(0) < 0.0)
    p1.set(0.0, 0);

  if (p1.get(0) > theSim->tgrid->max_X)
    p1.set(theSim->tgrid->max_X, 0);

  if (p1.get(1) < 0.0)
    p1.set(0.0, 1);

  if (p1.get(1) > theSim->tgrid->max_Y)
    p1.set(theSim->tgrid->max_Y, 1);
  
  
  return p1;
}

GVector
ResUnit::currentVel() {
  double t1 = sim->clock();
  assert (t1 >= t0);
  return v0;
}

void 
ResUnit::perturbPosition(double amount) {  // in meters

  if (LTrue == aliveP) {
    GVector pTemp = currentPos();
    GVector vTemp = currentVel();
    pTemp = perturbGVector(pTemp, amount, sim->rng);
    pTemp.set(TGrid::StandardEntityHeight, 2); // %%% jerk to near sea level
    resetPV(pTemp, vTemp);
  }
  return;
}

void 
ResUnit::resetPV(GVector p1, GVector v1)
{
  TCell *tmpTCell0 = NULL;
  TCell *tmpTCell1 = NULL;
  const double now = sim->clock();

  const double x0 = p0.get(0);
  const double y0 = p0.get(1);
  //  const double z0 = p0.get(2);
  const double x1 = p1.get(0);
  const double y1 = p1.get(1);
  const double z1 = p1.get(2);

  const double vx1 = v1.get(0);
  const double vy1 = v1.get(1);
  const double vz1 = v1.get(2);

  // notice that we do NOT prevent dead units from moving. This
  // allows "magic moves" at any time.
  if ((LTrue != aliveP) && (true == ACPSim::traceMoves))
    cout << "Moving dead unit " << simEntID;

  if (LTrue != tgrid->onGrid(x1, y1)) {
    // leave p0 unchanged
    v0 = GVector(0.0, 0.0, 0.0);
    t0 = now;
    if (true == ACPSim::traceMoves){
      cout << "Attempted moving unit " << simEntID;
      cout << " off the terrain grid. Not done."<<endl<<flush;
    }
  }
  else { 
    tmpTCell1 = tgrid->getTCell(x1, y1);

    if (NULL == tcell) { // first move ever

      if (true == ACPSim::traceMoves)
	cout << "First move of " <<simEntID<< flush;

      tmpTCell1->addOccupant(this); // resets this->tcell
      assert (tcell == tmpTCell1);
    }
    else { 
      tmpTCell0 = tgrid->getTCell(x0, y0);
      if (true == ACPSim::traceMoves)
	cout << "Later move of " <<simEntID<< endl<<flush;

      if (tmpTCell0 != tmpTCell1) {
	if (true == ACPSim::traceMoves) {
	  cout << "Resetting t-cell of ResUnit " << simEntID;
	  cout << " from "<<tmpTCell0->getR() << ", "<<tmpTCell0->getC();
	  cout << " to "<<tmpTCell1->getR() << ", "<<tmpTCell1->getC();
	  cout << endl << flush;
	}

	assert (tcell == tmpTCell0);
	tmpTCell0->removeOccupant(this); // resets this->tcell
	assert (tcell == NULL);

	tmpTCell1->addOccupant(this); // resets this->tcell
	assert (tcell == tmpTCell1);

	if (true == ACPSim::traceMoves)
{
 	cout << "Old  ";
 	tmpTCell0->displayOccupants();
 	cout << "New  ";
 	tmpTCell1->displayOccupants();
	}
	

      }
      else {
	if (true == ACPSim::traceMoves) {
	  cout << "Not resetting t-cell of ResUnit " << simEntID << " because no cell change"<<endl;
	}
      }

      //p0 = p1, without the overhead of copy constructor
      p0.set(x1, 0);
      p0.set(y1, 1);
      p0.set(z1, 2);

      // v0 = v1, without the overhead of copy constructor
      v0.set(vx1, 0);
      v0.set(vy1, 1);
      v0.set(vz1, 2);

      t0 = now;
      // land-surface and water-surface things are
      // locked to a fixed height above terrain
      if ((LandPE == environment) || (WaterPE == environment)) {
	p0.set(TGrid::StandardEntityHeight + tcell->height, 2); // %%% default entity height
	v0.set(0.0, 2);
      }
    }

  }
  	if (true == ACPSim::traceMoves) {
  cout << "ResUnit "<<simEntID<< " at time " << now;
  cout << " has p="<<p0<<", v="<<v0<<endl<<flush;
	}
  return;
}

// this could be used to bring everything up to date simultaneously
void 
ResUnit::makePVCurrent() {
  GVector p = currentPos(); // dead reckoned
  GVector v = currentVel(); // dead reckoned
  resetPV(p,v);
  return;
}



// NOTE WELL: Situation-Awareness !=Subordinate-Situation-Awareness, by a long shot
//
// in this very simple model of Subordinate-Situation-Awareness,
// the only friendly in the list is the unit itself.
// the full situation awareness is a copy of the merged picture from higher up
void  ResUnit::updateFriendlySSA(bool) {
  ssa->clearFriendly();
  ssa->mergeFriendly(this);
  return;
}

void ResUnit::updateEnemySSA(bool) {
  ssa->clearEnemy();
  // scan for enemies, and mergeEnemy(u)
  GVector pt = currentPos();
  double searchRange = sensorRange / 3.0;
  vector<ResUnit*> *units = tgrid->resUnitsNearLoc(pt.get(0), pt.get(1),
						    searchRange, false);
  ResUnit* u = NULL;
  unsigned long int i = 0;
  unsigned long int n = units->size();
  for (i=0; i<n; i++) {
    u = (*units)[i];
    if ((LTrue == u->aliveP) && (LTrue == opposedByP( u->side, side)))
      ssa->mergeEnemy(u);
  }

  delete units;
  units = NULL;

  return;
}




// this whole updatePVRoute function should get folded into
// the moveController object
void
ResUnit::updatePVroute() {
  //   ACPSimEvent *sse;
  //   double t1;
  //   GVector displacement, p1, v1;
  //   double dt = 0.0;
  //   double desSpeed = 0.0;
  //   double distance = 0.0;
  //   unsigned int i = 0;
  //   dt = 0.0;

  //   // if this is the very first invokation with this
  //   // gRecord, start at its first waypoint
  // //   if (NULL == wpNode)   {
  // //     wpNode = gRecord->wpNode;
  // //   }
  // //   assert(NULL != wpNode);

  //   // update current position
  //   t1 = sim->clock();
  //   p1 = currentPos();

  //   //  wp = (*wpNode)[0]; // just the first
  //   if (true == ACPSim::traceMoves) {
  //     cout << endl;
  //     cout << "Desired waypoint of ResUnit " << simEntID<<" is " << wp->pos << endl;
  //     cout << " ResUnit " << simEntID<<" is at " <<p1 << endl;
  //   }
  
  //   displacement = (wp->pos - p1);
  //   distance = norm( displacement);
  //   if (distance <= ResUnit::posTolerance) { // close enough
  //     if (true == ACPSim::traceMoves) {
  //       cout << endl;
  //       cout << "At time "<<t1 <<" Waypoint change by ResUnit " << simEntID << endl;
  //       cout << "first waypoint of ResUnit " << simEntID<<" is " << wp->pos << endl;
  //     }
  //     reportPV();
  //     if (1 == wpNode->size()) {  // no next wp
  //       p1 = wp->pos;
  //       v1 = GVector(0,0,0);
  //       displacement = GVector(0,0,0);
  //       distance = 0;
  //       dt = ResUnit::maxUpdateInterval;
  //       if (true == ACPSim::traceMoves) {
  // 	cout << "No next wpNode for ResUnit " << simEntID;
  // 	cout << " so we wait max interval"<<endl<<flush;
  //       }
  //     } // end of no next wp
  //     else { // have another wp
  // //       wpNode = wpNode->next;
  // //       wp = (WayPoint*)(wpNode->data);
  //       evector<WayPoint*> *nuWPL = new evector<WayPoint*>();
  //       for (i=1; i<wpNode->size(); i++)
  // 	nuWPL->push_back(  (*wpNode)[i]);

  //       delete wpNode;
  //       wpNode = nuWPL;

  //       wp = (*wpNode)[0];

  //       displacement = ( wp->pos - p1 );
  //       distance = norm(displacement);
  //       if (true == ACPSim::traceMoves) {
  // 	cout << "Next wpNode for ResUnit " << simEntID << " is " << wp->pos;
  // 	cout << " so we set interval by time to arrive"<<endl<<flush;
  //       }
  //     } // end of have another wp
  //     if (true == ACPSim::traceMoves)
  //       cout << "Next waypoint of ResUnit " << simEntID<<" is " << wp->pos << endl;
  //   } // end of close enough


  //   if (distance > ResUnit::posTolerance) { // not at the final waypoint
  //     switch (wp->type) {
  //     case  timeWP:
  //       if (wp->scalar > t1)
  // 	desSpeed = norm(displacement) / (wp->scalar - t1);
  //       else
  // 	desSpeed = maxSpeed; // if you missed the deadline, get there ASAP
  //       break;
  //     case speedWP:
  //       assert (wp->scalar > 0);
  //       desSpeed = wp->scalar;
  //       break;
  //     default:
  //       cout << "Unrecognized waypoint type" << endl << flush;
  //       assert (false);
  //       break;
  //     }
  //     if (maxSpeed > desSpeed)
  //       desSpeed = maxSpeed;
  //     assert (desSpeed > 0);

  //     distance = norm(displacement);

  //     if (true == ACPSim::traceMoves)
  //       cout << " expect to arrive at time " << t1 + (distance / desSpeed) << endl;

  //     if (distance > maxStepDist) {
  //       distance = maxStepDist * sim->rng->uniform(.80, 1.00); // gradually desynchronize
  //     }
  //     v1 = displacement;
  //     v1.scale_to(desSpeed);
  //     dt = distance / desSpeed;
  //   }  // end of not at the final waypoint

  //   assert (dt > 0); // else we failed

  //   sse = new ACPSimEvent(sim, SSStateUpdate);
  //   assert (NULL != sse);
  //   sse->data = (void*)(this);
  //   sse->processFN = updatePV;

  //   //   sim->schedule(t1+dt, sse);
  //   scheduleNextEvent(t1+dt, sse);
  //   if (true == ACPSim::traceMoves) {
  //     cout << "Scheduling ResUnit " << simEntID << " for event at ";
  //     cout << t1+dt << endl;
  //   }
  //   p1 = perturbPos(p1, posNoise, sim->rng);
  //   resetPV(p1,v1);

  cout << "UpdatePVRoute is a no-op"<<endl<<flush;
  return;
}

void
ResUnit::updatePVintercept() {
  //   ACPSimEvent *sse;
  //   //   MissileDetonationRecord *mdr;
  //   double t1;
  //   GVector tPos, displacement, p1, v1, tVel;
  //   GVector interceptPos, desVel;
  //   //   double desSpeed;
  //   double distance, dt2;
  //   //   WayPoint *wp;

  //   assert (NULL != sim);
  //   assert (NULL != gRecord);
  //   assert (NULL == gRecord->wpNode);
  //   assert (NULL != gRecord->target);
  //   assert (interceptResUnitGT== gRecord->type);

  //   // update current position
  //   t1 = sim->clock();
  //   p1 = currentPos();

  //   if (LTrue == gRecord->target->aliveP) {
  //     tPos = gRecord->target->currentPos();
  //     tVel = gRecord->target->currentVel();
  //     if (true == ACPSim::traceMoves) {
  //       cout << "ResUnit " << simEntID << " at P: " << p1 << " is trying to intercept ResUnit ";
  //       cout << gRecord->target->simEntID <<endl;
  //       cout << "  which is at P: " << tPos << " and V: " << tVel <<endl ;
  //       cout << " At time " << t1 << " and maxSpeed " << maxSpeed <<  endl;
  //     }
  //     displacement = tPos - p1;
  //     distance = norm(displacement);

  //     if (dt == 0) { // never run before
  //       dt = timeToIntercept( tPos, tVel, p1, maxSpeed ) / 2.5;
  //     }
  //     // dt was set when this missile began chasing its target
  //     assert (dt > 0);

  //     // the 0.99 protects against roundoff error, even with double precision
  //     dt2 = timeToIntercept( tPos, tVel, p1, 0.99 * maxSpeed );
  //     interceptPos = tPos + (tVel * dt2);
  //     desVel = ( (tPos - p1) / dt2 ) + tVel;
  //     if (true == ACPSim::traceMoves) {
  //       cout << " estimated time remaining to intercept is " << dt2 << " (dt = " << dt << " )"<<endl;
  //       cout << " expected interception location is " << interceptPos << endl;
  //       cout << " remaining distance to intercept is " << dist(interceptPos, p1) << endl;
  //       cout << " desired velocity is " << desVel << " with speed " << norm(desVel) << endl;
  //     }
  //     assert (norm(desVel) <= maxSpeed);
  //     v1 = desVel;

  //     if (dt2 < dt) { // we are close enough
  //       this->doTerminalIntercept(t1+dt2);
  //     }
  //     else   {	// schedule a PV update event after time dt
  //       sse = new ACPSimEvent(sim, SSStateUpdate);
  //       assert (NULL != sse);
  //       sse->data = (void*)(this);
  //       sse->processFN = updatePV;

  //       if (true == ACPSim::traceMoves) {
  // 	cout << "Scheduling ResUnit " << simEntID << " for event at ";
  // 	cout << t1+dt << endl;
  //       }
  //       scheduleNextEvent(t1+dt, sse);
  //     }

  //     resetPV(p1,v1);
  //   }
  //   else  {
  //     // for now, assume we are a missile and DIE as soon as target dies
  //     die();
  //   }


  cout << "UpdatePVIntercept is a no-op"<<endl<<flush;

  return;
}

// this needs to be replaced by something else,
// like a movement controller method
void
ResUnit::doTerminalIntercept(double time) {
  //   // schedule a detonation event at time
  //   ACPSimEvent *sse;
  //   MissileDetonationRecord *mdr;

  //   if (true == ACPSim::traceMoves) {
  //     cout << endl;
  //     cout << " At time " << sim->clock();
  //     cout << " scheduling ResUnit::doTerminalIntercept for time " << time << endl;
  //   }
  //   assert (time >= sim->clock());
  //   sse = new ACPSimEvent(sim, SSDetonation);
  //   assert (NULL != sse);
  //   mdr = new MissileDetonationRecord;
  //   assert (NULL != mdr);
  //   mdr->missile = this;
  //   mdr->target = gRecord->target;
  //   mdr->Pk = 0.75;
  //   mdr->time = time;
  //   sse->data = ((void*) mdr);
  //   sse->processFN = detonateMissileFN;
  //   sim->schedule(time, sse);
  cout << "DoTerminalIntercept is a no-op"<<endl<<flush;
  return;
}

// ------------------------------------------
// yes, this does not belong here.
// further, it does not work unless you hack CmndUnit::inAreaP
// to only check the superior's CG, not whether each subordinate
// is in the box!
// ------------------------------------------

// this is the one that gets used, not CmndUnit::make Corridor FSM(Corridor*) !
//
// FSM* 
// CmndUnit::make Corridor FSM(TempCorridor* corr) {
//   FSM *fsm = NULL;

//   Node *bxNode; // data of type Box* - the box
//   Node *strtNode; // data of type float* - time to arrive in box
//   Node *endNode; // data of type float* - time to leave box

//   float* fPtr = NULL; // a pointer for retrieving floats
//   int stateNum = 0;

//   float prev_dist = 0;
//   float curr_dist = 0;
//   GVector a,b,c,d;
//   State *curr_st, *prev_st, *final_st;
//   GVector curr_pt, prev_pt, pt;
//   float curr_arrive_time = 0;
//   float curr_exit_time = 0;
//   float prev_arrive_time = 0;
//   float prev_exit_time = 0;
//   Predicate *stest1, *stest2;
//   Conjunction *stest;
//   Action *action;
//   Box *currBox = NULL;
//   Box *prevBox = NULL;
//   Box *rbx1 = NULL;
//   Box *rbx2 = NULL;
//   Box *moveBox = NULL;
// //    Box *tbx1 = NULL;
// //    Box *tbx2 = NULL;

//   int n = corr->boxes->length();
//   if (true == ACPSim::tracePlanning)
//     cout << "make corridor fsm for temp corridor of length " << n << endl << flush;

//   fsm = new FSM();
//   assert (NULL != fsm);

//   if (true == ACPSim::tracePlanning)
//     cout << "CmndUnit " << simEntID <<" is building FSM for TempCorridor " << endl;

//   curr_st = NULL;
//   prev_st = NULL;
//   final_st = NULL;
//   stateNum = 0;
//   // setup St0, the start state

//   curr_st = new State();
//   assert (NULL != curr_st);
//   fsm->addState(curr_st);
//   fsm->setState(curr_st);


//   for (bxNode = corr->boxes->first,
// 	 strtNode = corr->sTimes->first,
// 	 endNode = corr->eTimes->first;

//        ((NULL != bxNode) && (NULL != strtNode) && (NULL != endNode));
       
//        bxNode = corr->boxes->nextNode(bxNode),
//        strtNode = corr->sTimes->nextNode(strtNode),
// 	 endNode = corr->eTimes->nextNode(endNode)) {

//     prevBox = currBox;
//     prev_pt = curr_pt;
//     currBox = ((Box*) bxNode->data);
//     assert (NULL != currBox);
//     curr_pt = currBox->center;

//     prev_dist = curr_dist;
//     a = currBox->get_A();
//     b = currBox->get_B();
//     c = currBox->get_C();
//     d = currBox->get_D();
//     curr_dist = (dist(a,b) + dist(b,c) + dist(c,d) + dist(d,a)) / (4.0 * 2.0);

//     prev_arrive_time = curr_arrive_time;
//     fPtr = ((float*) strtNode->data);
//     curr_arrive_time = *fPtr;

//     prev_exit_time = curr_exit_time;
//     fPtr = ((float*) endNode->data);
//     curr_exit_time = *fPtr;

//     assert (prev_arrive_time <= prev_exit_time);
//     assert (prev_exit_time <= curr_arrive_time);
//     assert (curr_arrive_time <= curr_exit_time);

//     // hack
//     // temporary accomdation to convertCorridor
//     // should be strict <
//     assert (prev_arrive_time <= curr_exit_time);

//     if (NULL == prev_st) {     // first state
//       assert (0 == stateNum);
//       prev_st = curr_st; // remember state
//       curr_st = new State(); // this is St1
//       stest1 = new AlwaysTrue();
//       assert (NULL != curr_st);
//       assert (NULL != stest1);

//       if (NULL != prevBox) {
// 	rbx1 = NULL;
// 	rbx2 = NULL;
// 	regularizeBoxPair(prevBox, currBox, rbx1, rbx2);
// 	assert (NULL != rbx1);
// 	assert (NULL != rbx2);

// 	// I need to compute a move-box here, so that it can look for enemies.
// 	// it stretches from prevBox to  rbx2?
// 	moveBox = NULL;

// 	// this memory leaking hack is to let me see intermediate products
// 	theSim->boxes->push_back( rbx1 );
// 	theSim->boxes->push_back( rbx2 );

//       }
//        else {

// 	 rbx2 = currBox; // can't get currentPos() because the sim may not have started

// 	// I need to compute a move-box here, so that it can look for enemies.
// 	// it is currBox?
// 	moveBox = currBox;

//        }


//       action = new OrderCUnitAcrossBox(this, 
// 				       moveBox,  // the "move-box", in which we look for opp's
// 				       rbx2,  // the "end-box", into which we are going
// 				       curr_arrive_time, curr_st);
//       assert (NULL != action);

//       prev_st->addTransition(stest1, action, curr_st);

//       if (true == ACPSim::tracePlanning) {
// 	cout << "State " << stateNum <<"  action is to go to point ";
// 	cout << curr_pt << " by time " << curr_arrive_time << endl;
//       }

//     }
//     else { // not on the first state
//       prev_st = curr_st; // remember St(i)
//       curr_st = new State();  // this is St(i+1)
//       stest1 = new TimePassed(prev_exit_time, theSim);
//       assert (NULL != curr_st);
//       assert (NULL != stest1);

//       // with regularized things, this actually tests if
//       // it is in the start of this pair, NOT if it was in
//       // the end of the previous pair. Because they might
//       // not be perfectly aligned, it is possible to be in
//       // the end of the previous pair, as ordered, but not
//       // in the start of this pair - leading the unit to
//       // wait forever. Notice that I hacked
//       // CmndUnit::inAreaP so this is less likely to happen.
//       //
//       // the real solution is totally re-write this code!
//       //
//       stest2 = new UnitInArea(this, prevBox);

//       stest = new Conjunction();
//       assert (NULL != stest2);
//       assert (NULL != stest);

//       stest->add_predicate(stest1);
//       stest->add_predicate(stest2);

//       if (NULL != prevBox) {
// 	rbx1 = NULL;
// 	rbx2 = NULL;
// 	regularizeBoxPair(prevBox, currBox, rbx1, rbx2);
// 	assert (NULL != rbx1);
// 	assert (NULL != rbx2);

// 	// this memory leaking hack is to let me see intermediate products
// 	theSim->boxes->push_back( rbx1 );
// 	theSim->boxes->push_back(  rbx2 );
//       }

//       // what should this be?
//       // stretched from prevBox to rbx2?
//       moveBox = NULL;

//       action = new OrderCUnitAcrossBox(this, 
// 				       moveBox,  // the "move-box", in which we look for opp's
// 				       rbx2,  // the "end-box", into which we are going
// 				       curr_arrive_time, curr_st);
//       assert (NULL != action);


//       if (true == ACPSim::tracePlanning) {
// 	cout << "State " << stateNum <<"  action is to go to point ";
// 	cout << curr_pt << " by time " << curr_arrive_time;
// 	cout << " from  point " << prev_pt << endl;
//       }

//       prev_st->addTransition(stest, action, curr_st);
//     }

//     if (true == ACPSim::tracePlanning)
//       cout << endl << flush;

//     stateNum++;
//     fsm->addState(curr_st);
//   }

//   final_st = new State();
//   assert (NULL != final_st);
//   stest1 = new UnitInArea(this, currBox);
//   assert (NULL != stest1);
//   curr_st->addTransition(stest1, NULL, final_st);
//   fsm->addState(final_st);

//   return fsm;
// }

// ------------------------------------------

FSM* 
ResUnit::makeCorridorFSM(TempCorridor* corr) {
  FSM *fsm = NULL;

  unsigned int i = 0;
  unsigned int n = 0;

  int stateNum = 0;

  float prev_dist = 0;
  float curr_dist = 0;
  GVector a,b,c,d;
  State *curr_st, *prev_st, *final_st;

  GVector pt = GVector(0.0, 0.0, 0.0);
  GVector curr_pt = GVector(0.0, 0.0, 0.0);
  GVector prev_pt = GVector(0.0, 0.0, 0.0);

  float curr_arrive_time = 0;
  float curr_exit_time = 0;
  float prev_arrive_time = 0;
  float prev_exit_time = 0;
  Predicate *stest1, *stest2;
  Conjunction *stest;
  Action *action;
  Box *bx = NULL;

  fsm = new FSM();
  assert (NULL != fsm);

  if (true == ACPSim::tracePlanning)
    cout << "ResUnit " << simEntID <<" is building FSM for TempCorridor " << endl;

  curr_st = NULL;
  prev_st = NULL;
  final_st = NULL;
  stateNum = 0;
  // setup St0, the start state

  curr_st = new State();
  assert (NULL != curr_st);
  fsm->addState(curr_st);
  fsm->setState(curr_st);

  n = corr->boxes->size();

  for (i=0; i<n; i++) {

    //     cout << "curr_pt dimension: " << curr_pt.getDim();
    //     cout << "curr_pt: " << curr_pt << endl;
    //     cout << flush;
    prev_pt = curr_pt;

    // this is just to keep the lines short and 
    // minimize the chance of typos. we do not
    // change it or use it later.
    bx = (*(corr->boxes))[i] ;

    curr_pt = bx->center;

    prev_dist = curr_dist;
    a = bx->get_A();
    b = bx->get_B();
    c = bx->get_C();
    d = bx->get_D();

    bx = NULL; 

    curr_dist = (dist(a,b) + dist(b,c) + dist(c,d) + dist(d,a)) / (4.0 * 2.0);

    prev_arrive_time = curr_arrive_time;
    curr_arrive_time = (*(corr->sTimes))[i];

    prev_exit_time = curr_exit_time;
    curr_exit_time = (*(corr->eTimes))[i];


    if (NULL == prev_st) {     // first state
      assert (0 == stateNum);
      assert (0 == i);
      prev_st = curr_st; // remember state
      curr_st = new State(); // this is St1
      stest1 = new AlwaysTrue();
      action = new OrderResUnitToPoint(this, curr_pt, curr_arrive_time);
      assert (NULL != curr_st);
      assert (NULL != stest1);
      assert (NULL != action);

      prev_st->addTransition(stest1, action, curr_st);

      if (true == ACPSim::tracePlanning) {
	cout << "State " << stateNum <<"  action is to go to point ";
	cout << curr_pt << " by time " << curr_arrive_time << endl;
      }

    }
    else { // not on the first state
      assert (i > 0);
      prev_st = curr_st; // remember St(i)
      curr_st = new State();  // this is St(i+1)
      stest1 = new TimePassed(prev_exit_time, theSim);
      stest2 = new UnitNearPoint(this, 
				 prev_pt,  // center of Box(i)
				 prev_dist,
				 true); // use 2D map distance
      stest = new Conjunction();
      assert (NULL != curr_st);
      assert (NULL != stest1);
      assert (NULL != stest2);
      assert (NULL != stest);
      stest->addPred(stest1);
      stest->addPred(stest2);
      action = new OrderResUnitToPoint(this, curr_pt, curr_arrive_time);
      assert (NULL != action);

      if (true == ACPSim::tracePlanning) {
	cout << "State " << stateNum <<"  action is to go to point ";
	cout << curr_pt << " by time " << curr_arrive_time;
	cout << " from  point " << prev_pt << endl;
	cout << flush;
      }

      prev_st->addTransition(stest, action, curr_st);
    }

    if (true == ACPSim::tracePlanning)
      cout << endl << flush;

    stateNum++;
    fsm->addState(curr_st);
  }

  final_st = new State();
  stest1 = new UnitNearPoint(this, curr_pt, curr_dist, true);
  assert (NULL != final_st);
  assert (NULL != stest1);
  curr_st->addTransition(stest1, NULL, final_st);
  fsm->addState(final_st);

  // leave moveController alone until this is actually applied
  return fsm;
}


FSM* ResUnit::makeNCW1FSM() {
  // %%% no-op
  return NULL;
}


// ------------------------------------------
// no longer used

//  FSM* 
//  ResUnit::makeCorridorFSM(Corridor* corr) {
//    FSM *fsm = NULL;
//    State *curr_st, *prev_st, *final_st;
//    float curr_dist, prev_dist;
//    float curr_time, prev_time;
//    GVector *curr_pt, *prev_pt;
//    GVector pt;
//    Predicate *stest;
//    Action *action;
//    Box *bx;
//    Node *abND; // data of type Box*
//    Node *etND; // data of type float*
//    float *fPtr;

//    fsm = new FSM();

//    if (true == ACPSim::tracePlanning) {
//      cout << "ResUnit " << simEntID <<" is building corridor FSM for corridor " << endl;
//      cout << *corr << endl;
//    }
//    curr_time = 0;
//    curr_dist = 0;
//    curr_pt = NULL;
//    curr_st = new State();
//    fsm->addState(curr_st);
//    fsm->setState(curr_st); 

//    // this ASSUMES that the box's labels are assigned in a certain way.
//    // that is, if the sub is to come into the box from the left,
//    // then the labels correspond to the points in the order
//    //
//    //                   D    C
//    //  unit ==>
//    //                   A    B
//    //
//    //  so that B is the right front corner, C is the left front corner,
//    // and the box is to be entered across the D-A segment.
//    // if this assumption is violated, it will probably crash with an ill-formed box.

//    //   for (corr->area_boxes.resetFirst(), corr->end_times.resetFirst(); 
//    //        (corr->area_boxes.isEnd() == False)&&(corr->end_times.isEnd()==False); 
//    //        corr->area_boxes.next(), corr->end_times.next())
//    for (abND = corr->area_boxes->first, etND = corr->end_times->first;
//         ((abND != NULL) && ( etND != NULL));
//         abND = corr->area_boxes->nextNode(abND), 
//  	 etND = corr->end_times->nextNode(etND))
//      {
//        //       bx = corr->area_boxes.item();
//        bx = ((Box*) abND->data);

//        prev_st = curr_st;
//        prev_pt = curr_pt;
//        prev_dist = curr_dist;
//        prev_time = curr_time;

//        //       curr_time = corr->end_times.item();
//        fPtr = ((float*) etND->data);
//        curr_time = *fPtr;

//        curr_st = new State();
//        // set the objective near but not quite on the front edge
//        pt = ((bx->get_B() + bx->get_C())*9.0 + (bx->get_A() + bx->get_D()))/20.0;
//        curr_pt = new GVector(pt.get(0), pt.get(1));
//        curr_dist = norm(bx->get_B() - bx->get_C())/ 4;
//        if (prev_pt == NULL) {
//  	stest = new AlwaysTrue();
//        }
//        else {
//    	stest = new UnitNearPoint(this, *prev_pt, prev_dist);
//  //  	stest = new UnitNearPoint();
//        }
//        // note the default time of curr_time. because
//        // this time will have passed by the time the resunit
//        // gets there, this defaults to "get there ASAP"
//        // this is the best I can do for a generic method,
//        // unless someone else can think of better.
//        action = new OrderResUnitToPoint(this, *curr_pt, curr_time);

//        if (true == ACPSim::tracePlanning)
//  	cout << "  action is to go to point " << *curr_pt << " by time " << curr_time << endl;

//        prev_st->addTransition(stest, action, curr_st);
//        fsm->addState(curr_st);
//      }

//    final_st = new State();
//    stest = new UnitNearPoint(this, *curr_pt, curr_dist);

//    if (true == ACPSim::tracePlanning) {
//      cout << "  Goto point " << *curr_pt << " by time " << curr_time << endl;
//      cout << flush;
//    }
//    curr_st->addTransition(stest, NULL, final_st);
//    fsm->addState(final_st);

//    // so we won't accidentally try to execute garbage!
//    gRecord = NULL;
//  //   wpNode = NULL;
//    return fsm;
//  }

// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
