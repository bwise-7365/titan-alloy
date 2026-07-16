// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: includes, evector -> std::vector,
// DES::DESimulation -> ACPSim.
//
//-------------------------------------------------------------------------


#ifndef ORDERS_HEADER
#define ORDERS_HEADER

// ------------------------------------------------------

#include "frwrdec.h"
#include "struct.h"

#include "aaa.h"
#include "des.h"
#include "tdv.h"
#include "fsm.h"
#include "unit.h"
#include "runit.h"
#include "cunit.h"

#include <vector>

// ------------------------------------------------------

class Box;
class Corridor;
class TempCorridor;
class CmndUnit;
class ResUnit;

// ------------------------------------------------------


class UnitNearPoint : public AAA::Predicate
{
public:

  //  UnitNearPoint();
  UnitNearPoint(Unit* mu, AAA::GVector pnt, float cDist, bool mDP); 
  ~UnitNearPoint();
  virtual AAA::Logical test();

  Unit* force;
  AAA::GVector  point;
  float critical_dist;
  bool mapDistP; // true iff we use 2D map distance, false for 3D distance

protected:

private:

};


class UnitInArea : public AAA::Predicate
{
public:

  //  UnitInArea();
  UnitInArea(Unit*, Box*); 
  ~UnitInArea();
  virtual AAA::Logical test();

  Unit* force;
  Box* area;

protected:

private:

};

//------------------------------------------------------

class EnemyInArea : public AAA::Predicate
{
public:

  //  EnemyInArea();
  EnemyInArea(Unit* u, Box*, float); 
  ~EnemyInArea();
  virtual AAA::Logical test();

  Unit* unit;
  Alignment my_side;
  Box* area;
  float min_size;

protected:

private:

};

//------------------------------------------------------

class TimePassed : public AAA::Predicate
{
public:

  //    TimePassed();
    TimePassed(float, ACPSim*);
    ~TimePassed();
    virtual AAA::Logical test();

protected:

private:
    
    float time;
    ACPSim *reference_sim;

};


//------------------------------------------------------
// notice ordering resunits to a point make sense because
// they are points, while composite units are given corridors
// (equivalent to proceed to a point, with a width to use).

class OrderResUnitToPoint : public AAA::Action
{
public:
  //  OrderResUnitToPoint();
  ~OrderResUnitToPoint();
  OrderResUnitToPoint(ResUnit*, AAA::GVector, float);
  void perform();

protected:

private:

  ResUnit* unit;
  AAA::GVector point;
  float arrival_deadline;
};


//------------------------------------------------------

class OrderResUnitEngageEnemyInArea : public AAA::Action
{
public:
  //  OrderResUnitEngageEnemyInArea();
  ~OrderResUnitEngageEnemyInArea();
  OrderResUnitEngageEnemyInArea(ResUnit*, Box*);
  void perform();

protected:

private:

  ResUnit* unit;
  Box* area;

};

//------------------------------------------------------
class OrderUnitToArea : public AAA::Action
{
public:
  //  OrderUnitToArea();
  ~OrderUnitToArea();
  OrderUnitToArea(Unit*, Box*, float);


  // this was a no-op in the original ACP code ?!
  void perform();

protected:

private:

  Unit* unit;
  Box* area;
  float arrival_deadline;
};

//------------------------------------------------------

class OrderUnitAlongCorridor : public AAA::Action
{
public:
  //  OrderUnitAlongCorridor();
  ~OrderUnitAlongCorridor();
  OrderUnitAlongCorridor(Unit*, Corridor*);
  OrderUnitAlongCorridor(Unit*, TempCorridor*);
  void perform();

protected:

private:

  Unit* unit;
  Corridor *corridor;
  TempCorridor *tCorridor;
};


//------------------------------------------------------
// Assuming the unit is already centered in the box,
// order it to re-arrange in place (e.g. rotate)

class OrderUnitReformInPlace : public AAA::Action
{
public:
  //  OrderUnitReformInPlace();
  ~OrderUnitReformInPlace();
  OrderUnitReformInPlace(Unit*, Box*, float);
  void perform();

protected:

private:

  Unit* unit;
  Box *nuBox;
  float eTime;
};


//------------------------------------------------------

class OrderUnitDefendAreas : public AAA::Action
{
public:
  //  OrderUnitDefendAreas();
  ~OrderUnitDefendAreas();
  OrderUnitDefendAreas(Unit*, Box* wait_area, Box* response_area, float);
  void perform();

protected:

private:

  Unit* unit;
  Box* waitArea;
  Box* responseArea;
  float theta;
};


//------------------------------------------------------
// this is what you do when you might do opposed move
// or unopposed move

class OrderCUnitAcrossBox : public AAA::Action
{
public:
  //  OrderCUnitAcrossBox();
  ~OrderCUnitAcrossBox();
  OrderCUnitAcrossBox(CmndUnit*, Box *move, Box *end, 
		      float end_t, AAA::State *tls);
  void perform();

protected:

private:

  virtual void order_unopposed_move();

  // these were NOT carried over from ACP
  //
   virtual void order_opposed_move(float enemy_strength, AAA::GVector enemy_cg);
   virtual void order_right_hook(float enemy_strength, AAA::GVector enemy_cg);
   virtual void order_left_hook(float enemy_strength, AAA::GVector enemy_cg);

  CmndUnit* unit;
  Box *mv_bx;
  Box *end_bx;
  float e_time;
  AAA::State *higher_level_state;
};


// ------------------------------------------------------
// the end_t is the time by which the movement
// into the box should be ended
class OrderCUnitIntoBox : public AAA::Action
{
public:
  OrderCUnitIntoBox(CmndUnit* cu, Box *end, 
		      double end_t);

  ~OrderCUnitIntoBox();

  void perform();

protected:

private:

  // this matches subordinates to sub-boxes
  // of the end Box, and does orderSubAlongCorridor
  void orderUnopposedMove();

  // this creates a corridor appropriate to get
  // the subordinate into the box, and applies
  // it immediately
  void orderSubAlongCorridor(Unit* u, Box* ebx, double et);

  CmndUnit* cUnit;
  Box *eBox;
  double eTime;
};


// ------------------------------------------------------
// notice that sub-cp's have a definite tactical life, and
// when spawned should also have an absorbtion-time defined.
// otherwise, they would be part of a permanent C2 hierarchy,
// and not really temporary at all.

class SpawnSubCP : public AAA::Action
{

public:
  //  SpawnSubCP();
  ~SpawnSubCP();
  SpawnSubCP(CmndUnit *pcu, 
	     std::vector<Unit*> *utr,
	     CmndUnit *tcu);
  virtual void perform();

protected:

private:

CmndUnit *parent_unit;
  std::vector<Unit*> *units_to_reorg;
CmndUnit *tmp_cu;
};



// ------------------------------------------------------

class AbsorbSubCP : public AAA::Action
{

public:
  //  AbsorbSubCP();
  ~AbsorbSubCP();
  AbsorbSubCP(CmndUnit *pu, CmndUnit *su);
  virtual void perform();

protected:

private:
  CmndUnit *parent_unit;
  CmndUnit *sub_unit;
};


// ------------------------------------------------------

#endif

// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
