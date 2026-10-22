// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------

#include "aaa.h"
#include "frwrdec.h"
#include "struct.h"
#include "cunit.h"
#include "orders.h"
#include "tthread.h"

using AAA::FSM;
using AAA::Action;

// ------------------------------------------------------


// OrderUnitReformInPlace::OrderUnitReformInPlace() {
// }

OrderUnitReformInPlace::~OrderUnitReformInPlace() {
}

OrderUnitReformInPlace::OrderUnitReformInPlace(Unit* u, Box* nb, float et) {
  assert (NULL != u);
  assert (NULL != nb);
  assert (et >= 0);

  unit = u;
  nuBox = nb;
  eTime = et;

  if ( true == ACPSim::traceOrders) {
    cout << "Created OrderUnitReformInPlace for unit " << unit->getSimEntID();
    cout << " by time "<<eTime<<" in box " << *nuBox << endl << flush;
  }

}

void
OrderUnitReformInPlace::perform() {  

  unit->setFSM(unit->makeReformFSM(nuBox, eTime));
  // now,  deschedule any future events and force
  // the unit to do an update
   unit->descheduleNextEvent();
   unit->update();
}


// ------------------------------------------------------
// this works.

FSM* 
ResUnit::makeReformFSM(Box* nuBox, float eTime) {
  FSM *fsm = NULL;
  Action *action = NULL;
  
  action = new OrderResUnitToPoint(this, nuBox->center, eTime);
  assert (NULL != action);
  action->perform();
  return fsm;

}

// ------------------------------------------------------
// the CmndUnit::makeCorridorFSM function will build an
// fsm with a series of OrderCUnitAcrossBox actions.
// each such action, when perform-ed, will either
// order_unopposed_move or order_opposed_move, depending
// on what it finds at the time it is applied.
// the way it orders_unopposed_move is create corridors
//  for all its subordinates, and recurse down.
//
// for us, we do not build an FSM across corridors.

FSM* 
CmndUnit::makeReformFSM(Box* nuBox, float eTime) {
  FSM *fsm = NULL;
  // in reality, you need not really do anything:
  // they are basically where they belong!
  //  stubFN("can not do CmndUnit::makeReformFSM", 0);
  cout << "can not do CmndUnit::makeReformFSM" << endl << flush;
  return fsm;
}

// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
