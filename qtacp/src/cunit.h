// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: include list, evector -> std::vector.
//
// Units: kilograms, meters, seconds, liters.
//
// ------------------------------------------

#ifndef CMND_UNITS_H
#define CMND_UNITS_H

// ------------------------------------------

#include "des.h"
#include "tgrid.h"
#include "struct.h" 
#include "unit.h" 
#include "runit.h" 
#include "components.h"

#include <vector>

// ------------------------------------------
class CmndUnit : virtual public Unit {
public:
  CmndUnit(ACPSim* sm, Alignment a, AAA::GVector initPos, AAA::GVector initVel);
  virtual ~CmndUnit();
  virtual void initialize();
  virtual void update(); // this is the event function!
  void makePVCurrent();

  void updateFriendlySSA(bool recursiveP);
  void updateEnemySSA(bool recursiveP);

  std::vector<Unit*> *nthSubordinates(unsigned long int n);


  AAA::GVector currentPos();
  AAA::GVector currentVel();
  void perturbPosition(double amount);  // in meters
  virtual float currentStrength();
  AAA::Logical inAreaP(Box* b); 

  ResUnit *cmnd_veh;
  void add_sub(Unit*);
  void remove_sub(Unit*);

  int hasSubP(Unit*);

  void resetPV(AAA::GVector p, AAA::GVector v);
  void die();

  //  AAA::FSM* makeCorridorFSM(Corridor*);
  AAA::FSM* makeCorridorFSM(TempCorridor*);
  void checkTCorridor(TempCorridor*);
  virtual AAA::FSM* makeReformFSM(Box*, float);
  virtual AAA::FSM* makeNCW1FSM();

  virtual AAA::FSM* makeDefendAreasFSM(Box* wait_area,
				       Box* response_area,
				       float theta);

  // this takes in a box, and returns an evector of sub-boxes.
  // they are determined by counting up the subordinates, looking
  // at the formation type, and laying out that number of sub-boxes
  // in that formation. For example, a two-layer formation with
  // 60% forward and 40% in reserve. The sub-boxes are all
  // within the given box, but are in no special order.
  std::vector<Box*> *compute_formation(Box* , formation_type);

  double maxSpeed;   // zero for a structure


  std::vector<Unit*> *subordinates;
  int numLiveSubs();


  void spawn_sub_cp(CmndUnit *new_cp,
		    std::vector<Unit*> *transfer_list);


  void absorb_sub_cp(CmndUnit* sub_cp);


protected:
  void updatePVroute();
  void updatePVintercept();

  AAA::GVector wayPoint;

  static double maxUpdateInterval;  // in seconds, for ALL CmndUnits
  double dt;

private:
};

// ------------------------------------------
#endif
// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
