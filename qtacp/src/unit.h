// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: Unit now subclasses
// Simulation::Entity (was DES::SimEntity); the sim pointer is a
// typed ACPSim* member; simEntID comes from panj::Numbered;
// descheduling is lazy cancellation through an ACPSimEvent
// pointer (see acpsim.h); evector becomes std::vector; the
// legacy RNG parameter becomes panj::PRNG.
//
// Units: kilograms, meters, seconds, liters.
//
//  standardized jammer power:
//    0 is no jamming
//    10 is enough to halve the detection and ident
//    ranges, when jammed from 150KM
// ------------------------------------------

#ifndef UNITS_H
#define UNITS_H

// ------------------------------------------

#include "acpsim.h"
#include "tgrid.h"
#include "struct.h"
#include "fsm.h"

#include <vector>

class Box;
class ResUnit;
class CmndUnit;
class Missile;
class Jammer;
class Sensor;
class Corridor; // defined in tthread.h
struct TempCorridor; // defined in tthread.h

class SubSA;
class SubSA1;

// ------------------------------------------

void launchMissileFN(void* missileLaunchRecordPtr);
void sensorScanFN(void* sensorScanRecordPtr);
void detonateMissileFN(void* missileDetonationRecordPtr);
void updateMissileFN(void* missilePtr);
void updatePV(void* ruPtr); // really, a ResUnit*


AAA::GVector perturbGVector(AAA::GVector pos,
			    double amount,  // in meters
			    panj::PRNG *rng);

// would a2 want to attack a1?
AAA::Logical  opposedByP(Alignment a1, Alignment a2);
// ------------------------------------------


class Unit : public Simulation::Entity {
public:
  Unit(ACPSim* s);
  virtual ~Unit();
  virtual void initialize();
  virtual void update() = 0; // this is the event function!
  virtual void makePVCurrent() = 0;

  // Simulation::Entity requires this; qtacp units are driven by
  // their events (update() above), not by the entity hook
  virtual void process() {}

  unsigned long int getSimEntID() const { return simEntID; }

  virtual void updateFriendlySSA(bool recursiveP) = 0;
  virtual void updateEnemySSA(bool recursiveP) = 0;

  // the 1st parent is the superior.
  // the 0th parent returns NULL
  CmndUnit* nthSuperior(unsigned long int n);

  // the 1st subordinates are just the suborinates
  // the 0th subordinates are NULL
  virtual std::vector<Unit*> *nthSubordinates(unsigned long int n) = 0;
  //
  // note that the nthSubordinates of my nthSuperior
  // are my siblings

  SubSA1* ssa;

// For ResUnits: extrapolate from last one
// For CmndUnits: aggregate from below
  virtual AAA::GVector currentPos() = 0;
  virtual AAA::GVector currentVel() = 0;
  virtual void perturbPosition(double amount)=0;  // in meters

  // return some standardized measure of strength
  // this is NOT likely to be comparable across environments
  virtual float currentStrength() = 0;

  virtual void reportPV(); // report current center-of-mass positon and velocity
  virtual void die(); // kill self

  AAA::Logical allowSubPlanning;
  AAA::Logical allowManeuverPlanning;
  Alignment side;
  CmndUnit* superior;
  ACPSim *sim; // typed back-pointer (the old SimEntity carried this)
  unsigned long int simEntID; // from panj::Numbered::getID()
  TGrid *tgrid;

  AAA::Logical aliveP; // alive or dead?
  AAA::Logical brokenP; // definitely alive, but disabled or not?
  double birthTime;
  double deathTime;

  // this is the PV pair, and when it was set
  // For resunits, it used in "dead reckoning" or extrapolation
  AAA::GVector p0;
  AAA::GVector v0;
  double t0;

  double maxStepDist;  // in meters, for this Unit (probably by echelon)
  double maxUpdateInterval;  // in seconds, for  this Unit (probably by echelon)

  double posTolerance;  // in meters, for this Unit (probably by echelon)
  // sometimes this is used to see if the center of gravity is 'close enough'
  // sometimes we must make sure that all subordinates are within specified area


  // say 10 m/s for a tank
  // lower for CmndUnits, that have to plan on coordination delays
  double maxSpeed;

  // say 2000m for a tank
  // much longer for CmndUnits, that can deploy sensors
  double sensorRange;


  // say 1000m for a tank
  // much longer for CmndUnits, that can draw upon long-range fires
  double weaponRange;

  MovementRule *moveController;


// is the Unit's own center of mass in the given Box? (ignores extent of unit!)
  AAA::Logical centerInAreaP(Box* b);

  // is the Unit entirely withing the area?
  virtual AAA::Logical inAreaP(Box* b)=0;

// sets the CG and strength. No enemy == strength of 0.0
  void enemyStrengthInArea(Box* b, float& strength, AAA::GVector& center_of_gravity)  ;

// sets the CG and strength. No friends == strength of 0.0
  void friendlyStrengthInArea(Box* b, float& strength, AAA::GVector& center_of_gravity)  ;

  // returns the friendly and enemy strength in a rectangle
  void strengthsInArea(double x0, double y0, double x1, double y1, float& fS, float& eS);


// sets the CG and strength. No friends == strength of 0.0
  std::vector<ResUnit*>* friendlyRUInArea(Box* b)  ;

  void setFSM(AAA::FSM *fsm);


  virtual AAA::FSM* makeCorridorFSM(TempCorridor*)=0;
  virtual AAA::FSM* makeReformFSM(Box*, float)=0;
  virtual AAA::FSM* makeNCW1FSM()=0;

  virtual AAA::FSM* makeDefendAreasFSM(Box* wait_area, Box* response_area, float theta)=0;

  // schedule the unit's next self-update event, remembering it so
  // that descheduleNextEvent can cancel it. the event's owner
  // back-pointer keeps nextEvent from dangling: ACPSimEvent's
  // processEvent clears it before dispatch.
  virtual void scheduleNextEvent(double scheduled_time, ACPSimEvent *event);
  virtual void descheduleNextEvent();

  // the pending self-update event, if any. public because
  // ACPSimEvent::processEvent clears it before dispatch.
  ACPSimEvent *nextEvent;

protected:

  AAA::FSM *current_fsm;

private:
};

// ------------------------------------------
#endif
// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
