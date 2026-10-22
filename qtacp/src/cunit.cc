// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: include list, evector -> std::vector,
// inListP -> std::find.
// ------------------------------------------

// ------------------------------------------

#include "frwrdec.h"
#include "struct.h"

#include "cunit.h"
#include "tthread.h"
#include "orders.h"


#include "acpsim.h"
#include "subsa.h"

#include <vector>
#include <algorithm>

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


// ------------------------------------------

class ResUnit;

// seconds (layered cu scenario is rather insensitive to this
double CmndUnit::maxUpdateInterval = 3 * 60; 

// ------------------------------------------

CmndUnit::CmndUnit(ACPSim* sm, Alignment a, GVector initPos, GVector initVel) : Unit(sm) {
  initialize();
  side = a;
  sm->addCmndUnit( this);
  tgrid = sm->tgrid;

  assert (NULL != sim);
  // be careful to create it at the correct time,
  // or reset it.
  birthTime = sim->clock();

  assert(NULL != tgrid);
  return;
}


CmndUnit::~CmndUnit()
{
  // do nothing;
  if (NULL != subordinates)
    delete subordinates; // just the list, not the contents
  subordinates = NULL;


  return;
}


void
CmndUnit::initialize() {
  // do noting;
  maxSpeed = 10.0; // meters per second
  subordinates = NULL;
  wayPoint = GVector(0,0,0);
  
  cmnd_veh = NULL;
  assert (NULL != sim);
  maxUpdateInterval = CmndUnit::maxUpdateInterval;
  return;
}

// this is the event function!void 
void
CmndUnit::update() {
  assert (NULL != sim);
  double waitInterval = sim->rng->uniform(0.25, 0.33) * maxUpdateInterval;  
  double now = sim->clock();
  ACPSimEvent *sse;
  if (true == ACPSim::tracePlanning) {
    cout << endl;
    cout << side << " CmndUnit " << simEntID << " update at time " << now;
    cout << endl << flush;
    if (LTrue != aliveP)
      cout << "CmndUnit " << simEntID <<" is dead. No more processing."<<endl<<flush;
  }
  if (LTrue == aliveP) {

    if (true == ACPSim::traceSensors) {
      cout << side << " CmndUnit " << simEntID << " updating friendly SSA at "<<now;
      cout<<endl<<flush;    
    }
    updateFriendlySSA(false);
    if (true == ACPSim::traceSensors) {
      cout << side << " CmndUnit " << simEntID << " is tracking " << ssa->friendly->size() << " friendlies";
      cout << endl<< flush;    
    }

    if (true == ACPSim::traceSensors) {
      cout << side << " CmndUnit " << simEntID << " updating enemy SSA at "<<now;
      cout<<endl<<flush;    
    }
    updateEnemySSA(false);
    if (true == ACPSim::traceSensors) {
      cout << side << " CmndUnit " << simEntID << " is tracking " << ssa->enemy->size() << " enemies";
      cout << endl<< flush;    
    }


    if (NULL == current_fsm) {
      if (true == ACPSim::tracePlanning) {
// 	cout << side << " CmndUnit " << simEntID << " has no FSM at time ";
// 	cout << now << endl << flush;
      }
    }
    if (NULL != current_fsm) {
      if (true == ACPSim::tracePlanning) {
// 	cout << "CmndUnit " << simEntID << " has FSM "<<current_fsm->getID();
// 	cout << " at time " << now << endl << flush;
      }
      current_fsm->execute();
    }
    sse = new ACPSimEvent(theSim, SSCmndUnitUpdate);
    assert (NULL != sse);
    sse->data = ( this);
    sse->processFN = updatePV;

    if (true == ACPSim::tracePlanning) {
//       cout << "Scheduling CmndUnit " << simEntID << " for event at ";
//       cout << now + waitInterval << endl;
    }
    //   sim->schedule(now + waitInterval, sse);
    scheduleNextEvent(now + waitInterval, sse);
  }
  return;
}


void 
CmndUnit::makePVCurrent()
{
  // should aggregate from subordinates, or look up old
  return;
}

void  CmndUnit::updateFriendlySSA(bool recursiveP) {
  unsigned long int i = 0;
  unsigned long int j = 0;
  unsigned long int n = subordinates->size();
  unsigned long int m = 0;
  Unit* sub = NULL;
  ResUnit* fu = NULL;
  vector<ResUnit*> *fuList = NULL;

  ssa->clearFriendly();
  
  for (i=0; i<n; i++) {
    sub = (*subordinates)[i];
    if (recursiveP)
      sub->updateFriendlySSA(recursiveP);

    if (LTrue == sub->aliveP) {
      fuList = sub->ssa->friendly;
      m = fuList->size();
      for (j=0; j<m; j++) {
	fu = (*fuList)[j];

	assert (side == fu->side);

	ssa->mergeFriendly(fu);
      }
    }
  }
  return;
}

void CmndUnit::updateEnemySSA(bool recursiveP) {
  unsigned long int i = 0;
  unsigned long int j = 0;
  unsigned long int n = subordinates->size();
  unsigned long int m = 0;
  Unit* sub = NULL;
  ResUnit* eu = NULL;
  vector<ResUnit*> *euList = NULL;

  ssa->clearEnemy();
  
  for (i=0; i<n; i++) {
    sub = (*subordinates)[i];
    if (recursiveP)
      sub->updateEnemySSA(recursiveP);

    if (LTrue == sub->aliveP) {
      euList = sub->ssa->enemy;
      m = euList->size();
      for (j=0; j<m; j++) {
	eu = (*euList)[j];
	assert (LTrue == opposedByP(eu->side, side));
	ssa->mergeEnemy(eu);
      }
    }
  }
  return;
}

vector<Unit*> *
CmndUnit::nthSubordinates(unsigned long int n) {
  unsigned long int i = 0;
  unsigned long int j = 0;
  unsigned long int nSubs = subordinates->size();
  vector<Unit*> *nSubList = NULL;
  Unit* u1 = NULL;
  Unit* u2 = NULL;
  //  evector<Unit*> *subs = NULL;
  unsigned long int nSubSubs = 0;

  if (0 == n) {
    nSubList = NULL;
  }
  else {
    // n >= 1
    nSubList = new vector<Unit*>();
    if (1 == n) {
      for (i=0; i<nSubs; i++) {
	u1 = (*subordinates)[i];
	nSubList->push_back(u1);
      }
    }
    else { // n > 1
      for (i=0; i<nSubs; i++) {
	u1 = (*subordinates)[i];
	nSubList = u1->nthSubordinates(n-1);
	if (NULL != nSubList) {
	  nSubSubs = nSubList->size();
	  for (j=0; j<nSubSubs; j++) {
	    u2 = (*nSubList)[j];
	    nSubList->push_back(u2);
	  }
	  delete nSubList;
	  nSubList = NULL;
	}
      }
    }
    
  }
  return nSubList;
}



float
CmndUnit::currentStrength()
{
  float rslt = 0.0;
  Unit *su;
  unsigned int i = 0;
  unsigned int n = subordinates->size();
  for (i=0; i<n; i++) {
    su = (*subordinates)[i];
    assert (NULL != su);
    rslt = rslt + su->currentStrength();
  }
  return rslt;
}


Logical
CmndUnit::inAreaP(Box* b) {
  //  cout << "CmndUnit "<<simEntID<<" checking if it is in "<<(*b)<<endl<<flush;

  assert (NULL != subordinates);

  Logical rslt = LTrue;
  Logical subRslt;
  GVector subPos;
  Unit *su;
  unsigned int i = 0;
  unsigned int n = subordinates->size();

  assert ( n > 0 );
  assert (NULL != b);
  subPos = currentPos();

  subRslt = b->insideP(subPos);
  rslt = subRslt;

  for (i=0; i<n; i++) {
    su = (*subordinates)[i];
    subRslt = su->inAreaP(b);
    if (LTrue != subRslt)
      rslt = LFalse;
  }
  cout << flush;
  return rslt;
}


GVector 
CmndUnit::currentPos() {
  GVector nuPos = GVector(0,0,0);
  GVector subPos;
  Unit *su;
  unsigned int i = 0;
  unsigned int n = subordinates->size();
  double m = 0.0;

  if (true == ACPSim::traceMoves) {
    cout << "At time " << sim->clock() << " CmndUnit " << simEntID << " has " << n << " subs: " << endl;
  }
  for (i=0; i<n; i++) {
    su = (*subordinates)[i];
    assert (NULL != su);
    if (LTrue == su->aliveP) {
      subPos = su->currentPos();
      if (true == ACPSim::traceMoves){
	//	cout << "   live subordinate " << su->getSimEntID() << " is centered at " << subPos << endl;
      }
      nuPos = nuPos + subPos;
      m = m + 1.0;
    }
  }

  // if absolutely all subordinates are dead, then
  // the command unit should have stopped running. But the
  // GUI may interrogate it for position anyway.
  if (0 == m) {
    nuPos = p0;
    if (true == ACPSim::traceMoves)
      cout << "CmndUnit " << simEntID << " is DEAD and centered at pos " << nuPos << endl;
    assert (LFalse == aliveP);
  }
  else {
    nuPos = nuPos / m;
    if (true == ACPSim::traceMoves)
      cout << "CmndUnit " << simEntID << " is alive and centered at pos " << nuPos << endl;
  }
  return nuPos;
}

int 
CmndUnit::numLiveSubs() {
  unsigned int i = 0;
  unsigned int numSubs = subordinates->size();
  unsigned int m = 0;
  Unit *su = NULL;
  for (i=0; i<numSubs; i++) {
    su = (*subordinates)[i];
    assert (NULL != su);
    if (LTrue == su->aliveP) {
      m = m + 1;
    }
  }
  return m;
}


GVector 
CmndUnit::currentVel() {
  GVector nuVel = GVector(0,0,0);
  Unit *su;
  unsigned int i = 0;
  unsigned int n = subordinates->size();
  for (i=0; i<n; i++) {
    su = (*subordinates)[i];
    assert (NULL != su);
    nuVel = nuVel + su->currentVel();
  }
  nuVel = nuVel / ((double) n);
  return nuVel;
}


void 
CmndUnit::perturbPosition(double amount) {  // in meters
  unsigned int i = 0;
  unsigned int n = subordinates->size();
  Unit *su;
  for (i=0; i<n; i++) {
    su = (*subordinates)[i];
    assert (NULL != su);
    su->perturbPosition(amount);
  }
  return;
}

void 
CmndUnit::resetPV(GVector p, GVector v)
{
  p0 = p;
  v0 = v;
  t0 = sim->clock();
  return;
}


void 
CmndUnit::die() {
  Unit::die();
  return;
}
// ------------------------------------------

void 
CmndUnit::add_sub(Unit *u) {
  assert (NULL != u);
  if (subordinates==NULL) {
    subordinates = new vector<Unit*>();
    assert (NULL != subordinates);
  }

  assert (0 == hasSubP(u));
  assert (NULL == u->superior);
  u->superior = this;
  subordinates->push_back( u);

  if (true == ACPSim::tracePlanning) {
    cout << "CmndUnit " << simEntID << " added subordinate " << u->getSimEntID() << endl;
    cout << "Subordinate "<< u->getSimEntID()<<" is centered at "<<u->currentPos() << endl<<flush;
    cout << "CmndUnit " << simEntID << " now has " << subordinates->size() << " subordinates" << endl << flush;
    cout << "CmndUnit " << simEntID << " is now centered at  " << currentPos()<<endl<<flush;
    cout << flush;
  }

  assert (1 == hasSubP(u));
  return;
}

int
CmndUnit::hasSubP(Unit *u) {
  assert (NULL != u);
  int foundP = 0;
  Unit *su;
  unsigned int i = 0;
  unsigned int n = subordinates->size();
  for (i=0; i<n; i++) {
    su = (*subordinates)[i];
    assert (NULL != su);
    if (u == su)
      foundP = 1;
  }
  return foundP;
}

void 
CmndUnit::remove_sub(Unit *u) {
  assert (NULL != u);
  assert (NULL != subordinates);
  assert (this == u->superior);
  //  NodeList* nuSubs = new NodeList();
  vector<Unit*> *nuSubs = new  vector<Unit*>();

  assert (NULL != nuSubs);
  Unit *su;
  assert (1 == hasSubP(u));
  unsigned int i = 0;
  unsigned int n = subordinates->size();
  for (i=0; i<n; i++) {
    su = (*subordinates)[i];
    assert (NULL != su);
    if (u != su)
      nuSubs->push_back(su);
  }

  delete subordinates;
  subordinates = NULL;

  subordinates = nuSubs;

  if (true == ACPSim::tracePlanning) {
    cout << "CmndUnit " << simEntID << " removed subordinate " << u->getSimEntID() << endl;
    cout << "CmndUnit " << simEntID << " now has " << subordinates->size() << " subordinates" << endl << flush;
  }

  return;
}


// ------------------------------------------


// makeCorridorFSM is the top-level function
// that basically decides to move from area_box to area_box,
// planning the next move as it finishes the current.
// dispatches to CCS, etc.

// the "action" of an OrderUnitAlongCorridor(unit, corr)
// is to just have that unit->makeCorridorFSM(corr),
// so only the CmndUnit version matters here.

FSM* 
CmndUnit::makeCorridorFSM(TempCorridor* corr) {
  FSM *fsm = NULL;
  unsigned int i = 0;
  const unsigned int numBoxes = corr->boxes->size();
  ACPSim* aSim = ((ACPSim*) sim);
  if (true == ACPSim::tracePlanning) {
    cout << "CmndUnit "<<simEntID<<" is building FSM for TempCorridor of length "<< numBoxes;
    cout << endl << flush;
  }

  checkTCorridor(corr);
  Box* bx1 = NULL;
  Box* bx2 = NULL;
  Box* rbx1 = NULL;
  Box* rbx2 = NULL;
  Box* goalBox = NULL;
  Box* testBox = NULL;
  State* stateInitial = NULL;
  State* curr_st = NULL;
  State* prev_st = NULL;
  State* stateFinal = NULL;
  Conjunction *stest = NULL;
  Predicate *stest1 = NULL;
  Predicate *stest2 = NULL;
  Action *action = NULL;

  vector<State*> *states = new vector<State*>();

  fsm = new FSM();
  assert (NULL != fsm);

  // setup first state
  // this just immediately orders them
  // to get into the first box
  stateInitial = new State();
  fsm->addState(stateInitial);

  fsm->setState(stateInitial);

  for (i=0; i<numBoxes; i++) {
    curr_st = new State();
    fsm->addState(curr_st);
    states->push_back(curr_st);
  }

  stateFinal = new State();
  fsm->addState(stateFinal);


  curr_st = (*states)[0];
  stest1 = new AlwaysTrue();
  // remember, OrderCUnitIntoBox copies the box
  action = new OrderCUnitIntoBox(this, (*(corr->boxes))[0], (*(corr->sTimes))[0]);
  assert (NULL != action);
  stateInitial->addTransition(stest1, action, curr_st);

  testBox = (*(corr->boxes))[0];
  for (i=1; i<numBoxes; i++) {
    // stest1: in Box(i-1) and eT(i-1) passed?
    bx1 = (*(corr->boxes))[i-1];
    bx2 = (*(corr->boxes))[ i ];
    rbx1 = NULL;
    rbx2 = NULL;
    regularizeBoxPair(bx1, bx2, rbx1, rbx2);
    // we do not use rbx1, but we need
    // not delete and clear it at it is on
    // sim->boxes
    //delete rbx1;
    //rbx1 = NULL;
    aSim->boxes->push_back(rbx1);
    aSim->boxes->push_back(rbx2);
    stest1 = new TimePassed( (*(corr->eTimes))[i-1], sim);
    // UnitInArea makes its own local copy
    //    stest2 = new UnitInArea( this, bx1);
    stest2 = new UnitInArea( this, testBox);
    stest = new Conjunction();
    stest->addPred(stest1);
    stest->addPred(stest2);

    // action: into Box(i) by sT(i)
    // remember, OrderCUnitIntoBox copies the box
    if (i > 2) {
    action = new OrderCUnitIntoBox(this, rbx2,  (*(corr->sTimes))[i]);
    testBox = rbx2;
    }
    else {
     action = new OrderCUnitIntoBox(this, bx2,  (*(corr->sTimes))[i]);
     testBox = bx2;
    }

    prev_st = (*states)[i-1];
    curr_st = (*states)[i];
    prev_st->addTransition(stest, action, curr_st);
  }

  // stest1: in Box[numBoxes-1]?
  // action: NULL;
  // UnitInArea makes its own copy
  stest1 = new UnitInArea( this, (*(corr->boxes))[numBoxes-1]);
  action = NULL;
  prev_st = (*states)[numBoxes-1];
  prev_st->addTransition(stest1, NULL, stateFinal);

  assert (NULL != fsm);
  delete states;
  states = NULL;
  return fsm;
}


void CmndUnit::checkTCorridor(TempCorridor *corr) {
  unsigned int i = 0;
  assert (NULL != corr);

  const unsigned int numBoxes = corr->boxes->size();

  float st1 = 0.0;
  float et1 = 0.0;
  float st2 = 0.0;
  float et2 = 0.0;


  Box *bx1 = NULL;
  Box *bx2 = NULL;

  for (i=1; i<numBoxes; i++) {
    bx1 = (*(corr->boxes))[i-1];
    st1 = (*(corr->sTimes))[i-1];
    et1 = (*(corr->eTimes))[i-1];

    bx2 = (*(corr->boxes))[i];
    st2 = (*(corr->sTimes))[i];
    et2 = (*(corr->eTimes))[i];

    assert (NULL != bx1);
    assert (LTrue == bx1->check_integrity());

    assert (NULL != bx2);
    assert (LTrue == bx2->check_integrity());

    assert (LFalse == box_intersect_p(bx1, bx2));

    assert (st1 <= et1);
    assert (et1 <= st2);
    assert (st2 <= et2);

    // these can occaisonally be ==
    // and I'm not quite sure what to do
    // about those cases
    assert (st1 < st2);
    assert (et1 < et2);


  }

  if (true == ACPSim::tracePlanning) {
    cout << "CmndUnit "<<simEntID<<" OK's structure of TempCorridor";
    cout << endl << flush;
  }


  return;
}


FSM* CmndUnit::makeNCW1FSM() {
  // %%% no-op
  return NULL;
}



// ------------------------------------------

vector<Box*>* CmndUnit::compute_formation(Box* end_bx, formation_type ft) {
  assert (NULL != end_bx);
  vector<Box*> *sub_end_boxes;
  int i=0;
  Box *sub_bx;
  GVector d1, d2, d3, d4;
  GVector alpha, beta, cp_goal;
  GVector end_a, end_b, end_c, end_d;
  GVector tmp_a, tmp_b, tmp_c, tmp_d;

  int numLS=numLiveSubs();
  int num_front, num_rsrv;
  

  if (true == ACPSim::tracePlanning) {
    cout << "CmndUnit "<<simEntID<<" will compute standard formation for ";
    cout << numLS << " subs in "<<(*end_bx)<<endl;
    cout << flush;
  }

  // lay out a simple formation filling the end box,
  // considering only maneuver units. future formations
  // should consider ALL units (sensors, CP, arty, etc.)
  assert (StandardFormation == ft);

  // note that if we have 1 or 2 units, it
  // will choose 0 reserves and will place
  // them line-abreast up front.
  // total   front   rsrv
  // 1          1          0
  // 2          2          0
  // 3          2          1
  // 4          3          1
  // 5          3          2
  // and so on
  num_rsrv = ((int)(numLS * 0.4));
  num_front = numLS - num_rsrv;

  //  sub_end_boxes = new NodeList(); // data of tyep Box*
  sub_end_boxes = new vector<Box*>();
  
  assert (NULL != sub_end_boxes);

  
  end_a = end_bx->get_A();
  end_b = end_bx->get_B();
  end_c = end_bx->get_C();
  end_d = end_bx->get_D();
  cp_goal = (((end_a * 5) + end_b) + ((end_d * 5) + end_c))/12;

  if (1 == numLS) { // possible, after attrition!
    sub_bx = new Box(end_a, end_b, end_c, end_d);
    assert (NULL != sub_bx);
    sub_end_boxes->push_back(sub_bx);
  }
  if (2 == numLS) {
    tmp_a = end_a;
    tmp_b = end_b;
    tmp_c = (end_c + end_b)/2.0;
    tmp_d = (end_a + end_d)/2.0;
    sub_bx = new Box(tmp_a, tmp_b, tmp_c, tmp_d);
    assert (NULL != sub_bx);
    sub_end_boxes->push_back(sub_bx);
    tmp_a = (end_a + end_d)/2.0;
    tmp_b = (end_c + end_b)/2.0;
    tmp_c = end_c;
    tmp_d = end_d;
    sub_bx = new Box(tmp_a, tmp_b, tmp_c, tmp_d);
    assert (NULL != sub_bx);
    sub_end_boxes->push_back(sub_bx);
  }
  if (2 < numLS) {
    alpha = (end_c + end_d)/2.0;
    beta  = (end_a + end_b)/2.0;
  
    d1 = (end_b - end_c)/num_front;
    d2 = (beta  - alpha)/num_front;

    d3 = (beta  - alpha)/num_rsrv;
    d4 = (end_a - end_d)/num_rsrv;

    for (i=0; i<num_front; i++) {
      tmp_a = alpha + d2*(i+1);
      tmp_b = end_c + d1*(i+1);
      tmp_c = end_c + d1*i;
      tmp_d = alpha + d2*i;
      sub_bx = new Box(tmp_a, tmp_b, tmp_c, tmp_d);
      assert (NULL != sub_bx);
      sub_end_boxes->push_back(sub_bx);
    }

    for (i=0; i<num_rsrv; i++) {
      tmp_a = end_d + d4*(i+1);
      tmp_b = alpha + d3*(i+1);
      tmp_c = alpha + d3*i;
      tmp_d = end_d + d4*i;
      sub_bx = new Box(tmp_a, tmp_b, tmp_c, tmp_d);
      assert (NULL != sub_bx);
      sub_end_boxes->push_back(sub_bx);
    }
  }

  return sub_end_boxes;
}

// ------------------------------------------

void
CmndUnit::spawn_sub_cp(CmndUnit *new_sub_cp,
		       //		       NodeList *transfer_list // <Unit*>
		       vector<Unit*> *transfer_list
		       )
{
  assert (NULL != new_sub_cp);
  assert (NULL != transfer_list);
  //  NodeList *my_new_subs = new NodeList(); // <Unit*>
  vector<Unit*> *my_new_subs = new vector<Unit*>(); // <Unit*>

  assert (NULL != my_new_subs);
  Unit *sub;

  cout << "Cmnd Unit " << simEntID << " spawning new_sub-cp ";
  cout << new_sub_cp->getSimEntID() << endl;

  my_new_subs->push_back(new_sub_cp);

  unsigned int i = 0;
  unsigned int n = subordinates->size();

  for (i=0; i<n; i++) {
    sub = (*subordinates)[i];
    assert (NULL != sub);

    if (true == (transfer_list->end() != std::find(transfer_list->begin(), transfer_list->end(), sub))) // was in transfer list
      new_sub_cp->add_sub(sub);
    else // was not in transfer list
      my_new_subs->push_back(sub);
  }

  // now my subordinate list is split into two parts:
  // the new_sub_cp's subordinates, and my_new_subs

  delete subordinates; // wipe out old list structure
  subordinates = my_new_subs;

  new_sub_cp->makePVCurrent();

  return;
}


// ------------------------------------------

void
CmndUnit::absorb_sub_cp(CmndUnit* sub_cp)
{
  Unit *sub;
  cout << "Cmnd Unit " << simEntID << " absorbing sub-cp ";
  cout << sub_cp->getSimEntID() << endl;

  assert (NULL != sub_cp); 
  assert (true == (subordinates->end() != std::find(subordinates->begin(), subordinates->end(), sub_cp))); // it'd better be own subordinate!
  unsigned int i = 0;
  unsigned int n = sub_cp->subordinates->size();

  for (i=0; i<n; i++) {
    sub = (*(sub_cp->subordinates))[i];
    assert (NULL != sub);

    // his sub can't have been my sub simultaneoulsy
    assert (false == (subordinates->end() != std::find(subordinates->begin(), subordinates->end(), sub)));
    subordinates->push_back(sub);

  }
  return;
}

// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
