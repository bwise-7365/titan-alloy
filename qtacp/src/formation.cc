// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: include list, evector -> std::vector
// (pop_back no longer returns the element), inListP -> std::find.
// ------------------------------------------

#include "frwrdec.h"
#include "struct.h"
#include "tthread.h"
#include "tdv.h"

#include <vector>
#include <algorithm>

using AAA::GVector;
using std::vector;

// ------------------------------------------------------

Formation::Formation()
{
  points = new vector<GVector*>(); // crashes because 0 == dimension
  moved_points = new vector<GVector*>(); // crashes because 0 == dimension
  units = new vector<Unit*>();
  matched_units = new vector<Unit*>();

  assert (NULL != points);
  assert (NULL != moved_points);
  assert (NULL != units);
  assert (NULL != matched_units);

}

Formation::~Formation() {
  GVector* pt = NULL;
  while (points->size() > 0) {
    pt = points->back();
    points->pop_back();
    delete pt;
    pt = NULL;
  }
  delete points;
  points = NULL;

  while (moved_points->size() > 0) {
    pt = moved_points->back();
    moved_points->pop_back();
    delete pt;
    pt = NULL;
  }
  delete moved_points;
  moved_points = NULL;
  
  if (NULL != units) {
    delete units;
    units = NULL;
  }

//   if (NULL != matched_units) {
//     delete matched_units; 
//     matched_units = NULL;
//   }
}

void 
Formation::add_point(GVector* v) {
  points->push_back(v);
  return;
}

void 
Formation::add_unit(Unit *u) {
  units->push_back(u);
  return;
}


void 
Formation::match_units() {
  move_template_onto_units();
  inner_match_units();
  return;
}

vector<GVector*>*
Formation::get_assigned_positions() {
  return moved_points;
}

vector<Unit*>*
Formation::get_assigned_units() {
  return matched_units;
}

ostream& operator << (ostream& s, Formation& f) {
  cout << "Can not print formations"<<endl;
  return s;
}


// the method of doing this is simple:
// compute the mean and stdv of the
// units' x coordinates, and transform
// the template's x coordinates to have
// the same mean and stdv.
// repeat for y.
//
// at the end, the matched_points are in the 
// same order as the original points, so
// the one-to-one mapping is clear

void 
Formation::move_template_onto_units() {
  GVector unitMean = GVector(0.0, 0.0, 0.0);
  GVector unitStdv = GVector(0.0, 0.0, 0.0);
  GVector tmpltMean = GVector(0.0, 0.0, 0.0);
  GVector tmpltStdv = GVector(0.0, 0.0, 0.0);
  GVector pt = GVector(0.0, 0.0, 0.0);;
  GVector* gvptr = NULL;
  GVector pt2 = GVector(0.0, 0.0, 0.0);;
//   float n = ((float) units->size());
  vector<GVector*> *unit_ctrs = new vector<GVector*>();
  assert (NULL != unit_ctrs);
  Unit *u = NULL;
//   Node *uND = NULL;
//   Node *pND = NULL;
  float coord;
  int i, j;
  unsigned int n = units->size();
  assert (n == points->size());

  for (j=0; j<n; j++) {
    u = (*units)[j];
    gvptr = new GVector(u->currentPos());
    pt = *gvptr;
    unit_ctrs->push_back( gvptr );
  }
  // stats of units
  //  cout << "Getting unit mean point"<<endl<<flush;
  unitMean = mean_pt(unit_ctrs);
  //  cout << "Getting unit stdv vector"<<endl<<flush;
  unitStdv = stdvVctr(unit_ctrs); 

  // stats of template
  //  cout << "Getting template mean point"<<endl<<flush;
  tmpltMean = mean_pt(points);
  //  cout << "Getting template stdv vector"<<endl<<flush;
  tmpltStdv = stdvVctr(points); 

  // now build the transformed list
  if (moved_points != NULL) {
    while (moved_points->size() > 0) {
      gvptr = moved_points->back();
      moved_points->pop_back();
      delete gvptr;
      gvptr = NULL;
    }
    delete moved_points;
    moved_points = NULL;
  } 
  moved_points = new vector<GVector*>();
  assert (NULL != moved_points);

  n = points->size();
  for (j=0; j<n; j++) {
    gvptr = (*points)[j];
    pt = (*gvptr);

    pt2 = GVector(VctrRows);

    for (i=0; i<VctrRows; i++) {
      coord = pt.get(i); // get it
      if (tmpltStdv.get(i) > 0) {
	coord = (coord - tmpltMean.get(i))/ tmpltStdv.get(i);
	coord = (coord * unitStdv.get(i)) + unitMean.get(i);
      }
      else
	coord = unitMean.get(i);
      pt2.set(coord, i);
    }
    moved_points->push_back( new GVector(pt2) );
  }

  while (unit_ctrs->size() > 0) {
    gvptr = unit_ctrs->back();
    unit_ctrs->pop_back();
    delete gvptr;
    gvptr = NULL;
  }
  delete unit_ctrs;
  unit_ctrs = NULL;
  return;
}


// From before, the matched_points are in the 
// same order as the original points. They have
// the same mean and stdv-vctr as the units' points.
//
// Now, we sort the units so that they line
// up with the points to which they are closest.
//
// At the end of inner_match_units, the units
// are now also matched up one-to-one with both
// the original points and the moved points, so
// the one-to-one mapping is clear.
//
// note, this is a greedy algorithm.

void
Formation::inner_match_units() {
  GVector *gvptr = NULL;
  GVector pt, u_pos;
  Unit *u, *u_closest;
  float dstnc, dstnc_closest;
//   Node *uND, *pND;
  u_closest = NULL;

  if (NULL != matched_units)
    delete matched_units;

  matched_units = new vector<Unit*>();
  assert (NULL != matched_units);

  unsigned int i = 0;
  unsigned int n = moved_points->size();
  unsigned int j = 0;
  unsigned int m = units->size();

  for (i=0; i<n; i++) {
    gvptr = (*moved_points)[i];
    pt = *gvptr;
    dstnc_closest = -1;  // an impossible value
    for (j=0; j<m; j++) {
      u = (*units)[j];
      if ((matched_units->end() != std::find(matched_units->begin(), matched_units->end(), u)) == false) {
	u_pos = u->currentPos();
	dstnc = dist(u_pos, pt);
	if ((dstnc_closest < 0) || (dstnc < dstnc_closest)) {
	  dstnc_closest = dstnc;
	  u_closest = u;
	}
      }
    }
    // now "u_closest" is the closest unassigned unit to "pt"
    matched_units->push_back( u_closest);
  }
  // now, the units in "matched_units" are 1-to-1 
  // matched with "moved_points"

  return;
}

// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
