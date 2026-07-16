//-------------------------------------------------
//  Copyright Ben Paul Wise. All Rights Reserved.
//-------------------------------------------------
// Vendored from pershing/aaa for the qtacp port.
// Patch relative to the original: evector members are replaced
// by std::vector.
//
// This file must shadow abzar's unrelated fsm.h, so the qtacp
// include path lists src/aaa before abzar/libsrc.
//-------------------------------------------------

// ----------------------------------
// NEVER share actions, predicates, or states between
// fsm's! Why? Deleting one FSM will delete all the
// actions, predicates and states inside it,
// leaving the other FSM to either use garbage
// or try to do a double-delete
// ----------------------------------

#ifndef FSM_H
#define FSM_H

// ----------------------------------

#include "aaa.h"
#include <vector>

// ----------------------------------

using namespace std;

// ----------------------------------

// the primary function of the whole FSM class (as opposed
// to just a State) is to make clean up and delete easier,
// as well as give the user an intuitive interface (syntactic sugar).
// Hook predicates and actions directly to the states.

class AAA::FSM
{
public:

  FSM();
  virtual ~FSM();
  virtual void addState(State*);
  virtual void setState(State*);
  virtual int getID();
  virtual void execute();

  static int debugFSM;

protected:

private:
  State* currState;
  std::vector<State*> *states;
  static int highestFSMID;
  int fsmID;
  int numStates;

};

// ----------------------------------

class AAA::State
{
public:

  State();
  virtual ~State();

  virtual State* transition();
  virtual void addTransition(Predicate*, Action*, State*);
  virtual void setProcess(FSM*);
  virtual void setStateID(int);
  virtual int  getStateID();
  virtual void setParent(FSM*);
  virtual int getParentID();

protected:

private:

  std::vector<Predicate*> *testFN;
  std::vector<State*> *nextState;
  std::vector<Action*> *actionFN;

  FSM* parent;
  FSM* process;
  int stateID;

};

// ----------------------------------

class AAA::Predicate
{
public:
    Predicate();
    virtual ~Predicate();
    virtual Logical test()=0;
protected:

private:
};

// ----------------------------------

class AAA::Action
{
public:
    Action();
    virtual ~Action();
    virtual void perform()=0;
protected:
private:
};

// ----------------------------------

class AAA::AlwaysTrue : public Predicate
{
public:
  AlwaysTrue();
  ~AlwaysTrue();
  Logical test();
protected:
private:
};

// ----------------------------------

class AAA::Negation : public Predicate
{
public:
  Negation();
  ~Negation();
  void setPred(Predicate*);
  Logical test();
protected:
private:
  Predicate* basicFN;
};

// ----------------------------------
// note that Conjunction and Disjunction
// can give infinite looping behavior, so we may need
// to add error checking (maintain a counter and complain
// if it exceeds 1000?).

class AAA::Conjunction : public Predicate
{
public:
  Conjunction();
  ~Conjunction();
  void addPred(Predicate*);
  Logical test();
protected:
private:
  std::vector<Predicate*> *fns;
};

// ----------------------------------

class AAA::Disjunction : public Predicate
{
public:
  Disjunction();
  ~Disjunction();
  void addPred(Predicate*);
  Logical test();
protected:
private:
  std::vector<Predicate*> *fns;
};

// ----------------------------------

class AAA::AllActions : public Action
{
public:
  AllActions();
  ~AllActions();
  void addAction(Action*);
  void perform();
protected:
private:
  std::vector<Action*> *actions; // data of type Action*
};


// ----------------------------------

#endif

//-------------------------------------------------
//  Copyright Ben Paul Wise. All Rights Reserved.
//-------------------------------------------------
