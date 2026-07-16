// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: banner, removed the
// outputQueue references.
// ------------------------------------------

#include "aaa.h"
#include "frwrdec.h"
#include "struct.h"
#include "runit.h"
#include "components.h"


extern ACPSim* theSim;
using AAA::expt;
using AAA::dexpt;

// ------------------------------------------


PlatformComponent::PlatformComponent()
{
  platform = NULL;
  sim = NULL;
}

PlatformComponent::~PlatformComponent()
{
}

void
PlatformComponent::connectToPlatform(ResUnit *ru)
{
  assert (NULL == platform);
  assert (NULL == sim);

  assert (NULL != ru);
  platform = ru;
  //  assert (NULL != ru->sim);
  sim = theSim;
  return;
}

// ------------------------------------------

CommNode::CommNode()
{
  // initialize
}

CommNode::~CommNode()
{
}


void
CommNode::connectToPlatform(ResUnit *ru)
{
  PlatformComponent::connectToPlatform(ru);
  // possible other actions
  return;
}



// ------------------------------------------

Sensor::Sensor()
{
  jammer = NULL;
  platform = NULL;
  Rdet = standardDetectDistance;
  Rid =  standardIdentifyDistance;
}

Sensor::~Sensor()
{
}


void
Sensor::connectToPlatform(ResUnit *ru)
{
  PlatformComponent::connectToPlatform(ru);
  sim->sensors->push_back(this);
  return;
}


void
Sensor::scanTarget(ResUnit *trgt) {
  assert(NULL != trgt);
  assert(NULL != platform);
  assert(NULL != sim);
  assert (Rid > 0);
  assert (Rdet > Rid);

  double csT = trgt->crossSection;
  AAA::GVector tP = trgt->currentPos();
  AAA::GVector sP = platform->currentPos();
  bool visibleP;
  //  double visDist = 0.0;

  double trgtR = norm(tP - sP);
  double effR = trgtR;
  double jmR,  jmP, f;

  if (true == ACPSim::traceSensors) {
    cout << "Sensor on platform " << platform->getSimEntID();
    cout << " is trying to scan target " << trgt->getSimEntID();
    cout << " at time " << theSim->clock()  << endl;
  }

  visibleP = sim->tgrid->terrainVisibleP(tP, sP);

  if (true == ACPSim::traceSensors) {
    if (true == visibleP)  {
	cout << "  - not masked by terrain" << endl;
      }
    else  {
	cout << "   - masked by terrain" << endl;
      }
  }

  if (true == visibleP)  {
      if (NULL != jammer) {
	  jmR = norm(sP - jammer->platform->currentPos());
	  jmP =  jammer->power;
	  assert(jmP > 0);
	  f = standardJamDistance / jmR;
	  effR = trgtR * dexpt(2.0, jmP * expt(f,2)  / 10.0);
	}


      //  insert terrain effects here
      effR = effR * sim->tgrid->terrainClutterEffect(trgt);
      f = Rid / effR;
      if ( csT * expt(f, 4) > 1) {
	  emitID(trgt);
	}
      else {
	f = Rdet / effR;
	if ( csT * expt(f, 4) > 1)  {
	    emitDetect(trgt);
	  }
	else  {
	    if (true == ACPSim::traceSensors)
	      cout << " Can not even detect the target" << endl;
	  }
      }
    }
  return;
}

void
Sensor::emitDetect(ResUnit *ru) {
  assert (NULL != ru);
  if (true == ACPSim::traceSensors)
    cout << " Can detect - but not ID - the target" << endl;
  return;
}


void
Sensor::emitID(ResUnit *ru) {
  assert (NULL != ru);
  if (true == ACPSim::traceSensors)
    cout << " Can detect and ID the target" << endl;
  return;
}


// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
