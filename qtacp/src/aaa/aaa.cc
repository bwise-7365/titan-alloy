//-------------------------------------------------
//  Copyright Ben Paul Wise. All Rights Reserved.
//-------------------------------------------------
// Vendored from pershing/aaa for the qtacp port; unchanged
// apart from this note.
//-------------------------------------------------

#include "aaa.h"

using AAA::Logical;
using AAA::LTrue;
using AAA::LFalse;
using AAA::LUnknown;

// ----------------------------------

ostream& AAA::operator << (ostream& s, Logical r)  {
  switch (r)
    {
    case LTrue:
      s << "True";
      break;
    case LFalse:
      s << "False";
      break;
    case LUnknown:
      s << "Unknown";
      break;
    };
  return s;
};

// ---------------------------------
unsigned int  AAA::indexFromRowClm(const unsigned int r,
				   const unsigned int c,
				   const unsigned int rows,
				   const unsigned int clms) {
  assert (r < rows);
  assert (c < clms);
  unsigned int i = (r * clms) + c;
  return i;
}

unsigned int  AAA::rFromIndex(const unsigned int i,
			      const unsigned int rows,
			      const unsigned int clms) {
  assert (i < (rows * clms));
  unsigned int r = i / clms;
  return r;
}

unsigned int  AAA::cFromIndex(const unsigned int i,
			      const unsigned int rows,
			      const unsigned int clms) {
  assert (i < (rows * clms));
  unsigned int c = i % clms;
  return c;
}

double AAA::dabs(double x) {
  if (x<0)
    return -x;
  else
    return x;
}

int AAA::iabs (int x) {
  if (x<0)
    return -x;
  else
    return x;
}

int AAA::imax (int a, int b) {
  int result  = a;
  if (b > a)
    result = b;
  return result;
}

int AAA::imin (int a, int b) {
  int result  = a;
  if (b < a)
    result = b;
  return result;
}

double AAA::dmax (double a, double b) {
  double result = a;
  if (b > a)
    result = b;
  return result;
}

double AAA::dmin (double a, double b) {
  double result = a;
  if (b < a)
    result = b;
  return result;
}

// ------------------------------------------

double AAA::expt(double x, int n) {
  double y = 1.0;
  assert (n >= 0);
  while (n>0)
    {
      y = y*x;
      n = n-1;
    }
  return y;
}

double AAA::dexpt(double x, double e) {
  double z = log(x);
  return exp(e*z);
}

int AAA::iround(double x) {
  return ((int) dround(x,0));
}

double AAA::dround(double x, int n) {
  assert (n >= 0);
  double y;
  double rslt;
  bool negativeP = false;
  if (x < 0.0) {
    negativeP = true;
    x = - x;
  }
  int xInteger = ((int) x);
  double xFraction = x - xInteger;

  if (n==0)
    y = 1.0;
  else
    y = expt(10.0, n);

  xFraction = ((int)(0.5+ y*xFraction ))/y;
  rslt = xInteger + xFraction;

  if (negativeP) {
    rslt = - rslt;
  }

  return rslt;
}


int AAA::ifloor(double x)  {
  return ((int) floor(x));
}

int AAA::oddP(int x) {
  return ( ( x % 2) == 1);
}

int AAA::evenP(int x) {
  return ( ( x % 2) == 0);
}

double AAA::delta(double a, double b) {
  return (200.0 * dabs(a-b)) / (dabs(a) + dabs(b));
}

int AAA::iceiling(double x) {
  int j = 0;
  int i = (int)x;

  if (x>=0)   // (int)(3.14) == 3, c(3.14) == 4
    {
      if (i<x)
 	j = i+1;
      else
 	j = i;
    }

  if (x<0)   // (int)(-3.14) == -3, c(-3.14) == -3
    j = i;

  return  j;
}


// iterative implementation of Euclid's algorithm
int AAA::gcd_iter(int a, int b)  {
  int c, e;
  int done = 0;
  e = a;
  while (0 == done)
    {
      if (a > b)
	{
	  c = a;
	  a = b;
	  b = c;
	}
      assert (a <= b);
      if ((a == b) || (a == 0) || (a == 1))
	{
	  e = a;
	  done = 1;
	}
      else
	b = b - a;
    }
  return e;
}

// this occaisonally crashes with extremely deep recursion,
// so I had to replace it with the iterative version above.
// even better: replace repeated subtraction with computation
// of remainder!

int AAA::gcd_orig(int a, int b)
  {
    int c, e;
    assert (a>=0);
    assert (b>=0);
    if (a>b)
      e =  gcd(b,a);
    else  if ((a==b) || (0 == a) || (1 == a))
      e = a;
    else {
      // a< b
      c = b-a;

      if (c<a)
        e = gcd(c,a);
      else
        e = gcd(a,c);
    }
    return e;
  }

unsigned long int AAA::gcd(unsigned long int a, unsigned long int b) {
  unsigned long int e;
  unsigned long int d, n;
  if (a>b)
    e =  gcd(b,a);
  else  if ((a==b) || (0 == a) || (1 == a))
    e = a;
  else {   // now a< b
    n = b / a;
    d = b - (n * a);
    // now b = n * a + d, 0 <= d < a
    if (0 == d)
      e = a;
    else
      e = gcd (a, d);
  }
  return e;
}

// ----------------------------------

char * AAA::stringDuplicate(const char * s) {
  assert (NULL != s);

  unsigned int slength = strlen(s);
  unsigned int length = slength + 1;
  char* rslt = new char[length];

  for (unsigned int i = 0; i < slength; i++) {
    rslt[i] = s[i];
  }

  for (unsigned int i = slength; i < length; i++) {
    rslt[i] = 0;
  }

  return rslt;
}

float AAA::sqr(float x) {
  return (x * x );
}

unsigned long int AAA::trng(unsigned long int seed) {
  const unsigned long int P = 0x859ad865;
  seed = seed + P;
  return (seed) * ((2*seed) + 1);  // mod 2^^32, of course
}

//-------------------------------------------------
//  Copyright Ben Paul Wise. All Rights Reserved.
//-------------------------------------------------
