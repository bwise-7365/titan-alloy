// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp.
// Changes: GUI include removed; evector becomes std::vector; AAA::RNG becomes panj::PRNG; AAA::Logical usings added.
// ------------------------------------------
#include "tdv.h"

#include <vector>

// ------------------------------------------

using std::vector;
using AAA::GVector;
using AAA::sqr;

using AAA::Logical;
using AAA::LTrue;
using AAA::LFalse;

// ------------------------------------------


GVector
weighted_mean_pt(std::vector<GVector*>* pt_list,  // data of type GVector*
		 std::vector<float>* wt_list) // data of type float*
{
  unsigned long int i = 0;
  unsigned long int n = wt_list->size();
  assert (pt_list->size() == n);
  float mean_x, mean_y, wt, wt_sum;
  GVector pt = GVector(3);
  GVector* gvptr = NULL;

  // calculate statistics for pt_list' points
  mean_x = mean_y = 0;
  wt_sum = 0;

  for (i=0; i<n ; i++)  {
    gvptr = (*pt_list)[i];
    pt = *gvptr;
    wt = (*wt_list)[i];
    mean_x = mean_x + (pt.get(0))*wt;
    mean_y = mean_y + (pt.get(1))*wt;
    wt_sum = wt_sum + wt;
  }

  mean_x = mean_x / wt_sum;
  mean_y = mean_y / wt_sum;

  return GVector(mean_x, mean_y, 0.0);

}


GVector
mean_pt(std::vector<GVector*>* pt_list) {
  unsigned long int i = 0;
  unsigned long int n = pt_list->size();
  float mean_x, mean_y;
  GVector pt;
  GVector* gvptr = NULL;

  // calculate statistics for pt_list' points
  mean_x = mean_y = 0;

  for (i=0; i<n ; i++)  {
    gvptr = (*pt_list)[i];
    pt = *gvptr;
    mean_x = mean_x + pt.get(0);
    mean_y = mean_y + pt.get(1);
  }

  mean_x = mean_x / ((float) n);
  mean_y = mean_y / ((float) n);

  return GVector(mean_x, mean_y, 0.0);

}



GVector
stdvVctr(std::vector<GVector*> *pt_list) { 
  GVector rslt = GVector(0.0, 0.0, 0.0);
  unsigned int i = 0;
  unsigned int j = 0;
  double c;
  double mc[VctrRows];
  double mcc[VctrRows];
  GVector tv = GVector(0.0, 0.0, 0.0);
  GVector* gvptr = NULL;
  //  Node *nd;
  double  n = ((double) pt_list->size());

  for (i=0; i<VctrRows; i++) {
    mc[i] = 0.0;
    mcc[i] = 0.0;
  }

  if (n > 0) {
    //     for (nd = pt_list->first; nd != NULL; nd = pt_list->nextNode(nd)) {
    //       tvPtr = ((GVector*) nd->data);
    for (j=0; j<n; j++) {
      gvptr = (*pt_list)[j];
      tv = *gvptr;
      //       cout << " next point " << endl;
      for (i=0; i<VctrRows; i++) {
	c =  tv.get(i);
	mc[i] = mc[i] + c;
	mcc[i] = mcc[i]+ (c*c);
	// 	cout << "c[" << i << "] = " << c << endl << flush;
	// 	cout << "mc[" << i << "] = " << mc[i] << endl << flush;
	// 	cout << "mcc[" << i << "] = " << mcc[i] << endl << flush;
      }
    }

    for (i=0; i<VctrRows; i++) {
      mc[i] = mc[i]/ n;
      mcc[i] = mcc[i]/ n;
    }
    for (i=0; i<VctrRows; i++) {
      assert (mcc[i] * 1.001 >= (mc[i] * mc[i])); // allow for roundoff error
      rslt.set(sqrt(mcc[i] - (mc[i] * mc[i])), i);
    }
  }
  return rslt;
}

double effectiveSpacing(std::vector<GVector*> *ptList) {
  double eSpace = 0.0;
  double d2Sum = 0.0;
  double d2 = 0.0;
  unsigned long int i = 0;
  unsigned long int n = ptList->size();
  assert (n > 1); // else eSpace = 0.0

  GVector* pt = NULL;


  GVector ctr = mean_pt(ptList);

  // here is the key
  // if we have N points, in an M-by-M regular array
  // (N = M^^2, of course), spaced s apart,
  // then the sum of the squared distances from the
  // points to their center is s^^2*N*(N-1)/6

  d2Sum = 0.0;
  for (i=0; i<n; i++) {
    pt = (*ptList)[i];
    d2 = dist(*pt, ctr);
    d2 = d2*d2;
    d2Sum = d2Sum + d2;
  }
  eSpace = sqrt ( 6.0 * d2Sum / (n * (n-1.0)));

  return eSpace;
}


std::vector<unsigned int>* targetNumSubs(unsigned int numSubs, unsigned int subCUSize) {
  std::vector<unsigned int>* subVector = new std::vector<unsigned int>();

  unsigned long int i = 0;
  unsigned long int n = numSubs / subCUSize;
  unsigned long int r = numSubs  - (n * subCUSize);

  for (i=0; i<n; i++)
    subVector->push_back(subCUSize);


  if (0 == r) { // perfect fit
    return subVector;
  }


  subVector->push_back(r);

  unsigned int m = subVector->size(); //subCUSize + 1

  assert (m == (n + 1));
  unsigned int largest = subCUSize; // same as all but last
  unsigned int iLargest = 0;

  unsigned int smallest = r; // smaller than all others
  unsigned int iSmallest = m - 1;

  while (largest > smallest + 1) {
    // note:
    // l > s + 1
    // ==> l >= s + 2
    // ==> l - 1 >= s + 1
    (*subVector)[iLargest] = (*subVector)[iLargest] - 1;
    (*subVector)[iSmallest] = (*subVector)[iSmallest] + 1;

    iLargest = 0;
    iSmallest = 0;
    largest = (*subVector)[iLargest];
    smallest = (*subVector)[iSmallest];

    for (i=0; i<m; i++) {

      if (largest < (*subVector)[i]) {
	iLargest = i;
	largest = (*subVector)[i];
      }
      if (smallest > (*subVector)[i]) {
	iSmallest = i;
	smallest = (*subVector)[i];
      }

    }
  }

  return subVector;
}

//------------------------------------------------------
/*

// cut out everything involving node lists


// //------------------------------------------------------
// ------------------------------------------------------
// NodeList* // DLList<GVector*>
// slide_and_scale_points(NodeList *q_list,  // DLList<GVector*> 
// 		       NodeList *p_list,  // DLList<GVector*> 
// 		       float &rms);

NodeList* // data of type GVector*
slide_and_scale_points(NodeList *q_list, NodeList *p_list, float &rms) {
NodeList *r_list; 
Node *pND, *qND;
GVector *qi, *pi, all_q, all_p, *riPtr, all_r, slide;
GVector ri;
int n;
float scale, qp, q_all_p, qq, q_all_q, dx, dy, difference;

r_list = new NodeList(); // DLList<GVector*>();
assert (NULL != r_list);

all_q = threeV(0,0);
all_p = threeV(0,0);
all_r = threeV(0,0);
qp = 0;
q_all_p = 0;
qq = 0;
q_all_q = 0;
difference = 0;

n = q_list->size();
if (n != p_list->size())
{
warnUser( "Slide_and_scale tried to compare two point-sets \n of differing size. \n Results are meaningless and unpredictable",
1);
}
for (qND = q_list->first, pND = p_list->first;
((qND != NULL) && (pND != NULL));
qND = q_list->nextNode(qND), pND = p_list->nextNode(pND))
{
  qi = ((GVector*) qND->data);
  pi = ((GVector*) pND->data);
  all_q = all_q + *qi;
  all_p = all_p + *pi;
}

for (qND = q_list->first, pND = p_list->first;
     ((qND != NULL) && (pND != NULL));
     qND = q_list->nextNode(qND), pND = p_list->nextNode(pND))
{
  qi = ((GVector*) qND->data);
  pi = ((GVector*) pND->data);

  qp      =    qp   + (*qi) * (*pi);
  q_all_p = q_all_p + (*qi) * all_p;
  qq      =    qq   + (*qi) * (*qi);
  q_all_q = q_all_q + (*qi) * all_q;
}

scale = ((n * qp) - q_all_p) / ((n * qq) - q_all_q);

for (qND = q_list->first, pND = p_list->first;
     ((qND != NULL) && (pND != NULL));
     qND = q_list->nextNode(qND), pND = p_list->nextNode(pND))
{
  qi = ((GVector*) qND->data);
  pi = ((GVector*) pND->data);
  //       riPtr = new GVector(pi->x - (scale * qi->x), pi->y - (scale * qi->y));
  //       all_r = all_r + (*riPtr);
  ri = GVector ( pi->get(0) - (scale * qi->get(0)), pi->get(1) - (scale * qi->get(1)), 0.0);
  all_r = all_r + ri;
}

slide = all_r / n;

for (qND = q_list->first, pND = p_list->first;
     ((qND != NULL) && (pND != NULL));
     qND = q_list->nextNode(qND), pND = p_list->nextNode(pND))
{
  qi = ((GVector*) qND->data);
  pi = ((GVector*) pND->data);
  riPtr = new GVector(slide.get(0) + (scale * qi->get(0)), 
		      slide.get(1) + (scale * qi->get(1)),
		      0.0);
  assert (NULL != riPtr);

  dx = pi->get(0) - riPtr->get(0);
  dy = pi->get(1) - riPtr->get(1);
  r_list->append((void*) riPtr);
  difference = difference + (dx*dx + dy*dy);
}

rms = sqrt(difference/n);

return r_list;
}

*/

// ------------------------------------------------------


// float
// vctr_stdv(DLList<GVector*> pt_list, GVector unit_vctr)
// {

//   float n, coord, mean_c, mean_c2, stdv_c;
//   GVector pt;
//   n = (float)(pt_list->population());

//   // calculate statistics for pt_list' points
//   mean_c = mean_c2 = 0;
//   for (pt_list->resetFirst(); pt_list->isEnd()== False; pt_list->next())
//     {
//       pt = *(pt_list->item());
//       coord = pt * unit_vctr;
//       mean_c = mean_c + coord;
//       mean_c2 = mean_c2 + (coord * coord);
//     }

//   mean_c = mean_c / n;
//   mean_c2 = mean_c2 / n;

//   stdv_c = sqrt(mean_c2 - mean_c*mean_c);

//   return stdv_c;
// }


// float
// weighted_vctr_stdv(DLList<GVector*> pt_list, 
// 		   GVector unit_vctr, 
// 		   DLList<float>wt_list)
// {

//   float coord, mean_c, mean_c2, stdv_c, wt, wt_sum;
//   GVector pt;

//   wt = wt_sum = 0.0;

//   // calculate statistics for pt_list' points
//   mean_c = mean_c2 = 0;
//   for (pt_list->resetFirst(), wt_list->resetFirst(); 
//        (pt_list->isEnd()== False)&& (wt_list->isEnd()== False); 
//        pt_list->next(), wt_list->next())
//     {
//       pt = *(pt_list->item());
//       wt = wt_list->item();
//       coord = pt * unit_vctr;
//       mean_c = mean_c + (coord*wt);
//       mean_c2 = mean_c2 + (coord * coord * wt);
//       wt_sum = wt_sum + wt;
//     }

//   mean_c = mean_c / wt_sum;
//   mean_c2 = mean_c2 / wt_sum;

//   stdv_c = sqrt(mean_c2 - mean_c*mean_c);

//   return stdv_c;
// }



// float
// x_stdv(DLList<GVector*> pt_list)
// {
//   float n, mean_x, mean_x2, stdv_x;
//   GVector pt;
//   n = (float)(pt_list->population());

//   // calculate statistics for pt_list' points
//   mean_x = mean_x2 = 0;
//   for (pt_list->resetFirst(); pt_list->isEnd()== False; pt_list->next())
//     {
//       pt = *(pt_list->item());
//       mean_x = mean_x + pt.x;
//       mean_x2 = mean_x2 + (pt.x)*(pt.x);
//     }

//   mean_x = mean_x / n;
//   mean_x2 = mean_x2 / n;

//   stdv_x = sqrt(mean_x2 - mean_x*mean_x);

//   return stdv_x;
// }

// float
// y_stdv(DLList<GVector*> pt_list)
// {
//   float n, mean_y, mean_y2, stdv_y;
//   GVector pt;
//   n = (float)(pt_list->population());

//   // calculate statistics for pt_list' points
//   mean_y = mean_y2 = 0;
//   for (pt_list->resetFirst(); pt_list->isEnd()== False; pt_list->next())
//     {
//       pt = *(pt_list->item());
//       mean_y = mean_y + pt.x;
//       mean_y2 = mean_y2 + (pt.x)*(pt.x);
//     }

//   mean_y = mean_y / n;
//   mean_y2 = mean_y2 / n;

//   stdv_y = sqrt(mean_y2 - mean_y*mean_y);

//   return stdv_y;
// }

//------------------------------------------------------
istream&
operator >> (istream& s, Box& b) {
  Logical ok = LFalse;
  
  while (LFalse == ok) {
    cout << "Specify the points in counter-clockwise order" << endl;
    cout << "Specify a point: " << endl;
    //    s >> b.A;
    cout << "Specify a point: " << endl;
    //    s >> b.B;
    cout << "Specify a point: " << endl;
    //    s >> b.C;
    cout << "Specify a point: " << endl;
    //    s >> b.D;

    b.center = ((b.A + b.B) + (b.C + b.D))/4.0;
    b.set_area_and_perimeter();
    // don't forget to check integrity!
    ok = b.check_integrity();
    if (ok != LTrue) {
      cout << "Failed check_integrity(). Try again. " << endl;
    }
  }

  return s;
}


ostream&
operator << (ostream& s, Box& b)
{
  s << "[" << b.A << " " << b.B << " " << b.C << " " << b.D << "]";
  return s;
}


Box::Box()
{
  perimeter_internal = 0.0;  // the signal that it is uninitialized!
  area_internal = 0.0;       // the signal that it is uninitialized!

};


Box::~Box() {
};


Box::Box(GVector a, GVector b, GVector c, GVector d) {
  Logical ok;
  A = a;
  B = b;
  C = c;
  D = d;

  center = ((A+B)+(C+D))/4.0;
    
  ok = this->check_integrity();
  if (ok != LTrue)  {
    cout << endl << flush;
    cout << "Box(GVector, GVector, GVector, GVector) tried to make ";
    cout << "an ill-formed box!" << endl;
    cout << "It was: " << *this << endl;
    cout << endl << flush;
    assert(LFalse);
  }

  set_bounds();
  perimeter_internal = 0.0;  // the signal that it is uninitialized!
  area_internal = 0.0;       // the signal that it is uninitialized!
  my_x = threeV(0,0);
  my_y = threeV(0,0); 
};

Box::Box(Box *bx) {
  Logical ok;
  A = bx->A;
  B = bx->B;
  C = bx->C;
  D = bx->D;

  center = ((A+B)+(C+D))/4.0;
    
  ok = this->check_integrity();
  if (ok != LTrue)
    {
      cout << "Box(GVector, GVector, GVector, GVector) tried to make ";
      cout << "an ill-formed box!" << endl;
      cout << "It was: " << *this << endl;
      assert(LFalse);
    }

  set_bounds();
  perimeter_internal = 0.0;  // the signal that it is uninitialized!
  area_internal = 0.0;       // the signal that it is uninitialized!
  my_x = threeV(0,0);
  my_y = threeV(0,0);
}

void
Box::set_bounds() {

  min_x_internal = A.get(0);
  max_x_internal = A.get(0);
  min_y_internal = A.get(1);
  max_y_internal = A.get(1);

  if (min_x_internal > B.get(0)) min_x_internal = B.get(0);
  if (min_x_internal > C.get(0)) min_x_internal = C.get(0);
  if (min_x_internal > D.get(0)) min_x_internal = D.get(0);

  if (min_y_internal > B.get(1)) min_y_internal = B.get(1);
  if (min_y_internal > C.get(1)) min_y_internal = C.get(1);
  if (min_y_internal > D.get(1)) min_y_internal = D.get(1);

  if (max_x_internal < B.get(0)) max_x_internal = B.get(0);
  if (max_x_internal < C.get(0)) max_x_internal = C.get(0);
  if (max_x_internal < D.get(0)) max_x_internal = D.get(0);

  if (max_y_internal < B.get(1)) max_y_internal = B.get(1);
  if (max_y_internal < C.get(1)) max_y_internal = C.get(1);
  if (max_y_internal < D.get(1)) max_y_internal = D.get(1);
  return;
}

// local_x points from the left side (D-C) to the right side (A-B)
//
// I compute local_x in this slightly odd way because
// it tends to be used where the C-D and A-B distances
// are much more than the D-A and C-B distances, as
// when planning a move down a corridor
GVector
Box::local_x() {
  if ((my_x.get(0) == 0)&&(my_x.get(1) ==0))  // TODO figure out how to replace unsafe == with ~
    {
      my_x = ((C-D)+(B-A)) / -2.0;  // points from B-C to D-A
      my_x.scale_to(1.0);
      //       my_x.drotate_left();          // points from C-D to A-B
      my_x = rotateCCW(my_x);
    }
  return my_x;
}


// local_y points from the near end (A-D) to the far end (B-C)
//
GVector
Box::local_y() {
  if ((my_y.get(0) == 0)&&(my_y.get(1) ==0))
    {
      my_y = ((C-D)+(B-A)) / 2.0;  // points from D-A to B-C
      my_y.scale_to(1.0);
    }
  return my_y;
}

GVector
Box::randomPoint(panj::PRNG* rng) {
  assert (NULL != rng);
  float a = rng->uniform(0.0, 1.0);
  float b = rng->uniform(0.0, 1.0);
  GVector rslt = B * a * b 
    + A * a * (1.0-b)
    + C * (1.0-a) * b
    + D * (1.0-a) * (1.0-b);

  return rslt;
}

// for keeping track useful data, but too expensive
// to call on every single Box.
//
void
Box::set_area_and_perimeter() {
  GVector d;
  if (area_internal ==0.0)
    {
      center = ((A+B)+(C+D))/4.0;
      area_internal = 
	triangleArea(A,B, center) +
	triangleArea(B,C, center) +
	triangleArea(C,D, center) +
	triangleArea(D,A, center);

      perimeter_internal = gvDistance(A,B) + gvDistance(B,C) + gvDistance(C,D) + gvDistance(D,A);
    }
  return;
}


//------------------------------------------------------


// this test is non-obvious, but efficient
// and correct:
// if all the same-side-p tests are passed, then
// the polygon is definitely convex, 
// etc.
// then we check that they really are in counter-clockwise
// order. (if the point were on the right, it would be clockwise).
Logical
Box::check_integrity() {
  Logical rslt;
  
  if (sameSideP(C, D,  A, B) &&
      sameSideP(D, A,  B, C) &&
      sameSideP(A, B,  C, D) &&
      sameSideP(B, C,  D, A) &&
      leftP (D, A, B))
    rslt = LTrue;
  else
    rslt = LFalse;
  return rslt;
  return probe_box_integrity(A, B, C, D);
};

// check PROPOSED a, b, c, and d would make
// a well-formed box, without comitting to it
// or having to construct a dummy box.
//
Logical
probe_box_integrity(GVector pa, GVector pb, GVector pc, GVector pd) {
  Logical rslt = LTrue;

  if (sameSideP(pc, pd,  pa, pb) &&
      sameSideP(pd, pa,  pb, pc) &&
      sameSideP(pa, pb,  pc, pd) &&
      sameSideP(pb, pc,  pd, pa) &&
      leftP (pd, pa, pb))
    rslt = LTrue;
  else
    rslt = LFalse;

  //   if ((LTrue == rslt) && ( pa.get(0) < 0)) rslt = LFalse;
  //   if ((LTrue == rslt) && ( pa.get(1) < 0)) rslt = LFalse;

  //   if ((LTrue == rslt) && ( pb.get(0) < 0)) rslt = LFalse;
  //   if ((LTrue == rslt) && ( pb.get(1) < 0)) rslt = LFalse;

  //   if ((LTrue == rslt) && ( pc.get(0) < 0)) rslt = LFalse;
  //   if ((LTrue == rslt) && ( pc.get(1) < 0)) rslt = LFalse;

  //   if ((LTrue == rslt) && ( pd.get(0) < 0)) rslt = LFalse;
  //   if ((LTrue == rslt) && ( pd.get(1) < 0)) rslt = LFalse;

  return rslt;
};


// given that this is a well-formed box, we need only
// check the sided-ness
Logical
Box::insideP(GVector p) {
  Logical rslt;

  if (leftP(p,  A, B) &&
      leftP(p,  B, C) &&
      leftP(p,  C, D) &&
      leftP(p,  D, A))
    rslt = LTrue;
  else
    rslt = LFalse;
  return rslt;

};


Logical
Box::standard_orientation_p(GVector p) {
  Logical result = LTrue;
  GVector x, y, a1, b1, c1, d1;
  x = p - center;
  //   y = x.rotate_left();
  y = rotateCCW(x);
  a1 = A - center;
  b1 = B - center;
  c1 = C - center;
  d1 = D - center;
  if (
      0 == x*y  // just to satisfy compiler!
      )
    result = LFalse;
  return result;
}



Box*
Box::bounding_box() {
  Box *b2;

  set_bounds();
  b2 = new Box(threeV(min_x_internal, min_y_internal),
	       threeV(max_x_internal, min_y_internal),
	       threeV(max_x_internal, max_y_internal),
	       threeV(min_x_internal, max_y_internal));
  assert (NULL != b2);

  return b2;
}

Box*
Box::shift(GVector s) {
  Box *b2;
  b2 = new Box (A+s, B+s, C+s, D+s);
  assert (NULL != b2);
  return b2;
}


void
Box::d_shift(GVector s) {
  A = A+s;
  B = B+s;
  C = C+s;
  D = D+s;
  return;
}


void
Box::d_expand(float f) {
  assert (f > 0);
  GVector offset;
  set_area_and_perimeter();

  offset = A - center;
  A = center + offset * f;

  offset = B - center;
  B = center + offset * f;

  offset = C - center;
  C = center + offset * f;

  offset = D - center;
  D = center + offset * f;

  return;
}

Box*
Box::permute_ccw() {
  Box *b2;
  
  b2 = new Box (B, C, D, A);
  assert (NULL != b2);
  return b2;
}


void
Box::d_permute_ccw() {
  GVector tmp_a;
  tmp_a = A;
  A = B;
  B = C; C = D;
  D = tmp_a;

}


// compute the rectangle (a,b,c,d) which
// minimizes the sum of the squared distances
// (A-a)^2 + (B-b)^2 + (C-c)^2 + (D-d)^2
// you have to do a Lagrangian minimization of
// the error, s.t. the base vectors are orthogonal,
// to see why this code works.
Box* 
Box::rectangularize() {
  GVector d1, d2;
  GVector alpha, beta;
  Box* rectangle;
  float g, lambda;

  d1 = A-C;
  d2 = D-B;
  g = sqrt((d1*d1)/(d2*d2));
  lambda = 2.0*(g-1)/(g+1);
  
  alpha = d1/(2+lambda);
  beta  = d2/(2-lambda);
  rectangle = new Box(
		      center + alpha,
		      center - beta,
		      center - alpha,
		      center + beta
		      );
  assert (NULL != rectangle);
  return rectangle;
}



// compute the trapezoid (a,b,c,d) which
// minimizes the sum of the squared distances
// (A-a)^2 + (B-b)^2 + (C-c)^2 + (D-d)^2
Box* 
nearest_trapezoid(GVector A, GVector B, GVector C, GVector D) {
  GVector center, alpha, beta;
  GVector ta, tb, tc, td;
  Box* trpzd;
  center = ((A+B)+(C+D))/4.0;
  alpha = (A-C)/2.0;
  beta  = (B-D)/2.0;
  ta = center + alpha;
  tb = center + beta;
  tc = center - alpha;
  td = center - beta;
  // got CCW order
  if (probe_box_integrity(ta, tb, tc, td) == LTrue)
    trpzd = new Box(ta, tb, tc, td);
  else
    // swap from CW to CCW order
    trpzd = new Box(td, tc, tb, ta);
  assert (NULL != trpzd);
  return trpzd;
}




Logical
Box::intersect_p(GVector l1, GVector l2) {

  if (this->insideP(l1) == LTrue)
    return LTrue;
  if (this->insideP(l2) == LTrue)
    return LTrue;
  if (segIntersectP(A, B, l1, l2) == LTrue)
    return LTrue;
  if (segIntersectP(B, C, l1, l2) == LTrue)
    return LTrue;
  if (segIntersectP(C, D, l1, l2) == LTrue)
    return LTrue;

  // a segment can not intersect a convex polygon
  // along only one edge, unless one point is in
  // the interior (which we already checked), so 
  // we can safely skip the last test
  //
  //    if (segIntersectP(D, A, l1, l2) == LTrue)
  //      return LTrue;
  return LFalse;
}

float 
Box::min_x()  {
  return min_x_internal;
}

float 
Box::max_x()  {
  return max_x_internal;
}

float 
Box::min_y() {
  return min_y_internal;
}

float 
Box::max_y() {
  return max_y_internal;
}

float 
Box::area() {
  if (area_internal == 0)
    set_area_and_perimeter();
  return area_internal;
}


float 
Box::perimeter() {
  if (perimeter_internal == 0)
    set_area_and_perimeter();
  return perimeter_internal;
}

GVector Box::get_A(){    return A;}
GVector Box::get_B(){    return B;}
GVector Box::get_C(){    return C;}
GVector Box::get_D(){    return D;}

void Box::set_A(GVector pt){A=pt;    return;}
void Box::set_B(GVector pt){B=pt;    return;}
void Box::set_C(GVector pt){C=pt;    return;}
void Box::set_D(GVector pt){D=pt;    return;}



// ------------------------------------------
Logical
box_intersect_p(Box *b1, Box *b2) {
  Logical rslt = LFalse; 

  b1->set_bounds();
  b2->set_bounds();

  if (b1->min_x() > b2->max_x()) rslt = LFalse; // b1 completely to right of b2
  if (b2->min_x() > b1->max_x()) rslt = LFalse; // b2 completely to right of b1

  if (b1->min_y() > b2->max_y()) rslt = LFalse; // b1 completely above b2
  if (b2->min_y() > b1->max_y()) rslt = LFalse; // b2 completely above b1


  // various edges might intersect the  other box
  if ((LFalse == rslt) && (LTrue == b1->intersect_p(b2->get_A(), b2->get_B())))
    rslt = LTrue;
  if ((LFalse == rslt) && (LTrue == b1->intersect_p(b2->get_B(), b2->get_C())))
    rslt = LTrue;
  if ((LFalse == rslt) && (LTrue == b1->intersect_p(b2->get_C(), b2->get_D())))
    rslt = LTrue;
  if ((LFalse == rslt) && (LTrue == b1->intersect_p(b2->get_D(), b2->get_A())))
    rslt = LTrue;

  if ((LFalse == rslt) && (LTrue == b2->intersect_p(b1->get_A(), b1->get_B())))
    rslt = LTrue;
  if ((LFalse == rslt) && (LTrue == b2->intersect_p(b1->get_B(), b1->get_C())))
    rslt = LTrue;
  if ((LFalse == rslt) && (LTrue == b2->intersect_p(b1->get_C(), b1->get_D())))
    rslt = LTrue;
  if ((LFalse == rslt) && (LTrue == b2->intersect_p(b1->get_D(), b1->get_A())))
    rslt = LTrue;

  // various corners might be inside the other box
  if ((LFalse == rslt) &&(b1->insideP(b2->get_A()) == LTrue)) 
    rslt = LTrue;
  if ((LFalse == rslt) &&(b1->insideP(b2->get_B()) == LTrue)) 
    rslt = LTrue;
  if ((LFalse == rslt) &&(b1->insideP(b2->get_C()) == LTrue)) 
    rslt = LTrue;
  if ((LFalse == rslt) &&(b1->insideP(b2->get_D()) == LTrue)) 
    rslt = LTrue;

  if ((LFalse == rslt) &&(b2->insideP(b1->get_A()) == LTrue)) 
    rslt = LTrue;
  if ((LFalse == rslt) &&(b2->insideP(b1->get_B()) == LTrue)) 
    rslt = LTrue;
  if ((LFalse == rslt) &&(b2->insideP(b1->get_C()) == LTrue)) 
    rslt = LTrue;
  if ((LFalse == rslt) &&(b2->insideP(b1->get_D()) == LTrue)) 
    rslt = LTrue;

  return rslt;
}   

// ------------------------------------------


GVector threeV(float x, float y) {
  GVector rslt = GVector(3);
  rslt.set(x, 0);
  rslt.set(y, 1);
  rslt.set(0.0, 2);
  return rslt;
}

GVector threeV(float x, float y, float z) {
  GVector rslt = GVector(3);
  rslt.set(x, 0);
  rslt.set(y, 1);
  rslt.set(z, 2);
  return rslt;
}


// these handle only the x,y coords. z and above are ignored, and zero returned.
GVector
rotateCCW(GVector v) {
  return threeV( -v.get(1),  v.get(0));
}

GVector
rotateCW(GVector v) {
  return threeV(v.get(1), -v.get(0));
}


Logical 
leftP(GVector p, GVector l1, GVector l2) {
  GVector v, q;
  Logical rslt = LFalse;

  v = rotateCCW(l2-l1);

  q = nearestPoint(p, l1, l2);
  if ((v * (p-q)) > 0)
    rslt = LTrue;
  else
    rslt = LFalse;

  return rslt;
}


Logical 
sameSideP (GVector p1, GVector p2,GVector l1, GVector l2) {
  GVector q1, q2;
  Logical rslt;
    

  //    cout << "Checking if " << p1 << " and " << p2;
  //    cout << " are on the same side of " << l1 << " -> " << l2 << endl;
  //    cout << flush; 
  q1 = nearestPoint(p1, l1, l2);
  q2 = nearestPoint(p2, l1, l2);

  if (((p1 - q1) * (p2 - q2)) > 0)
    rslt = LTrue;
  else
    rslt = LFalse;

  //    cout << "Result was " << rslt << endl;
  return rslt;
}


// you have to do a Lagrangian minimization of
// the error (X-A)^^2 + (Y-B)^^2, s.t. XY = 0,
// to see why this code works.
// the trick is to see that X = A + fY  and Y = B + fX (same f!),
// do the infinite expansion to get
// X = (A+fB)/(1-sqr(f)) and Y=(B+fA)/(1-sqr(f)),
// and solve (A+fB)*(B+fA) = 0 = (AB) + f (AA+BB) + sqr(f) (AB)
void perpendicularize(GVector A, GVector B, GVector& X, GVector& Y) {
  double ab = A * B;
  double aa = A * A;
  double bb = B * B;

  double a = ab;
  double b = aa + bb;
  double c = ab;

  // I SUSPECT that xPlus is always the right one
  double xPlus = -b + sqrt (b*b - 4*a*c)/(2*a);
  //  double xMinus = -b - sqrt (b*b - 4*a*c)/(2*a);

  X = (A + (B * xPlus)) / (1 - sqr(xPlus));
  Y = (B + (A * xPlus)) / (1 - sqr(xPlus));

  return;
}

GVector bound(GVector gv1, double x) {
  double z = norm(gv1);
  double factor = x / z;
  if (factor < 1.0)
    return (gv1 * factor);
  else 
    return GVector(gv1);
}

// ------------------------------------------
// END of tdv.cc
// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
