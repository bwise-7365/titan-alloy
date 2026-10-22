// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------


#ifndef BPW_CMND_UNIT_DEFEND_CC
#define BPW_CMND_UNIT_DEFEND_CC

// ------------------------------------------------------
#include "aaa.h"
#include "frwrdec.h"
#include "struct.h"


#include "unit.h"
#include "runit.h"
//#include "tprim.h"
#include "orders.h"
#include "acpsim.h"

extern ACPSim* theSim;

using AAA::FSM;
using AAA::State;
using AAA::Predicate;
using AAA::Action;
using AAA::AlwaysTrue;


using AAA::GVector;


// ------------------------------------------------------

FSM*
ResUnit::makeDefendAreasFSM(Box* wait_area, Box* response_area, float theta)
{
  FSM *fsm;
  State *waiting, *engaging;
  Predicate *test;
  Action *action;
  GVector pt;
  float curr_time;
  assert (0.0 <= theta);
  assert (1.0 >= theta);

  fsm = new FSM();
  assert (NULL != fsm);

  waiting = new State();
  assert (NULL != waiting);
  engaging = new State();
  assert (NULL != engaging);

  // if enemy there, attack
  test = new EnemyInArea(this, response_area, this->currentStrength()/10.0);
  action = new OrderResUnitEngageEnemyInArea(this, response_area);
  assert (NULL != test);
  assert (NULL != action);

  waiting->addTransition(test, action, engaging);


  // if enemy still there, attack toward current cg
  test = new EnemyInArea(this, response_area, this->currentStrength()/10.0);
  action = new OrderResUnitEngageEnemyInArea(this, response_area);
  assert (NULL != test);
  assert (NULL != action);
  engaging->addTransition(test, action, engaging);

// if enemy not there, return to near-middle of wait_area
  test = new AlwaysTrue(); 
  assert (NULL != test);
  pt = ((wait_area->center * theta) + (wait_area->randomPoint(theSim->rng) * (1.0 - theta)));
  curr_time = this->sim->clock();
  action = new OrderResUnitToPoint(this, pt, curr_time);
  assert (NULL != action);
  engaging->addTransition(test, action, waiting);

  fsm->addState(waiting);
  fsm->addState(engaging);
  fsm->setState(engaging);  //  thus, it goes to the right place no matter what

  return fsm;
}

// two states:
// S1, waiting
//
//  predicate == (T==EnemyInArea(defender, responseArea, defender->strength()/10.0))
//  action == OrderResUnitEngageEnemyInArea(responseArea);
//  next state == S2
//     this action means to always head for his CG, fighting anything
//     encountered along the way.

// S2, engaging
//
//  predicate == (T==EnemyInArea(defender, responseArea, defender->strength()/10.0))
//  action == OrderResUnitEngageEnemyInArea(responseArea);
//  next state == S2
//    thus, as long as any enemy is present, constantly re-aim at his CG.
//
//  predicate == (F==EnemyInArea(defender, responseArea, defender->strength()/10.0))
//  action == OrderResUnitToPoint(defender, waitArea->center, waitArea->width/4);
//  next state == S1

// ------------------------------------------------------

FSM*
CmndUnit::makeDefendAreasFSM(Box* wait_area, Box* response_area, float theta) {
  FSM *fsm = NULL;
  cout << "Can't make CmndUnit makeDefendAreasFSM" << endl;
  cout << "Assigning all subs to same make_defend__areas mission"<<endl;
  appBell();
  cout << flush;

  //  Node* suND = NULL;
  unsigned int i = 0;
  unsigned int n = 0;
  Unit* sub = NULL;
  for (i=0; i<n; i++) {
    sub = (*subordinates)[i];
    assert (sub != NULL);
    if (AAA::LTrue == sub->aliveP) {
      sub->setFSM(sub->makeDefendAreasFSM(wait_area, response_area, theta));
      sub->descheduleNextEvent();
      sub->update();
    }
  }

  return fsm;
}


#endif


// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
