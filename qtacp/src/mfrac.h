// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: banner, the legacy RNG
// and ml includes go away, the RNG type becomes panj::PRNG.
// ------------------------------------------

//#include <iostream>

#include "math.h"
#include "aaa.h"
#include "abzar.h"

void makeFractalPatch(double* z, unsigned long int rows, unsigned long int clms,
		      double dx, double dy,
		      unsigned long int iA, unsigned long int iD, // min and max rows
		      unsigned long int jA, unsigned long int jD, // min and max clms
		      double f, panj::PRNG* rng);


//this makes and returns a fractal terrain of the given dimensions
// they must be within about a factor of 2 of each other, or else it
// will crash.
//
// ideally, rows = clms = 1 + power(2,N), for some integer N
//
// you can then downsample / average to your desired dimensions
double* makeFractalTerrain(unsigned long int rows, unsigned long int clms,
			   double dx, double dy, double h0,
			   double f, panj::PRNG* rng);

void showMatrix(double *H, unsigned long int rows, unsigned long int clms);

// integer power function
unsigned long int power(unsigned long int b, unsigned long int p);

// returns the smallest n s.t. x <= power(2,n)
unsigned long int smallestGEPower(unsigned long int x);



// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
