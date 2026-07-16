// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: includes, evector -> std::vector
// (pop_back split), const array bound for MSVC.
//
// ------------------------------------------

#include "aaa.h"
#include "fsm.h"
#include "runit.h"
#include "tthread.h"
#include "orders.h"

#include "acpsim.h"
#include "mcontrol.h"
#include "lanch.h"

#include <vector>

extern ACPSim* theSim;


using AAA::Logical;
using AAA::LTrue;
using AAA::LFalse;
using AAA::LUnknown;

using AAA::GVector;
using std::vector;
using AAA::dmin;

// ------------------------------------------

MovementRule::MovementRule(Unit* ru) {
  assert (NULL != ru);
  myUnit = ru;
}

MovementRule::~MovementRule() {
  myUnit = NULL;
}


// ------------------------------------------


MRConst::MRConst(Unit* u, GVector v) : MovementRule(u) {
  vel = v;
}

MRConst::~MRConst() {
  // no-op
}

void MRConst::desiredVelocity(GVector& gv, double& dt) {
  dt = myUnit->maxUpdateInterval * theSim->rng->uniform(.80, 1.00); // seconds
  gv = vel;
  return;
}


// ------------------------------------------


MRPoint::MRPoint(Unit* ru, GVector gv, float tp, WayPointType wpt) : MovementRule(ru) {
  aimPoint = gv;
  wpType = wpt;
  tauPt = tp;
  assert (tauPt > 0.0);
}

MRPoint::~MRPoint() {
  // no-op
}



// for now, this ignores the wpType, and just returns
// the maximum speed vector toward the aimpoint
// (speed declines as it gets close)
void MRPoint::desiredVelocity(GVector& gv, double& dt) {
  gv = GVector(0.0, 0.0, 0.0);

  // get current position
  GVector p1 = myUnit->currentPos();

  // we ignore maxAccel for now

  float fSA = 0.0;
  float eSA = 0.0;
  float fSB = 0.0;
  float eSB = 0.0;
  float fS = 0.0;
  float eS = 0.0;

  if (true) {
    // %%% this is just a memory-leak test
    myUnit->strengthsInArea(p1.get(0) - 1000.0, p1.get(1) - 1000.0,
			    p1.get(0) + 1000.0, p1.get(1) + 1000.0,
			    fSA, eSA);
    myUnit->strengthsInArea(p1.get(0) - 500.0, p1.get(1) - 500.0,
			    p1.get(0) + 500.0, p1.get(1) + 500.0,
			    fSB, eSB);

    // %%% this is a memory leak test
    strengthInNonOverlapping(p1.get(0) - 1000.0, p1.get(1) - 1000.0,
			     p1.get(0) + 1000.0, p1.get(1) + 1000.0,
			     p1.get(0) - 500.0, p1.get(1) - 500.0,
			     p1.get(0) + 500.0, p1.get(1) + 500.0,
			     myUnit, fS, eS, true);

    // %%% this is a correctness check:
    if ((fS != fSA - fSB) || (eS != eSA - eSB )) {
      cout <<"Test of strengthInNonOverlapping"<<endl;
      cout << "fSA: "<<fSA<<", fSB: "<<fSB<<", fS:"<<fS<<endl;
      cout << "eSA: "<<eSA<<", eSB: "<<eSB<<", eS:"<<eS<<endl;
      cout<<endl<<flush;
    }
  }

  if (true == ACPSim::traceMoves) {
    cout << endl;
    cout << "Desired aimpoint of ResUnit " << myUnit->getSimEntID()<<" is " << aimPoint << endl;
    cout << " ResUnit " << myUnit->getSimEntID()<<" is at " <<p1 << endl;
  }
  double speed = 0.0;
  GVector displacement = aimPoint - p1;
  double distance = norm(displacement); // meters
  double stepDist = distance;

  assert (myUnit->maxStepDist > 0.0); // it had better be properly set
  assert (myUnit->maxUpdateInterval > 0.0); // it had better be properly set

  if (stepDist > myUnit->maxStepDist) {// gradually desynchronize
    stepDist = myUnit->maxStepDist * theSim->rng->uniform(.80, 1.00);
  }

  if (distance <= ( myUnit->posTolerance / 2.0)) {  // close enough to stop? 
    // no-op
    dt = myUnit->maxUpdateInterval * theSim->rng->uniform(.80, 1.00); // seconds
    gv = GVector(0.0, 0.0, 0.0);
  }
  else { // not close enough to stop
    assert (myUnit->maxSpeed > 0.0); // it had better to be properly set
    gv = bound( displacement/tauPt, myUnit->maxSpeed);
    speed = norm(gv);
    dt = stepDist / speed;
  }

  return;
}


void MRPoint::setAimPoint(GVector ap) {
  aimPoint = ap;
  return;
}


MRIntercept::MRIntercept(Unit* ru, Unit* trgt) : MovementRule(ru) {
  assert (NULL != trgt);
  trgtRU = trgt;
}

MRIntercept::~MRIntercept() {
  // no-op
}


void MRIntercept::desiredVelocity(GVector& gv, double& dt) {
  dt = 0.0;  // seconds
  double sPlan = 0.9 * myUnit->maxSpeed;
  GVector p = trgtRU->currentPos();
  GVector v = trgtRU->currentVel();
  GVector q = myUnit->currentPos();
  double tti = timeToIntercept(p, v, q, sPlan);

  if (tti > 0.0) {
    // interception is possible, with
    // P + V*tti = Q + W*tti
    gv = ((p - q) / tti) + v;
    dt = dmin (tti / 4.0, myUnit->maxUpdateInterval);
    dt = dt * theSim->rng->uniform(.80, 1.00);
  }
  else if (tti < 0.0) {
    // interception is not possible
    // do a ballistic pursuit instead
    gv = p - q;
    gv.scale_to(sPlan);
  }
  else {
    // you are there now!
    // %%% I do not know what to do, so I crash (fix this)
    assert (false);
  }

  return;
}



// ------------------------------------------

MRBuddies1::MRBuddies1(double sr, double s, double tb, Unit* ru) : MovementRule(ru) {

  assert (s > 0.0);
  assert (sr > s);
  assert (tb > 0.0);
  scanRange = sr;
  spacing = s;
  tauB = tb;
}

MRBuddies1::~MRBuddies1() {
  // no-op
}



void MRBuddies1::desiredVelocity(GVector& gv, double& dt) {
  const unsigned int NumNearBuddies = 4;
  bool buddiesPresentP = false;
  dt = myUnit->maxUpdateInterval * theSim->rng->uniform(.80, 1.00); // seconds
  gv = GVector(0.0, 0.0, 0.0);

  // without this, clusters of them will just keep
  // drifting, as each keeps his speed up, merely in
  // order to keep up with the others who are merely
  // keeping their speed up ...
  float speedDamping = 0.8;

  unsigned long int i = 0;
  unsigned long int j = 0;
  double di = 0.0;
  double dj = 0.0;
  double dTmp = 0.0;

  GVector qi;
  GVector ai;
  GVector vi;
  double wi;
  GVector piStar;
  GVector pAim;
  GVector pStar;
  GVector vStar;

  ResUnit* ruTmp = NULL;

  GVector ctr = myUnit->currentPos();
  GVector dx = GVector(scanRange, 0.0, 0.0);
  GVector dy = GVector(0.0, scanRange, 0.0);
  GVector a = ctr + dx - dy;
  GVector b = ctr + dx + dy;
  GVector c = ctr - dx + dy;
  GVector d = ctr - dx - dy;
  Box* bx = new Box(a,b,c,d);
  ResUnit* ru = NULL;
  vector<ResUnit*> *nearBuddies = myUnit->friendlyRUInArea(bx);
  unsigned long int numB = nearBuddies->size();

  // if there is only 1 friendly near, it is just myself
  if (numB > 1) {
    buddiesPresentP = true;
    //    cout << "Found " << numB << " buddies (including self)"<<endl<<flush;
    if (numB-1 > NumNearBuddies) {
      // need to find closest NumNearBuddies
      //      cout << "my center is at " << ctr << endl << flush;
      vector<float> *distances = new vector<float>();
      for (i=0; i<numB; i++) {
	//	cout << i << "-th buddy is "<<flush;
	ru = (*nearBuddies)[i];
	//	cout << ru->getSimEntID() << endl << flush;
	distances->push_back( dist(ctr, ru->currentPos()));
	//	cout << " he is " << (*distances)[i] << " away" << endl << flush;
      }

      // %%% could this sort be done by resUnitsNearLoc?
      // sort nearest to front
      for (i=0; i<numB-1; i++) {
	for (j=i+1; j<numB; j++) {
	  di = (*distances)[i];
	  dj = (*distances)[j];
	  if (dj < di) {
	    dTmp = dj;
	    (*distances)[j] = (*distances)[i];
	    (*distances)[i] = dTmp;

	    ruTmp = (*nearBuddies)[j];
	    (*nearBuddies)[j] = (*nearBuddies)[i];
	    (*nearBuddies)[i] = ruTmp;

	  }

	}
      } // end of bubble sort

      delete distances;
      distances = NULL;

      while (nearBuddies->size() > 1+NumNearBuddies)
	nearBuddies->pop_back();

    }
    numB = nearBuddies->size();
    assert (numB-1 <= NumNearBuddies);
    assert (numB > 1);


    pAim = GVector(0.0, 0.0, 0.0);
    for (i=0; i<numB; i++) {
      ru = (*nearBuddies)[i];
      if (myUnit != ru) { // skip yourself
	qi = ru->currentPos();
	ai = ctr - qi;
	wi = spacing/norm(ai);
	vi = ru->currentVel()  * speedDamping ;
	// piStar is where I would like to be, to make
	// my distance to my i-th buddy be exactly 'spacing'
	piStar = qi + ai*wi;

	// but he is moving:
	pAim = pAim + piStar + (vi*tauB);
      }
    }
    // actually aim for the mean of all those which are not yourself:
    pAim = pAim / (numB-1);

    vStar = (pAim - ctr) / tauB;


    assert (myUnit->maxSpeed > 0.0); // it had better to be properly set
    gv = bound(vStar, myUnit->maxSpeed);

    if (true == buddiesPresentP)
      dt = dt / 3.0;

  }
  else {
    // no buddies nearby, so desired velocity to
    // get into formation with them is zero
  }

  if (true == ACPSim::traceMoves) {
    cout << "With " << numB-1 << " buddies nearby, MRBuddies1 recommends vStar = " << gv;
    cout << " for unit " << myUnit->getSimEntID() << endl << flush;
  }

  delete bx;
  bx = NULL;

  delete nearBuddies;
  nearBuddies = NULL;
  return;
}


// ------------------------------------------

MRBuddies2::MRBuddies2(unsigned long int ns, double s,
		       double tb, Unit* ru) : MovementRule(ru) {

  assert (ns > 0);
  assert (s > 0.0);
  assert (tb > 0.0);
  nthSup = ns;
  spacing = s;
  tauB = tb;
  buddies = NULL;
}

MRBuddies2::~MRBuddies2() {
  if (NULL != buddies) {
    delete buddies;
    buddies = NULL;
  }
}

void MRBuddies2::desiredVelocity(GVector& gv, double& dt) {
  const unsigned int NumNearBuddies = 4;
  bool buddiesPresentP = false;
  unsigned long int i = 0;
  unsigned long int j = 0;
  double di = 0.0;
  double dj = 0.0;
  double dTmp = 0.0;

  double now = theSim->clock();

  float speedDamping = 0.8;
  GVector qi;
  GVector ai;
  GVector vi;
  double wi;
  GVector piStar;
  GVector pAim;
  GVector pStar;
  GVector vStar;

  bool hiEchelon;

  if (spacing > 750.0)
    hiEchelon = true;
  else
      hiEchelon = false;
  

  // %%% this little bit of debugging code revealed that higher levels
  // were never running, and hence to the realization that higher level
  // code needed to be written which was quite different than for low level.
  if (false) {
  if (hiEchelon) {
    cout << "Computing MRBuddies2::desiredVelocity for hiEchelon " << myUnit->side;
    cout << " unit " << myUnit->getSimEntID() << " at time "<<now<< endl;
    cout << " spacing: " << spacing << endl;
    
  }
else {
    cout << "Computing MRBuddies2::desiredVelocity for lowEchelon " << myUnit->side;
    cout << " unit " << myUnit->getSimEntID() << " at time "<<now<< endl;
    cout << " spacing: " << spacing << endl;
    
  }
  }
  unsigned long int numB = 0;
  dt = myUnit->maxUpdateInterval * theSim->rng->uniform(.80, 1.00); // seconds
  gv = GVector(0.0, 0.0, 0.0);
  Unit* u = NULL;
  Unit* u2 = NULL;
  GVector ctr = myUnit->currentPos();
  vector<Unit*> *nearBuddies = NULL;

  // notice that we really only compute this once,
  // to determine who are our nth-siblings in the hierachy
  if (NULL == buddies) {
    CmndUnit* cu = myUnit->nthSuperior(nthSup);
    if (NULL != cu) {
      buddies = cu->nthSubordinates(nthSup);
      assert (NULL != buddies);
    }
  }

  if (NULL != buddies) {
    numB = buddies->size();
    assert (numB > 0);// at least myUnit must be there

    nearBuddies = new vector<Unit*>();
    
    // pick out the live ones that are not myUnit
    for (i=0; i<numB; i++) {
      u = (*buddies)[i];
      if ((LTrue == u->aliveP) && ( u != myUnit))
	nearBuddies->push_back(u);
    }

    numB = nearBuddies->size();
    if (numB > 0) { // if there are some live buddies nearby

      if (numB > NumNearBuddies) { // sort them
	vector<float> *distances = new vector<float>();
	for (i=0; i<numB; i++) {
	  u = (*buddies)[i];
	  distances->push_back( dist (ctr, u->currentPos() ) );
	}
	for (i=0; i<numB-1; i++) {
	  for (j=i; j<numB; j++) {
	    di = (*distances)[i];
	    dj = (*distances)[j];
	    if (dj < di) {
	      dTmp = dj;
	      (*distances)[j] = (*distances)[i];
	      (*distances)[i] = dTmp;

	      u2 = (*nearBuddies)[j];
	      (*nearBuddies)[j] = (*nearBuddies)[i];
	      (*nearBuddies)[i] = u2;

	    }
	  }
	}
	delete distances;
	distances = NULL;
      } // end of if (numB > NumNearBuddies)

      while (nearBuddies->size() > NumNearBuddies)
	nearBuddies->pop_back();

      numB = nearBuddies->size();
      // we started with more than one, so we should
      // still have at least one
      assert (numB > 0);
      buddiesPresentP = true;
      // we should not have more than the maximum
      assert (numB <= NumNearBuddies);

      // at this point, we have some positive number of near, live buddies
      // so we try to find a reasonable compromise aim point

      pAim = GVector(0.0, 0.0, 0.0);
      for (i=0; i<numB; i++) {
	u = (*nearBuddies)[i];
	qi = u->currentPos();
	ai = ctr - qi;
	wi = spacing/norm(ai);
	vi = u->currentVel()  * speedDamping ;
	// piStar is where I would like to be, to make
	// my distance to my i-th buddy be exactly 'spacing'
	piStar = qi + ai*wi;

	// but he is moving:
	pAim = pAim + piStar + (vi*tauB);

      }
    // actually aim for the mean of all those:
    pAim = pAim / numB;

    vStar = (pAim - ctr) / tauB;


    assert (myUnit->maxSpeed > 0.0); // it had better to be properly set
    gv = bound(vStar, myUnit->maxSpeed);

    if (true == buddiesPresentP)
      dt = dt / 3.0;


    } // end of if (numB > 0)

  } // end of if (NULL != buddies)


  if (NULL != nearBuddies) {
    delete nearBuddies;
    nearBuddies = NULL;
  }

  return;
}

// ------------------------------------------


MRForce1::MRForce1(double w, double h, double l,
		   double a, double b,
		   double ur, double ub,
		   double tf, Unit* ru) : MovementRule (ru) {

  width = w;
  height = h;

  lambda = l;
  alpha = a;
  beta = b;

  uRed = ur;
  uBlue = ub;

  tauF = tf;
  assert (width > 0.0);
  assert (height > 0.0);

  assert (lambda > 0.0);
  assert (1.0 >= lambda);

  assert (alpha > 0.0);
  assert (beta > 0.0);

  assert (uRed > 0.0);
  assert (uBlue > 0.0);
  assert (tauF > 0.0);

}

MRForce1::~MRForce1() {
  // no-op
}

// box B must be strictly inside box A
void strengthInNonOverlapping (double ax0, double ay0, double ax1, double ay1,
			       double bx0, double by0, double bx1, double by1,
			       Unit* ru, float &fS, float &eS,
			       bool useSmallP) {

  const double areaA = (ax1-ax0)*(ay1-ay0);
  const double areaB = (bx1-bx0)*(by1-by0);

  if (false) {
    cout << flush;
    cout << "strengthInNonOverlapping: " << endl;
    cout << ax0 << endl;
    cout << ay0 << endl;

    cout << ax1 << endl;
    cout << ay1 << endl;

    cout << bx0 << endl;
    cout << by0 << endl;

    cout << bx1 << endl;
    cout << by1 << endl;


    cout << "area: " << areaA << " > " << areaB << endl;

    // must have positive area
    cout << ax0 << " < " << ax1 << endl << flush;
    cout << ay0 << " < " << ay1 << endl << flush;

    // must have positive area
    cout << bx0 << " < " << bx1 << endl << flush;
    cout << by0 << " < " << by1 << endl << flush;

    cout << endl << flush;
  }
  assert (ax0 < ax1);
  assert (ay0 < ay1);
  assert (bx0 < bx1);
  assert (by0 < by1);

  assert (ax0 <= bx0);
  assert (bx1 <= ax1);

  assert (ay0 <= by0);
  assert (by1 <= ay1);

  assert (areaA > areaB);

  float fsA = 0.0;
  float esA = 0.0;
  float fsB = 0.0;
  float esB = 0.0;

  ru->strengthsInArea(ax0, ay0, ax1, ay1, fsA, esA);
  ru->strengthsInArea(bx0, by0, bx1, by1, fsB, esB);

  if (useSmallP) {
    if (fsB > fsA) {
      cout << "Found fsA="<<fsA<<" but fsB="<<fsB<<endl;
      fS = 0.0;
    }
    else
      fS = fsA - fsB;
  }
  else
    fS = fsA;

  if (useSmallP) {
    if (esB > esA) {
      cout << "Found esA="<<esA<<" but esB="<<esB<<endl;
      eS = 0.0;
    }
    else
      eS = esA - esB;
  }
  else
    eS = esA;

  return;
}


// finds strength in the big box which is not in the small box,
// assumes the small box is inside the large box
//
// dxB must point strictly in positive X direction (right, east)
// dyB must point strictly in positive Y direction (down, south)
// similarly for dxS, dyS
//
void strengthInNonOverlapping (GVector ctrB, GVector dxB, GVector dyB, 
			       GVector ctrS, GVector dxS, GVector dyS, 
			       Unit* ru, float &fS, float &eS,
			       bool useSmallP) {
  float fsB = 0.0;
  float esB = 0.0;
  float fsS = 0.0;
  float esS = 0.0;

  float areaB = 0.0;
  float areaS = 0.0;

  GVector tmpV = GVector(0.0, 0.0, 0.0);

  GVector aB = ctrB + dxB - dyB;
  GVector bB = ctrB + dxB + dyB;
  GVector cB = ctrB - dxB + dyB;
  GVector dB = ctrB - dxB - dyB;

  Box* boxB = new Box(aB, bB, cB, dB);
  areaB = boxB->area();
  ru->friendlyStrengthInArea(boxB, fsB, tmpV);
  ru->enemyStrengthInArea(boxB, esB, tmpV);
  delete boxB;
  boxB = NULL;

  if (useSmallP) {
    GVector aS = ctrS + dxS - dyS;
    GVector bS = ctrS + dxS + dyS;
    GVector cS = ctrS - dxS + dyS;
    GVector dS = ctrS - dxS - dyS;

    Box* boxS = new Box(aS, bS, cS, dS);
    areaS = boxS->area();
    ru->friendlyStrengthInArea(boxS, fsS, tmpV);
    ru->enemyStrengthInArea(boxS, esS, tmpV);
    delete boxS;
    boxS = NULL;
  }

  fS = fsB - fsS;
  eS = esB - esS;

  if (false) {
    cout << "areaB: " << areaB << ", areaS: " << areaS;
    if (useSmallP)
      cout << ", ratio: " << areaB/areaS;
    cout << endl;
    cout << "fsB: " << fsB << ", fsS: " << fsS << " so fS: " << fS << endl;
    cout << "esB: " << esB << ", esS: " << esS << " so eS: " << eS << endl;
    cout << flush;
  }

  // You might think these were assured,
  // but they are not.
  // Logically, they should be. It appears
  // to due to flaky numerical effects
  // when things are right at the border.
  //   assert (fsB >= fsS);
  //   assert (esB >= esS);
  if (fS < 0.0)
    fS = 0.0;

  if (eS < 0.0)
    eS = 0.0;

  // if the small box is inside the big one,
  // the following must be true. it is trivially
  // true if we do not use the small box.
  //
  assert (areaB > areaS);

  return;
}

// we are using screen coordinates here: 
// East is increasing X,
// South is increasing Y
// boxes must be CW order
void MRForce1::desiredVelocityTMP(GVector& gv, double& dt) {

  // seconds
  dt = myUnit->maxUpdateInterval * theSim->rng->uniform(.80, 1.00);
  bool enemiesPresentP = false;
  gv = GVector(0.0, 0.0, 0.0);
  float mB = 0.0; // strength of blue in current box
  float mR = 0.0; // strength of Red in current box

  // each list is in the same order:
  // current, N, S, E, W, NE, NW, SE, SW
  vector<float> *utils = new vector<float>();
  vector<GVector*> *boxCenters = new vector<GVector*>();

  float nB = 0.0;
  float nR = 0.0;

  float u = 0.0;

  GVector p0 = myUnit->currentPos();

  if (true == ACPSim::traceMoves) {
    cout << "for MRForce1::desiredVelocityTMP, Unit "<<myUnit->getSimEntID() << " is at " << p0;
    cout << " at time " << theSim->clock () << endl << flush;
  }

  // location and dimensions of the box being considered
  GVector* ctr = NULL;
  GVector dx = GVector(width/2.0, 0.0, 0.0);
  GVector dy = GVector(0.0, height/2.0, 0.0);

  // -------------
  // current, N, S, E, W, NE, NW, SE, SW

  // current
  cout << "Current:" << endl << flush;
  ctr = new GVector(p0);
  strengthInNonOverlapping(*ctr, dx, dy,
			   ((*ctr) + p0)/2.0, dx, dy,
			   myUnit, mB, mR,
			   false);
  assert (mB > 0.0); // at least this unit itself is there
  if (mR > 0)
    enemiesPresentP = true;
  boxCenters->push_back(ctr);
  u = blueUtil(mB, mR, alpha, beta, uRed, uBlue);

  // suppose we have an enemy who is in the current scan box,
  // and (say) the north one, but outside weapon range as long
  // as we stay in the current one.
  // the utilities are tied, so we stay put: seeing him,
  // wanting to go kill him, but not pursuing him.
  // so we very slightly lower the utility of the current box,
  // so that we move in this tied case.
  if (u > 0.0)
    u = (u * 0.975);

  utils->push_back(u);

  if (true == ACPSim::traceMoves) {
    cout << "Current box " << *ctr << " has B=" << mB << ", R="<<mR<<", U="<<u;
    cout << " so we compare B=" << mB << " vs R="<<mR<<", U="<<u;
    cout << endl << endl<<flush;
  }
  // N
  cout << "North:" << endl << flush;
  ctr = new GVector(p0 - dy);
  strengthInNonOverlapping(*ctr, dx, dy,
			   ((*ctr) + p0)/2.0, dx, dy/2.0,
			   myUnit, nB, nR,
			   true);
  if (nR > 0)
    enemiesPresentP = true;

  boxCenters->push_back(ctr);
  u = blueUtil(mB + (lambda * nB), nR, alpha, beta, uRed, uBlue);
  utils->push_back(u);
  if (true == ACPSim::traceMoves) {
    cout << "North box " << *ctr << " has B=" << nB << ", R="<<nR;
    cout << " so we compare B=" << mB+(lambda*nB) << " vs R="<<nR<<", U="<<u;
    cout << endl << endl<<flush;
  }

  // S
  cout << "South:" << endl << flush;
  ctr = new GVector(p0 + dy);
  strengthInNonOverlapping(*ctr, dx, dy,
			   ((*ctr) + p0)/2.0, dx, dy/2.0,
			   myUnit, nB, nR,
			   true);
  if (nR > 0)
    enemiesPresentP = true;

  boxCenters->push_back(ctr);
  u = blueUtil(mB + (lambda * nB), nR, alpha, beta, uRed, uBlue);
  utils->push_back(u);
  if (true == ACPSim::traceMoves) {
    cout << "South box " << *ctr << " has B=" << nB << ", R="<<nR;
    cout << " so we compare B=" << mB+(lambda*nB) << " vs R="<<nR<<", U="<<u;
    cout << endl << endl<<flush;
  }


  // E
  cout << "East:" << endl << flush;
  ctr = new GVector(p0 + dx);
  strengthInNonOverlapping(*ctr, dx, dy,
			   ((*ctr) + p0)/2.0, dx/2.0, dy,
			   myUnit, nB, nR,
			   true);
  if (nR > 0)
    enemiesPresentP = true;

  boxCenters->push_back(ctr);
  u = blueUtil(mB + (lambda * nB), nR, alpha, beta, uRed, uBlue);
  utils->push_back(u);
  if (true == ACPSim::traceMoves) {
    cout << "East box " << *ctr << " has B=" << nB << ", R="<<nR;
    cout << " so we compare B=" << mB+(lambda*nB) << " vs R="<<nR<<", U="<<u;
    cout << endl << endl<<flush;
  }


  // W
  cout << "West:" << endl << flush;
  ctr = new GVector(p0 - dx);
  strengthInNonOverlapping(*ctr, dx, dy,
			   ((*ctr) + p0)/2.0, dx/2.0, dy,
			   myUnit, nB, nR,
			   true);
  if (nR > 0)
    enemiesPresentP = true;

  boxCenters->push_back(ctr);
  u = blueUtil(mB + (lambda * nB), nR, alpha, beta, uRed, uBlue);
  utils->push_back(u);
  if (true == ACPSim::traceMoves) {
    cout << "West box " << *ctr << " has B=" << nB << ", R="<<nR;
    cout << " so we compare B=" << mB+(lambda*nB) << " vs R="<<nR<<", U="<<u;
    cout << endl << endl<<flush;
  }


  // NE
  cout << "N-East:" << endl << flush;
  ctr = new GVector(p0 - dy + dx);
  strengthInNonOverlapping(*ctr, dx, dy,
			   ((*ctr) + p0)/2.0, dx/2.0, dy/2.0,
			   myUnit, nB, nR,
			   true);
  if (nR > 0)
    enemiesPresentP = true;

  boxCenters->push_back(ctr);
  u = blueUtil(mB + (lambda * nB), nR, alpha, beta, uRed, uBlue);
  utils->push_back(u);
  if (true == ACPSim::traceMoves) {
    cout << "NE box " << *ctr << " has B=" << nB << ", R="<<nR;
    cout << " so we compare B=" << mB+(lambda*nB) << " vs R="<<nR<<", U="<<u;
    cout << endl << endl<<flush;
  }


  // NW
  cout << "N-West:" << endl << flush;
  ctr = new GVector(p0 - dy - dx);
  strengthInNonOverlapping(*ctr, dx, dy,
			   ((*ctr) + p0)/2.0, dx/2.0, dy/2.0,
			   myUnit, nB, nR,
			   true);
  if (nR > 0)
    enemiesPresentP = true;

  boxCenters->push_back(ctr);
  u = blueUtil(mB + (lambda * nB), nR, alpha, beta, uRed, uBlue);
  utils->push_back(u);
  if (true == ACPSim::traceMoves) {
    cout << "NW box " << *ctr << " has B=" << nB << ", R="<<nR;
    cout << " so we compare B=" << mB+(lambda*nB) << " vs R="<<nR<<", U="<<u;
    cout << endl << endl<<flush;
  }


  // SE
  cout << "S-East:" << endl << flush;
  ctr = new GVector(p0 + dy + dx);
  strengthInNonOverlapping(*ctr, dx, dy,
			   ((*ctr) + p0)/2.0, dx/2.0, dy/2.0,
			   myUnit, nB, nR,
			   true);
  if (nR > 0)
    enemiesPresentP = true;

  boxCenters->push_back(ctr);
  u = blueUtil(mB + (lambda * nB), nR, alpha, beta, uRed, uBlue);
  utils->push_back(u);
  if (true == ACPSim::traceMoves) {
    cout << "SE box " << *ctr << " has B=" << nB << ", R="<<nR;
    cout << " so we compare B=" << mB+(lambda*nB) << " vs R="<<nR<<", U="<<u;
    cout << endl << endl<<flush;
  }


  // SW
  cout << "S-West:" << endl << flush;
  ctr = new GVector(p0 + dy - dx);
  strengthInNonOverlapping(*ctr, dx, dy,
			   ((*ctr) + p0)/2.0, dx/2.0, dy/2.0,
			   myUnit, nB, nR,
			   true);
  if (nR > 0)
    enemiesPresentP = true;

  boxCenters->push_back(ctr);
  u = blueUtil(mB + (lambda * nB), nR, alpha, beta, uRed, uBlue);
  utils->push_back(u);
  if (true == ACPSim::traceMoves) {
    cout << "SW box " << *ctr << " has B=" << nB << ", R="<<nR;
    cout << " so we compare B=" << mB+(lambda*nB) << " vs R="<<nR<<", U="<<u;
    cout << endl << endl<<flush;
  }


  if (true == enemiesPresentP)
    dt = dt / 4.0;

  // pick best
  unsigned long int iCurr = 0;
  float uCurr = 0.0;
  const unsigned long int n = utils->size(); // happens to be 9

  // stay put unless better exists
  unsigned long int iBest = 0;
  float uBest = (*utils)[iBest];
  ctr = (*boxCenters)[iBest];


  for(iCurr = 1; iCurr < n; iCurr++) {
    uCurr = (*utils)[iCurr];
    if (uCurr > uBest) {
      uBest = uCurr;
      iBest = iCurr;

    }
  }
  ctr = (*boxCenters)[iBest];
  if (true == ACPSim::traceMoves) {
    cout << "Best center is " <<iBest<<": "<< *ctr << " with " << uBest << endl << flush;
    cout << "Tau-Force is " << tauF <<endl << flush;
  }

  assert (myUnit->maxSpeed > 0.0); // it had better to be properly set
  gv = bound( ( (*ctr) - p0)/tauF, myUnit->maxSpeed);

  // cleanup


  while (boxCenters->size() > 0) {
    ctr = boxCenters->back();
    boxCenters->pop_back();
    delete ctr;
    ctr = NULL;
  }
  delete boxCenters;
  boxCenters = NULL;

  delete utils;
  utils = NULL;

  if (true == ACPSim::traceMoves) {
    cout << "MRForce1::desiredVelocityTMP recommends " <<gv;
    cout << " for unit " << myUnit->getSimEntID() << endl<<flush;
  }

  return;
}

// ------------------------------------------
// this is where I'll test out the coordinate-based
// scanning

void MRForce1::desiredVelocity(GVector& gv, double& dt) {

  // seconds
  dt = myUnit->maxUpdateInterval * theSim->rng->uniform(.80, 1.00);
  bool enemiesPresentP = false;
  gv = GVector(0.0, 0.0, 0.0);
  float mB = 0.0; // strength of blue in current box
  float mR = 0.0; // strength of Red in current box
  unsigned long int i = 0;
  unsigned long int iCurr = 0;
  float uCurr = 0.0;
  const unsigned long int n = 9;

  // each list is in the same order:
  // current, N, S, E, W, NE, NW, SE, SW
  float utils[n];
  float cxs[n];
  float cys[n];
  for (i=0; i<n; i++) {
    cxs[i] = 0.0;
    cys[i] = 0.0;
    utils[i] = 0.0;
  }

  float nB = 0.0;
  float nR = 0.0;

  float u = 0.0;

  // location of unit and center of actual box occupied
  GVector p0 = myUnit->currentPos();
  double Cx = p0.get(0);
  double Cy = p0.get(1);

  if (true == ACPSim::traceMoves) {
    cout << "for MRForce1::desiredVelocity, Unit "<<myUnit->getSimEntID();
    cout << " is at " << p0 << " at time " << theSim->clock ();
    cout << endl << flush;
  }

  // location and dimensions of the box being considered
  double cx = Cx;
  double cy = Cy;
  double dx = width/2.0; // same as dx of actual box
  double dy = height/2.0; // same as dy of actual box

  // -------------
  // current, N, S, E, W, NE, NW, SE, SW

  // current
  //  cout << "C:" << endl << flush;
  cx = ((Cx - dx) + (Cx + dx))/2.0;
  cy = ((Cy - dy) + (Cy + dy))/2.0;
  strengthInNonOverlapping(Cx - dx,   Cy - dy,   Cx + dx,   Cy + dy,
			   Cx - dx/2, Cy - dy/2, Cx + dx/2, Cy + dy/2,
			   myUnit, mB, mR,
			   false); // small box not used

  assert (mB > 0.0); // at least this unit itself is there
  if (mR > 0)
    enemiesPresentP = true;

  u = blueUtil(mB, mR, alpha, beta, uRed, uBlue);

  cxs[0] = cx;
  cys[0 ] = cy;
  utils[0] = u;

  if (true == ACPSim::traceMoves) {
    cout << "Current box, with center (" << Cx << ", " << Cy;
    cout << "), has B=" << mB << ", R="<<mR<<", U="<<u;
    cout << " so we compare B=" << mB << " vs R="<<mR<<", U="<<u;
    cout << endl << endl<<flush;
  }

  // N
  //  cout << "N:" << endl << flush;
  cx = ((Cx - dx) + (Cx + dx)) / 2.0;
  cy = ((Cy - 2*dy) + (Cy)) / 2.0;
  strengthInNonOverlapping(Cx - dx, Cy - 2*dy, Cx + dx, Cy,
			   Cx - dx, Cy - dy,   Cx + dx, Cy,
			   myUnit, nB, nR,
			   true);

  if (nR > 0)
    enemiesPresentP = true;

  u = blueUtil(mB + (lambda * nB), nR, alpha, beta, uRed, uBlue);

  cxs[1] = cx;
  cys[1 ] = cy;
  utils[1] = u;

  if (true == ACPSim::traceMoves) {
    cout << "North box, with center (" << Cx << ", " << Cy - dy;
    cout << "), has B=" << nB << ", R="<<nR<<", U="<<u;
    cout << " so we compare B=" << mB + (lambda*nB) << " vs R="<<nR<<", U="<<u;
    cout << endl << endl<<flush;
  }

  // S
  //  cout << "S:" << endl << flush;
  cx = ((Cx - dx) + (Cx + dx)) / 2.0;
  cy = ((Cy) + (Cy + 2*dy)) / 2.0;
  strengthInNonOverlapping(Cx - dx, Cy, Cx + dx, Cy + 2*dy,
			   Cx - dx, Cy, Cx + dx, Cy + dy,
			   myUnit, nB, nR,
			   true);

  if (nR > 0)
    enemiesPresentP = true;

  u = blueUtil(mB + (lambda * nB), nR, alpha, beta, uRed, uBlue);

  cxs[2] = cx;
  cys[2 ] = cy;
  utils[2] = u;

  if (true == ACPSim::traceMoves) {
    cout << "South box, with center (" << Cx << ", " << Cy + dy;
    cout << "), has B=" << nB << ", R="<<nR<<", U="<<u;
    cout << " so we compare B=" << mB + (lambda*nB) << " vs R="<<nR<<", U="<<u;
    cout << endl << endl<<flush;
  }

  // E
  //  cout << "E:" << endl << flush;
  cx = ((Cx) + (Cx + 2*dx)) / 2.0;
  cy = ((Cy - dy) + (Cy + dy)) / 2.0;
  strengthInNonOverlapping(Cx, Cy - dy, Cx + 2*dx, Cy + dy,
			   Cx, Cy - dy, Cx + dx,   Cy,
			   myUnit, nB, nR,
			   true);

  if (nR > 0)
    enemiesPresentP = true;

  u = blueUtil(mB + (lambda * nB), nR, alpha, beta, uRed, uBlue);

  cxs[3] = cx;
  cys[3 ] = cy;
  utils[3] = u;

  if (true == ACPSim::traceMoves) {
    cout << "East box, with center (" <<Cx + dx<< ", " << Cy;
    cout << "), has B=" << nB << ", R="<<nR<<", U="<<u;
    cout << " so we compare B=" << mB + (lambda*nB) << " vs R="<<nR<<", U="<<u;
    cout << endl << endl<<flush;
  }

  // W
  //  cout << "W:" << endl << flush;
  cx = ((Cx - 2*dx) + (Cx)) / 2.0;
  cy = ((Cy - dy) + (Cy + dy)) / 2.0;
  strengthInNonOverlapping(Cx - 2*dx, Cy - dy, Cx, Cy + dy,
			   Cx - dx,   Cy - dy, Cx, Cy,
			   myUnit, nB, nR,
			   true);

  if (nR > 0)
    enemiesPresentP = true;

  u = blueUtil(mB + (lambda * nB), nR, alpha, beta, uRed, uBlue);

  cxs[4] = cx;
  cys[4 ] = cy;
  utils[4] = u;

  if (true == ACPSim::traceMoves) {
    cout << "West box, with center (" <<Cx - dx<< ", " << Cy;
    cout << "), has B=" << nB << ", R="<<nR<<", U="<<u;
    cout << " so we compare B=" << mB + (lambda*nB) << " vs R="<<nR<<", U="<<u;
    cout << endl << endl<<flush;
  }

  // NE
  //  cout << "NE:" << endl << flush;
  cx = ((Cx) + (Cx + 2*dx)) / 2.0;
  cy = ((Cy - 2*dy) + (Cy)) / 2.0;
  strengthInNonOverlapping(Cx, Cy - 2*dy, Cx + 2*dx, Cy,
			   Cx, Cy - dy,   Cx + dx,   Cy,
			   myUnit, nB, nR,
			   true);

  if (nR > 0)
    enemiesPresentP = true;

  u = blueUtil(mB + (lambda * nB), nR, alpha, beta, uRed, uBlue);

  cxs[5] = cx;
  cys[5 ] = cy;
  utils[5] = u;

  if (true == ACPSim::traceMoves) {
    cout << "NE box, with center (" <<Cx + dx<< ", " << Cy - dy;
    cout << "), has B=" << nB << ", R="<<nR<<", U="<<u;
    cout << " so we compare B=" << mB + (lambda*nB) << " vs R="<<nR<<", U="<<u;
    cout << endl << endl<<flush;
  }

  // NW
  //  cout << "NW:" << endl << flush;
  cx = ((Cx - 2*dx) + (Cx)) / 2.0;
  cy = ((Cy - 2*dy) + (Cy)) / 2.0;
  strengthInNonOverlapping(Cx - 2*dx, Cy - 2*dy, Cx, Cy,
			   Cx - dx,   Cy - dy,   Cx, Cy,
			   myUnit, nB, nR,
			   true);

  if (nR > 0)
    enemiesPresentP = true;

  u = blueUtil(mB + (lambda * nB), nR, alpha, beta, uRed, uBlue);

  cxs[6] = cx;
  cys[6 ] = cy;
  utils[6] = u;

  if (true == ACPSim::traceMoves) {
    cout << "NW box, with center (" <<Cx - dx<< ", " << Cy - dy;
    cout << "), has B=" << nB << ", R="<<nR<<", U="<<u;
    cout << " so we compare B=" << mB + (lambda*nB) << " vs R="<<nR<<", U="<<u;
    cout << endl << endl<<flush;
  }

  // SE
  //  cout << "SE:" << endl << flush;
  cx = ((Cx) + (Cx + 2*dx)) / 2.0;
  cy = ((Cy) + (Cy + 2*dy)) / 2.0;
  strengthInNonOverlapping(Cx, Cy, Cx + 2*dx, Cy + 2*dy,
			   Cx, Cy, Cx + dx,   Cy + dy,
			   myUnit, nB, nR,
			   true);

  if (nR > 0)
    enemiesPresentP = true;

  u = blueUtil(mB + (lambda * nB), nR, alpha, beta, uRed, uBlue);

  cxs[7] = cx;
  cys[7 ] = cy;
  utils[7] = u;

  if (true == ACPSim::traceMoves) {
    cout << "SE box, with center (" <<Cx + dx<< ", " << Cy + dy;
    cout << "), has B=" << nB << ", R="<<nR<<", U="<<u;
    cout << " so we compare B=" << mB + (lambda*nB) << " vs R="<<nR<<", U="<<u;
    cout << endl << endl<<flush;
  }

  // SW
  //  cout << "SW:" << endl << flush;
  cx = ((Cx - 2*dx) + (Cx)) / 2.0;
  cy = ((Cy) + (Cy + 2*dy)) / 2.0;
  strengthInNonOverlapping(Cx - 2*dx, Cy, Cx, Cy + 2*dy,
			   Cx - dx,   Cy, Cx, Cy + dy,
			   myUnit, nB, nR,
			   true);

  if (nR > 0)
    enemiesPresentP = true;

  u = blueUtil(mB + (lambda * nB), nR, alpha, beta, uRed, uBlue);

  cxs[8] = cx;
  cys[8 ] = cy;
  utils[8] = u;

  if (true == ACPSim::traceMoves) {
    cout << "SW box, with center (" <<Cx + dx<< ", " << Cy + dy;
    cout << "), has B=" << nB << ", R="<<nR<<", U="<<u;
    cout << " so we compare B=" << mB + (lambda*nB) << " vs R="<<nR<<", U="<<u;
    cout << endl << endl<<flush;
  }



  if (true == enemiesPresentP)
    dt = dt / 4.0;

  // stay put unless better exists
  unsigned long int iBest = 0;
  float uBest = utils[iBest];


  for(iCurr = 1; iCurr < n; iCurr++) {
    uCurr = utils[iCurr];
    if (uCurr > uBest) {
      uBest = uCurr;
      iBest = iCurr;

    }
  }
  GVector ctr = GVector( cxs[iBest], cys[iBest], 0.0);
  if (true == ACPSim::traceMoves) {
    cout << "Best center is " <<iBest<<": "<< ctr << " with " << uBest << endl << flush;
    cout << "Tau-Force is " << tauF <<endl << flush;
  }

  assert (myUnit->maxSpeed > 0.0); // it had better to be properly set
  gv = bound( ( (ctr) - p0)/tauF, myUnit->maxSpeed);

  // cleanup: no-op

  if (true == ACPSim::traceMoves) {
    cout << "MRForce1::desiredVelocity recommends " <<gv;
    cout << " for unit " << myUnit->getSimEntID() << endl<<flush;
  }

  return;
}

// ------------------------------------------


MRNCW1::MRNCW1(GVector ap, Unit* ru) : MovementRule(ru) {
  wGoal = 2.00;
  wFriends = 1.00;
  wForce = 4.00;
  wSuperior = 0.50;

  assert (myUnit->maxSpeed > 0.0); // it had better to be properly set
  assert (myUnit->weaponRange > 0.0); // it had better to be properly set
  assert (myUnit->sensorRange > 0.0); // it had better to be properly set

  sPlan = ru->maxSpeed;

  double scanBoxSize = 0.0;
  //  scanBoxSize = ((3.0 * ru->weaponRange) + (2.0 * ru->sensorRange))/5.0;

  // we set this so that an enemy just on the edge of the scan box
  // is actually in range. Otherwise, we have a tendency to get
  // them just barely inside the scan box, then stop pursuing them
  // because they are in the box - but not killing them because
  // they are out of range!
  scanBoxSize = dmin( ru->sensorRange, ru->weaponRange);
  scanBoxSize = scanBoxSize * theSim->rng->uniform(1.75, 1.90);



  // notice that with alpha == beta,
  // ua == ub, it most prefers to attack at
  // 2.7 : 1 force ratios
  mcForce = new MRForce1(scanBoxSize, scanBoxSize, 0.2,
			 1.0, 1.0, // alpha, beta
			 1.0, 1.0, // uRed, uBlue losses
			 120.0, // 120 seconds
			 ru);

  assert (3 == ap.getDim());
  mcGoal = new MRPoint(ru, ap,
		       300.0, // 300 second adjust time to goal
		       speedWP);

  mcFriends = new MRBuddies1(ru->weaponRange / 3.0, // look within 1/3 weapon range
			     ru->weaponRange / 10.0, // space at 1/10 weapon range
			     180.0, // 200 second adjust time
			     ru);

  // the aimpoint will be reset to the superior CG
  // whenever we need it, so we start out with
  // this plausible dummy value
  mcSuperior =  new MRPoint(ru, ap,
			    250.0, // 250 second adjust time
			    speedWP);

}

MRNCW1::~MRNCW1() {
  if (NULL != mcGoal) {
    delete mcGoal;
    mcGoal = NULL;
  }

  if (NULL != mcFriends) {
    delete mcFriends;
    mcFriends = NULL;
  }

  if (NULL != mcForce) {
    delete mcForce;
    mcForce = NULL;
  }

  if (NULL != mcSuperior) {
    delete mcSuperior;
    mcSuperior = NULL;
  }

}
void MRNCW1::desiredVelocity(GVector& gv, double& dt) {

  // these must all be properly set
  assert (myUnit->maxStepDist > 0.0);
  assert (myUnit->maxUpdateInterval > 0.0);
  assert (myUnit->weaponRange > 0.0);
  assert (myUnit->sensorRange > 0.0);

  dt = theSim->rng->uniform(0.8, 1.0) * myUnit->maxUpdateInterval;
  GVector vGl = GVector(0.0, 0.0, 0.0);
  double dtGl = dt;

  GVector vFn = GVector(0.0, 0.0, 0.0);
  double dtFn = dt;

  GVector vFr = GVector(0.0, 0.0, 0.0);
  double dtFr = dt;

  GVector vSp = GVector(0.0, 0.0, 0.0);
  double dtSp = dt;


  // these 'cout' statements verify that it spends
  // most of its time in MRFriends::desiredVelocity
  //
  //  cout << "computing MRGoal::desiredVelocity"<<endl<<flush;
  if (wGoal > 0.)
    mcGoal->desiredVelocity(vGl, dtGl);
  

  //  cout << "computing MRForce1::desiredVelocity"<<endl<<flush;
  if (wForce > 0.)
    mcForce->desiredVelocity(vFr, dtFr);

  //  cout << "computing MRSuperior::desiredVelocity"<<endl<<flush;
  if (wSuperior > 0.) {
    mcSuperior->setAimPoint(myUnit->superior->currentPos());
    mcSuperior->desiredVelocity(vSp, dtSp);
  }

  //  cout << "computing MRFriends::desiredVelocity"<<endl<<flush;
  if (wFriends > 0.)
    mcFriends->desiredVelocity(vFn, dtFn);

  dt = dmin( dmin(dtGl, dtFn), dmin(dtFr, dtSp));

  if (norm(vFr) > 0.50)
    gv = ((vGl*wGoal)+(vFn*wFriends)+(vFr*wForce)+(vSp*wSuperior))
      /
      (wGoal+wFriends+wForce+wSuperior);
  else 
    gv = ((vGl*wGoal)+(vFn*wFriends)+(vSp*wSuperior))
      /
      (wGoal+wFriends+wSuperior);

  gv = bound(gv, sPlan);

  // for now, I take 'essentially stationary' things
  // and stop them completely
  if (norm(gv) < 0.05)
    gv = GVector(0.0, 0.0, 0.0);

  // if (BlueSide == myUnit->side) {
  if (true == ACPSim::traceMoves) {
    cout << "For " <<myUnit->side << " Unit " << myUnit->getSimEntID()<<": "<<endl;
    cout << "  MRNCW1 goal:      " << vGl << ", " << dtGl << endl;
    cout << "  MRNCW1 formation: " << vFn << ", " << dtFn << endl;
    cout << "  MRNCW1 force:     " << vFr << ", " << dtFr << endl;
    cout << "  MRNCW1 superior:  " << vSp << ", " << dtSp << endl;
    cout << "  MRNCW1 overall:   " << gv << ", " << dt << endl;
    cout << endl << flush;
  }
  return;
}

// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
