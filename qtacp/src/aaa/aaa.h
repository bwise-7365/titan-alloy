//-------------------------------------------------
//  Copyright Ben Paul Wise. All Rights Reserved.
//-------------------------------------------------
// Vendored from pershing/aaa for the qtacp port.
// Patches relative to the original: the RNG class family and the
// RNG-parameterized test generators are removed (randomness now comes
// from panj::PRNG, owned by the DESim engine); evector is removed
// (call sites use std::vector).
//-------------------------------------------------

#ifndef AAA_H
#define AAA_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <assert.h>
#include <string.h>


// ------------------------------------------

using namespace std;

typedef unsigned char octet;

// ------------------------------------------
// make this defined to shut off all assert-checking,
// and boost performance and risk accordingly

//#define NDEBUG

// ------------------------------------------
// Xt does '#define Boolean int', which ruins
// any reasonable name for the following.
// worse, it means I can't use 'Boolean' without
// loading Xt  - confining my code to X/*nix !
//
// Therefore, I must use 'Logical' everywhere I
// can, and Logical only for those things which
// X/Xt/Xaw requires to be Boolean

namespace AAA {

  // ----------------------------------
  // Low-level interfaces to OS-specific functions
  unsigned long int aaaTime();
  unsigned long int aaaPID();

  // ----------------------------------
  // Various enumerations
  enum Logical {LFalse, LTrue, LUnknown};

  // ----------------------------------
  // Finite State Machines
  class FSM;
  class Predicate;
  class Action;
  class State;
  class AlwaysTrue;
  class Negation;
  class Conjunction;
  class Disjunction;
  class AllActions;

  // ----------------------------------------
  // Magic constants from RC5/RC6 key setup

  // PW = odd ((  e  - 2 ) * (2^^w)),  e  = 2.718282...
  // note that P32 = 3084996963, in decimal
  const unsigned int P32 = 0xB7E15163;

  // QW = odd (( phi - 1 ) * (2^^w)), phi = 1.618034...
  // note that Q32 = 2654435769, in decimal
  const unsigned int Q32 = 0x9E3779B9;


  // ----------------------------------------

  // these are bit-manipulation functions derived from
  // the macros for 32-bit words
  //
  // why are these not of type 'unsigned long int' ??
  //
  unsigned int rotl(unsigned int x, unsigned int y);
  unsigned int rotr(unsigned int x, unsigned int y);

  // and these are the same, but for 8-bit
  unsigned char rotl8(unsigned char x, unsigned char y);
  unsigned char rotr8(unsigned char x, unsigned char y);

  //#define ROTL(x,y) (((x)<<(y&(w-1))) | ((x)>>(w-(y&(w-1)))))
  //#define ROTR(x,y) (((x)>>(y&(w-1))) | ((x)<<(w-(y&(w-1)))))
  // where 'w' is the word size in bits, e.g. 32

  // ----------------------------------------
  // this is a cute 1-to-1 mapping from Rivest's RC6.
  // As it is 1-to-1, it preserves entropy. But it may have
  // short cycles: it is quite possible to have qT( qT (a )) = a.
  // qT(0) = 0, for example.
  //  Sidebar:  qT(x) = x
  //          x(2x+1) = x + nM
  //          2xx + x = x + nM
  //              2xx = nM
  // so if M = 2^32, then x = a * 2^(16+k) works, with n = (a^2) * (2^(2k+1))
  // Changing any single input bit has prob 0.0 of flipping any lower output bit,
  // prob 1.0 of flipping the same output bit, and approx. prob 0.5 of flipping
  // each higher output bit.
  // it works correctly for arbitrary word length, not just
  // 32-bit words
  //
  // see stir.cc and stir.txt for the discussion of why
  // quadTransform(x + P32) is better than quadTransform(x)
  unsigned long int quadTransform(unsigned long int s0);
  //
  // you could actually replace qT(s) = s(2s+1) by qT(s) = (s+P)(ns+c)
  // for any even n and odd c, as discussed in stir.txt



  // ----------------------------------
  // simple geometric vectors, and associated matrices
  class GVector;
  class GMatrix;
  double norm(GVector gv);
  double gvDistance(GVector gv1, GVector gv2);
  GVector round(GVector gv, int n);

  // these little matrix access functions are common enough to standardize
  unsigned int indexFromRowClm(const unsigned int r,
			       const unsigned int c,
			       const unsigned int rows,
			       const unsigned int clms);
  unsigned int rFromIndex(const unsigned int i,
			  const unsigned int rows,
			  const unsigned int clms);

  unsigned int cFromIndex(const unsigned int i,
			  const unsigned int rows,
			  const unsigned int clms);

  GMatrix round(GMatrix gv, int d);

  GMatrix identMatrix(int n);
  GMatrix h_join(GMatrix left, GMatrix right);
  GMatrix v_join(GMatrix top, GMatrix bottom);
  GMatrix inv(GMatrix gm);
  GMatrix trans(GMatrix gm);

  // for converting a Nx1 matrix to a column vector,
  // without getting wrapped up in confusing copy constructors
  GVector vectorize(GMatrix gm);


  // only for 3 vectors
  GVector cross(GVector a, GVector b);

  double radAngleBetween(GVector gv1, GVector gv2);

  // returns -1 if no intercept possible
  double timeToIntercept(GVector P, GVector V, GVector Q, double s);

  // returns the LLS solution to b = Ax, to minimize
  GVector linearLeastSquares(GVector b, GMatrix A);

  // returns the weifhted LLS solution to b = Ax
  GVector linearLeastSquaresWeighted(GVector b, GMatrix A, GMatrix Q);

  // considering (l1, l2) as defining an infinite line,
  // compute and return that point on the line which
  // is closest to p.
  GVector nearestPoint (GVector p, GVector l1, GVector l2);


  // set s and t to minimize (p+sv)-(q+wt)
  void
  nearIntersection(GVector p, GVector v, GVector q, GVector w, double& s, double& t);


  // sets the values of a and b to min (x-a)^2 + (y-b)^2  s.t.  a*b=0
  void
  nearPerpendicular(GVector x, GVector y, GVector& a, GVector& b);

  double triangleArea(GVector a,GVector b, GVector c);



  // considering (p1, p2) and (l1, l2) as defining two infinite
  // lines, determine if the lines cross.
  // if we are in more than 2D, they might be skew.
  bool
  lineIntersectP (GVector p1, GVector p2,GVector l1, GVector l2);

  // same thing, but limited to intersection ON both segments
  // (the segments might not cross even if the two infinite lines do cross).
  bool
  segIntersectP (GVector p1, GVector p2,GVector l1, GVector l2);



  double operator * (GVector gv1, GVector gv2);
  GVector operator * (GVector gv1, double c);
  GVector operator / (GVector gv1, double c);
  GVector operator + (GVector gv1, GVector gv2);
  GVector operator - (GVector gv1, GVector gv2);
  ostream& operator << (ostream& s, GVector gv);

  ostream& operator << (ostream& s, GMatrix gm);
  // ----------------------------------
  // a basic set of errors
  class AAAException {};
  class Overflow : public AAAException  { }; // putting too much in
  class Underflow : public AAAException { }; // taking too much out
  class OutOfRange : public AAAException { }; // looking/changing/resizing
  class OutOfMemory : public AAAException { }; // if new returned NULL
  class DoubleDelete : public AAAException { }; // strict memory debugging
  class DimensionalError  : public AAAException { }; // matrix operations
  class InvariantViolation : public AAAException { }; // generic checking for invariants

  ostream& operator << (ostream& s, AAA::Logical r);

  // ------------------------------------------
  // a more informative 'assert'

#define ASSERT(x, msg) \
if ((x) == 0) { cerr<<__FILE__<<":"<<__LINE__<<"   " <<msg<<endl;   exit(0); }

  // ------------------------------------------
  // some very common math operations
  double dabs (double x);
  double dmax (double a, double b);
  double dmin (double a, double b);

  int iabs (int x);
  int imax(int a, int b);
  int imin(int a, int b);

  // return x ^ n
  double expt (double x, int n);

  // return x ^ e
  double dexpt (double x, double e);

  int oddP(int x);
  int evenP(int x);

  double delta(double a, double b); // symettric % difference
  int iround(double x);
  double dround (double x, int n);
  int gcd_iter(int a, int b);
  int gcd_orig(int a, int b);
  unsigned long int gcd(unsigned long int a, unsigned long int b);

  //  the /usr/include/math.h covers floor but not ceiling.
  // and /usr/include/math.h/floor returns a float, not an int
  int ifloor (double x);
  int iceiling (double x);

  // ------------------------------------------
    //Given a NULL-terminated string, allocates sufficient memory for it
    //and makes a NULL-terminated copy.
    //Incredibly, MinGW does not appear to supply the strdup function??
    //So I made a portable version here.

  char * stringDuplicate(const char * s);

  float sqr(float x);

  // trivial RNG, useful ONLY for testing
  // when you really do not care about
  // the quality of the numbers
  unsigned long int trng(unsigned long int seed);

}  // end of AAA namespace


// ------------------------------------------
#endif


//-------------------------------------------------
//  Copyright Ben Paul Wise. All Rights Reserved.
//-------------------------------------------------
