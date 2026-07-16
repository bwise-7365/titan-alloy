// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp.
// Changes: evector becomes std::vector; AAA::RNG becomes panj::PRNG; inListP becomes std::find; zTemp VLA becomes a std::vector (MSVC); delete[] on the two new[] allocations (cells, fractal zTemp).
// ------------------------------------------

#include "frwrdec.h"
#include "struct.h"
#include "tgrid.h"
#include "unit.h"
#include "runit.h"
#include "mfrac.h"

#include <vector>
#include <algorithm>

// ------------------------------------------

using AAA::GVector;
using std::vector;

using AAA::ifloor;
using AAA::iceiling;
using AAA::sqr;

// ------------------------------------------
// this is just short of 6 feet
double TGrid::StandardEntityHeight = 1.83; // %%% default entity height
// ------------------------------------------

TGrid::TGrid()
{
  initialize();
}

TGrid::TGrid(int n, int m, double max_x, double max_y)
{
  int i, j;
  initialize();
  rows = n;
  clms = m;
  assert (rows > 1);
  assert (clms > 1);
  assert (max_x > 0);
  assert (max_y > 0);

  max_X = max_x;
  max_Y = max_y;

  double dx = max_X / rows;
  double dy = max_Y / rows;
  double ds = sqrt( (dx*dx) + (dy*dy) );
  stepLOS = ds / 5.0; // each step is about 1/5 of a column's diagonal distance

  //   cells = new (TCell*)[sizeof(TCell) * rows*clms];
  cells = new TCell*[ rows * clms ];
  assert (NULL != cells);

  for (i=0; i<rows; i++)
    for (j=0; j<clms; j++)
      {
	cells [ j + (clms * i) ] = new TCell(i, j, 0.0);
      }
}


TGrid::~TGrid()
{
  int i, j, k;
  for (i=0; i<rows; i++)
    for (j=0; j<clms; j++)
      {
	k = j + (clms * i);
	delete cells [k];
	cells [k] = NULL;
      }
  delete [] cells; // array form: cells came from new TCell*[]
  cells = NULL;
}


void
TGrid::initialize()
{
  rows = 5;// a reasonable minimum
  clms = 5;// a reasonable minimum
  max_X = 0;
  max_Y = 0;
  cells = NULL;
  return;
}

// this is the public interface
TCell*
TGrid::getTCell(const double x, const double y)
{
  int i = 0;
  int j = 0;
  gridFromPhysical(i, j, x, y);

  //   assert (LTrue == _onGrid(i,j));
  //   TCell *tc = cells [ j + (clms * i) ];
  //   assert (NULL != tc);
  //   return tc;

  TCell* tc = _getTCell(i,j);
  assert (i == tc->r);
  assert (j == tc->c);
  return tc;

}


// this is the private function
inline TCell* TGrid::_getTCell(const int i, const int j)
{
  assert (AAA::LTrue == _onGrid(i,j));
  TCell *tc = cells [ j + (clms * i) ];
  assert (NULL != tc);
  assert (i == tc->r);
  assert (j == tc->c);
  return tc;
}


// ------------------------------------------
std::vector<ResUnit*>* TGrid::units_in_ranges(double x0, double y0,
					  double x1, double y1)
{
  std::vector<ResUnit*> *found = new std::vector<ResUnit*>();
  std::vector<ResUnit*> *local_found = NULL;

  if (x0 < 0.0)
    x0 = 0.0;

  if (y0 < 0.0)
    y0 = 0.0;

  if (max_X < x0)
    x0 = max_X;

  if (max_Y < y0)
    y0 = max_Y;

  if (x1 < 0.0)
    x1 = 0.0;

  if (y1 < 0.0)
    y1 = 0.0;

  if (max_X < x1)
    x1 = max_X;

  if (max_Y < y1)
    y1 = max_Y;

  assert (x0 <= x1);
  assert (y0 <= y1);

  unsigned long int min_i = ifloor((x0 * rows) / max_X);
  unsigned long int max_i = iceiling((x1 * rows) / max_X);

  unsigned long int min_j = ifloor((y0 * clms) / max_Y);
  unsigned long int max_j = iceiling((y1 * clms) / max_Y);

  unsigned long int i = 0;
  unsigned long int j = 0;
  unsigned long int k = 0;
  unsigned long int numFound = 0;

  ResUnit* ru = NULL;

  for (i=min_i; i<max_i+1; i++)
    {
      for (j=min_j; j<max_j+1; j++) 
	{
	  // cout << i << " " << j << endl << flush;
	  if  (_onGrid(i,j) == AAA::LTrue) 
	    {
	      if (numResUnits(i,j) > 0) 
		{
		  // this is the original list, in the TCell
		  local_found = resUnitsInSqr(i,j);

		  numFound = local_found->size();

// 		  cout << "TCell at " << i << ", " << j << " has " << numFound;
// 		  for (unsigned int q=0; q<numFound; q++)
// 		    cout << ((*local_found)[q])->getSimEntID()<<"  ";
// 		  cout << endl << flush;
// 		  assert (numFound == numResUnits(i,j));



		  for (k = 0; k < numFound; k++) 
		    {
		      ru = (*local_found)[k];
		      found->push_back(ru);
		    }
		  local_found = NULL; // clear the pointer
		}
	    }
	}
    }

//   cout << "Found "<<found->size()<< " units within these ranges, "<<endl;
//   cout << "X: " << x0 << " " << x1 << endl;
//   cout << "Y: " << y0 << " " << y1 << endl;
//   cout << endl << flush;
//   for (i=0; i<found->size(); i++) 
//     {
//       ru = (*found)[i];
//       cout << "  ResUnit "<<ru->getSimEntID() << endl;
//     }

  return found;
}

std::vector<ResUnit*>*
TGrid::units_in_area(Box* area) 
{ 
  std::vector<ResUnit*> *local_found  = NULL;
  std::vector<ResUnit*> *found = NULL;;

  int i2, j2;
  int min_i, max_i, min_j, max_j;
  unsigned long int k, numFound;
  ResUnit* element;
  GVector el_pos;

  found = new std::vector<ResUnit*>();
  assert (NULL != found);

  local_found = NULL;
  element = NULL;

  // scan the bounding box, checking for any res-units
  // whose center is in the box.

  min_i = ifloor((area->min_x() * rows) / max_X);
  max_i = iceiling((area->max_x() * rows) / max_X);

  min_j = ifloor((area->min_y() * clms) / max_Y);
  max_j = iceiling((area->max_y() * clms) / max_Y);

  for (i2=min_i; i2<max_i+1; i2++)
    for (j2=min_j; j2<max_j+1; j2++)
      
      
      if ((_onGrid(i2,j2) == AAA::LTrue) && (numResUnits(i2,j2) > 0))	
	{

	  local_found = resUnitsInSqr(i2,j2);

	  numFound = local_found->size();
	  for (k = 0; k < numFound; k++)   
	    {
	      element = (*local_found)[k];
	      el_pos = element->currentPos();
	      if (area->insideP(el_pos ) == AAA::LTrue)
		found->push_back(element);
	    }
	  local_found = NULL; // clear the pointer
	}

  return found;
}

// ------------------------------------------

inline std::vector<ResUnit*>* TGrid::resUnitsInSqr(int i, int j)
{
  TCell *tc;

  assert (AAA::LTrue == _onGrid(i,j));

  tc = _getTCell(i,j);
  assert (NULL != tc);
  assert (NULL != tc->occupants);

  assert (i == tc->r);
  assert (j == tc->c);

  // return the original list
  return tc->occupants;

}



std::vector<ResUnit*>*
TGrid::resUnitsNearLoc(double x, double y, double srchRange, bool sortP) 
{
  int i, j;
  unsigned int k = 0;
  int i2, j2;
  int di, dj;
  unsigned int numFound = 0;
  float dx2 = 0.0;
  float dy2 = 0.0;
  float dx = 0.0;
  float dy = 0.0;
  GVector p = GVector(0.0, 0.0, 0.0);
  std::vector<ResUnit*>* found = new std::vector<ResUnit*>();
  assert (NULL != found);
  std::vector<ResUnit*>* local_found = NULL;
  ResUnit* ru1 = NULL;
  ResUnit* ru2 = NULL;
  ResUnit* ruTmp = NULL;

  float r2 = srchRange*srchRange;
  int mx2 = (int)(max_X * max_X);
  int my2 = (int)(max_Y * max_Y);

  float iDist2 = 0.0;
  float jDist2 = 0.0;

  gridFromPhysical(i, j, x, y);
  gridFromPhysical(di, dj, srchRange, srchRange);

  for (i2 = i-di; i2<= i+di; i2++)
    for (j2 = j-dj; j2<= j+dj; j2++)
      if ((_onGrid(i2,j2) == AAA::LTrue) &&
	  (numResUnits(i2,j2) > 0))
	{
	  // better to test exact center-to-center distance
	  dx2 = (i-i2)*(i-i2)* (mx2 / r2);
	  dy2 = (j-j2)*(j-j2)* (my2 / r2);

	  local_found = resUnitsInSqr(i2,j2);
	  numFound = local_found->size();
	  for (k=0; k<numFound; k++) 
	    {

	      ru1 = (*local_found)[k];
	      found->push_back(ru1);
	    }
	  local_found = NULL; // clear the pointer
	}


  numFound = found->size();
  if ((true == sortP) && (numFound > 1)) 
    {
      for (i2 = 0; i2 < numFound - 1; i2++) 
	{
	  for (j2 = i2+1; j2 < numFound; j2++) 
	    {
	      ru1 = (*found)[i2];
	      p = ru1->currentPos();
	      dx = p.get(0) - x;
	      dy = p.get(1) - y;
	      dx2 = dx*dx;
	      dy2 = dy*dy;
	      iDist2 = dx2 + dy2;

	      ru2 = (*found)[j2];
	      p = ru2->currentPos();
	      dx = p.get(0) - x;
	      dy = p.get(1) - y;
	      dx2 = dx*dx;
	      dy2 = dy*dy;
	      jDist2 = dx2 + dy2;
	
	      if (jDist2 < iDist2) 
		{
		  ruTmp = (*found)[i2];
		  (*found)[i2] = (*found)[j2];
		  (*found)[j2] = ruTmp;
		}

	    }
	}
    }
  return found;
}


AAA::Logical
TGrid::onGrid(const double x, const double y)
{
  int i = 0;
  int j = 0;
  gridFromPhysical(i, j, x, y);

  //   Logical rslt = LFalse;
  //   if ((0 <= i) && ( i < rows) && (0 <= j) && (j < clms))
  //     rslt = LTrue;
  //   return rslt;

  return _onGrid(i,j);
}


inline AAA::Logical TGrid::_onGrid(int i, int j)
{
  AAA::Logical rslt = AAA::LFalse;
  if ((0 <= i) && ( i < rows) && (0 <= j) && (j < clms))
    rslt = AAA::LTrue;
  return rslt;
}

inline int TGrid::numResUnits(int i, int j)
{
  int rslt = 0;
  assert (AAA::LTrue == _onGrid(i,j));
  TCell* tc = _getTCell(i,j);
  rslt = tc -> numOccupants();
  return rslt;
}

inline void TGrid::gridFromPhysical(int &i, int &j, const double x, const double y)
{
  i = (int)(  ( x * rows) / max_X);
  j = (int)(  ( y * clms) / max_Y);
  return;
}


inline void TGrid::physicalFromGrid(double &x, double &y, const int i, const int j)
{
  x = ((0.5 + i) *max_X)/rows;
  y = ((0.5 + j) *max_Y)/clms;
  return;
}

// ground clutter effect is most pronounced at ground level
// it falls off as the square of the target altitude, but
// the scale / steepness of that fall-off is variable.
float 
TGrid::terrainClutterEffect(ResUnit *ru)
{
  double effect = 1.0;
  /*
    assert (NULL != ru);
    const double sqrtTwo = sqrt(2.0);
    double z;
    GVector ruPos = ru->currentPos();
    int i, j;
    double groundLevelRoughEffect = 10.5;
    double referenceHeight = 250.0; // height in meters at which ground effect is halved
    double heightEffect;
    TCell *tc;

    gridFromPhysical( i , j, ruPos.get(0), ruPos.get(1));
    tc = getTCell(i, j);
    assert (NULL != tc);
    z = ruPos.get(2);

    switch (tc->type)
    {
    case Water:
    case LandSmooth:
    case LandRoad:
    effect = 1.0;
    break;
    case LandRough:
    if (z <= 0)
    effect = groundLevelRoughEffect;
    else {
    heightEffect  = (sqrtTwo * referenceHeight) / (z + referenceHeight);
    effect = groundLevelRoughEffect * heightEffect * heightEffect;
    // note that if z == referenceHeight,
    // heightEffect * heightEffect = 1/2;
    }
    break;
    }
  */
  return effect;
}


// this assumes flat earth, and rectangular columns of terrain
void TGrid::lineOfSight(GVector pFrom, GVector pTo, bool& visibleP, double& distance)
{
  GVector losV = pTo - pFrom;
  //  GVector losPt;
  double losDistance = norm(losV);
  // ensure there are always at least 2 steps
  // to cover endpoint and midpoint
  unsigned long int numSteps = ((unsigned long int) (2.50 + (losDistance / stepLOS)));
  GVector losDP = losV / numSteps;
  int i = 0;
  int j = 0;
  unsigned long int n = 0;
  float x0 = pFrom.get(0);
  float y0 = pFrom.get(1);
  float z0 = pFrom.get(2);

  float dx = (pTo.get(0) - x0)/numSteps;
  float dy = (pTo.get(1) - y0)/numSteps;
  float dz = (pTo.get(2) - z0)/numSteps;
  double ds = sqrt ( (dx*dx) + (dy*dy) + (dz*dz) );

  double x = 0.0;
  double y = 0.0;
  double z = 0.0;

  visibleP = true;
  distance = 0;
  TCell *tc = NULL;
  // I do check both end points, just in case either
  // the eyepoint or target is somehow below ground
  for (n=0; ((true == visibleP) && (n<=numSteps)); n++)
    {
      x = x0 + (n * dx);
      y = y0 + (n * dy);
      z = z0 + (n * dz);
      distance = n * ds;

      gridFromPhysical( i , j, x, y);
      tc = _getTCell(i, j);
      if (z < tc->height)
	visibleP = false;
    }
  return;
}


// this assumes flat earth, and rectangular columns of terrain
void TGrid::clearBallistic(GVector pFrom, GVector vel, double fTime,
			   bool& clearP, double& distance)
{
  GVector gravVector = GVector(0.0, 0.0, -earthG);
  GVector pTmp = pFrom + (vel * fTime) - ((gravVector * fTime * fTime) / 2.0);
  double losDistance = dist(pFrom, pTmp);

  // ensure there are always at least 2 steps
  // to cover endpoint and midpoint
  unsigned long int numSteps = ((unsigned long int) (2.50 + (losDistance / stepLOS)));
  double dt = fTime / numSteps;
  double tTmp = 0.0;

  clearP = false;
  distance = 0.0;

  int i = 0;
  int j = 0;
  unsigned long int n = 0;

  double dx = 0.0;
  double dy = 0.0;

  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  TCell* tc = NULL;

  // I do check both end points, just in case either
  // the shooter or target is somehow below ground
  for (n=0; ((true == clearP) && (n<=numSteps)); n++)
    {
      tTmp = n*dt;
      pTmp = pFrom + (vel * tTmp) - ((gravVector * tTmp * tTmp) / 2.0);

      x = pTmp.get(0);
      y = pTmp.get(1);
      z = pTmp.get(2);

      gridFromPhysical( i , j, x, y);
      tc = _getTCell(i, j);
      if (z < tc->height)
	{
	  clearP = false;
	  dx = x - pFrom.get(0);
	  dy = y - pFrom.get(1);
	  distance = sqrt ( (dx*dx) + (dy*dy) );
	}
    }
  return;
}

// this really combines two things:
// (A) short range LOS based on terrain profiles,
// (B) long range LOS based on curvature of earth
bool
TGrid::terrainVisibleP(GVector p1, GVector p2)
{
  bool result = true;
  bool visibleP = true;
  double visDist = 0.0;

  double h1 = p1.get(2);
  double h2 = p2.get(2);
  if (h1 < 0)
    h1 = 0;
  if (h2 < 0)
    h2 = 0;
  // this is the distance at which LOS from p1 just grazes Earth
  double d1 = sqrt ( (2.0 * earthRadius + h1) * h1);

  // this is the distance at which LOS from p2 just grazes Earth
  double d2 = sqrt ( (2.0 * earthRadius + h2) * h2);

  double dActual = dist(p1, p2);

  if (true == ACPSim::traceSensors)
    {
      cout << endl;
      cout << "terrainMasked grazing dist = " << d1 + d2;
      cout << " actual = " << dActual << endl;
    }

  if (dActual <= d1 + d2)
    result = true; // not blocked by curvature
  else
    result = false; // blocked by curvature

  if (true == result)
    {
      // we could be clever and compare the altitudes of
      // p1 and p2. If they are both higher than the maximum
      // altitude of any terrain, then they have intervisibility.
      // If one is higher and one is not, we can use trigonometry
      // to see the radius of the circle around the lower one
      // within which any possibly blocking terrain would have to
      // reside. if p1 is high, and p2 is low, we could
      // assume that p2 is on a perfectly flat region, bounded
      // by terrain of the maximum height. how wide could that
      // region be and still block p1's view? That's the maximum
      // radius 
      // 
      // %%% But I did not do that.
      // Use the comment above if you want to code it.
      lineOfSight(p1, p2, visibleP, visDist);

      result = visibleP;

      if (true == ACPSim::traceSensors)
	{
	  if (true == result)
	    cout << "no masking by curvature, and no masking by local terrain"<<endl;
	  if (false == result)
	    cout << "no masking by curvature, but masking by local terrain"<<endl;

	}

    }


  return result;
}



// for now, we use the slopes that would result if one
// fit a parabola through evenly spaced left, middle,
// and right heights (h1, h2, h3), which conveniently
// is (h3-h1)/2 at the middle
void TGrid::setSlopes()
{
  TCell *tc1 = NULL; // left or top neighbor (lower x or y)
  TCell *tc2 = NULL; // cell whose slope we are setting
  TCell *tc3 = NULL; // right or bottom neighbor (higher x or y)
  int i = 0;
  int j = 0;
  for (i=0; i<rows; i++)
    for (j=0; j<clms; j++)
      {
	tc2 = _getTCell(i,j);

	// do slope along x
	if (AAA::LTrue == _onGrid(i, j-1))
	  tc1 = _getTCell(i,j-1);
	else
	  tc1 = tc2;

	if (AAA::LTrue == _onGrid(i, j+1))
	  tc3 = _getTCell(i,j+1);
	else
	  tc3 = tc2;

	tc2->slopeX = (tc3->height - tc1->height) / 2.0;


	// do slope along y
	if (AAA::LTrue == _onGrid(i-1, j))
	  tc1 = _getTCell(i-1,j);
	else
	  tc1 = tc2;

	if (AAA::LTrue == _onGrid(i+1, j))
	  tc3 = _getTCell(i+1,j);
	else
	  tc3 = tc2;

	tc2->slopeY = (tc3->height - tc1->height) / 2.0;
      }

  return;
}

void TGrid::synthesizeTerrainGaussian(panj::PRNG* rng,
				      double minZ, double maxZ,
				      double weight,
				      unsigned long int iter)
{
  assert (maxZ > minZ);
  assert (0.0 < weight);
  assert (weight < 1.0);
  assert (iter > 0);
  TCell *tc0 = NULL; // cell whose slope we are setting

  // the four top, bottom, left, and right neighbors
  TCell *tc1 = NULL;
  TCell *tc2 = NULL;
  TCell *tc3 = NULL;
  TCell *tc4 = NULL;
  int i = 0;
  int j = 0;
  unsigned long int k = 0;
  double currMin = maxZ;
  double currMax = minZ;

  std::vector<std::vector<double>> zTemp(rows, std::vector<double>(clms)); // was a VLA; MSVC has none
  // scaling factors
  double a = 0.0;
  double b = 0.0;

  for (i=0; i<rows; i++)
    {
      for (j=0; j<clms; j++)
	{
	  tc0 = _getTCell(i,j);
	  tc0->height = minZ + ((maxZ-minZ) * sqr(rng->uniform(0.0, 1.0)));
	}
    }

  for (k=0; k<iter; k++)
    {
      currMin = maxZ;
      currMax = minZ;
      for (i=0; i<rows; i++)
	{
	  for (j=0; j<clms; j++)
	    {

	      assert (AAA::LTrue == _onGrid(i,j));

	      tc0 = _getTCell(i,j);
	      assert (NULL != tc0);

	      if (AAA::LTrue == _onGrid(i-1, j-1))
		tc1 = _getTCell(i-1,j-1);
	      else
		tc1 = tc0;

	      if (AAA::LTrue == _onGrid(i-1, j+1))
		tc2 = _getTCell(i-1,j+1);
	      else
		tc2 = tc0;

	      if (AAA::LTrue == _onGrid(i+1, j-1))
		tc3 = _getTCell(i+1,j-1);
	      else
		tc3 = tc0;

	      if (AAA::LTrue == _onGrid(i+1, j+1))
		tc4 = _getTCell(i+1,j+1);
	      else
		tc4 = tc0;
      


	      zTemp[i][j] = (weight * tc0->height) + (1.0-weight)*((tc1->height + 
								    tc2->height + 
								    tc3->height + 
								    tc4->height)/4.0);
	      if (zTemp[i][j] < currMin)
		currMin = zTemp[i][j];
	      if (zTemp[i][j] > currMax)
		currMax = zTemp[i][j];

	    }
	}

      // set the scaling factors so that 
      // a *currMax + b = maxZ
      // a *currMin + b = minZ
      // or
      a = (maxZ - minZ) / (currMax - currMin);
      b = maxZ - (a * currMax);

      for (i=0; i<rows; i++)
	{
	  for (j=0; j<clms; j++)
	    {
	      tc0 = _getTCell(i,j);
	      tc0->height = (a * zTemp[i][j]) + b;
	    }
	}

    } // end of iterations over k

  setSlopes();

  min_Z = minZ;
  max_Z = maxZ;

  return;
}


void TGrid::synthesizeTerrainFractal(panj::PRNG* rng,
				     double minZ, double maxZ,
				     double frac)
{

  assert (maxZ > minZ);
  assert (0.0 < frac);

  TCell *tc0 = NULL;


  int i = 0;
  int j = 0;
  unsigned long int k = 0;
  double currMin = maxZ;
  double currMax = minZ;

  double dx = max_X / clms;
  double dy = max_Y / clms;

  double* zTemp = makeFractalTerrain(rows, clms, dx, dy, (minZ + maxZ)/2.0, frac, rng);

  currMin = zTemp[0];
  currMax = zTemp[0];
  for (i=0; i<rows; i++)
    {
      for (j=0; j<clms; j++)
	{

	  assert (AAA::LTrue == _onGrid(i,j));

	  tc0 = _getTCell(i,j);
	  assert (NULL != tc0);

	  k = j + (i * clms);

	  if (zTemp[k] < currMin)
	    currMin = zTemp[k];
	  if (zTemp[k] > currMax)
	    currMax = zTemp[k];
	}
    }

  // set the scaling factors so that 
  // a *currMax + b = maxZ
  // a *currMin + b = minZ
  // or
  double a = (maxZ - minZ) / (currMax - currMin);
  double b = maxZ - (a * currMax);

  for (i=0; i<rows; i++)
    {
      for (j=0; j<clms; j++)
	{
	  k = j + (i * clms);
	  tc0 = _getTCell(i,j);
	  tc0->height = (a * zTemp[k]) + b;
	}
    }


  setSlopes();

  min_Z = minZ;
  max_Z = maxZ;


  delete [] zTemp; // array form: makeFractalTerrain returns new double[]
  zTemp = NULL;

  return;
}

// ------------------------------------------

TCell::TCell(int i, int j, double h)
{
  r = i;
  c = j;
  height = h; // can be negative
  occupants = new std::vector<ResUnit*>();
  assert (NULL != occupants);

  // make most smooth, and a few rough
  if ((0 == i%5)&&(0 == j%3))
    type = LandRough;
  else
    type = LandSmooth;


  // these are set for all TCells simultaneously by
  // TGrid::setSlopes();
  slopeX = 0.0;
  slopeY = 0.0;
}


// TCell::TCell()
// {
//   r = 0;
//   c = 0;
//   occupants = new std::vector<ResUnit*>();
//   assert (NULL != occupants);
//   type = LandSmooth;
// }


TCell::~TCell()
{
  if (NULL != occupants)
    delete occupants;
  occupants = NULL;
}

void TCell::displayOccupants() 
{
  cout << "TCell at "<<r<<", "<<c<<" contains:  ";
  unsigned int i = 0;
  unsigned int n = occupants->size();
  ResUnit* ru2 = NULL;
  for (i=0;i<n;i++)
    {
      ru2 = (*occupants)[i];
      cout << ru2->getSimEntID() << "  ";
      assert (ru2->tcell == this);
    }
  cout << endl << flush;
  return;
}

int TCell::getR() {
  return r;
}

int TCell::getC() {
  return c;
}

void 
TCell::addOccupant(ResUnit* ru)
{
  assert (NULL != ru);
  assert (NULL != ru->tgrid);

  assert (false ==  (occupants->end() != std::find(occupants->begin(), occupants->end(), ru)));

  if (true == ACPSim::traceMoves)
    {
      cout << "ResUnit " << ru->getSimEntID() << " entering TCell( "<<r<<" , " << c << " ), "<<endl;
      cout << "at height " << height << endl;

      cout <<endl << flush;
    }
  
  occupants->push_back(ru);
  ru->tcell = this;

  assert (true ==  (occupants->end() != std::find(occupants->begin(), occupants->end(), ru)));

  //  displayOccupants();
  return;
}


void 
TCell::removeOccupant(ResUnit* ru)
{
  unsigned int i = 0;
  unsigned int n = occupants->size();

  ResUnit* ru2 = NULL;
  assert (NULL != ru);

  assert (true ==  (occupants->end() != std::find(occupants->begin(), occupants->end(), ru)));

  if (true == ACPSim::traceMoves)
    {
      cout << "ResUnit " << ru->getSimEntID() << " leaves TCell( "<<r<<" , " << c << " )"<<endl;
    }


  std::vector<ResUnit*> *nuOcc = new std::vector<ResUnit*>();
  assert (NULL != nuOcc);

  for (i=0; i<n; i++)
    {
      ru2 = (*occupants)[i];
      if (ru != ru2)
	nuOcc->push_back(ru2);
    }
  delete occupants; // just the evector, not its contents
  occupants = nuOcc;


  assert (false ==  (occupants->end() != std::find(occupants->begin(), occupants->end(), ru)));

  ru->tcell = NULL;

  //  displayOccupants();
  return;
}

int
TCell::numOccupants()
{
  assert (NULL != occupants);
  return occupants->size();
}

// ------------------------------------------
// END of tgrid.cc
// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
