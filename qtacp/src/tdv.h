// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp.
// Changes: evector becomes std::vector; AAA::RNG becomes panj::PRNG.
// ------------------------------------------


#ifndef BPW_TDV_H
#define BPW_TDV_H

// ------------------------------------------

#include "aaa.h"
#include "mat.h"
//#include <stl.h>
//#include <vector>

//#include "nodes.h"

#include "acpsim.h"

#include <vector>

const unsigned int VctrRows = 3; // these are strictly 3D vectors

// ------------------------------------------

AAA::Logical probe_box_integrity(AAA::GVector pa, AAA::GVector pb, AAA::GVector pc, AAA::GVector pd);

AAA::GVector threeV(float x, float y);
AAA::GVector threeV(float x, float y, float z);

AAA::GVector rotateCCW(AAA::GVector v1);
AAA::GVector rotateCW(AAA::GVector v1);

// make sure X and Y are initialized before calling
//
// I think 'perpendicularize' is correct, but I have
// NOT exhaustively tested it yet.
//
void perpendicularize(AAA::GVector A, AAA::GVector B, AAA::GVector& X, AAA::GVector& Y);

AAA::GVector bound(AAA::GVector gv, double x);


// considering (l1, l2) as defining a directed ray from l1 to l2
// and beyond, determine if p is on the left side of that 
// ray in the (X,Y) plane
//
AAA::Logical 
leftP (AAA::GVector p, AAA::GVector l1, AAA::GVector l2);

// considering (l1, l2) as defining an infinite line,
// determine if p1 and p2 lie on the same side of that line
// in the (X,Y) plane.
AAA::Logical 
sameSideP (AAA::GVector p1, AAA::GVector p2,AAA::GVector l1, AAA::GVector l2);


AAA::GVector
weighted_mean_pt(std::vector<AAA::GVector*>* pt_list, std::vector<float>* wt_list);

AAA::GVector mean_pt(std::vector<AAA::GVector*> *pt_list);
AAA::GVector stdvVctr(std::vector<AAA::GVector*> *pt_list);
double effectiveSpacing(std::vector<AAA::GVector*> *ptList);

// given that we have numSub subordinates to be put in groups,
// how should we organize them to get as close as possible to
// the right subCUSize in each group?
std::vector<unsigned int>* targetNumSubs(unsigned int numSubs,
					  unsigned int subCUSize);



// ------------------------------------------

// cut out everything involving node lists

/*


NodeList* // DLList<AAA::GVector*>
slide_and_scale_points(NodeList *q_list,  // planned, DLList<AAA::GVector*> 
		       NodeList *p_list,  // actual, DLList<AAA::GVector*> 
		       float &rms);
// q_list is the template,
// p_list is the actual situation
// so it returns r_list with
// r[i] = a + q[i]*s, with vector a and scalar s,
// to minimize the rms error between r[i] and p[i].
// also resets rms to the rms mean-distance between actual and
// transformed.
//
// One use is to slide and scale the point-set "planned" so
// as to come as close as possible to the set "actual".
// 


// float
// vctr_stdv(DLList<AAA::GVector*> pt_list, AAA::GVector basis_vctr);

// float
// weighted_vctr_stdv(DLList<AAA::GVector*> pt_list,AAA::GVector unit_vctr,DLList<float>wt_list);

*/

//------------------------------------------------------
// A,B,C,D must be listed in a specific order.
// if your coordinate system is "x to the right, y up", it is CCW.
// if your coordinate system is "x to the right, y down", it is CW.
// the LATTER describes the standard X window coordinate system.
//
// when putting them into order for a corridor,
// A->B is the right edge, B->C is the "far" or "front" edge, 
// C->D is the left edge, D->A is the "base" or "back" edge.
// the far edge connects to the NEXT box in time order,
// the base edges connects to the PREVIOUS box in time order.
// thus, the front of Box0 connects to the back of Box1,
// the front of Box1 connects to the back of Box2, etc.
class Box {

public:

  Box();
  virtual ~Box();
  Box(AAA::GVector a, AAA::GVector b, AAA::GVector c, AAA::GVector d);
  Box(Box*);
  virtual AAA::Logical check_integrity();
  virtual AAA::Logical insideP(AAA::GVector p);
  virtual AAA::Logical standard_orientation_p(AAA::GVector p);
  virtual AAA::Logical intersect_p(AAA::GVector p1, AAA::GVector p2);
  virtual void set_area_and_perimeter();
  virtual void set_bounds();

  Box* bounding_box();
  Box* shift(AAA::GVector);
  void d_shift(AAA::GVector);
  void d_expand(float); // destructively contract/expand around center
  Box* permute_ccw();
  void d_permute_ccw();
  Box* rectangularize();

  
  float min_x();
  float min_y();
  float max_x();
  float max_y();
  float area();
  float perimeter();

  AAA::GVector get_A();
  AAA::GVector get_B();
  AAA::GVector get_C();
  AAA::GVector get_D();

  void set_A(AAA::GVector);
  void set_B(AAA::GVector);
  void set_C(AAA::GVector);
  void set_D(AAA::GVector);

  AAA::GVector local_x();
  AAA::GVector local_y();

  AAA::GVector randomPoint(panj::PRNG*);

  friend istream& operator >> (istream& s, Box& t);
  friend ostream& operator << (ostream& s, Box& t);

  AAA::GVector center;
 
protected:
private:

  AAA::GVector A;
  AAA::GVector B;
  AAA::GVector C;
  AAA::GVector D;
    
  float min_x_internal;
  float max_x_internal;
  float min_y_internal;
  float max_y_internal;

  AAA::GVector my_x;
  AAA::GVector my_y;

  float area_internal;
  float perimeter_internal;

};


//------------------------------------------------------
// compute the trapezoid (a,b,c,d) which
// minimizes the sum of the squared distances
// (A-a)^2 + (B-b)^2 + (C-c)^2 + (D-d)^2
Box* 
nearest_trapezoid(AAA::GVector A, AAA::GVector B, AAA::GVector C, AAA::GVector D);


AAA::Logical box_intersect_p(Box*, Box*);


//------------------------------------------------------
#endif
// ------------------------------------------
// END of tdv.h
// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
