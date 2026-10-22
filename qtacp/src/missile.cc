// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: include list, evector -> std::vector.
// ------------------------------------------

#include "aaa.h"
#include "frwrdec.h"
#include "struct.h"

#include "acpsim.h"
#include "tgrid.h"
#include "unit.h"
#include "runit.h"
#include "components.h"


#include "acpsim.h"

#include <vector>

extern ACPSim* theSim;
// ------------------------------------------

// launchMissileFN(void* missileLaunchRecordPtr)
void 
launchMissileFN(void* )
{
  return;
}

void 
sensorScanFN(void* sensorScanRecordPtr) {
  double time;
  ACPSimEvent *sse;
  //  Node *sNode, *uNode;
  SensorScanRecord *ssr = (SensorScanRecord *) sensorScanRecordPtr;
  assert (NULL != ssr);
  std::vector<Sensor*> *sensors = ssr->sensors;
  std::vector<ResUnit*> *units = ssr->units;
  ACPSim *sim = ssr->sim;
  Sensor *snsr = NULL;
  ResUnit *ru = NULL;
  unsigned int i = 0;
  unsigned int j = 0;
  unsigned int n = sensors->size();
  unsigned int m = units->size();
 
  assert (NULL != sim);
  assert (n > 0);
  assert (m > 0);
  for (i=0; i<n; i++) {
    snsr = (*sensors)[i];
      assert (NULL != snsr);
      assert (NULL != snsr->platform);
      if (LTrue == snsr->platform->aliveP)
	for (j=0; j<m; j++) {
	  ru = (*units)[j];
	    assert (NULL != ru);
	    // do not waste time scanning self or dead targets 
	    if  ((LTrue == ru->aliveP) &&(ru != snsr->platform))
	      snsr->scanTarget(ru);
	  }

    }
  sse = NULL;
  if (LTrue == ssr->perpetualP)
    {
      sse = new ACPSimEvent(sim, SSSensorScan);
      assert (NULL != sse);
    }
  else if (ssr->remaining > 0)
    {
      ssr->remaining = ssr->remaining - 1;
      sse = new ACPSimEvent(sim, SSSensorScan);
      assert (NULL != sse);
    }
  if (NULL != sse) {
    sse->data = (void*) ssr;
    sse->processFN = sensorScanFN;
    time = sim->clock() + ssr->interval;
    sim->schedule(time, sse);
  }
  return;
}

void 
detonateMissileFN(void* missileDetonationRecordPtr ) {
  double p, time;
  MissileDetonationRecord *mdr = (MissileDetonationRecord *) missileDetonationRecordPtr;
  ResUnit *miss = mdr->missile;
  ResUnit *trgt = mdr->target;
  double pk = mdr->Pk;
  assert (NULL != trgt);
  assert (NULL != miss);
  assert (0 <= pk);
  assert (pk <= 1 );

  p = trgt->sim->rng->uniform(0,1);
  time = trgt->sim->clock();
  if (p < pk) // kill
    {
      if (true == ACPSim::traceShots) {
	cout << endl;
	cout << "Target ResUnit " << trgt->unitID << " killed by Missile " << miss->unitID;
	cout << " at time " << time << endl;
      }
      trgt->die();
    }
  else
    {
      // miss
      if (true == ACPSim::traceShots) {
	cout << "Target ResUnit " << trgt->unitID << " missed by Missile " << miss->unitID;
	cout << " at time " << time << endl;
      }
    }
  miss->die();
  return;
}

void 
updateMissileFN(void*)
{
  return;
}

// moved void updatePV(void* ruPtr)
// to unit.cc

// moved GVector perturbPos(GVector p0, double amount, RNG* rng)
// to unit.cc

// ------------------------------------------

Missile::Missile() : ResUnit()
{
  Missile::initialize();
  launchPosition = p0;
  lastDistanceCheck = launchPosition;
}


Missile::Missile(ACPSim* sm, Alignment a, GVector initPos, GVector initVel, ResUnit* trgt)
  : ResUnit(sm, a, initPos, initVel)
{
  Missile::initialize();
  sim = sm;
//   resetPV(initPos, initVel); // already done by ResUnit(....)
  assert (sim != NULL);
  assert (NULL != trgt);
  target = trgt;
  launchPosition = p0;
  lastDistanceCheck = launchPosition;
}



Missile::~Missile()
{
}

void
Missile::initialize()
{
  target = NULL;
  maxSpeed = Mach * 2.5 ; // Mach 2.5 for standard missile
  environment = AirPE;

  // for testing whether missiles die as they exceed range (they do)
  range = 50.0 * 1000.0; // 50KM range

  distanceFlown = 0.0;
  lastDistanceCheck = GVector(0,0,0);


  Pk = 0.75; //
  return;
}


void
Missile::update() {
//   // this just does basic PV update
//   assert (NULL != sim);
//   assert (NULL != gRecord);
//   assert (followRouteGT != (gRecord->type));
//   GVector p1 = currentPos();
  
//   //   double distFlown = dist(p1, launchPosition);
//   distanceFlown = distanceFlown + dist(p1, lastDistanceCheck);
//   lastDistanceCheck = p1;

//   if (true == ACPSim::traceMoves) {
//     cout << endl;
//     cout << "At time " << sim->clock() << " missile " << unitID;
//     cout  << " has flown " << distanceFlown <<" / " << range << endl;
//   }
  
//   if ( distanceFlown > range)
//     {
//       if (true == ACPSim::traceMoves)
// 	cout << " Missile " << unitID << " exceeded its range of " << range << endl;
//       die();
//     }
//   else 
//     switch (gRecord->type)
//       {
//       case followRouteGT:
// 	// do nothing (except satisfy the compiler)
// 	break;
//       case interceptResUnitGT:
// 	updatePVintercept();
// 	break;
//       };

  cout << "Missile::update() is a no-op"<<endl<<flush;
  return;
}

void
Missile::doTerminalIntercept(double time) {
//   // schedule a detonation event at time
//   ACPSimEvent *sse;
//   MissileDetonationRecord *mdr;

//   if (true == ACPSim::traceMoves) {
//     cout << endl;
//     cout << " At time " << sim->clock();
//     cout << " scheduling MIssile::doTerminalIntercept for time " << time << endl;
//   }
//   assert (time >= sim->clock());
//   sse = new ACPSimEvent(sim, SSDetonation);
//   assert (NULL != sse);
//   mdr = new MissileDetonationRecord;
//   assert (NULL != mdr);
//   mdr->missile = this;
//   mdr->target = gRecord->target;
//   mdr->Pk = Pk;
//   mdr->time = time;
//   sse->data = ((void*) mdr);
//   sse->processFN = detonateMissileFN;
//   sim->schedule(time, sse);


  cout <<"Missile::doTerminalIntercept is a no-op"<<endl << flush;
  return;
}

// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
