// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: banner, includes,
// removed the TQProcessor output queue.
// ------------------------------------------
//
// Units: kilograms, meters, seconds, liters.
//
//  standardized jammer power:
//    0 is no jamming
//    10 is enough to halve the detection and ident
//    ranges, when jammed from 150KM
// ------------------------------------------

#ifndef COMPONENTS_H
#define COMPONENTS_H

// ------------------------------------------

#include "aaa.h"
#include "frwrdec.h"
#include "struct.h"
#include "des.h"
#include "tgrid.h"

//  #include "unit.h"
//  #include "runit.h"

// ------------------------------------------

class CommNet;

// ------------------------------------------
class PlatformComponent {
public:
  PlatformComponent();
  virtual ~PlatformComponent();
  virtual void connectToPlatform(ResUnit *ru)=0;

  ResUnit *platform;
  ACPSim *sim;

protected:

private:

};

class Sensor : public PlatformComponent {
public:
  Sensor();
  ~Sensor();

  virtual void connectToPlatform(ResUnit *ru);
  void scanTarget(ResUnit *ru);
  void emitDetect(ResUnit *ru);
  void emitID(ResUnit *ru);


  Jammer *jammer;
  // range in meters to detect standrd 1m^2 target
  double Rdet;

  // range in meters to identify standrd 1m^2 target
  double Rid;

protected:

private:

};

class CommNode : public PlatformComponent {
public:
  CommNode();
  ~CommNode();

  virtual void connectToPlatform(ResUnit *ru);

  // what net I am connected to. This is the WHOLE net,
  // not just the subpart I talk to easily.
  CommNet *net;
protected:

private:

};


class Jammer : public PlatformComponent {
public:
  Jammer();
  ~Jammer();

  virtual void connectToPlatform(ResUnit *ru);

  //  evector<Sensor*> *targets;

// standardized power
  double power;
  int maxNumTargets;

protected:

private:

};

// ------------------------------------------
#endif
// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
