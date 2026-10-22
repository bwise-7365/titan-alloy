// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: includes, evector -> std::vector.
//
// Units: kilograms, meters, seconds, liters.
//
// ------------------------------------------

#ifndef MOVEMENT_CONTROL_H
#define MOVEMENT_CONTROL_H


// ------------------------------------------

#include "frwrdec.h" 
#include "struct.h" 
#include "des.h"
#include "tgrid.h"
#include "unit.h" 
#include "runit.h" 
#include "components.h"

#include <vector>

// ------------------------------------------


void strengthInNonOverlapping (double ax0, double ay0, double ax1, double ay1,
			       double bx0, double by0, double bx1, double by1,
			       Unit* ru, float &fS, float &eS,
			       bool useSmallP);

void strengthInNonOverlapping (AAA::GVector ctrB, AAA::GVector dxB, AAA::GVector dyB, 
			       AAA::GVector ctrS, AAA::GVector dxS, AAA::GVector dyS, 
			       Unit* ru, float &fS, float &eS,
			       bool useSmallP);

// ------------------------------------------
//
// this is the low-level ResUnit controller.
// it models short-term, local decisions to guide 
// physical movement, and physical limits there on. 
//
// it can be used in cooperation with a behavioral FSM.
// For example, the FSM steps through a route, while
// the low-level MC keeps the unit aimed at the current
// waypoint each move-event.
//
// It can also be used as the basic controller
// of a 'dot wars', ISAAC type model


class MovementRule {

public:

  MovementRule(Unit* ru);
  virtual ~MovementRule();

  virtual void desiredVelocity(AAA::GVector& gv, double& dt)=0;

protected:

  Unit* myUnit;

private:

};


// just return a fixed vector,
// which can be externally changed, of course!
class MRConst : public MovementRule {

public:
  MRConst(Unit* u, AAA::GVector v);
  ~MRConst();

  void desiredVelocity(AAA::GVector& gv, double& dt);
  void setVel(AAA::GVector v);

protected:
  AAA::GVector vel;

private:

};



class MRPoint : public MovementRule {

public:

  MRPoint(Unit* ru, AAA::GVector gv, float tp, WayPointType wpt);
  ~MRPoint();

  void desiredVelocity(AAA::GVector& gv, double& dt);
  void setAimPoint(AAA::GVector ap);

protected:
  AAA::GVector aimPoint;
  WayPointType wpType; // fixed speed, or fixed arrival time
  double speedParameter;  // the fixed speed, or the fixed arrival time
  float tauPt;


private:

};

class MRIntercept : public MovementRule {

public:

  MRIntercept(Unit* ru, Unit* trgt);
  ~MRIntercept();

  void desiredVelocity(AAA::GVector& gv, double& dt);

protected:
  Unit* trgtRU;

private:

};


// %% OPTIMIZATION:
//
// it is pretty clear that MRBuddies1 and MRBuddies2
// could be combined into one class, with a pointer
// to different scan methods/objects
//
//  MRForce1 and MRForce2 could be combined similarly

// move toward proper spacing from buddies
// 
// this does its own scanning to look for resunits near
class MRBuddies1 : public MovementRule {
public:
  MRBuddies1(double sr, double s, double tb, Unit* ru);
  virtual ~MRBuddies1();
  
  void desiredVelocity(AAA::GVector& gv, double& dt);

protected:

  double scanRange;
  double spacing;
  double tauB;
  
private:

};


// move toward proper spacing from buddies
// 
// this uses the shared situation awareness of 
// superior CmndUnits to look up buddies
class MRBuddies2 : public MovementRule {
public:
  MRBuddies2(unsigned long int ns, double s, double tb, Unit* ru);
  virtual ~MRBuddies2();
  
  void desiredVelocity(AAA::GVector& gv, double& dt);

protected:

  unsigned long int nthSup; // must be positive
  double spacing;
  double tauB;
  std::vector<Unit*> *buddies; // the nthSubs of my nthSup
  
private:

};


// move toward advantageous battles
// 
// this does its own scanning to look for 
// friendly or enemy resunits near
class MRForce1 : public MovementRule {
public:
  MRForce1(double w, double h, double l,
	  double a, double b,
	  double ur, double ub,
	  double tf, Unit* ru);

  virtual ~MRForce1();
  
  void desiredVelocity(AAA::GVector& gv, double& dt);
  void desiredVelocityTMP(AAA::GVector& gv, double& dt);

protected:
  
  double width; // x span
  double height; // y span
  double lambda; 
  double alpha;
  double beta;
  double uRed;
  double uBlue;
  double tauF;

private:

};


// move toward advantageous battles
// 
// this uses the shared situation awareness of 
// superior CmndUnits to look up buddies
class MRForce2 : public MovementRule {
public:
  MRForce2(unsigned long int ns, 
	   double w, double h, double l,
	   double a, double b,
	   double ur, double ub,
	   double tf, Unit* ru);

  virtual ~MRForce2();
  
  void desiredVelocity(AAA::GVector& gv, double& dt);

protected:

  unsigned long int nthSuperior;
  double width; // x span
  double height; // y span
  double lambda; 
  double alpha;
  double beta;
  double uRed;
  double uBlue;
  double tauF;

private:

};


// first stab at NCW movement controller for resunit
class MRNCW1 : public MovementRule {

public:
  MRNCW1(AAA::GVector ap, Unit* ru);
  virtual ~MRNCW1();


  // a composite vector, from component controllers
  void desiredVelocity(AAA::GVector& gv, double& dt);

  // weights on all those movement goals
  double wGoal;
  double wFriends;
  double wForce;
  double wSuperior;

protected:

  MRPoint* mcGoal; // aim at goal
  MRBuddies1* mcFriends; // move toward proper spacing from friendlies
  MRForce1* mcForce; // move toward advantageous battles
  MRPoint* mcSuperior; // aim at superior's CG

  double sPlan;

private:

};


// ------------------------------------------

class NCWFSM1 : public AAA::FSM {
  
public:
  NCWFSM1(AAA::GVector ap, unsigned int nsm, unsigned int nsf, Unit* u);
  virtual ~NCWFSM1();


  void setAimPoint(AAA::GVector ap, double at);
  void setSibSpacing(double ss);

  // weights on all those movement goals
  double wGoal;
  double wFriends;
  double wForce;
  double wSuperior;

protected:

  double sibSpacing;
  unsigned int nthSiblingsMnvr;
  unsigned int nthSiblingsFire;

  Unit* myUnit;
  MRPoint* mcGoal; // aim at goal, by a time
  MRBuddies2* mcFriends; // move toward proper spacing from friendlies
  MRForce2* mcForce; // move toward advantageous battles
  MRPoint* mcSuperior; // aim at superior's CG

  double sPlan;

private:

};

// ------------------------------------------
#endif
// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
