//----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
//----------------------------------------------
// Vendored from pershing/aaa for the qtacp port.
// Patches relative to the original: evector<double> is replaced
// by std::vector<double>, and the RNG-based test entry point is
// removed (randomness now comes from panj::PRNG).
//----------------------------------------------

// --------------------------------------------
// These are geometric vectors and matrices
//
// The GVector class tries to be efficient in
// allocating and deallocating memory, by using
// a double* to store data rather than a vector<double>.
// Serious use of the GMatrix class would necessitate
// the same treatment, plus more optimizations.
// --------------------------------------------

#ifndef MAT_HEADER
#define MAT_HEADER

// --------------------------------------------

using namespace std;

#include "aaa.h"
#include <vector>
#include <string>

// ----------------------------------
double det(double a11, double a12, double a21, double a22);


// ----------------------------------

// this is a column vector
class AAA::GVector {

public:
  GVector(); // this is needed for things like an array of GVector
  GVector(unsigned int d); // make a vector of arbitrary dimensions
  GVector(double x, double y, double z); // make a 3-vector

  GVector(const GVector& gv); // copy constructor (heavily used)
  GVector& operator = (const GVector& gv); // copy assignment

  ~GVector();

  double get(const unsigned int &d); // (heavily used)
  unsigned int getDim();
  void set(double x, unsigned int d);
  void scale_to(double x);

  friend ostream& AAA::operator << (ostream& s, GVector gv);
  string toString();

protected:
  unsigned int dim;
  double* coords;

private:

};

// ----------------------------------

AAA::GMatrix operator * (AAA::GMatrix gv1, AAA::GMatrix gv2);
AAA::GVector operator * (AAA::GMatrix gm, AAA::GVector gv1);
AAA::GMatrix operator * (AAA::GMatrix gv1, double c);
AAA::GMatrix operator / (AAA::GMatrix gv1, double c);

AAA::GMatrix operator + (AAA::GMatrix gv1, AAA::GMatrix gv2);
AAA::GMatrix operator - (AAA::GMatrix gv1, AAA::GMatrix gv2);

// this is your basic matrix
class AAA::GMatrix {

public:
  GMatrix();
  GMatrix(unsigned int r, unsigned int c);
  ~GMatrix();
  GMatrix(const GMatrix& gm); // uninvoked?

  double get(unsigned int r, unsigned int c);
  unsigned int getRows();
  unsigned int getClms();
  void set(double x, unsigned int r, unsigned int c);

  GMatrix& operator = (const GMatrix& gm);
  friend ostream& AAA::operator << (ostream& s, GMatrix gm);
  string toString();

  // destructive pivoting
  void pivot(unsigned int r, unsigned int c);

  // make and return an inverse matrix
  GMatrix inv();
  // report pivots while inverting?
  static const bool showGMatrixPivotingP = false;

  GMatrix trans();

  void show(const char * fS); // formatted print to cout

protected:

  unsigned int rows;
  unsigned int clms;
  std::vector<double> *vals;

private:
  int indexFromRC(const int r, const int c);
  int rowFromIndex(const int i);
  int clmFromIndex(const int i);
};

// --------------------------------------------
#endif

//----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
//----------------------------------------------
