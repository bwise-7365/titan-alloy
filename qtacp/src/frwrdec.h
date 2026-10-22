// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: the old DES engine header
// is replaced by the abzar DESim header; the RNG and memory-leak
// checker includes are removed; evector becomes std::vector.
// ------------------------------------------

#ifndef FORWARDECS_H
#define FORWARDECS_H

// ------------------------------------------

#include "aaa.h"
#include "fsm.h"
#include "des.h"
#include "mat.h"

#include <vector>

class Missile;
class Unit;
class ResUnit;
class CmndUnit;
class Sensor;
class Jammer;
class ACPSim;
class ACPSimEvent; // declared here so the acpsim.h/struct.h/tgrid.h/unit.h
                   // include cycle sees it regardless of entry point
class TGrid;
class TCell;
class Box;

class MovementRule; // abstract base class for low-level controller
class MRPoint; // move toward a given point
class MRIntercept; // intercept a resunit

// using own sensors
class MRBuddies1; // move toward proper spacing from buddies
class MRForce1; // move toward advantageous battles

// using shared situation awareness
class MRBuddies2; // move toward proper spacing from buddies
class MRForce2; // move toward advantageous battles

struct MissileLaunchRecord;
struct TempCorridor;

enum TGridType { Water,
		 LandSmooth,
		 LandRough,
		 LandRoad };

enum ACPSimEventType { SSNullEvent,
		       SSSensorScan,
		       SSDetonation,
		       SSCmndUnitUpdate,
		       SSStateUpdate};


// ------------------------------------------

double const Mach = 330; // 330 meters/sec = Mach 1
double const earthRadius = 6.37 * 1000 * 1000; // meters radius of the earth
double const earthG = 9.8; // meter/sec/sec
double const standardJamDistance = 150.0 * 1000.0; // meter
double const standardDetectDistance = 300.0 * 1000.0; // meter
double const standardIdentifyDistance = 250.0 * 1000.0; // meter

double const standardRUmaxStepDist = 1500; // meters

//  meters, random variation in position
double const posNoise = 5.0;

//  meters/sec, velocity below which things are considered stationary
// and have their velocity clipped to (0,0,0);
// this is primarily a guard against roundoff errors
// 1/10 of a cm / second
double const essentiallyStationary = 0.001;

// roughly the number of steps to a stationary target
int const numMissileSteps = 3;

double const scaleFactor = 1.25;


// ------------------------------------------

enum WayPointType {
  timeWP, // arrive at a given time
  speedWP // travel at a given speed
};

enum GuidanceType {
  followRouteGT,
  interceptResUnitGT
};

enum PlatformEnvironment {
  LandPE,
  WaterPE,
  AirPE,
  UnderwaterPE
};


// this is just a huge list of platform types.
// it could be used to set parameters, like range, speed, Rid, etc.
enum PlatformType {
  AirToAirMissile,
  SurfaceToSurfaceMissile,
  ALCM,
  GLCM,
  Tank,
  Truck,
  FighterFWA,
  ReconUAV,
  Aerostat
};

enum formation_type {
  StandardFormation,
  LineFormation,
  ColumnFormation,
  BoxFormation,
  WedgeFormation,
  VeeFormation
};

// possible nationalities.
// these are NOT assumed to be constant for a unit
// over its existance
enum Alignment {
  BlueSide, // 0
  RedSide, // 1, generic opposing side (recent code for USSR, old code for Canada)
  GreenSide, // 2, future code for Islamo-fascists?
  OrangeSide, // 3, (old code for Japan)
  PurpleSide, // 4, (old code for Central American)
  BlackSide,  // 5, (old code for Germany)
  WhiteSide,  // 6, used for known Civilians (old code for domestic uprising)
  GreySide,  // 7, used for unknown, indeterminate, not decided, etc. (old code for Caribbean)
  GoldSide,  // 8, (old code for France)
  CrimsonSide  // 9, (old code for Britain)
};


// ------------------------------------------

// this is a record describing the engagement
struct MissileEngagement {
  //  Missile *missile;
  //  ResUnit *target;
  double startTime;
};

// ------------------------------------------

// notice that the position is NOT recorded.
// if we get close enough that the time_to_intercept < dt,
// we assume a precision attack with the specified Pk
struct MissileDetonationRecord {
  ResUnit* missile;
  ResUnit* target;
  double Pk;
  double time;
};

struct SensorScanRecord {
  ACPSim *sim;
  std::vector<ResUnit*> *units; // list of res units to consider
  std::vector<Sensor*> *sensors; // list of sensors to pair against them
  double interval;  // seconds until next scan
  AAA::Logical perpetualP; // does this continue forever?
  int remaining;   // number of scan remaining ( if not perpetual)
};



#endif
// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
