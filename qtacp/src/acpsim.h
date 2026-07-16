// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. This file carries the whole
// adaptation from the old DES engine to the abzar DESim engine:
// ACPSim subclasses Simulation::DESim and preserves the three
// call-site spellings the rest of the code uses (clock(),
// rng->uniform(a,b), and the two-argument schedule); ACPSimEvent
// subclasses Simulation::Event and keeps the type-enum dispatch.
//
// Event ownership under DESim: the engine deletes an event once it
// has been processed and no queue entry references it. Application
// code must never delete, reuse, or re-schedule a processed event;
// recurring behavior constructs a new event each cycle. Cancellation
// is lazy: a cancelled event stays in the queue, and its processing
// is a no-op (see ACPSimEvent::cancel and Unit::descheduleNextEvent).
//
// Units: kilograms, meters, seconds, liters.
// ------------------------------------------

#ifndef ACPSIM_H

#define ACPSIM_H

// ------------------------------------------

#include "frwrdec.h"
#include "des.h"
#include "struct.h"
#include "tgrid.h"

#include <vector>

// ------------------------------------------

class ResUnit;
class CmndUnit;
class ACPSimEvent;

// rings the terminal/application bell.
// the old code took this from the yr library (yxtapp.h);
// the Qt port defines it in xtdemo.cc via QApplication::beep().
void appBell();

// ------------------------------------------

class  ACPSim : public Simulation::DESim
{
 public:
  ACPSim();  // random seed
  ACPSim(int s);  // repeatable seed
  virtual ~ACPSim();

  // alias for the engine-owned generator, preserving the
  // pervasive sim->rng->uniform(a,b) call sites
  panj::PRNG *rng;

  // the simulation clock, under its old name
  double clock() const { return getCurrTime(); }

  // the old engine allowed setting the clock; scen.cc uses it to
  // zero a fresh simulation. legal only when the queue is empty
  // (rewinding under queued events would violate DESim's
  // no-past-scheduling rule).
  void setClock(double t) {
    assert (false == eventsPending());
    currTime = t;
  }

  // two-argument schedule preserving the old call sites.
  // the event must not already be queued. a time earlier than the
  // current clock is clamped to the clock (DESim throws on past
  // times, where the old engine tolerated them).
  void schedule(double t, ACPSimEvent* ev);
  using Simulation::DESim::schedule; // keep the one-argument form visible

  // is at least one event queued?
  bool eventsPending() const;

  // execute exactly one event, if one is queued.
  // DESim keeps step() protected; the GUI single-steps through these.
  void stepOnce();
  void stepN(int n); // n stepOnce() calls, stopping early on an empty queue

  // count of events executed via stepOnce().
  // DESim has no event counter; this one is correct because qtacp
  // never runs the engine through go().
  long int eventNum() const { return eventsProcessed; }

  TGrid *tgrid;
  std::vector<ResUnit*> *rUnits;
  std::vector<CmndUnit*> *cUnits;
  std::vector<Sensor*> *sensors; // data of type Sensor*
  std::vector<Jammer*> *jammers; // data of type Jammer*

  std::vector<TempCorridor*> *tempCorridors;

  std::vector<Box*> *boxes;


  static bool traceOrders;
  static bool traceMoves;
  static bool traceShots;
  static bool tracePlanning;
  static bool traceGeometry;
  static bool traceFSM;
  static bool traceSensors;


  static bool RepeatableSeedP;

  static unsigned long int RepeatableSeed;


  void addResUnit(ResUnit*);
  void addCmndUnit(CmndUnit*);

 // not just dead things in the sim, but no longer existing in the sim
  void removeResUnit(ResUnit*);
  void removeCmndUnit(CmndUnit*);

 protected:

  void initialize();
  long int eventsProcessed;

 private:

};

// ------------------------------------------
// this is a fairly generic event. it executes some user-specified
// function on user-given data.

class ACPSimEvent : public Simulation::Event
{
 public:
  ACPSimEvent(ACPSim* d, ACPSimEventType t);
  virtual ~ACPSimEvent();
  void processEvent(); // override of Simulation::Event

  // Event::simTime is protected, so this derived class may set it.
  // legal only before the event is queued; ACPSim::schedule is the
  // sole caller.
  void setSchedTime(double t) { simTime = t; }

  // lazy cancellation: the event stays queued, but processing
  // becomes a no-op. the engine deletes it when its time arrives.
  void cancel() { cancelled = true; }

  ACPSimEventType type;
  void (*processFN)(void*);
  void *data; //

  // the Unit whose nextEvent pointer references this event, if any.
  // processEvent() clears that pointer before dispatch so that a
  // rescheduling update() is not clobbered afterward.
  Unit* owner;

  bool cancelled;

 protected:

 private:
  void initialize();
  ACPSim* mySim; // typed back-pointer; replaces the old des->clock() access
};



// Euclidean distance between two vectors
double dist(AAA::GVector a, AAA::GVector b);

// how long it would take a purely ballistic
// projectile to fly the given distance,
// fired at 45 degrees, on a flat, non-rotating
// earth.
// as per the normal units, time is in seconds
// and range is in meters
double ballisticFlightTime(double range);


// minimum ballistic speed to reach a
// range. Thus, this is the 45-degree
// trajectory
double minBallisticSpeed(double range);



// this computes the minimum energy ballistic
// trajectory to travel from one point to another.
//
// the key parameter is the flight time.
//
// xt = x0 + vx*t
// zt = z0 + vz*t - g*t*t/2
//
// and the kinetic energy is m*(vx*vx + vz+vz)/2
//
// given x0, z0, x1, z1, and t, then vx and vz
// are fixed:
//
// vx = (x1-x0)/t
// vz = (z1-z0)/t + (g*t)/2
//
// so we choose t to minimize the kinetic energy,
// giving the desired vx and vz
//
// if you want the precise velocities,
// they are easy to retrieve
double minBallisticTime(AAA::GVector pFrom, AAA::GVector pTo);


// this WILL compute the two ballistic trajectories
// that a fixed speed weapon would use to hit a
// stationary target.
// That is, given P1, P0, and s,
// we want P1 = P0 + V*T - G*t*t/2, s.t. |V| = s
//
// clearly, V = (P1 - P0)/t + (G*t)/2
// so s^^2 = (P1-P0)^^2 / t^^2
//          + (P1 - P0) * G
//          + (G^^2/4) * t^^2
//
// substitute tau = t^^2, and we have a
// quadratic in tau that is easily solved.
// based on the discriminant (b^^2 - 4*a*c),
// we can decide if any real solutions exist,
// then how many of them give positive time.
//
// Thus, we get 0, 1, or 2 solutions.
//
void ballisticTargetingTrajectories(AAA::GVector P0, AAA::GVector P1, double s,
				    unsigned int &num,
				    AAA::GVector *v1,
				    AAA::GVector *v2);
// This could be easily extended to an iterative
// algorithm for hitting a moving target (if it can be
// hit at all). There is a complication that did not
// appear in the constant-velocity intercept problem.
// With a ballistic trajectory, it can happen that you
// are unable to hit his current position, and yet he
// will be in range by the time your shot gets to him.
//
// Depending on how far away he is and how fast he is
// moving, you could fall arbitrarily far short of
// his current position, yet still hit him. If he
// starts far out of range, and zooms almost onto you,
// you could hit him with an almost-zero-energy
// projectile!
//
//
// Suppose the target is at Q, moving W,
// and we want Q + Wt = P + Vt - Gtt/2
// Iteration 0:
// Q(0) = Q
// t(0) = time to hit a stationary target at Q0, projectile speed s
// You can try either the high or the low trajectory (?)
//
// Iteration (i+1):
// Q(i+1) = Q + W * t(i)
// t(i+1) = time to hit a stationary target at Q(i+1), projectile speed s
//
// Do a few iterations, then see if | t(i+1) - t(i) | is shrinking.
// If so, you are converging on a solution. Otherwise, you can't.


// ------------------------------------------
#endif
// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
