//-------------------------------------------------
//  Copyright Ben Paul Wise. All Rights Reserved.
//-------------------------------------------------
// Vendored from pershing/aaa for the qtacp port.
// Patch relative to the original: evector is replaced
// by std::vector.
//-------------------------------------------------

// ----------------------------------

#include <stdio.h>
#include "fsm.h"

// ----------------------------------

int AAA::FSM::highestFSMID = 1000;

int AAA::FSM::debugFSM = 0; // default is OFF

// ----------------------------------

AAA::Predicate::Predicate()  {
  // no-op
}


AAA::Predicate::~Predicate()  {
  if (1 == FSM::debugFSM)
    cout << "deleting Predicate "  << endl;
}

// ----------------------------------

AAA::Action::Action()  {
  //no-op
}


AAA::Action::~Action()  {
  if (1 == FSM::debugFSM)
    cout << "deleting Action "  << endl;
}


// ----------------------------------

AAA::State::State()   {
  nextState = new std::vector<State*>();
  testFN = new std::vector<Predicate*>();
  actionFN = new std::vector<Action*>();
  parent = NULL;
  process = NULL;
  stateID = 0;
}


AAA::State::~State()  {
  unsigned int i, n;
  if (1 == FSM::debugFSM)
    cout << "deleting State "  << endl;

  if (process != NULL)    {
    delete process;
    process = NULL;
  }

  assert (NULL != testFN);
  assert (NULL != actionFN);
  assert (NULL != nextState);

  n = actionFN->size();

  // delete testFN items and list
  for (i=0; i<n; i++)    {
    delete (*testFN)[i];
    (*testFN)[i] = NULL;
  }
  delete testFN;
  testFN = NULL;

  // delete actionFN items and list
  for (i=0; i<n; i++)    {
    delete (*actionFN)[i];
    (*actionFN)[i] = NULL;
  }
  delete actionFN;
  actionFN = NULL;

  // do NOT delete next_state items (FSM will do that), just the list
  delete nextState;
  nextState = NULL;

  if (1 == FSM::debugFSM)
    cout << "  deleted State "  << endl;
}



AAA::State * AAA::State::transition()  {
  State *rslt = NULL;
  Logical found;
  Predicate *fn = NULL;
  State *s = NULL;
  Action *act = NULL;
  unsigned int i = 0;
  unsigned int n = testFN->size();
  found = LFalse;
  rslt = this;
  if (1 == FSM::debugFSM) {
    cout << "FSM.state " << parent->getID() << "." << stateID;
  }

  if (process != NULL) {
    if (1 == FSM::debugFSM) {
      cout << " will execute its process " << process->getID() << endl;
    }
    process->execute();
  }

  if (process == NULL)
    if (1 == FSM::debugFSM) {
      cout << " has no process" << endl;
    }

  for (i=0;   ((LFalse == found) && (i<n));   i++) {
    fn = (*testFN)[i];
    s = (*nextState)[i];
    act = (*actionFN)[i];

    assert (NULL != fn);
    assert (NULL != s);
    // action is often NULL
    if (LTrue == fn->test()) {
      found = LTrue;
      rslt = s;
      if (NULL != act) {
	act->perform();
      }
    }
  }

  return rslt;
}



// --------------------------------

void AAA::State::addTransition (Predicate * pred, Action * act, State * seg)  {
  if (pred == NULL) {
    if (1 == FSM::debugFSM) {
      cout << "AAA::State::addTransition missing a predicate"<<endl;
    }
    assert(LFalse);
  }

  if (seg == NULL) {
    if (1 == FSM::debugFSM) {
      cout << "AAA::State::addTransition missing a State" << endl;
    }
    assert(LFalse);
  }

  // but actions may often be NULL

  testFN->push_back (pred);
  actionFN->push_back (act);
  nextState->push_back (seg);
  assert (actionFN->size() == testFN->size());
  assert (actionFN->size() == nextState->size());
  return;
}


void AAA::State::setProcess (FSM * f)  {
  if (f == NULL) {
    if (1 == FSM::debugFSM) {
      cout << "Setting FSM.state " << getParentID();
      cout << "." << stateID << " process to NULL" << endl;
    }
  }
  else {
    if (1 == FSM::debugFSM) {
      cout << "Setting FSM.state " << getParentID();
      cout << "." << stateID << " process to FSM" << f->getID() << endl;
    }
  }
  process = f;
  return;
}


void AAA::State::setStateID (int i)  {
  if (i < 1) {
    if (1 == FSM::debugFSM)  	{
      cout << "AAA::State::setStateID(int) invalid state id number" << endl;
    }
    assert(LFalse);
  }

  if (stateID == 0)
    stateID = i;
  else    {
    stateID = i;
    if (1 == FSM::debugFSM)  	{
      cout << "AAA::State::setStateID(int) reset existing id"<<endl;
    }
    assert(LFalse);
  }
  return;
}


int AAA::State::getStateID()  {
  return stateID;
}



void AAA::State::setParent (FSM * prnt)  {
  if (prnt == NULL)    {
    if (1 == FSM::debugFSM) 	{
      cout << "AAA::State::setParent(FSM*) null parent" << endl;
    }
    assert(prnt != NULL);
  }

  if (parent == NULL)
    parent = prnt;
  else    {
    parent = prnt;
    if (1 == FSM::debugFSM) 	{
      cout << "AAA::State::setParent(FSM*) reset existing parent" << endl;
    }
    assert(LFalse);
  }
  return;
}


int AAA::State::getParentID()  {
  return parent->getID();
}

// ----------------------------------

AAA::FSM::FSM()  {
  currState = NULL;
  states = new std::vector<State*>();
  fsmID = highestFSMID++;
  numStates = 0;
  if (1 == FSM::debugFSM)     {
    cout << "Created FSM " << fsmID << endl;
  }
}



AAA::FSM::~FSM()  {
  unsigned int i;
  if (1 == FSM::debugFSM)
    cout << "deleting FSM " << fsmID << endl;

  assert (NULL != states);

  for (i=0; i< states->size(); i++)    {
    delete (*states)[i];
    (*states)[i] = NULL;
  }
  delete states;
  states = NULL;

  if (1 == FSM::debugFSM)
    cout << "  deleted FSM " << fsmID << endl;
}


void AAA::FSM::addState (State * s)  {
  states->push_back (s);
  numStates = numStates + 1;
  s->setStateID (numStates);
  s->setParent (this);
  return;
}


void AAA::FSM::setState (State * s)   {
  assert (NULL != s);
  currState = s;
  return;
}


int AAA::FSM::getID()  {
  return fsmID;
}


void AAA::FSM::execute()  {
  int old_state, new_state;
  if (1 == FSM::debugFSM) {
    cout << "Starting executing FSM " << fsmID << endl;
  }
  assert (NULL != currState);
  old_state = currState->getStateID();
  if (1 == FSM::debugFSM) {
    cout << "FSM " << fsmID << " is currently in state " << old_state << endl;
  }
  currState = currState->transition();
  assert (NULL != currState);
  new_state = currState->getStateID();
  if (1 == FSM::debugFSM) {
    cout << "FSM " << fsmID << " is now in state " << new_state << endl;
    cout << "Done executing FSM " << fsmID << endl;
  }
  return;
}




// ----------------------------------

AAA::AlwaysTrue::AlwaysTrue()  {
}


AAA::AlwaysTrue::~AlwaysTrue()  {
}


AAA::Logical AAA::AlwaysTrue::test()  {
  if (1 == FSM::debugFSM)
    cout << "AlwaysTrue::test() returns LTrue " << endl<<flush;

  return LTrue;
}


// ----------------------------------

AAA::Negation::Negation()  {
  basicFN = NULL;
}



AAA::Negation::~Negation()  {
  if (basicFN != NULL)
    delete basicFN;
  basicFN = NULL;
}


void AAA::Negation::setPred (Predicate * pred)  {
  basicFN = pred;
}


AAA::Logical AAA::Negation::test()  {
  Logical result = LFalse;
  assert (NULL != basicFN);
  switch (basicFN->test())
    {
    case LTrue:
      result = LFalse;
      break;
    case LFalse:
      result = LTrue;
      break;
    case LUnknown:
      result = LUnknown;
      break;
    }
  return result;
}


// ----------------------------------

AAA::Conjunction::Conjunction()  {
  fns = new std::vector<Predicate*>();
}



AAA::Conjunction::~Conjunction()  {
  unsigned int i;
  assert (NULL != fns);
  for (i=0; i<fns->size();i++)    {
    delete (*fns)[i];
    (*fns)[i] = NULL;
  }
  delete fns;
  fns = NULL;
}


void AAA::Conjunction::addPred (Predicate * pred)  {
  assert (NULL != fns);
  fns->push_back (pred);
}


AAA::Logical AAA::Conjunction::test()  {
  Logical rslt;
  unsigned int i;
  Predicate * fn;
  rslt = LTrue;
  assert (NULL != fns);
  if (1 == FSM::debugFSM)
    cout << "Evaluating a conjunctive test ..." << endl;

  for (i=0; ((LTrue == rslt) && (i<fns->size())); i++) {
    fn = (*fns)[i];
    assert (NULL != fn);
    if (fn->test() == LFalse)
      rslt = LFalse;
  }

  if (1 == FSM::debugFSM)
    cout << "... result was " << rslt << endl << flush;

  return rslt;
}




// ----------------------------------

AAA::Disjunction::Disjunction()   {
  fns = new std::vector<Predicate*>();
}



AAA::Disjunction::~Disjunction()  {
  unsigned int i;
  assert (NULL != fns);
  for (i=0; i<fns->size();i++)    {
    delete (*fns)[i];
    (*fns)[i] = NULL;
  }
  delete fns;
  fns = NULL;
}


void AAA::Disjunction::addPred (Predicate * pred)  {
  fns->push_back (pred);
}


AAA::Logical AAA::Disjunction::test()  {
  Logical rslt;
  unsigned int i;
  Predicate * fn;
  rslt = LFalse;
  assert (NULL != fns);

  if (1 == FSM::debugFSM)
    cout << "Evaluating a disjunctive test ..." << endl;

  for (i=0; ((LFalse == rslt) && (i<fns->size())); i++) {
    fn = (*fns)[i];
    assert (NULL != fn);
    if (fn->test() == LTrue)
      rslt = LTrue;
  }
  if (1 == FSM::debugFSM)
    cout << "... result was " << rslt << endl << flush;

  return rslt;
}




// --------------------------------

AAA::AllActions::AllActions()  {
  actions = new std::vector<Action*>();
}


AAA::AllActions::~AllActions()  {
  unsigned int i;
  assert (NULL != actions);
  for (i=0; i<actions->size();i++)    {
    delete (*actions)[i];
    (*actions)[i] = NULL;
  }
  delete actions;
  actions = NULL;
}


void AAA::AllActions::addAction (Action * a)  {
  actions->push_back (a);
  return;
}


void AAA::AllActions::perform()  {
  unsigned int i;
  Action * a;
  assert (NULL != actions);

  for (i=0; i< actions->size(); i++)   {
    a = (*actions)[i];
    a->perform();
  }
  return;
}


//-------------------------------------------------
//  Copyright Ben Paul Wise. All Rights Reserved.
//-------------------------------------------------
