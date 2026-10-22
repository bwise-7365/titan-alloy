// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: GUI includes removed;
// AAA::RNG becomes panj::PRNG.
// ------------------------------------------

#include "xtdemo.h"

// ----------------------------------

#include "xtsim.h"
#include "acpsim.h"
#include "runit.h"
#include "cunit.h"
#include "tthread.h"
#include "mcontrol.h"

using AAA::expt;
using AAA::Logical;
using AAA::LTrue;
using AAA::LFalse;

// ----------------------------------

extern ACPSim* theSim;

//  create a random corridor of N boxes,
TempCorridor* makeRandomCorridor(panj::PRNG *rng,
				 double xMax, double yMax,
				 unsigned int numBoxes);

Box* createBox( panj::PRNG *rng, double xMin, double yMin, double dx, double dy);


// create a CmndUnit with a simple hierarchy below it
// minimum depth is 1: a CmndUnit and 5 ResUnit below it
// above that, each CmndUnit has 3 subordinates
//
// it also schedules the subordinates so that all subordinates
// start before superiors, right up the chain
CmndUnit* simpleHierarchy1(ACPSim* sim,
			   Alignment side,
			   AAA::GVector center,
			   double minT, double dt,
			   unsigned long int d);

// a different way of creating a simple hierarchy
void attachResUnits(ACPSim* sim, CmndUnit *cu1, int numSubs,
		    float minX, float minY,
		    float maxX, float maxY);
void attachSubUnits(ACPSim* sim, CmndUnit *cu1, int numSubs, int depth,
		    float minX, float minY,
		    float maxX, float maxY);

// ----------------------------------

void
SimGUIModule::scenarioNull() {
  // run test functions, if desired
  return;
}

void
SimGUIModule::setupScenarioOneRU() {
  using AAA::GVector;
  cout << " starting SimGUIModule::setupScenarioOneRU "<<endl<<flush;
  ACPSim* nuSim = NULL;

  if (true == ACPSim::RepeatableSeedP) {
    cout << "ScenarioOneRU using sim seed: " << ACPSim::RepeatableSeed << endl;
    nuSim = new ACPSim(ACPSim::RepeatableSeed);
  }
  else {
    ACPSim::RepeatableSeed = 0;
    cout << "ScenarioOneRU using irrepeatabile sim seed" << endl;
    nuSim = new ACPSim();
  }

  float ty = 5500.0; // about
  float tx = ty * 1.80;
  int r = 17;
  int c = 33;

  ResUnit* ru1 = NULL;
  Alignment side;
  unsigned int i = 0;
  unsigned int j = 0;

  double mapX = 0.0;
  double mapY = 0.0;
  unsigned int k = 0;

  nuSim->tgrid = new TGrid(r, c, tx, ty);
  //  nuSim->tgrid->synthesizeTerrainGaussian(nuSim->rng, -ty/100.0,ty/20.0, 0.6, 6);

  nuSim->tgrid->synthesizeTerrainFractal(nuSim->rng, -ty/100.0,ty/20.0, 0.10);


  centerY = ty/2.0;
  centerX = tx/2.0;
  GVector center = GVector(centerY, centerX, 0.0);
  GVector tmpGV;

  for (i=0; i<3; i++) {
    mapX = (i * tx)/2.01;
    for (j=0; j<3; j++) {
      mapY = (j * ty)/2.01;
      tmpGV = GVector(mapX, mapY, 0.0);
      // we ensure that there are no dead-looking
      // black units. note that these colors are
      // not all friendly, and they will shoot
      // each other as the approach their
      // common destination.
      if (5 == k)
	k++;
      side = ((Alignment) (k%10));

      ru1 = new ResUnit(nuSim, side,
			tmpGV, GVector(0.0, 0.0, 0.0));
      k++;
      tmpGV = GVector(tx / nuSim->rng->uniform(2.2, 2.7),
		      ty / nuSim->rng->uniform(2.2, 2.7),
		      0.0);
      MovementRule* mc = new MRPoint(ru1, tmpGV, 180.0, speedWP);
      ru1->moveController = mc;
      ACPSimEvent*  sse = new ACPSimEvent(nuSim, SSStateUpdate);
      assert (NULL != sse);
      sse->data = (void*)(ru1);
      sse->processFN = updatePV;
      nuSim->schedule(1.0, sse); // first event after 1 second
    }
  }

  cout << "Setup ScenarioOneRU"<<endl;
  mySim = nuSim;
  return;
}

// ----------------------------------
// Blues hunt for enemies and get in formation
// They start out in a bunch on the left
//
// Reds head for the Blue corner,
//
//
void
SimGUIModule:: setupScenarioOneCU() {
  using AAA::GVector;
  cout << " starting SimGUIModule::setupScenarioOneCU "<<endl<<flush;
  ACPSim* nuSim = NULL;

  if (true == ACPSim::RepeatableSeedP) {
    cout << "ScenarioOneCU using sim seed: " << ACPSim::RepeatableSeed << endl;
    nuSim = new ACPSim(ACPSim::RepeatableSeed);
  }
  else {
    ACPSim::RepeatableSeed = 0;
    cout << "ScenarioOneCU using irrepeatabile sim seed" << endl;
    nuSim = new ACPSim();
  }

  ACPSimEvent*  sse = NULL;

  float ty = 10 * 1000.0;
  float tx = ty * 1.40;
  int r = 32 + 1;
  int c = 64 + 1;

  float x = 0.0;
  float y = 0.0;

  ResUnit* ru1 = NULL;

  unsigned int i = 0;
  unsigned int nB = 0;
  unsigned int nR = 0;
  unsigned int N = 150;
  double fractionBlue = 0.50;
  double offSetLo = 0.20;
  double offSetMd = 0.30;
  double offSetHi = 0.90;

  unsigned int numBlues = ((unsigned int) (0.5 + (fractionBlue * N)));

  nuSim->tgrid = new TGrid(r, c, tx, ty);

  nuSim->tgrid->synthesizeTerrainFractal(nuSim->rng, -ty/100.0, ty/20.0, 0.15);

  centerY = ty/2.0;
  centerX = tx/2.0;
  GVector center = GVector(centerY, centerX, 0.0);
  GVector tmpGV;

  CmndUnit* cuB = new CmndUnit(nuSim, BlueSide, center, GVector(0.0, 0.0, 0.0));
  CmndUnit* cuR = new CmndUnit(nuSim, RedSide,  center, GVector(0.0, 0.0, 0.0));

  MRNCW1* mc = NULL;
  for (i=0; i<N; i++) {

    if (i < numBlues) {
      // place Blue resunit
      tmpGV = GVector(tx * nuSim->rng->uniform(0.10, 0.25),
 		      ty * nuSim->rng->uniform(0.25, 0.40),
 		      nuSim->rng->uniform(0.0, 100.0));

      ru1 = new ResUnit(nuSim, BlueSide, tmpGV, GVector(0.0, 0.0, 0.0));
      nB++;
      ru1->sensorRange = 1.40 * ru1->sensorRange;
      ru1->weaponRange = 1.00* ru1->weaponRange;
      cuB->add_sub(ru1);
      // the original passed (0.99, 0.550): the old RNG accepted
      // reversed bounds, but std::uniform_real_distribution (inside
      // panj::PRNG) requires min <= max and asserts otherwise.
      // the interval, and hence the distribution, is unchanged.
      tmpGV = GVector(tx * nuSim->rng->uniform(0.550, 0.99),
		      ty * nuSim->rng->uniform(0.550, 0.99),
		      nuSim->rng->uniform(0.0, 100.0));
      mc = new MRNCW1(tmpGV, ru1);
      mc->wGoal = 10.0;
      if (0 == (i % 2))
	mc->wFriends = 10.0 ;
      else
 	mc->wFriends = 0; // not even computed

      mc->wSuperior = 1.0;
      mc->wForce = 250.0;
    }
    else {
      // place Red resunit
      x = 0.0;
      y = 0.0;
      while ( (x/tx) + (y/ty) < 1.25 ) {
	x = tx * nuSim->rng->uniform(0.01, 0.99);
	y = ty * nuSim->rng->uniform(0.01, 0.99);
      }
      tmpGV = GVector(x, y, nuSim->rng->uniform(0.0, 100.0));

      ru1 = new ResUnit(nuSim, RedSide, tmpGV, GVector(0.0, 0.0, 0.0));
      nR++;
      ru1->sensorRange = 1.40 * ru1->sensorRange;
      ru1->weaponRange = 1.00* ru1->weaponRange;
      ru1->maxSpeed = 1.50 * ru1->maxSpeed;
      cuR->add_sub(ru1);
      // aim for Blue corner
      tmpGV = GVector(tx * nuSim->rng->uniform(0.01, 0.10),
		      ty * nuSim->rng->uniform(0.01, 0.10),
		      nuSim->rng->uniform(0.0, 100.0));

      mc = new MRNCW1(tmpGV, ru1);

      mc->wGoal = 80.0;
      if (0 == (i%3))
	mc->wFriends = 10.0;
      else
 	mc->wFriends = 0;
      mc->wSuperior = 1.0;
      mc->wForce = 60.0;
    }

    ru1->moveController = mc;
    sse = new ACPSimEvent(nuSim, SSStateUpdate);
    assert (NULL != sse);
    sse->data = (void*)(ru1);
    sse->processFN = updatePV;
    // start all resunits within first 60 seconds
    nuSim->schedule(nuSim->rng->uniform(1.0, 60.0), sse);
    sse = NULL;
  }

  // notice that we schedule CmndUnits to start after all
  // ResUnits have started, so that they do not get incomplete
  // data. It causes no harm or error, but it can be confusing
  // to the user as the situational picture builds up
  sse = new ACPSimEvent(nuSim, SSCmndUnitUpdate);
  assert (NULL != sse);
  sse->data = (void*)(cuR);
  sse->processFN = updatePV;
  nuSim->schedule(nuSim->rng->uniform(70.0, 80.0), sse);
  sse = NULL;

  sse = new ACPSimEvent(nuSim, SSCmndUnitUpdate);
  assert (NULL != sse);
  sse->data = (void*)(cuB);
  sse->processFN = updatePV;
  nuSim->schedule(nuSim->rng->uniform(70.0, 80.0), sse);
  sse = NULL;

  cout << "Created " << nB << " Blue units"<<endl<<flush;
  cout << "Created " << nR << "  Red units"<<endl<<flush;
  cout << "Setup ScenarioOneCU"<<endl;
  mySim = nuSim;
  return;
}

// ----------------------------------

void
SimGUIModule::setupScenarioLayeredCU() {
  using AAA::GVector;
  cout << " starting SimGUIModule::setupScenarioLayeredCU "<<endl<<flush;
  ACPSim* nuSim = NULL;

  if (true == ACPSim::RepeatableSeedP) {
    cout << "ScenarioLayeredCU using sim seed: " << ACPSim::RepeatableSeed << endl;
    nuSim = new ACPSim(ACPSim::RepeatableSeed);
  }
  else {
    ACPSim::RepeatableSeed = 0;
    cout << "ScenarioLayeredCU using irrepeatabile sim seed" << endl;
    nuSim = new ACPSim();
  }

  ACPSimEvent*  sse = NULL;

  //  float metersPerCell = 1500.0;
  float ty = 135 * 1000.0;
  float tx = ty * 1.250;
  //   int r = ((int) (0.5 + (ty/metersPerCell)));
  //   int c = ((int) (0.5 + (tx/metersPerCell)));

  int r = 256 + 1;
  int c = 256 + 1;

  double minT = 1.0;
  double dt = 10.0;
  unsigned long int d = 3;


  nuSim->tgrid = new TGrid(r, c, tx, ty);
  //  nuSim->tgrid->synthesizeTerrainGaussian(nuSim->rng, -ty/100.0,ty/20.0, 0.6, 6);
  nuSim->tgrid->synthesizeTerrainFractal(nuSim->rng, -ty/100.0,ty/20.0, 0.15);


  centerY = ty/2.0;
  centerX = tx/2.0;
  GVector center = GVector(centerX, centerY, 0.0);
  double x = tx * nuSim->rng->uniform(0.01, 0.99);
  double y = ty * nuSim->rng->uniform(0.01, 0.99);
  GVector tmpGV = GVector(x, y, nuSim->rng->uniform(0.0, 100.0));

  // cu1 must keep its concrete type: Unit is a VIRTUAL base of
  // CmndUnit, so a Unit* differs numerically from the CmndUnit*,
  // and processEvent casts the event's data back to CmndUnit*.
  CmndUnit* cu1 = NULL;
  Unit* ru1 = NULL;
  ru1 = new ResUnit(nuSim, BlueSide, tmpGV, GVector(0.0, 0.0, 0.0));

  MovementRule* mc = new MRBuddies2(1, 500.0, 60.0, ru1);
  delete mc;
  mc = NULL;


  cu1 = simpleHierarchy1(nuSim, BlueSide, center,
			 minT, dt, d);

  sse = new ACPSimEvent(nuSim, SSCmndUnitUpdate);
  assert (NULL != sse);
  sse->data = (void*)(cu1);
  sse->processFN = updatePV;
  double t0 = minT + (2.0 * d * dt);
  double t1 = t0 + dt;
  double t2 = nuSim->rng->uniform(t0, t1);
  cout << "Scheduling cmnd unit at " << d << " at time " << t2 << endl;
  nuSim->schedule(t2, sse);

  cout << "Setup ScenarioLayeredCU"<<endl;
  mySim = nuSim;
  return;  return;
}

void
SimGUIModule::setupScenarioRecursiveCorr() {
  using AAA::GVector;

  cout << " starting SimGUIModule::setupScenarioRecursiveCorr "<<endl<<flush;
  ACPSim* nuSim = NULL;


  if (true == ACPSim::RepeatableSeedP) {
    cout << "ScenarioRecursiveCorr using sim seed: " << ACPSim::RepeatableSeed << endl;
    nuSim = new ACPSim(ACPSim::RepeatableSeed);
  }
  else {
    ACPSim::RepeatableSeed = 0;
    cout << "ScenarioRecursiveCorr using irrepeatabile sim seed" << endl;
    nuSim = new ACPSim();
  }

  float metersPerCell = 1500.0;
  float tx = 65000.0;
  float ty = 40000.0;
  int r = ((int) (0.5 + (ty/metersPerCell)));
  int c = ((int) (0.5 + (tx/metersPerCell)));
  nuSim->tgrid = new TGrid(r, c, tx, ty);

  // %%% non-zero elevations make it crash?
  //  nuSim->tgrid->synthesizeTerrainGaussian(nuSim->rng, -ty/100.0,ty/20.0, 0.6, 6);

  centerX = tx/2.0;
  centerY = ty/2.0;
  GVector center = GVector (centerX, centerY, 0.0);
  unsigned int i = 0;
  unsigned int numBoxes = 5;

  Alignment side = BlueSide;
  GVector tmpGV = GVector (0.0, 0.0, 0.0);
  ACPSimEvent*  sse = NULL;

  float bxNs = 1500.0;

  Box* bx1 = NULL;
  Box* bx2 = NULL;
  Box* bx3 = NULL;
  Box* bx4 = NULL;
  float assumedSpeed = 7.0; // meters per second
  float t1 = 0.0;
  float t2 = 0.0;
  float t3 = 0.0;
  float t4 = 0.0;
  float dt = 0.0;
  TempCorridor* tc2 = NULL;
  //  double p = 0.0; // useful in randomly setting subDepth and subBranching
  unsigned int subDepth = 0;
  unsigned int subBranching = 0;
  CmndUnit *cu2 = NULL;


  subDepth = 3;
  subBranching = 6;

  if (true) {
  bx1 = new Box (GVector( 5000.0 + nuSim->rng->uniform(-bxNs, +bxNs),  5000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 0.0),
		 GVector(10000.0 + nuSim->rng->uniform(-bxNs, +bxNs),  5000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 0.0),
		 GVector(10000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 13000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 0.0),
		 GVector( 5000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 13000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 0.0));

  bx2 = new Box (GVector(50000.0 + nuSim->rng->uniform(-bxNs, +bxNs),  5000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 0.0),
		 GVector(58000.0 + nuSim->rng->uniform(-bxNs, +bxNs),  5000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 0.0),
		 GVector(58000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 10000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 0.0),
		 GVector(50000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 10000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 0.0));

  bx3 = new Box (GVector( 5000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 30000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 0.0),
		 GVector(10000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 30000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 0.0),
		 GVector(10000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 38000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 0.0),
		 GVector( 5000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 38000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 0.0));

  bx4 = new Box (GVector(50000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 30000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 0.0),
		 GVector(58000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 30000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 0.0),
		 GVector(58000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 35000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 0.0),
		 GVector(50000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 35000.0 + nuSim->rng->uniform(-bxNs, +bxNs), 0.0));



  t1 = 0.0;
  dt = dist( GVector(0.0, 0.0, 0.0),  bx1->center) / assumedSpeed;

  tc2 = createTempCorridor(bx1, t1+(0.95*dt), t1+dt);

  t2 = t1 + dt;
  dt = dist(bx1->center, bx2->center) / assumedSpeed;

  extendTempCorridor(tc2, bx2, t2 + (0.95 * dt), t2+dt);

  t3 = t2 + dt;
  dt = dist(bx2->center, bx3->center) / assumedSpeed;

  extendTempCorridor(tc2, bx3, t3 + (0.95 * dt), t3+dt);

  t4 = t3 + dt;
  dt = dist(bx3->center, bx4->center) / assumedSpeed;

  extendTempCorridor(tc2, bx4, t4 + (0.95 * dt), t4+dt);
  // the boxes we created above are NOT the ones actually
  // put into tc2, so we separately delete them and then tc2.
  // the copies in nusim->boxes will be deleted when the whole
  // sim is deleted
  delete bx1;
  bx1 = NULL;

  delete bx2;
  bx2 = NULL;

  delete bx3;
  bx3 = NULL;

  delete bx4;
  bx4 = NULL;

  }
  else {
    tc2 = makeRandomCorridor(theSim->rng, tx, ty, numBoxes);
  }

  // these are for display, as tc2 will clean up its own boxes
  for (i=0; i<tc2->size(); i++) {
    nuSim->boxes->push_back( new Box((*(tc2->boxes))[i] ) );
  }

  cu2 = new CmndUnit(nuSim, side, GVector(0.0, 0.0, 0.0), GVector(0.0, 0.0, 0.0));
  attachSubUnits(nuSim, cu2, subBranching, subDepth,
		 1500.0, 1500.0,
		 3000.0, 3000.0);


  GVector cuCG = cu2->currentPos();

  cout << "CU is at "<<cuCG << endl<<flush;
  AAA::FSM* fsm3 = cu2->makeCorridorFSM(tc2);

  delete tc2;
  tc2 = NULL;

  cu2->setFSM(fsm3);


  sse = new ACPSimEvent(nuSim, SSCmndUnitUpdate);
  sse->data = (void*)(cu2);
  sse->processFN = updatePV;
  nuSim->schedule(5.0, sse);


  CmndUnit* cu3 = NULL;
  subDepth = 3;
  subBranching = 4;

  cu3 = new CmndUnit(nuSim, RedSide, GVector(0.0, 0.0, 0.0), GVector(0.0, 0.0, 0.0));
  attachSubUnits(nuSim, cu3, subBranching, subDepth,
		 15000.0, 25000.0,
		 20000.0, 30000.0);
  cuCG = cu3->currentPos();
  sse = new ACPSimEvent(nuSim, SSCmndUnitUpdate);
  sse->data = (void*)(cu3);
  sse->processFN = updatePV;
  nuSim->schedule(5.0, sse);

  if (true) {
  CmndUnit* cu4 = new CmndUnit(nuSim, RedSide, GVector(0.0, 0.0, 0.0), GVector(0.0, 0.0, 0.0));
  subDepth = 3;
  subBranching = 4;

  attachSubUnits(nuSim, cu4, subBranching, subDepth,
		 20000.0,  5000.0,
		 50000.0, 10000.0);
  cuCG = cu4->currentPos();
  sse = new ACPSimEvent(nuSim, SSCmndUnitUpdate);
  sse->data = (void*)(cu4);
  sse->processFN = updatePV;
  nuSim->schedule(5.0, sse);
  }

  cout << "Setup ScenarioRecursiveCorr"<<endl;
  mySim = nuSim;
  return;
}

void
SimGUIModule::setupScenarioRandomCorr() {
  using AAA::GVector;

  cout << " starting SimGUIModule::setupScenarioRandomCorr "<<endl<<flush;
  ACPSim* nuSim = NULL;


  if (true == ACPSim::RepeatableSeedP) {
    cout << "ScenarioRandomCorr using sim seed: " << ACPSim::RepeatableSeed << endl;
    nuSim = new ACPSim(ACPSim::RepeatableSeed);
  }
  else {
    ACPSim::RepeatableSeed = 0;
    cout << "ScenarioRandomCorr using irrepeatabile sim seed" << endl;
    nuSim = new ACPSim();
  }
  nuSim->setClock(0.0);
  float metersPerCell = 250.0;
  float tx = 5000.0;
  float ty = 7000.0;
  int r = ((int) (0.5 + (ty/metersPerCell)));
  int c = ((int) (0.5 + (tx/metersPerCell)));
  nuSim->tgrid = new TGrid(r, c, tx, ty);
  nuSim->tgrid->synthesizeTerrainGaussian(nuSim->rng, -ty/100.0,ty/20.0, 0.6, 6);

  centerX = tx/2.0;
  centerY = ty/2.0;
  GVector center = GVector (centerX, centerY, 0.0);
  unsigned int i = 0;
  unsigned int n = 12;

  TempCorridor* tc = NULL;

  tc = makeRandomCorridor(nuSim->rng, tx, ty, n);
  assert (NULL != tc);

  Alignment side = BlueSide;
  GVector tmpGV = GVector (0.0, 0.0, 0.0);
  ResUnit* ru1 = new ResUnit(nuSim, side, tmpGV, tmpGV);

  AAA::FSM* fsm = ru1->makeCorridorFSM(tc);
  ru1->setFSM(fsm);

  for (i=0; i<n; i++) {
    nuSim->boxes->push_back( new Box((*(tc->boxes))[i] ) );
  }

  // the FSM has separate boxes, created from the corridor,
  // as does the simulation's 'boxes' vector<>,
  // so we can delete this w/o harmful interactions
  delete tc;
  tc = NULL;

  ACPSimEvent*  sse = new ACPSimEvent(nuSim, SSStateUpdate);
  assert (NULL != sse);
  sse->data = (void*)(ru1);
  sse->processFN = updatePV;
  nuSim->schedule(1.0, sse); // first event after 1 second


  cout << "Setup ScenarioRandomCorr"<<endl;
  mySim = nuSim;
  return;
}

// ----------------------------------



Box*
createBox(panj::PRNG *rng, double xMin, double yMin, double dx, double dy) {
  using AAA::GVector;
  if (true == ACPSim::traceGeometry) {
    cout<<endl<<"Creating random box"<<endl<<flush;
  }
  Box *bx1 = NULL;
  Logical rslt = LFalse;
  double xLow = xMin;
  double yLow = yMin;
  double offset = 0.20; // this must be under .3333 or it will generate concave, ill-formed boxes
  double xHigh = xLow + dx;
  double yHigh = yLow + dy;

  assert (xLow > 0);
  assert (xHigh > xLow);
  assert (yLow > 0);
  assert (yHigh > yLow);


  while (LFalse == rslt) {
    if (NULL != bx1) {
      delete bx1;
      bx1 = NULL;
    }
    bx1 = new Box (
		   GVector(xLow, yLow, 0.0),
 		   GVector(xHigh + (dx * rng->uniform(-offset, + offset)),
			   yLow + (dy * rng->uniform(-offset, + offset)),
			   0.0),
 		   GVector(xHigh + (dx * rng->uniform(-offset, + offset)),
			   yHigh + (dy * rng->uniform(-offset, + offset)),
			   0.0),
 		   GVector(xLow + (dx * rng->uniform(-offset, + offset)),
			   yHigh + (dy * rng->uniform(-offset, + offset)),
			   0.0)

		   );
    if (true == ACPSim::traceGeometry) {
      cout << "Created box " << *bx1 <<endl<<flush;
      rslt = bx1->check_integrity();
      cout << "  ... box was OK"<<endl<<flush;
    }
    rslt = bx1->check_integrity();
  }
  assert(LTrue == bx1->check_integrity());
  if (true == ACPSim::traceGeometry) {
    cout<<"Created random box"<<endl<<flush;
  }
  return bx1;
}

// ------------------------------------------


//  create a random corridor of N boxes,
// four basic patterns are possible, from little randomness to a lot
TempCorridor*
makeRandomCorridor(panj::PRNG* rng,
		   double xMax, double yMax,
		   unsigned int numBoxes) {
  if (true == ACPSim::traceGeometry) {
    cout<<"Creating random corridor with "<<numBoxes<<" boxes"<<endl<<flush;
  }
  TempCorridor *cr1 = NULL;
  Box *bx1 = NULL;
  Box *bx2 = NULL;
  Box *rbx1 = NULL;
  Box *rbx2 = NULL;

  // 0 = row
  // 1 = column
  // 2 = mixed
  // 3 = random (on same grid)
  // 4 = fully random (off grid)
  unsigned int corrType = 3;

  double nuTime, t1, t2;
  double assumedSpeed = 8.0; // 8 meters per second
  unsigned int i = 0;
  unsigned int j = 0;
  unsigned int k = 0;
  Logical overlapP = LTrue;

  double xRow = 0.0; // step to the right
  double yClm = 0.0; // step to the bottom

  double xClm = xMax / 2.0;
  double yRow = yMax / 2.0;

  double x = 0.0;
  double y = 0.0;

  double dx = xMax / (3.0 * numBoxes);
  double dy = yMax / (3.0 * numBoxes);

  double effDX = dx; // from dx to 2*dx
  double effDY = dy; // from dy to 2*dy


  // this creates a rotated box
  //   bx0 = new Box(GVector(dx,0), GVector(dx*2, dy), GVector(dx, dy*2), GVector(0, dy));

  switch (corrType) {
  case 0: // row
    bx1 = createBox(rng, dx, yRow, dx, dy);
    break;

  case 1: // column
    bx1 = createBox(rng, xClm, dy, dx, dy);
    break;

  case 2: // mixed
    bx1 = createBox(rng, dx, dy, dx, dy);
    break;

  case 3: // random, on grid
    bx1 = createBox(rng, dx, dy, dx, dy);
    break;

  default:
    assert (false);
    break;
  }

  if (true == ACPSim::traceGeometry) {
    cout<<"First box:"<<endl<<(*bx1)<<endl<<flush;
  }

  nuTime = 0;
  t1 = dist(AAA::GVector(0.0, 0.0, 0.0),  bx1->center) / assumedSpeed;
  cr1 = createTempCorridor(bx1, nuTime + 0.95 * t1, nuTime +  t1);

  for (i=1; i<numBoxes;i++) {
    nuTime = nuTime + t1;

    // in row, clm, and mixed, the boxes can not
    // possibly overlap. but the overlap checking
    // is still there in preparation for the
    // fully random case
    xRow = dx + (3.0 * i * dx);
    yClm = dy + (3.0 * i * dy);

    overlapP = LTrue;
    while (LTrue == overlapP) {

      effDX = rng->uniform(1.0, 2.0) * dx;
      effDY = rng->uniform(1.0, 2.0) * dy;

      switch (corrType) {
      case 0: // row
	cout << "row"<<endl<<flush;
	x = xRow;
	y = yRow;
	break;

      case 1: // clm
	cout << "clm"<<endl<<flush;
	x = xClm;
	y = yClm;
	break;

      case 2: // mixed
	cout << "mxd"<<endl<<flush;
	if (rng->uniform(0.0, 1.0) < 0.5) {
	  x = xRow;
	  y = yRow;
	}
	else {
	  x = xClm;
	  y = yClm;
	}

	break;

      case 3: // random, on grid
	j = ((int) (0.5 + rng->uniform(0.0, numBoxes-2.0)));
	k = ((int) (0.5 + rng->uniform(0.0, numBoxes-2.0)));
	x = dx + (3.0 * j * dx);
	y = dy + (3.0 * k * dy);

	break;
      default:
	assert (false);
	break;

      }

      // last one is always in the bottom-right corner
      if (numBoxes-1 == i) {
	x = xRow - dx;
	y = yClm - dy;
      }

      bx2 = createBox(rng, x, y, effDX, effDY);


      // make sure they do not directly overlap
      overlapP = box_intersect_p(bx1, bx2);
      if (overlapP) {
	if (true == ACPSim::traceGeometry) {

	  cout << "They overlapped: "<<endl;
	  cout << *bx1 << endl;
	  cout << *bx2 << endl;
	  cout << "Trying again ..."<<endl;
	  cout <<flush;
	}
	delete bx2;
	bx2 = NULL;
      }

      // We do not make sure bx2 is in front of bx1.
      // To require that, as well as that
      // bx2's back face bx1, would be over-constrained
      // and usually impossible to achieve.

    }

    if (true == ACPSim::traceGeometry) {
      cout<<i<<"-th box:"<<endl<<(*bx2)<<endl<<flush;
    }

    t2 = dist (bx1->center, bx2->center) / assumedSpeed;
    extendTempCorridor(cr1, bx2, nuTime + .95 * t2, nuTime +t2);

    // test regularization (note that bx2 was rotated by extendTempCorridor)
    rbx1 = NULL;
    rbx2 = NULL;
    if (false) {
      regularizeBoxPair(bx1, bx2, rbx1, rbx2);
      assert (NULL != rbx1);
      assert (NULL != rbx2);

      if (false) {
	// this memory leaking hack is to let me see intermediate products
	//      theSim->boxes->push_back(rbx1);
	//      theSim->boxes->push_back(rbx2);
      }
      else {
	delete rbx1;
	rbx1 = NULL;
	delete rbx2;
	rbx2 = NULL;
      }
      rbx1 = NULL;
      rbx2 = NULL;
    }

    nuTime = nuTime + t2;

    delete bx1;
    bx1 = bx2;
  }

  bx1 = NULL;
  delete bx2;
  bx2 = NULL;

  if (true == ACPSim::traceGeometry) {
    cout<<"Created random corridor with "<<numBoxes<<" boxes"<<endl<<flush;
  }
  return cr1;
}


// ----------------------------------

CmndUnit* simpleHierarchy1(ACPSim* sim,
			   Alignment side,
			   AAA::GVector center,
			   double minT, double dt,
			   unsigned long int depth) {
  using AAA::GVector;
  assert (NULL != sim);

  GVector v0 = GVector(0.0, 0.0, 0.0);
  double spread = 100.0;
  assert (depth > 0);
  unsigned long int i = 0;
  unsigned long int n = 0;
  ACPSimEvent*  sse = NULL;
  float subSpacing = expt(3.0, depth - 1) * 1000.0;
  float subTC = expt(1.5, depth - 1) * 60.0;
  double t2 = 0.0;
  Unit* su = NULL;
  CmndUnit* cu = new CmndUnit(sim, side, center, v0);
  MovementRule* mc = NULL;
  GVector dp;

  if (1 == depth) {
    n = 5;
    for (i=0; i<n; i++) {
      dp = GVector(sim->rng->uniform(-spread, spread),
		   sim->rng->uniform(-spread, spread),
		   sim->rng->uniform(-spread, spread)); // include vertical
      // rsu keeps the concrete type: Unit is a VIRTUAL base of
      // ResUnit, so a Unit* differs numerically from the ResUnit*,
      // and processEvent casts the event's data back to ResUnit*.
      ResUnit* rsu = new ResUnit(sim, side, center + dp, v0);
      su = rsu;
      // these are the guys at d=0
      // these RU should be ordered to stay 500m from their siblings
      mc = new MRBuddies2(1,  // nth siblings
			  500.0, // meters spacing
			  60.0, // seconds adjustment
			  su);

      su->moveController = mc;


      // schedule the resunit su
      sse = new ACPSimEvent(sim, SSStateUpdate);
      assert (NULL != sse);
      sse->data = (void*)(rsu);
      sse->processFN = updatePV;
      t2 = sim->rng->uniform(minT, minT+dt);
      //      cout << "Scheduling resunit for " << t2 << endl;
      sim->schedule(t2, sse);

      cu->add_sub(su);
      // this is the guy at d=1
      // this CU should be orderd to stay 1000m from siblings
      // he should be scheduled between minT+2dt and minT+3dt
    }
  }
  else { // depth > 1
    n = 3;
    for (i=0; i<n; i++) {
      dp = GVector(sim->rng->uniform(-spread, spread),
		   sim->rng->uniform(-spread, spread),
		   sim->rng->uniform(-spread, spread)); // include vertical

      // csu keeps the concrete type (see the note on rsu above)
      CmndUnit* csu
	= simpleHierarchy1(sim, side, center + dp, minT, minT+dt, depth - 1);
      su = csu;
      // this guy is at level d-1, and
      // this CU should be orderd to stay 3^^(d-1) * 1000 from siblings
      cout << "Creating MRBuddies2 for depth " << depth-1;
      cout << " with spacing " << subSpacing << " and ";
      cout << subTC << endl << flush;
      mc = new MRBuddies2(1,  // nth siblings
			  subSpacing, // meters spacing
			  subTC, // seconds adjustment
			  su);

      su->moveController = mc;



      // this guy is at level d-1, and
      // he should be scheduled between minT+2dt and minT+3dt
      sse = new ACPSimEvent(sim, SSCmndUnitUpdate);
      assert (NULL != sse);
      sse->data = (void*)(csu);
      sse->processFN = updatePV;
      double t0 = minT + (2.0 * (depth - 1.0) * dt);
      double t1 = t0 + dt;
      t2 = sim->rng->uniform(t0, t1);
      cout << "Scheduling cmnd unit "<<su->getSimEntID()<<" at " << depth - 1 << " at time " << t2 << endl;
      sim->schedule(t2, sse);

      cu->add_sub(su);
    }
  }
  cout << flush;
  return cu;
}


// ----------------------------------

void
attachSubUnits(ACPSim* sim, CmndUnit *cu1, int numSubs, int depth,
	       float minX, float minY,
	       float maxX, float maxY) {
  int i;
  CmndUnit *cu2;
  panj::PRNG* rng = sim->rng;
  ACPSimEvent* sse = NULL;
  float t2 = 0.0;
  if (depth > 1) {
    for (i=0; i<numSubs; i++) {

      cu2 = new CmndUnit(sim, cu1->side,
			 AAA::GVector(rng->uniform(1000.0, 1100.0),
				 rng->uniform(1000.0, 1100.0),
				 0.0),
			 AAA::GVector(0.0, 0.0, 0.0));
      attachSubUnits(sim, cu2, numSubs, depth-1, minX, minY, maxX, maxY);
      assert (numSubs == cu2->subordinates->size());
      sse = new ACPSimEvent(sim, SSCmndUnitUpdate);
      sse->data = (void*)(cu2);
      sse->processFN = updatePV;
      t2 = rng->uniform(3.0, 4.0);
      sim->schedule(t2, sse);

      cu1->add_sub(cu2);
    }
  }
  else // if adding just 1 more layer, make them all ResUnits
    attachResUnits(sim, cu1, numSubs, minX, minY, maxX, maxY);

  assert (numSubs == cu1->subordinates->size());
  return;
}


void attachResUnits(ACPSim* sim, CmndUnit *cu1, int numSubs,
		    float minX, float minY,
		    float maxX, float maxY) {
  ResUnit *ru;
  int i;
  panj::PRNG* rng = sim->rng;
  ACPSimEvent* sse = NULL;
  float t2 = 0.0;
  float x = 0.0;
  float y = 0.0;
  float z = 0.0;
  for (i=0; i<numSubs; i++) {
    // these all have the default max speed of 10.0 m/s
    x = rng->uniform(minX, maxX);
    y = rng->uniform(minY, maxY);
    z = TGrid::StandardEntityHeight; // %%% not always ground level!
    ru = new ResUnit(sim, cu1->side,
		     AAA::GVector(x, y, z),
		     AAA::GVector(0, 0, 0));
    printf("Created ResUnit %li at %.2f %.2f %.2f\n",
	   ru->getSimEntID(), x, y, z);
    sse = new ACPSimEvent(sim, SSStateUpdate);
    assert (NULL != sse);
    sse->data = (void*)(ru);
    sse->processFN = updatePV;
    t2 = rng->uniform(1.0, 2.0);
    cout << "Scheduling resunit for " << t2 << endl;
    sim->schedule(t2, sse);

    cu1->add_sub(ru);
  }
  return;
}

// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
