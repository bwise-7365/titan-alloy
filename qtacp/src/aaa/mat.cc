// --------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved
// --------------------------------------------
// Vendored from pershing/aaa for the qtacp port.
// Patches relative to the original: evector is replaced by
// std::vector; the RNG-based test functions are removed; the
// GVector destructor and copy assignment use delete[] to match
// the new[] allocation; the GVector(unsigned int) constructor
// zero-fills the whole coordinate array (the original memset
// covered only sizeof(double*) bytes).
// --------------------------------------------

// ----------------------------------

#include "aaa.h"
#include "mat.h"
#include <string.h>
#include <vector>

using AAA::GVector;
using AAA::GMatrix;
using AAA::nearestPoint;

using AAA::dabs;
using AAA::dround;
using AAA::dmin;

// ----------------------------------

GVector::GVector() {
   dim = 0;
   coords = NULL;
}


GVector::GVector(unsigned int d) {
  assert (d > 0);
  dim = d;
  coords = new double[dim];
  memset(coords, 0, dim * sizeof(double));
}


GVector::GVector(double x, double y, double z) {
  dim = 3;
  coords = new double[dim];

  coords[0] = x;
  coords[1] = y;
  coords[2] = z;
}

GVector::~GVector() {
  if (dim > 0) {
    assert (NULL != coords);
    delete[] coords;
    coords = NULL;
  }
  else {
    assert (NULL == coords);
  }
}


GVector::GVector(const GVector& gv) {
  unsigned int i = 0;
  dim = gv.dim;
  coords = new double[dim];

  for (i=0; i<dim; i++){
    coords[i] = (gv.coords)[i];
  }
}

GVector& GVector::operator = (const GVector& gv) {
  unsigned int i = 0;

  if (NULL != coords) {
    if (dim != gv.dim) {
      // wrong size, must re-allocate
      delete[] coords;
      coords = NULL;
      dim = gv.dim;
      coords = new double[dim];
    }
  }
  else { // coords was NULL
    dim = gv.dim;
    coords = new double[dim];
  }

  for (i=0; i<dim; i++){
    coords[i] = (gv.coords)[i];
  }
  return *this;
}

unsigned int GVector::getDim() {
  assert (NULL != coords);
  return dim;
}


double GVector::get(const unsigned int &i) {

  assert (dim > 0);
  assert (dim > i);
  assert (NULL != coords);

  return coords[i];
}

void GVector::set(double x, unsigned int i) {
  assert (dim > 0);
  assert (dim > i);
  assert (NULL != coords);
  coords[i] = x;
  return;
}

void GVector::scale_to(double x) {
  double nrm = norm(*this);
  unsigned int i;
  assert (nrm > 0.0);
  assert (x > 0.0);

  for (i=0; i<dim; i++) {
    coords[i] = (x * coords[i]) / nrm;
  }

  return;
}

ostream& AAA::operator << (ostream& s, GVector gv) {
  unsigned int i = 0;
  unsigned int n = gv.dim;

  assert (n > 0);

  s  <<"[ ";
  for (i=0; i<(n - 1); i++)
    s<<(gv.coords)[i]<<", ";

  s<<(gv.coords)[n-1]<<" ]";

  return s;
}

string GVector::toString()
{
	char buffer[32];
	string s;
	unsigned int i = 0;
	unsigned int n = dim;

	assert (n > 0);

	memset(buffer,0,sizeof(buffer));

	s = "";
	s += "[ ";

	for (i=0; i<(n - 1); i++)
    {
    	sprintf(buffer,"%f",(coords)[i]);
        s += buffer;
    }

	s += ", ";
   	sprintf(buffer,"%f",(coords)[n-1]);
    s += buffer;
	s += " ]";

	return s;
}

double AAA::operator * (GVector gv1, GVector gv2) {
  double sum = 0.0;
  unsigned int n = gv1.getDim();
  unsigned int i = 0;
  assert (n == gv2.getDim());
  assert (n > 0);
  for (i=0; i<n; i++)
    sum = sum + (gv1.get(i) * gv2.get(i));

  return sum;
}

GVector AAA::operator * (GVector gv1, double c) {
  unsigned int i = 0;
  unsigned int n = gv1.getDim();
  GVector gv = GVector(n);

  for (i=0; i<n; i++)
    gv.set( gv1.get(i) * c, i);

  return gv;
}

GVector AAA::operator / (GVector gv1, double c) {
  unsigned int i = 0;
  unsigned int n = gv1.getDim();
  GVector gv = GVector(n);

  for (i=0; i<n; i++)
    gv.set( gv1.get(i) / c , i);

  return gv;
}

GVector AAA::operator + (GVector gv1, GVector gv2) {
  unsigned int i = 0;
  unsigned int n = gv1.getDim();
  assert (n == gv2.getDim());

  GVector gv = GVector(n);

  for (i=0; i<n; i++)
    gv.set( gv1.get(i) + gv2.get(i), i);

  return gv;
}

GVector AAA::operator - (GVector gv1, GVector gv2) {
  unsigned int i = 0;
  unsigned int n = gv1.getDim();
  assert (n == gv2.getDim());

  GVector gv = GVector(n);

  for (i=0; i<n; i++)
    gv.set( gv1.get(i) - gv2.get(i), i);

  return gv;
}


double AAA::norm(GVector gv) {
  unsigned int i = 0;
  unsigned int n = gv.getDim();
  assert (n > 0);
  double sum = 0.0;
  double c = 0.0;

  for (i=0; i<n; i++) {
    c = gv.get(i);
    sum = sum + (c*c);
  }

  return sqrt(sum);
}

// for some bizarre reason, using 'distance' rather
// than something weird like 'gvDistance' gives a compiler error
double AAA::gvDistance(GVector gv1, GVector gv2) {
  unsigned int i = 0;
  unsigned int n1 = gv1.getDim();
  unsigned int n2 = gv2.getDim();
  assert (n1 > 0);
  assert (n1 == n2);
  double sum = 0.0;
  double c = 0.0;

  for (i=0; i<n1; i++) {
    c = gv1.get(i) - gv2.get(i);
    sum = sum + (c*c);
  }

  return sqrt(sum);
}

GVector AAA::round(GVector gv, int d) {
  int n = gv.getDim();
  int i = 0;
  assert (d >= 0);
  assert (n > 0);
  GVector gv2 = GVector(n);
  for (i=0; i<n; i++)
    gv2.set(dround(gv.get(i), d), i);
  return gv2;
}

// ----------------------------------

GMatrix::GMatrix() {
  rows = 0;
  clms = 0;
  vals = new std::vector<double>();
}

GMatrix::GMatrix(unsigned int r, unsigned int c) {
  unsigned int i = 0;
  unsigned int n = 0;

  assert (r > 0);
  assert (c > 0);

  rows = r;
  clms = c;

  n = rows * clms;
  vals = new std::vector<double>();
  vals->resize( n );

  for (i=0; i<n; i++)
    (*vals)[i] = 0;
}


// uninvoked?
GMatrix::GMatrix(const GMatrix& gm) {
  unsigned int i = 0;
  unsigned int n = 0;
  rows = gm.rows;
  clms = gm.clms;
  assert (rows > 0);
  assert (clms > 0);
  n = rows * clms;
  vals = new std::vector<double>();
  vals->resize( n );
  for (i=0; i<n; i++) {
    (*vals)[i] = (*(gm.vals))[i];
  }

}

GMatrix& GMatrix::operator = (const GMatrix& gm) {
  unsigned int i = 0;
  unsigned int n = 0;
  rows = gm.rows;
  clms = gm.clms;
  assert (rows > 0);
  assert (clms > 0);
  n = rows * clms;
  if (NULL != vals) {
    delete vals;
    vals = NULL;
  }
  vals = new std::vector<double>();
  vals->resize( n );


  for (i=0; i<n; i++) {
    (*vals)[i] = (*(gm.vals))[i];
  }
  return *this;
}

GMatrix::~GMatrix() {
  delete vals;
  vals = NULL;
}

// ----------------------------------


double GMatrix::get(unsigned int r, unsigned int c) {
  return (*vals)[indexFromRC(r,c)];
}

unsigned int GMatrix::getRows() {
  return rows;
}

unsigned int GMatrix::getClms() {
  return clms;
}


void GMatrix::set(double x, unsigned int r, unsigned int c) {
  (*vals)[indexFromRC(r,c)] = x;
  return;
}

ostream& AAA::operator << (ostream& s, GMatrix gm) {
  unsigned int i = 0;
  unsigned int j = 0;
  unsigned int r = gm.rows;
  unsigned int c = gm.clms;

  assert (r > 0);
  assert (c > 0);


  for (i=0; i<r; i++) {
    for (j=0; j<c; j++)
      s << gm.get(i,j)<<" , ";
    s << endl;
  }

  return s;
}

string GMatrix::toString()
{
	char buffer[32];
	string s;

	memset(buffer,0,sizeof(buffer));

	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int r = rows;
	unsigned int c = clms;

	assert (r > 0);
	assert (c > 0);

	s= "";
	for (i=0; i<r; i++)
	{
		for (j=0; j<c; j++)
		{
    	    sprintf(buffer,"%f",get(i,j));
            s += buffer;
			s += " , ";
		}

		s += "\n";
	}

	return s;
}

// ----------------------------------
// 'inline' functions must be redefined, not
// just redeclared, in every translation unit
// (aka file) in which they are used.
//
// As the following three functions are private
// to the GMatrix class, they can not be
// used anywhere but this file, so it is safe
// to inline them

inline int GMatrix::indexFromRC(const int r, const int c) {
  return indexFromRowClm(r, c, rows, clms);
}


inline int GMatrix::rowFromIndex(const int i) {
  return rFromIndex(i, rows, clms);
}


inline int GMatrix::clmFromIndex(const int i) {
  return cFromIndex(i, rows, clms);
}


GMatrix operator * (GMatrix gv1, GMatrix gv2) {
  double sum = 0.0;
  unsigned int r1 = gv1.getRows();
  unsigned int c1 = gv1.getClms();
  unsigned int r2 = gv2.getRows();
  unsigned int c2 = gv2.getClms();

  unsigned int i = 0;
  unsigned int j = 0;
  unsigned int k = 0;
  unsigned int n = 0;

  assert (r1  > 0);
  assert (c1  > 0);
  assert (r2  > 0);
  assert (c2  > 0);
  assert (c1 == r2);

  n = c1;

  GMatrix gm = GMatrix(r1, c2);

  for (i = 0; i< r1; i++) {
    for (j = 0; j< c2; j++) {
      sum = 0.0;
      for (k=0; k<n; k++)
	sum = sum + (gv1.get(i,k) * gv2.get(k,j));
      gm.set(sum, i, j);
    }
  }
  return gm;
}

GVector operator * (GMatrix gm, GVector gv1) {
  double sum = 0.0;
  unsigned int i = 0;
  unsigned int j = 0;
  unsigned int d1 = gv1.getDim();
  unsigned int r = gm.getRows();
  unsigned int c = gm.getClms();
  assert (d1 > 0);
  assert (r > 0);
  assert (c > 0);
  assert (c == d1);

  GVector gv2 = GVector(r);

  for (i=0; i<r; i++) {
    sum = 0.0;
    for (j=0; j<c; j++)
      sum = sum + gm.get(i,j) * gv1.get(j);

    gv2.set(sum, i);
  }
  return gv2;
}


GMatrix operator * (GMatrix gv1, double x) {
  unsigned int i = 0;
  unsigned int j = 0;
  unsigned int r = gv1.getRows();
  unsigned int c = gv1.getClms();
  GMatrix gv = GMatrix(r,c);

  for (i=0; i<r; i++)
    for (j=0; j<c; j++)
      gv.set( gv1.get(i,j) * x , i, j);

  return gv;
}

GMatrix operator / (GMatrix gv1, double x) {
  unsigned int i = 0;
  unsigned int j = 0;
  unsigned int r = gv1.getRows();
  unsigned int c = gv1.getClms();
  GMatrix gv = GMatrix(r,c);

  for (i=0; i<r; i++)
    for (j=0; j<c; j++)
      gv.set( gv1.get(i,j) / x , i, j);

  return gv;
}

GMatrix operator + (GMatrix gv1, GMatrix gv2) {
  unsigned int i = 0;
  unsigned int j = 0;
  unsigned int r1 = gv1.getRows();
  unsigned int c1 = gv1.getClms();
  unsigned int r2 = gv2.getRows();
  unsigned int c2 = gv2.getClms();
  assert (r1 == r2);
  assert (c1 == c2);
  assert (r1 > 0);
  assert (c1 > 0);
  GMatrix gv = GMatrix(r1,c1);


  for (i=0; i<r1; i++)
    for (j=0; j<c1; j++)
      gv.set( gv1.get(i,j) + gv2.get(i,j), i, j);

  return gv;
}


GMatrix operator - (GMatrix gv1, GMatrix gv2) {
  unsigned int i = 0;
  unsigned int j = 0;
  unsigned int r1 = gv1.getRows();
  unsigned int c1 = gv1.getClms();
  unsigned int r2 = gv2.getRows();
  unsigned int c2 = gv2.getClms();
  assert (r1 == r2);
  assert (c1 == c2);
  assert (r1 > 0);
  assert (c1 > 0);
  GMatrix gv = GMatrix(r1,c1);


  for (i=0; i<r1; i++)
    for (j=0; j<c1; j++)
      gv.set( gv1.get(i,j) - gv2.get(i,j), i, j);

  return gv;
}


GMatrix AAA::round(GMatrix gv, int d) {
  int r = gv.getRows();
  int c = gv.getClms();
  int i = 0;
  int j = 0;
  assert (r > 0);
  assert (c > 0);
  assert (d >= 0);
  GMatrix gv2 = GMatrix(r,c);
  for (i=0; i<r; i++)
    for (j=0; j<c; j++)
      gv2.set(dround(gv.get(i,j), d), i, j);
  return gv2;
}


void GMatrix::show(const char * fS) { // formatted print to cout
  for (unsigned int i=0; i<rows; i++) {
    for (unsigned int j=0; j<clms; j++) {
      printf(fS , get(i,j));
    }
    printf("\n");
  }
  return;
}

// ----------------------------------
// destructive pivoting


void GMatrix::pivot(unsigned int r, unsigned int c) {
  unsigned int i = 0;
  unsigned int j;
  assert(rows > 0);
  assert(clms > 0);
  assert(r < rows);
  assert(c < clms);

  // normalization of matrix prevents tiny pivot elements
  const double tinyPivotEl = 1e-9;
  double pivot_el = get(r,c);
  assert(fabs(pivot_el) > tinyPivotEl);

  // note that this double loop MUST come before the other
  for (i=0; i<rows; i++)
    for (j=0; j<clms; j++) {
      if ((i != r) && (j != c))
	set( get( i , j ) - (( get( r , j ) * get( i , c ))/pivot_el),
	     i, j );
    }

  for (i=0; i<rows; i++)
    for (j=0; j<clms; j++) {
      if ((i == r) && (j == c))
	set(1, i,  j);
      if((i != r) && (j == c))
	set(0, i,  j);
      if((i == r) && ( j != c))
	set( get( i , j )/pivot_el,
	     i, j);
    }

  return;
}

// ----------------------------------


GMatrix AAA::identMatrix(int n) {
  int i = 0;
  assert (n > 0);
  GMatrix idM = GMatrix(n,n); // zero-filled
  for (i=0; i<n; i++)
    idM.set(1, i, i);

  return idM;
}


GMatrix AAA::h_join(GMatrix left, GMatrix right) {
  GMatrix b3;
  int i, j;
  int leftRows = left.getRows();
  int leftClms = left.getClms();
  int rightRows = right.getRows();
  int rightClms = right.getClms();
  assert(leftRows == rightRows);
  b3 =  GMatrix(leftRows, leftClms + rightClms);

  for (i=0; i<leftRows; i++) {

    for (j=0; j<leftClms; j++)
      b3.set(left.get(i,j), i,  j);

    for(j=0; j<rightClms; j++)
      b3.set(right.get(i,j), i,  j+leftClms);
  }

  return b3;
}


GMatrix AAA::v_join(GMatrix top, GMatrix bottom) {
  GMatrix b3;
  int i, j;
  int topRows = top.getRows();
  int topClms = top.getClms();
  int bottomRows = bottom.getRows();
  int bottomClms = bottom.getClms();
  assert(topClms == bottomClms);

  b3 =  GMatrix(topRows + bottomRows, topClms);

  for (j=0; j<topClms; j++) {

    for (i=0; i<topRows; i++)
      b3.set(top.get(i,j), i,  j);

    for(i=0; i<bottomRows; i++)
      b3.set(bottom.get(i,j), i+topRows,  j);
  }

  return b3;
}


// ----------------------------------

// this should include some sort of check for nearly-singular
// matrices
GMatrix AAA::inv(GMatrix gm) {
  int i, j, k, n, uP;
  int rows = gm.getRows();
  int clms = gm.getClms();

  assert (rows==clms);
  assert (rows > 0);
  assert (clms > 0);
  assert (rows==clms);

  std::vector<int> *used = new std::vector<int>();
  double max_el;

  GMatrix merged = h_join(gm, identMatrix(rows));
  GMatrix invMat = GMatrix(rows, clms);

  used->resize(rows);
  for (i=0; i<rows; i++)
    (*used)[i] = 0; // mark all as unused

  // I have to pivot "rows"  times
  for(n=0; n<rows; n++) {
    k = -1; // no index selected yet
    max_el = 0.0; // largest ABS so far

    for (i=0; i<rows; i++) {
      uP = (*used)[i]; // 1 if used
      if ((0 == uP) && (dabs(merged.get(i,i)) > max_el)) {
	k = i;
	max_el = dabs(merged.get(i,i));
      }
    }

    if (true == GMatrix::showGMatrixPivotingP) {
      cout << "just prior to pivoting:"<<endl;
      cout << round(merged, 2)<<endl<<flush;
      cout << "k = " << k <<endl<<flush;
    }

    assert (k >= 0); // some element must have been selected
    (*used)[k] = 1; // mark it as used
    merged.pivot(k,k);  // destructively pivot on (k,k) element
  }

  for(i=0; i<rows; i++)
    for (j=0; j<rows; j++)
      invMat.set(merged.get(i,j+clms), i, j);

  delete used;
  used = NULL;

  return invMat;
};


GMatrix AAA::trans(GMatrix gm) {
  int r = gm.getRows();
  int c = gm.getClms();

  int i = 0;
  int j = 0;

  GMatrix tMat = GMatrix(c, r);

  for (i=0; i<r; i++)
    for (j=0; j<c; j++)
      tMat.set( gm.get(i,j), j, i);

  return tMat;
}

GVector AAA::vectorize(GMatrix gm) {
  int r = gm.getRows();
  int c = gm.getClms();
  int i = 0;
  assert (r > 0);
  assert (1 == c);

  GVector gv = GVector(r);
  for (i=0; i<r; i++)
    gv.set(  gm.get(i,0), i);

  return gv;
}

// only for 3 vectors
GVector AAA::cross(GVector a, GVector b) {
  assert (3 == a.getDim());
  assert (3 == b.getDim());

  GVector c = GVector(3);

  double ax, ay, az;
  double bx, by, bz;
  double cx, cy, cz;

  ax = a.get(0);
  ay = a.get(1);
  az = a.get(2);

  bx = b.get(0);
  by = b.get(1);
  bz = b.get(2);


  // to re-derive this, remember that
  // X x Y = Z, the right hand rule.
  // twist your right hand around to see that
  // Y x Z = X, Z x X = Y, W x V = - (V x W)
  // and remember that V x V = 0
  //
  // then multiply it out.

  cx = (ay * bz) - (az * by);
  cy = (az * bx) - (ax * bz);
  cz = (ax * by) - (ay * bx);

  c.set( cx , 0);
  c.set( cy , 1);
  c.set( cz , 2);

  return c;
}


// ----------------------------------


// target is at P, moving at V. we are at Q, and
// have a max speed of s. Find the intercept time t
// so that P+Vt = Q+Wt for some W, norm(W)=s
//
// notice that, given P,Q,V,t, we can recover the desired
// W by W = (P-Q)/t + V

// this returns -1 if no intercept is possible,
// 0 if the intercept is immediate,
// and a positive number if it is in the future
double
AAA::timeToIntercept(GVector P, GVector V, GVector Q, double s) {
  assert (s > 0.0);
  double t1 = 0.0;
  double t2 = 0.0;
  GVector PQ = P-Q;
  double c = PQ * PQ;
  double b = (V * PQ) * 2.0;
  double a = (V * V) - s*s;
  double disc = (b*b) - (4 * a * c);

  if (disc < 0)   // imaginary solution, no intercept possible
    return -1;

  disc = sqrt(disc);

  t1 = (-b + disc)/(2.0 * a);
  t2 = (-b - disc)/(2.0 * a);

  if ((t1 < 0) && (t2 < 0)) // no intercept possible
    return -1;
  if ((t1 < 0) && (t2 >=0))
    return t2;
  if ((t1>=0) && (t2 < 0))
    return t1;
  // t1>=0 and t2>=0, so two intercepts possible ?!
  // we can not tell which is smaller, because we do
  // not a priori know the sign of 'a'

  return dmin(t1, t2);
}


// One could also use the following iterative algorithm,
// which has the advantage of easily generalizing to
// ballistic trajectories. It just iteratively adjusts
// the aimpoint to account to flight time.
//
// Given P, s, Q, W, find t and V s.t. P + Vt = Q + Wt, |V| = s
//
// Q(0) = Q
// t(0) = | Q(0) - P | / s
// you could also set t(0) to the time to hit will a stationary
// target with a fixed-velocity ballistic weapon
//
// Q(i+1) = Q  +  W * t(i)
// t(i+1) = | Q(i+1) - P | / s
// (or time to hit with ballistic)
//
// continue until either t(i) clearly converges or diverges.


// TODO: get the compiler to recognize operator * for GMatrix

  // returns the LLS solution to b = Ax
GVector linearLeastSquares(GVector b, GMatrix A) {
      GMatrix At = trans(A);
      GVector x = inv(At*A)*(At*b);
      return x;
  }

  // returns the weighted LLS solution to b = Ax
GVector linearLeastSquaresWeighted(GVector b, GMatrix A, GMatrix Q) {
      GMatrix At = trans(A);
      GMatrix AtQ = At * Q;
      GVector x = inv(AtQ * A)*(AtQ * b);
      return x;
  }
// ----------------------------------


// considering (l1, l2) as defining an infinite line,
// compute and return that point on the line which
// is closest to p.
//
// it is NOT clipped to the segment,
// which would require 0 <= t <= 1
//
// this works for arbitrary dimensions

GVector AAA::nearestPoint (GVector p, GVector l1, GVector l2) {
  double t;
  GVector x = l1 - l2;
  GVector y =  p - l2;

  // now, if q = l2 + t*(l1-l2),  | p - q | = | y - (t * x) |
  // the latter, and hence the former, is minimized at the following t
  t = (x * y) / (x * x);
  return ((l1 * t) + (l2 * (1.0 - t)));


}


// set s and t to minimize (p+sv)-(q+wt)
//
// this works for arbitrarily high dimensions
// it is useful for calculating nearest-approach
void
AAA::nearIntersection(GVector p, GVector v, GVector q, GVector w, double& s, double& t) {

  double vv, vw, ww, pqw, pqv;
  double denom;

  GVector pq = p-q;
  pqv = pq * v;
  pqw = pq * w;

  vv = v*v;
  vw = v*w;
  ww = w*w;

  denom = det(vw, -vv, ww, -vw);
  s = det( vw, pqv,  ww, pqw) / denom;
  t = det(pqv, -vv, pqw, -vw) / denom;

  if (false) {
    cout <<"nearIntersection() of "<<endl;
    cout << p << " " << v << endl;
    cout << q << " " << w << endl;
    cout << " gave "<<endl;
    cout << "s = " << s <<endl;
    cout << "t = " << t <<endl;
    cout << "with nearest approaches at "<<endl;
    cout << p + (v * s) << endl;
    cout << q + (w * t) << endl;
    cout << flush;
  }
  return;
}


// sets the values of a and b to min (x-a)^2 + (y-b)^2  s.t.  a*b=0
// this is useful for various geometric transformations
// this is obviously closely related to the "perpendicularizer" function,
// but more convenient.
void
AAA::nearPerpendicular(GVector x, GVector y, GVector& a, GVector& b) {
  double tol = 1.0;
  const double thresh = 0.00001;
  double lambda = 0.0;
  int i = 0;
  GVector at = x;
  GVector bt = y;
  a = x;
  b = y;
  while (tol > thresh)  {
      tol = dabs(a*b)/sqrt(a*a + b*b);
      lambda = x*y/(a*a + b*b);
      at = x - b*lambda;
      bt = y - a*lambda;
      a = at;
      b = bt;
      i++;
    }
  return;
}

double AAA::triangleArea(GVector a, GVector b, GVector c) {

  // d is the point on the line (a,b)
  GVector d = nearestPoint(c, a, b);

  // h is the height of the triangle;
  double h = gvDistance(c,d);

  return (h * gvDistance(a,b) / 2.0);

}


// ----------------------------------
// considering (p1, p2) and (l1, l2) as defining two infinite
// lines, determine if the lines cross.
// if we are in more than 2D, they might be skew.
bool
AAA::lineIntersectP (GVector p1, GVector p2,GVector l1, GVector l2) {
  bool rslt = false;
  double s, t;

  double epsilon = 1e-10;  // round-off error
  double distRef = ( gvDistance(p1,p2) + gvDistance(l1,l2)) / 2.0;
  double distSep;

  rslt = false;

  nearIntersection(p1, p2-p1,
		   l1, l2 - l1,
		   s, t);
  GVector p3 = p1 + (p2-p1)*s;
  GVector l3 = l1 + (l2-l1)*t;
  distSep = gvDistance(p3,l3);
  if (distSep / distRef < epsilon)
    rslt = true;

  return rslt;
}


// same thing, but limited to intersection ON both segments
// (the segments might not cross even if the two infinite lines do cross).
bool
AAA::segIntersectP (GVector p1, GVector p2, GVector l1, GVector l2) {
  bool rslt = false;
  int inRangeP = 0;
  double s, t;
  GVector p3 = p1;
  GVector l3 = l1;
  double epsilon = 1e-3;  // round-off error
  double distRef = ( gvDistance(p1,p2) + gvDistance(l1,l2)) / 2.0;
  double distSep;

  unsigned int dim = p1.getDim();
  assert (dim == p2.getDim());
  assert (dim == l1.getDim());
  assert (dim == l2.getDim());

  rslt = false;

  nearIntersection(p1, p2-p1,
		   l1, l2 - l1,
		   s, t);
  inRangeP = ((0 - epsilon <= s) && (s <= 1+epsilon)
	      && (0-epsilon<= t) && (t <= 1+epsilon));

  if (1 == inRangeP) {
    // only compute these if it has not already proven rslt==false
    p3 = p1 + (p2-p1)*s;
    l3 = l1 + (l2-l1)*t;
    distSep = gvDistance(p3,l3);

    if (distSep / distRef < epsilon)
      rslt = true;
  }

  return rslt;
}

// ----------------------------------
// compute determinant of 2x2 Matrix
double det(double a11, double a12, double a21, double a22) {
  return ((a11*a22) - (a12*a21));
}

// ----------------------------------

double AAA::radAngleBetween(GVector gv1, GVector gv2) {
  return acos( gv1*gv2 / (norm(gv1) * norm(gv2) ) );
}

// --------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved
// --------------------------------------------
