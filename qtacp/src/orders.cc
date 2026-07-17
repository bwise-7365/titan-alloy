// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: includes/usings, evector ->
// std::vector (pop_back split), DES::DESimulation -> ACPSim.
//
//-------------------------------------------------------------------------

#include "aaa.h"

// #include "frwrdec.h"
// #include "struct.h"
#include "fsm.h"
#include "orders.h"
#include "tthread.h"
#include "acpsim.h"
#include "mcontrol.h"

#include <vector>

extern ACPSim* theSim;

using AAA::Logical;
using AAA::LTrue;
using AAA::LFalse;
using AAA::LUnknown;

using AAA::GVector;
using std::vector;
using AAA::FSM;
using AAA::Predicate;
using AAA::Action;
using AAA::State;
using AAA::dround;

// ------------------------------------------------------


// UnitNearPoint::UnitNearPoint() : Predicate()
// {
// }

UnitNearPoint::~UnitNearPoint()
{
}

UnitNearPoint::UnitNearPoint(Unit *mu, GVector pnt, float d, bool mDP) : Predicate()
{
  force = mu;
  point = pnt;
  critical_dist = d;
  mapDistP = mDP;

  if (true == ACPSim::traceFSM) {
    cout << "Built a ";
    if (true == mapDistP)
      cout << "2D";
    else
      cout << "3D";
    
    cout << " test if unit " << force->getSimEntID();
    cout << " is within " << critical_dist;
    cout << " of the point " << point << endl;
  }
}

Logical
UnitNearPoint::test() {
  Logical rslt;
  GVector f_pos = force->currentPos();
  double dx = f_pos.get(0) - point.get(0);
  double dy = f_pos.get(1) - point.get(1);
  double dz = f_pos.get(2) - point.get(2);
  double dist2 = (dx*dx) + (dy*dy);
  double dist1 = 0.0;

  if (true == mapDistP) {
    dist1 = sqrt(dist2);
  }
  else {
    dist1 = sqrt(dist2 + (dz*dz));
  }
  if (dist1 <= critical_dist)
    rslt = LTrue;
  else
    rslt = LFalse;

  if (true == ACPSim::traceFSM) {
    cout << "Tested if unit " << force->getSimEntID() << " at point ";
    cout << f_pos << " is within ";
    if (true == mapDistP)
      cout << "2D ";
    else
      cout << "3D ";
    cout << critical_dist << " of the point " << point << endl;
    cout << "Result was " << rslt << endl;
  }
  return rslt;
}

// ------------------------------------------------------


UnitInArea::UnitInArea(Unit* u, Box* b) : Predicate() {
  force = u;
  area = new Box(b);
}

UnitInArea::~UnitInArea() {
  delete area;
  area = NULL;
  force = NULL;
}

Logical
UnitInArea::test() {
  //  XtTestApp* xta = ((XtTestApp*) theApp);
  //  double now = xta->acpsim->clock();
  double now = theSim->clock();
  assert (NULL != force);

  assert (NULL != area);
  assert (LTrue == area->check_integrity());

  Logical rslt;
  if (true == ACPSim::traceFSM) {
    cout <<  "At time " << now << " tested if unit " << force->getSimEntID() << endl;
    cout << flush;
  }

  GVector fp = force->currentPos();
  if (true == ACPSim::traceFSM) {
    cout << " at " << fp << " was in " << *area << endl;
    cout << flush;
  }

  rslt =  force->inAreaP(area);

  if (true == ACPSim::traceFSM) {
    cout << "Result was " << rslt << endl;
    cout << flush;
  }

  return rslt;
}

// ------------------------------------------------------

// EnemyInArea::EnemyInArea() : Predicate()
// {
// }


EnemyInArea::~EnemyInArea()
{
}


EnemyInArea::EnemyInArea(Unit* u, Box* b, float ms) : Predicate()
{
  unit = u;
  my_side = unit->side;     // look for units which oppose this side
  area = b;                 // look in this area
  assert (ms > 0);
  assert (unit != NULL);
  min_size = ms;            // return True if opponent bigger than this found
}

Logical
EnemyInArea::test() { 
  GVector mean_u;
  float opp_strength;
  Logical result = LFalse;

  opp_strength = 0.0;

  if (true == ACPSim::traceSensors) {
    cout << "Scanning for opponents of side " <<my_side;
    cout << " in area" << endl;
  }

  unit->enemyStrengthInArea(area , opp_strength, mean_u);
  if (true == ACPSim::traceSensors) {
    cout << "Enemy strength in box is " << opp_strength;
    cout << " while threshold is " << min_size << endl;
  }

  if (opp_strength >= min_size)
    {
      if (true == ACPSim::traceSensors) {
	cout << "Enemy CG is at " << mean_u << endl;
      }
      result = LTrue;
    }
  return result; 
}

// ------------------------------------------------------

// TimePassed::TimePassed()
// {
// }

TimePassed::~TimePassed()
{
}

TimePassed::TimePassed(float tm, ACPSim *rs)
{
  time = dround(tm, 3);
  reference_sim = rs;

  //    cout << "Built a test if time " << time << " has passed" << endl;
}

Logical
TimePassed::test()
{
  Logical rslt;
  if (reference_sim->clock() >= time)
    rslt = LTrue;
  else
    rslt = LFalse;

  if (true == ACPSim::traceFSM) {
    cout << "At time " << reference_sim->clock();
    cout << ", tested if time " << time << " has passed" << endl;
    cout << "Result was " << rslt << endl;
  }

  return rslt;
}


// ------------------------------------------------------

// OrderResUnitToPoint::OrderResUnitToPoint() : Action()
// {
//   unit = NULL;
//   point = GVector(0.0, 0.0, 0.0);
//   arrival_deadline = 0;
// }

OrderResUnitToPoint::~OrderResUnitToPoint()
{
}

OrderResUnitToPoint::OrderResUnitToPoint(ResUnit* ru, GVector p, float t) {
  assert (NULL != ru);
  unit = ru;
  point = p;
  arrival_deadline = dround(t, 3);
  if ( true == ACPSim::traceOrders) {
    cout << "Creating order for unit " << unit->getSimEntID();
    cout << " to go to point " << point;
    cout << " arriving at time " << arrival_deadline << endl;
  }
}


void
OrderResUnitToPoint::perform() {
  // do not set his objective.
  //  unit->set_dsrd_loc(point, arrival_deadline);

  if ( true == ACPSim::traceOrders) {
    cout << "Performing order to unit " << unit->getSimEntID();
    cout << " to go to point " << point;
    cout << " arriving at time " << arrival_deadline << endl;
  }
  //     cout << " (objective remains " << unit->objective << ")" << endl;

  // resunit should directly toward 'point'
  // 
  // should I create a 1-state FSM that creates a MovementRule,
  // or should I just create a MovementRule?
  // 
  if (NULL != unit->moveController) {
    delete unit->moveController;
    unit->moveController = NULL;
  }

  MovementRule* mc = new MRPoint(unit, point, unit->posTC, speedWP);
  unit->moveController = mc;

  assert (NULL != unit->moveController);

  // For logical consistency, the former seems better.


}

// ------------------------------------------------------

// OrderUnitToArea::OrderUnitToArea() : Action()
// {

//   unit = NULL;
//   area = NULL;
//   arrival_deadline = 0;
// }

OrderUnitToArea::OrderUnitToArea(Unit *u, Box *b, float t) : Action()
{
  unit = u;
  area = b;
  arrival_deadline = t;
}

void
OrderUnitToArea::perform() {
  // do not set his objective.
  if ( true == ACPSim::traceOrders) {
    cout << "Ordering unit " << unit->getSimEntID();
    cout << " to go to area " << area << "(not really)";
    //    cout << " (objective remains " << unit->objective << ")" << endl;
    // this was a no-op in the original ACP code ?!
    cout << "STUB function OrderUnitToArea::perform !"<<endl;
  }
  assert (LTrue == LFalse);
}

OrderUnitToArea::~OrderUnitToArea()
{
}
// ------------------------------------------------------

// OrderUnitAlongCorridor::OrderUnitAlongCorridor() : Action()
// {
// }


OrderUnitAlongCorridor::~OrderUnitAlongCorridor()
{
}


OrderUnitAlongCorridor::OrderUnitAlongCorridor(Unit *u, Corridor *c)  : Action() {
  //   int stuff;
  unit = u;
  corridor = c;

  if ( true == ACPSim::traceOrders) {
    cout << "Created OrderUnitAlongCorridor for unit " << unit->getSimEntID();
    cout << " and corridor " << *corridor << endl << flush;
  }
  //   cout << "Type any int to continue: ";
  //   cin >> stuff;
  tCorridor = NULL;
}

OrderUnitAlongCorridor::OrderUnitAlongCorridor(Unit *u, TempCorridor *tc)  : Action() {
  //   int stuff;
  unit = u;
  corridor = NULL;
  tCorridor = tc;

  if ( true == ACPSim::traceOrders) {
    cout << "Created OrderUnitAlongCorridor for unit " << unit->getSimEntID();
    cout << " and temp-corridor " << *tCorridor << endl << flush;
  }
}

void
OrderUnitAlongCorridor::perform() {

  assert (NULL == corridor);
  assert (NULL != tCorridor);
  FSM* fsm = unit->makeCorridorFSM(tCorridor);

  assert (NULL != fsm);
  unit->setFSM(fsm);

  // now,  deschedule any future events and force
  // the unit to do an update
  unit->descheduleNextEvent();
  unit->update();

  return;
}

// ------------------------------------------------------
//  The idea is to create an object, which at time of application,
// will plan from their THEN-CURRENT locations to the end
// of the box, then order everyone to do it.

// OrderCUnitAcrossBox::OrderCUnitAcrossBox() : Action()
// {
// }


OrderCUnitAcrossBox::~OrderCUnitAcrossBox()
{
}



OrderCUnitAcrossBox::OrderCUnitAcrossBox(CmndUnit* u2, Box *mv_bx2, 
					 Box *end_bx2, float e_time2,
					 State *hls)
  : Action()
{
  unit = u2;
  mv_bx = mv_bx2;
  end_bx = end_bx2;
  e_time = dround(e_time2, 3);
  higher_level_state = hls;


  //  XXXX temporary
  if (true == ACPSim::tracePlanning) {
    if (NULL == mv_bx2) {
      cout << " at time " << theSim->clock() << " creating OrderCUnitAcrossBox " << endl;
      cout << " over NULL move-box, with end time " << e_time << endl;
      cout << flush;
      cout << flush;
    }
  }

  if ( true == ACPSim::traceOrders) {
    cout << " at time " << theSim->clock() << " creating OrderCUnitAcrossBox " << endl;
    cout << " with end time " << e_time << endl;
    cout << " and end box " << *end_bx << endl;
    cout << endl << flush;
  }
}


// notice that we have, for now, taken the quick-and-dirty
// approach of simply testing a force ratio and branching
// to opposed or unopposed move. CCS is used in planning
// an opposed move, but not an unopposed move.
//
// CCS should be invoked at all three steps, not just the
// "plan opposed move" step.

void
OrderCUnitAcrossBox::perform() {
  GVector mean_u;
  float friendly_strength, opp_strength;
  Logical move_unopposed_p;

  opp_strength=0.0;
  move_unopposed_p = LTrue;

  if ( true == ACPSim::traceOrders) {
    cout << endl << "At time " << theSim->clock() << ":" << endl;
    cout << "CmndUnit " << unit->getSimEntID() << " is getting ready to cross";  

    if (mv_bx != NULL)
      cout << " box " << *mv_bx;
    else
      cout << " the null box";

    cout << " by time " << e_time << endl;
  }

  if (LTrue == unit->allowSubPlanning)  {

    if ( true == ACPSim::traceOrders) {
      cout << "Scanning unit "<<unit->getSimEntID()<<" is of side ";
      cout <<unit->side<<endl;
    }

    friendly_strength = unit->currentStrength();
    opp_strength = 0.0;
    mean_u = GVector(0,0,0);
    move_unopposed_p = LTrue;
    unit->enemyStrengthInArea(mv_bx , opp_strength, mean_u);
    if (10.0 * opp_strength > friendly_strength)
      move_unopposed_p = LFalse;

    if ( true == ACPSim::traceOrders) 
      cout << "Enemy strength in box is " << opp_strength << endl;

  }
  else  {
    if ( true == ACPSim::traceOrders) 
      cout << "Subplanning minimized...proceeding frontally"<< endl;
  }

  // ----

  switch (move_unopposed_p) {
  case LTrue: {
    if ( true == ACPSim::traceOrders) 
      cout << "Unopposed move indicated for unit "<<unit->getSimEntID()<<endl;

    this->order_unopposed_move();
    break;
  }

  case LFalse: {
    if ( true == ACPSim::traceOrders) 
      cout << "Opposed move indicated for unit "<<unit->getSimEntID()<<endl;

    // %%% strictly a test, which needs to be restored
    // this->order_opposed_move(opp_strength, mean_u);

    if (LFalse ==unit->allowManeuverPlanning) {
      move_unopposed_p = LTrue;
      if (true == ACPSim::traceOrders) {
	appBell();
	cout << "Opposed move maneuver planning is inhibited"<<endl;
	cout << "Proceeding as if unopposed"<<flush;
      }
      this->order_unopposed_move();
    }
    else {
      cout << "Opposed move being ordered by unit "<<unit->getSimEntID()<<endl;
      this->order_opposed_move(opp_strength, mean_u);
    }
    break;
  }

  case LUnknown: // not possible, but satisfy compiler
    assert(LUnknown != move_unopposed_p);
    break;
  }

  return;
}

// ------------------------------------------------------

// SpawnSubCP::SpawnSubCP() : Action()
// {
// }

SpawnSubCP::~SpawnSubCP()
{
}

SpawnSubCP::SpawnSubCP(CmndUnit *pcu, 
		       vector<Unit*> *utr,
		       CmndUnit *tcu) : Action()
{
  parent_unit = pcu;
  units_to_reorg = utr;
  tmp_cu = tcu;
}


void
SpawnSubCP::perform()
{
  parent_unit->spawn_sub_cp(tmp_cu, units_to_reorg);
  return;
}

// ------------------------------------------------------

// AbsorbSubCP::AbsorbSubCP() : Action()
// {
// }


AbsorbSubCP::~AbsorbSubCP()
{
}

AbsorbSubCP::AbsorbSubCP(CmndUnit *pu, CmndUnit *su) : Action()
{
  parent_unit = pu;
  sub_unit = su;
}

void
AbsorbSubCP::perform()
{
  parent_unit->absorb_sub_cp(sub_unit);
  return;
}

// ------------------------------------------------------

// OrderResUnitEngageEnemyInArea::OrderResUnitEngageEnemyInArea() : Action()
// {
//   unit = NULL;
//   area = NULL;
// }

OrderResUnitEngageEnemyInArea::~OrderResUnitEngageEnemyInArea()
{
}

OrderResUnitEngageEnemyInArea::OrderResUnitEngageEnemyInArea(ResUnit* ru, 
							     Box* a) : Action()
{
  unit = ru;
  area = a;
}

void
OrderResUnitEngageEnemyInArea::perform()
{
  float perceived_strength;
  GVector enemy_cg;
  unit->enemyStrengthInArea(area, perceived_strength, enemy_cg);
  assert(perceived_strength > 0);  // it better be!
  assert (enemy_cg.get(0) >= 0.0); // on terrain
  assert (enemy_cg.get(1) >= 0.0); // on terrain
  // any time in past (e.g.  zero) means ASAP
  //  unit->set_dsrd_loc(enemy_cg, 0.0); 

  if ( true == ACPSim::traceOrders) {
    cout << "Ordering unit " << unit->getSimEntID();
    cout << " to attack enemy_cg at " << enemy_cg;
    //    cout << " (objective remains " << unit->objective << ")" << endl;
  }
  return;
}


// ------------------------------------------------------

// OrderUnitDefendAreas::OrderUnitDefendAreas() : Action()
// {
// }

OrderUnitDefendAreas::~OrderUnitDefendAreas()
{
}

OrderUnitDefendAreas::OrderUnitDefendAreas(Unit* u,
					   Box* wait_area, 
					   Box* response_area, float t) : Action()
{
  unit = u;
  waitArea = wait_area;
  responseArea = response_area;
  theta = t;

  // %%% The show-boxes should be fixed
  //   yacpApp* ya = (yacpApp*) theApplication;
  //   if (1 == ya->showBoxes) {
  //     theSim->boxes->push_back(wait_area);
  //     theSim->boxes->push_back(response_area);
  //   }

}

void
OrderUnitDefendAreas::perform()
{
  FSM *new_fsm;
  new_fsm = unit->makeDefendAreasFSM(waitArea,responseArea, theta);
  unit->setFSM(new_fsm);
  return;
}


// ------------------------------------------------------

OrderCUnitIntoBox::OrderCUnitIntoBox(CmndUnit* cu,
				     Box *endBox, 
				     double endTime) {
  cUnit = cu;
  eBox = new Box(endBox);
  eTime = endTime;
  assert (NULL != cUnit);
  assert (NULL != eBox);
  assert (LTrue == eBox->check_integrity());

  if (true == ACPSim::tracePlanning) {
    cout << "Creating OrderCUnitIntoBox for CmndUnit ";
    cout << cUnit->getSimEntID();
    cout << ", box "<<(*eBox);
    cout << " by time "<<eTime;
    cout<<endl<<flush;
  }
}


OrderCUnitIntoBox::~OrderCUnitIntoBox() {
  delete eBox;
  eBox = NULL;
  cUnit = NULL;
  eTime = 0.0;
}

void OrderCUnitIntoBox::perform() {
  orderUnopposedMove();
  return;
}

void OrderCUnitIntoBox::orderUnopposedMove() {
  //  cout << "Entering OrderCUnitIntoBox::orderUnopposedMove"<<endl<<flush;
  const unsigned int numLS = cUnit->numLiveSubs();
  unsigned int i = 0;
  unsigned int n = 0;
  Unit* sub = NULL;
  Formation* form = new Formation();
  Box* sub_bx = NULL;
  GVector *center = NULL;


  vector<Box*>* sub_end_boxes = cUnit->compute_formation(eBox, StandardFormation);
  assert (sub_end_boxes->size() == numLS);

  n = sub_end_boxes->size();
  for (i=0; i<n; i++) {
    sub_bx = (*sub_end_boxes)[i];
    center = new GVector(sub_bx->center.get(0),
			 sub_bx->center.get(1),
			 0.0);
    form->add_point(center);
  }

  n = cUnit->subordinates->size();
  for (i=0; i<n; i++) {
    sub = (*(cUnit->subordinates))[i];
    assert (sub != NULL);
    if (true == ACPSim::traceGeometry) {
      cout << "Trying to add unit "<<i<<" of "<<n<<" to formation"<<endl<<flush;
    }
    if (LTrue == sub->aliveP)
      form->add_unit(sub);
  }

  if (true == ACPSim::traceGeometry) {
    cout << "Trying to match units to formation"<<endl<<flush;
  }
  form->match_units();
  // at this point, all the units are matched up with the
  // final boxes they should go to. That is, the matched_units
  // in the formation are in one-to-one correspondance
  // with the boxes in sub_end_boxes!

  vector<Unit*> *matched_subs = form->get_assigned_units();
  //  assert (matched_subs->size() == unit->subordinates->size());
  assert (matched_subs->size() == numLS);


  n = sub_end_boxes->size();
  for (i=0; i<n; i++) {
    sub_bx = (*sub_end_boxes)[i];
    sub = (*matched_subs)[i];

    // we want to order this sub to get into that box.
    if (true == ACPSim::tracePlanning) {
      cout <<"    -  -  -  -" <<endl;
      cout <<"Unit " <<cUnit->getSimEntID() <<" is planning-2 for subordinate ";
      cout <<sub->getSimEntID() <<" to get into box:" <<endl <<flush;
      cout <<(*sub_bx) <<endl <<flush;
      // XXXX just a place to hook a breakpoint
      cout <<flush;
    }

    orderSubAlongCorridor(sub, sub_bx, eTime);
  }


  while (sub_end_boxes->size() > 0) {
    sub_bx = sub_end_boxes->back();
    sub_end_boxes->pop_back();
    delete sub_bx;
    sub_bx = NULL;
  }
  delete sub_end_boxes;
  sub_end_boxes = NULL;

  delete matched_subs;
  matched_subs = NULL;

  // %%% can we do this?
  delete form;
  form = NULL;
  // apparently only some parts of the formation
  // can be deleted, and others not

  //  cout << "Leaving OrderCUnitIntoBox::orderUnopposedMove"<<endl<<flush;
  return;
}


// ------------------------------------------------------
// There are several ways this can work out.
// Ideally, you would like to slide the goal box
// onto the unit to get a start box, average
// the two to get a middle box, and have the 
// unit make a corridor FSM from middle to goal box.
//
// (A) if the unit's CG and the goal box are very close
// together, the start box can overlap the goal box.
//
// (B) if the unit'CG and the goal box are fairly close
// together, the middle box can overlap the goal box,
// or the start box, or both
//
// (C) the unit's CG can be inside the goal box. However,
// it may be a CmndUnit that is not "inside" the box until
// all of its subordinates are in the box.
//
// In a sense, these problems are all symptoms of the problem
// that the goal box is too large relative to the distance
// from the unit CG to the edge of the goal box.
// We can not arbitrarily move the unit CG, and shrinking
// the start box only works sometimes. It is possible
// to shrink the start box to zero size, and still have
// the middle box overlap the goal, if the unit CG is less
// than half a box width outside the box.
// 
// The only general fix is to shrink the goal box
// until we are back in the ideal situation.


void OrderCUnitIntoBox::orderSubAlongCorridor(Unit* su, Box* ebx, double et) {
  double now = theSim->clock();

  unsigned int contractionCount = 0;
  Box* eBox2 = new Box(ebx);
  GVector shiftVctr = su->currentPos() - ebx->center;
  Box* tmpMBox = ebx->shift(shiftVctr/2.0);
  Box* sBox = ebx->shift(shiftVctr);

  Box* mBox = tmpMBox->rectangularize();
  delete tmpMBox;
  tmpMBox = NULL;

  double dt = et - now;
  double t0 = now;
  if (dt < 0.0) { // we are behind
    t0 = now;
    et = now - (dt / 100.0); // we hope this is only SLIGHTLY in the future
  }

  // the corridor times below interpolate between t0 and et, and
  // TempCorridor stores them as float. a deadline at or barely past
  // 'now' collapses the schedule so far that consecutive start times
  // round to equal floats, and checkTCorridor asserts on its strict
  // time ordering (the "occasionally ==" case its comment leaves
  // open). enforce a minimum spread so the degenerate schedule
  // cannot arise.
  const double minSpread = 1.0; // seconds
  if (et < (t0 + minSpread))
    et = t0 + minSpread;

  Logical smP = box_intersect_p(sBox, mBox);
  Logical meP = box_intersect_p(mBox, eBox2);
  Logical seP = box_intersect_p(sBox, eBox2);
  bool overlapP = ((LTrue == smP) || (LTrue == meP) || (LTrue == seP));

  // quickly or slowly, they will develop gaps
  while (true == overlapP) {
    sBox->d_expand(0.8);
    mBox->d_expand(0.8);
    eBox2->d_expand(0.8);
    contractionCount++;

    if (true == ACPSim::traceGeometry) {
      cout << " Contracted " <<contractionCount<<" times"<<endl<<flush;
    }
    overlapP = false;
    smP = box_intersect_p(sBox, mBox);
    if (LTrue == smP) 
      overlapP = true;
    else {
      meP = box_intersect_p(mBox, eBox2);
      if (LTrue == meP)
	overlapP = true;
	else {
	  seP = box_intersect_p(sBox, eBox2);
	  if (LTrue == seP)
	    overlapP = true;
	}
    }
  }



  TempCorridor *tc = createTempCorridor(sBox, t0, ((0.99*t0) + (0.01*et)));
  extendTempCorridor(tc, mBox, ((0.55*t0) + (0.45*et)), ((0.52*t0 + 0.48*et)));
  extendTempCorridor(tc, eBox2, ((0.01*t0) + (0.99*et)), et);

  FSM* fsm = su->makeCorridorFSM(tc);

  // %%% dogdog can these be deleted?
  delete sBox;
  sBox = NULL;
  delete mBox;
  mBox = NULL;
  delete eBox2;
  eBox2 = NULL;

  delete tc;
  tc = NULL;

  if (true == ACPSim::traceOrders) {
  cout << "At time " << now << " Unit "<<su->getSimEntID() << " gets FSM reset to FSM ";
  cout << fsm->getID() << endl<<flush;
  }
  su->setFSM(fsm);

  return;
}

// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
