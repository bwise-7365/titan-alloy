// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: include list, evector -> std::vector.
//
// Units: kilograms, meters, seconds, liters.
//
// ------------------------------------------

#ifndef RES_UNITS_H
#define RES_UNITS_H

// ------------------------------------------

#include "frwrdec.h" 
#include "struct.h" 
#include "des.h"
#include "tgrid.h"
#include "unit.h" 
#include "components.h"

#include <vector>

// ------------------------------------------
class ResUnit : virtual public Unit {
public:

  ResUnit(ACPSim* sm, Alignment a, AAA::GVector initPos, AAA::GVector initVel);
  virtual ~ResUnit();
  virtual void initialize();
  virtual void update(); // this is the event function!
  void makePVCurrent();

  void updateFriendlySSA(bool recursiveP);
  void updateEnemySSA(bool recursiveP);

  // for a ResUnit, the nth subordinates are NULL
  std::vector<Unit*> *nthSubordinates(unsigned long int n) { return NULL;};



  AAA::GVector currentPos();
  AAA::GVector currentVel();
  void perturbPosition(double amount);  // in meters
  float currentStrength();
  void resetPV(AAA::GVector p, AAA::GVector v);
  void die();
  AAA::Logical inAreaP(Box* b); 

  //  AAA::FSM* makeCorridor(Corridor*);

  AAA::FSM* makeCorridorFSM(TempCorridor*);
  virtual AAA::FSM* makeReformFSM(Box*, float);
  virtual AAA::FSM* makeNCW1FSM();


  AAA::FSM* makeDefendAreasFSM(Box* wait_area, Box* response_area, float theta);

  double maxAccel;   // zero for a structure


  TCell *tcell; // which cell I'm actually in
  double crossSection; // in meter^2


  // this restricts what kind of terrain the Res Unit can enter
  // Cmnd Units have no such restriction, as they might have a mix
  // of subordinate ResUnits, each with their own environment.
  PlatformEnvironment environment;


  double posTC;
  double velTC;

protected:
  virtual void doTerminalIntercept(double time);
  void updatePVroute();
  void updatePVintercept();

  virtual void doShootEvents();

  double dt;

  //   static int highestResUnitID;
private:
};

class Missile : public ResUnit {
public:
  Missile();
  Missile(ACPSim* sm, Alignment a, AAA::GVector initPos, AAA::GVector initVel, ResUnit* trgt);
  virtual ~Missile();
  virtual void initialize();
  virtual void update(); // this is the event function!

  ResUnit *target;
  double range;
  double distanceFlown;
  AAA::GVector lastDistanceCheck;
  double Pk; // prob of kill when detonate

protected:
  virtual void doTerminalIntercept(double time);
  AAA::GVector launchPosition; // if | position - launchPosition | > range, it dies
  virtual void doShootEvents() { }; // do nothing

private:

};


// ------------------------------------------
#endif
// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
