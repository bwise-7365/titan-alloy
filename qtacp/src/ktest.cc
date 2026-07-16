// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
//
// this show how, given any integer k, and
// n s.t. sqr(n+1) > k >= sqr(n),
// to get a and b s.t. k = a*(n+1) + b*n
//
// this is useful (but not ideal) for laying
// down k units in a hex-packed array
//
// ----------------------------------
#include "math.h"
#include "aaa.h"
#include "des.h"

int
main () {
  cout << "starting" << endl;

  unsigned long int k = 0;
  unsigned long int n = 0; 
  unsigned long int m = 0; 
  unsigned long int a = 0; 
  unsigned long int b = 0;
  unsigned long int c = 0;

  for (k=0; k<50000; k++) {
    n = ((int) sqrt((double) k));
    m = k - (n*n);
    if (m <= n) {
      c = k - (n*n);
      a = c;
      b = n-c;
    }
    else {
      c = k - (n + (n*n));
      a = c;
      b = n+1 - c;
      }
    assert (k == a*(n+1) + b*n);
  }

  cout << "finished" << endl;
  return 0;
}


// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------

