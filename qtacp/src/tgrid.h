// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp.
// Changes: evector/splay includes removed; evector becomes std::vector; AAA::RNG becomes panj::PRNG.
// ------------------------------------------
//
// Units: kilograms, meters, seconds, liters.
// 
// ------------------------------------------

// The TGrid supports a very crude terrain
// and intervisibility model. Each cell has uniform
// characteristics.
// In each cell is a terrain type, height, 
// X-slope, and Y-slope. Constant height means
// that they are rectangular pillars, for purposes
// of visibility. The slopes are obtained by looking
// at the surrounding cells, and interpolating a slope.
// 
// ------------------------------------------
//
// Intervisibility is checked by stepping from the 
// viewer's eye point toward the target point. The
// length of a step is 1/4 of the diagonal across a cell.
// If the LOS goes under the terrain at any point, then
// it considers LOS to be blocked. Notice that it can cut
// corners, which is not totally bad considering that a
// pillar really should represent a hill with slopes.
// Anyway, it is all quite approximate, as befits
// a proof-of-concept prototype. The point is to
// force the command logic to deal with issues like
// visibility (e.g. put your sensors up in the air,
// or on hills; hide your routes behind hills), not get
// every detail of some particular terrain correct.
//
// One could easily supplement (or replace) this by
// a "mean visible path length" (mvpl) model, as follows. The
// unconditional probability of LOS from a to b is
// simply exp(-dist/mvpl). Short mvpl makes the probability
// of LOS fall quickly.
// Why would you want both? because one kind of LOS blockage
// comes from trying to look across valleys or through
// hills, while another kind of blockage comes from
// trying to look across a flat plain or through a flat jungle.
//
// Combining them, one can model bare hills, jungly hills,
// flat plains, or flat jungle. 
// 
// ------------------------------------------


#ifndef TERRAIN_GRID_H
#define TERRAIN_GRID_H


// ------------------------------------------

#include "aaa.h"  // basic I/O, math, constants etc.
#include "des.h"
#include "struct.h"
#include "mat.h"
#include "tdv.h"
#include "unit.h"

#include <vector>

// ------------------------------------------

// the purpose of a terrain grid is to facilitate
// searching for nearby entities (like the services
// provided by ICTDB). It has nothing to do with
// networks or multi-cast groups.

class TGrid
{
public:
  TGrid();
  TGrid(int n, int m, double max_x, double max_y);
  ~TGrid();

  void initialize();

  // note option to sort, with nearest at front of list
  std::vector<ResUnit*>* resUnitsNearLoc(double x, double y,
				     double srch_range, 
				     bool sortP);

  // when searching arbitrary quadralaterals, use units_in_area
  std::vector<ResUnit*>* units_in_area(Box* b);

  // when searching perfect rectangles, units_in_ranges
  // is much faster and more efficient  - not least because
  // callers need not construct or deconstruct boxes
  std::vector<ResUnit*>* units_in_ranges(double x0, double y0,
					  double x1, double y1);


  AAA::Logical onGrid(const double x, const double y);
  TCell* getTCell(const double x, const double y);

  float terrainClutterEffect(ResUnit *ru);

  // this just steps down the vector along a straight line,
  // seeing if it hits any obstacle.
  // visibleP is set to true or false.
  // if false, 'distance' is set to the range from pFrom where the blockage is
  // from and to points can be general 3D points
  void lineOfSight(AAA::GVector pFrom, AAA::GVector pTo,
		   bool& visibleP, double& distance);

  // this just steps down the vector along a parabola,
  // seeing if it hits any obstacle before the flight time ends.
  //
  // For example, you could use minBallisticFlightTime to compute
  // the minimum energy flight time, fTime, to get from pFrom to pTo,
  // compute the implied velocity vector, vel, then give
  // this the (pFrom, vel, fTime)
  //
  // Or, you could have some tactical decision maker, who knows the to
  // and from points, as well as the velocity of his grenade launcher, decide
  // whether to take the high or low trajectory, and pass that velocity vector
  // and flight time to this function.
  //
  // clearP is set to true or false
  //
  // if false, 'distance' is set to the range from pFrom where the blockage is,
  // ignoring the z component. thus, it is the (x,y) map distance one would
  // use when looking down at a map, not the ful (x,y,z) distance.
  //
  void clearBallistic(AAA::GVector pFrom, AAA::GVector vel,
		      double fTime,
		      bool& clearP, double& distance);

  // this checks both LOS and earth curvature
  bool terrainVisibleP(AAA::GVector p, AAA::GVector q);


  static double StandardEntityHeight; // %%% default entity height
  int rows;
  int clms;
  double max_X;
  double max_Y;
  
  // useful for drawing shades of grey to indicate height
  double min_Z;
  double max_Z;

  double stepLOS;

  // after all cell have their heights set, compute slopes of this cell
  // remember: positive X is to right of the screen, and positive Y is down
  void setSlopes(); 
  // this uses a very simple model of terrain:
  // random initial heights, then several iterations of
  // weighted average with neighbors.
  //
  // it renormalizes the heights so that the minZ and maxX are reached.
  void synthesizeTerrainGaussian(panj::PRNG* rng,
				 double minZ, double maxZ,
				 double weight,
				 unsigned long int iter); 

  // this also generates terrain, but it uses a fractal algorithm
  // also forces min and max
  void synthesizeTerrainFractal(panj::PRNG* rng,
				double minZ, double maxZ,
				double frac);


  // min_X and min_Y are assumed to be zero, for now

  TCell*  *cells; // a dynamic array of data. Each entry is of type TCell*

protected:

private:
  std::vector<ResUnit*> *resUnitsInSqr(int i, int j);
  int numResUnits(int, int);

  // because there are public versions of these,
  // we preface each private one with an underscore
  AAA::Logical _onGrid(const int i, const int j);
  TCell* _getTCell(const int i, const int j);

  // given a physical coordinate, return the little
  // terrain cell in it: i = floor(x/dx)
  void gridFromPhysical(int &i, int &j, const double x, const double y);

  // given the coordinates of a grid cell, return
  // the physical coordinates of its center: x = (i+1/2)*dx
  void physicalFromGrid(double &x, double &y, const int i, const int j);



};

class TCell
{
  friend class TGrid; // has to manipulate occupants
public:
  TCell(int i, int j, double h);
  ~TCell();

  void addOccupant(ResUnit*);
  void removeOccupant(ResUnit*);
  int numOccupants();
  
  TGridType type;
  double height; // same height throughout cell (rectangular pillars)

  // slope along X and Y axiis, at middle of cell
  // remember: positive X is to right of the screen, and positive Y is down
  double slopeX; 
  double slopeY;

  void displayOccupants();
  int getR();
  int getC();

protected:
  std::vector<ResUnit*> *occupants;

private:
  int r;
  int c;

};

// ------------------------------------------
#endif
// ------------------------------------------
// END of tgrid.h
// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
