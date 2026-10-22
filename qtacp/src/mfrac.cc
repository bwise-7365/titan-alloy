// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: banner, the legacy RNG
// type becomes panj::PRNG.
// ------------------------------------------

#include "mfrac.h"


// ------------------------------------------
// this is the classic square-diamond algorithm for
// generating fractal terrain, slightly generalized
// to work on arbitrary rectangular arrays



// this works on one single patch
//z [ j + (clms * i) ] = value at (i,j)

// A is top left corner,
// B is top right,
// D is bottom right corner,
// C is bottom left
void makeFractalPatch(double* z, unsigned long int rows, unsigned long int clms,
		      double dx, double dy,
		      unsigned long int iA,	unsigned long int iD,
		      unsigned long int jA,	unsigned long int jD,
		      double f, panj::PRNG* rng) {
  assert (iD >= iA);
  assert (jD >= jA);

  assert ((iD > iA) || (jD > jA));

//   cout << "iA: " << iA << endl;
//   cout << "iD: " << iD << endl;
//   cout << "rows: " << rows << endl;

//   cout << "jA: " << jA << endl;
//   cout << "jD: " << jD << endl;
//   cout << "clms: " << clms << endl;
//   cout << endl<<flush;

  assert (iD < rows);
  assert (jD < clms);

  if ((iD == (1 + iA)) && (jD == (1 + jA)))
    return;

  float zTemp = 0.0;
  float hriz = (jD - jA)*dx;
  float vert = (iD - iA)*dy;
  float diag = sqrt( (hriz*hriz) + (vert*vert) );


  unsigned long int iB = iA;
  unsigned long int jB = jD;

  unsigned long int iC = iD;
  unsigned long int jC = jA;


  // ------------------
  // SQUARE step: average the values from the square
  // formed by generation 0 points, giving generation 1 points

  // E is the center point
  unsigned long int iE = (iA + iD) / 2;
  unsigned long int jE = (jA + jD) / 2;

  // get mean high of the 4 points of the square
  zTemp = (z[jA+(clms*iA)] + z[jA+(clms*iD)] +
	   z[jD+(clms*iA)] + z[jD+(clms*iD)])/4.0;

  // add a random amount, proportional to the square's diagonal length
  zTemp = zTemp + (f * rng->uniform(-1, +1) * diag/2.0);
  z[jE+(clms*iE)] = zTemp;

  // ------------------
  // DIAMOND step: average values from smaller diamonds
  // formed by generation 0 and 1 points,
  // giving generation 2 points

  // left square-point, F
  unsigned long int iF = iE;
  unsigned long int jF = (jE + ((jA + clms) - (1+jD))) % clms;

  // right square-point, G
  unsigned long int iG = iE;
  unsigned long int jG = (jE + ((1+jD) - jA)) % clms;

  // top square-point, H
  unsigned long int iH = (iE + ((iA + rows) - (1+iD))) % rows;
  unsigned long int jH = jE;

  // bottom square-point, I
  unsigned long int iI = (iE + ((1+iD) - iA)) % rows;
  unsigned long int jI = jE;

//   cout << "A: " << iA << " " << jA << endl;
//   cout << "B: " << iB << " " << jB << endl;
//   cout << "C: " << iC << " " << jC << endl;
//   cout << "D: " << iD << " " << jD << endl;
//   cout << "E: " << iE << " " << jE << endl;
//   cout << "F: " << iF << " " << jF << endl;
//   cout << "G: " << iG << " " << jG << endl;
//   cout << "H: " << iH << " " << jH << endl;
//   cout << "I: " << iI << " " << jI << endl;



  // the new diamond points are supposed to
  // lie in the middle of the 4 edges of
  // the original box


  // left diamond point, alpha
  unsigned long int jAlpha = jA;
  unsigned long int iAlpha = iE;

  // average heights of F, A, E, C
  //z [ j + (clms * i) ] = value at (i,j)
  zTemp = (z[jF+(clms*iF)] +z[jA+(clms*iA)] +z[jE+(clms*iE)] +z[jC+(clms*iC)])/4.0;

  // add a random amount, proportional to the diamond's diagonal length
  zTemp = zTemp + (f * rng->uniform(-1, +1) * diag/4.0);
  z[jAlpha+(clms*iAlpha)] = zTemp;


  // top diamond point, beta
  unsigned long int jBeta = jE;
  unsigned long int iBeta = iA;

  // average heights of A, H, B, E
  zTemp = (z[jA+(clms*iA)] +z[jH+(clms*iH)] +z[jB+(clms*iB)] +z[jE+(clms*iE)])/4.0;
  zTemp = zTemp + (f * rng->uniform(-1, +1) * diag/4.0);
  z[jBeta+(clms*iBeta)] = zTemp;


  // right diamond point, gamma
  unsigned long int jGamma = jD;
  unsigned long int iGamma = iE;

  // average heights of B, E, G, D
  zTemp = (z[jB+(clms*iB)] +z[jE+(clms*iE)] +z[jG+(clms*iG)] +z[jD+(clms*iD)])/4.0;
  zTemp = zTemp + (f * rng->uniform(-1, +1) * diag/4.0);
  z[jGamma+(clms*iGamma)] = zTemp;

  // bottom diamond point, delta
  unsigned long int jDelta = jE;
  unsigned long int iDelta = iD;

  // average heights of E, C, D, I
  zTemp = (z[jE+(clms*iE)] +z[jC+(clms*iC)] +z[jD+(clms*iD)] +z[jI+(clms*iI)])/4.0;
  zTemp = zTemp + (f * rng->uniform(-1, +1) * diag/4.0);
  z[jDelta+(clms*iDelta)] = zTemp;

  return;
}

// ----------------------------------


void showMatrix(double *H, unsigned long int rows, unsigned long int clms) {
  unsigned long int i = 0;
  unsigned long int j = 0;
  unsigned long int k = 0;
  for (i=0; i<rows; i++) {
    for (j=0; j<clms; j++) {
      k = j + (i*clms);
      printf(" %6.2f", H[k]);
    }
    cout << endl;
  }
}

unsigned long int smallestGEPower(unsigned long int x) {
  unsigned long int p = 0;
  while (power(2,p) < x)
    p++;

  return p;
}

unsigned long int power(unsigned long int b, unsigned long int p) {
  unsigned long int rslt = 1;
  if (0 == p)
    rslt = 1;
  else if (1 == p)
    rslt = b;
  else if (0 == b)
    rslt = 0;
  else if (1 == b)
    rslt = 1;
  else {
    const unsigned long int q = p/2;
    const unsigned long int r = p % 2;
    rslt = power (b, q);
    rslt = rslt * rslt;
    if (1 == r)
      rslt = b * rslt;
  }

  return rslt;
}


// ----------------------------------
// this is sensitive to the rows and clms. see comment
// at top of this file
double*  makeFractalTerrain(unsigned long int rows, unsigned long int clms,
			    double dx, double dy, double h0,
			    double f, panj::PRNG* rng) {

  unsigned long int i = 0;
  unsigned long int j = 0;
  unsigned long int k = 0;
  double *H; // a dynamic array of data, each of type double

  H = new double[ rows * clms ];

  //  double h0 = 50.0;


  for (i=0; i<rows; i++) {
    for (j=0; j<clms; j++) {
      k = j + (i*clms);
      H[k] = 0.0;
    }
  }

  k =     0    + ((rows-1)*clms);
  H[k] = h0;
  k = (clms-1) + ((rows-1)*clms);
  H[k] = h0;
  k =     0    + (   0    *clms);
  H[k] = h0;
  k = (clms-1) + (   0    *clms);
  H[k] = h0;


  //  showMatrix(H, rows, clms);

  unsigned long int height = rows - 1;
  unsigned long int width = clms - 1;
  unsigned long int iter = 0;

  unsigned long int nR = 0;
  unsigned long int mC = 0;

  while ((height > 1) || (width > 1)) {
    iter++;
    nR = (rows - 1) / height;
    mC = (clms - 1) / width;

    for (i=0; i < nR; i++) {
      for (j=0; j < mC; j++) {
	makeFractalPatch(H, rows, clms, dx, dy,
			 i*height, (i+1)*height,
			 j*width, (j+1)*width,
			 0.25, rng);
      }
    }

//     cout << endl << "Iter: " << iter<<endl;
//     showMatrix(H, rows, clms);
    height = height/2;
    width = width/2;
  }

  return H;
}

// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
