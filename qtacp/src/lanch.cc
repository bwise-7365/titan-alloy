// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: banner, the legacy RNG
// type becomes panj::PRNG.
// ------------------------------------------

// suppose (a,B) fights (b,R) according to
// classic Lanchester square law:
// dR/dt = - a * B, and dB/dt = - b * R
// then (a * B^2) - (b * R^2) is a constant,
// and the winner is whoever starts with a larger
// value of c * X^2.
//
// but if they are exactly equal, they fight down
// exactly (0,0) every time, which is unphysical.
// In the case of a*B^2 == b*R^2, you'd like 50/50 odds.
//
// Suppose P[B defeats R] = a*B^2 / (a*B^2 + b*R^2),
// because dR/dt = - (a * q) * B, and dB/dt = - (b / q) * R,
// where q is a random variable.
//
// a little algebra shows that B defeats R in exactly the case that
// (a * q) * B^2 > (b / q) * R^2,
// or q^2 >= b*R^2)/(a*B^2) =  a*B^2 / (a*B^2 + b*R^2) = x^2 = P[B defeats R]
//
// Combining that with the probability statement, we get
// Prob (q^2 > x^2) =  1  / (1 + x^2),
// Prob (q   >  x ) =  1  / (1 + x^2),
// Prob (q   <= x ) = x^2 / (1 + x^2),
//
// more algebra shows that, if m1 = median ( q | q <= a),
// m1 = a / sqrt( 2 + a^2). for large a, this is slightly under 1.0
//
// more algebra shows that, if m2 = median ( q | q >= a),
// m2 = sqrt( 1 + 2 * a^2). for tiny a, this is slightly over 1.0
//
//
// finally, we know that battles are not always fought until
// there are literally zero survivors on one side.
// assuming true lanchester square law, with q, we have
// (a*q)*(b0*b0 - bt*bt) = (b/q)*(r0*r0 - rt*rt) at all times,
// and eventual Blue win iff (a*q)*(b0*b0) > (b/q)*(r0*r0).
// Dividing the first = by the second >, we cancel a, b, and q to get
// (b0*b0 - bt*bt)/(b0*b0) < (r0*r0 - rt*rt)/(r0*r0)
// so as soon as things have gone along long enough to make the
// situation stand out from the random variations, we can guess what
// the outcome will be, and take appropriate action
// (e.g. press forward, pull back, call reinforcements, call follow-on-forces)

// ------------------------------------------

#include "lanch.h"

// ------------------------------------------

void simCBattle (const double b0, const double r0,
		 const double alpha, const double beta,
		 const double dt, const int K,
		 double& bf, double& rf) {

  double db = b0;
  double dr = r0;

  bf = b0;
  rf = r0;

  int i = 0;
  while ( ( db > 0.0) && ( dr > 0.0) && (bf > 0.0) && (rf > 0.0)) {

    db = ((int)(0.5 + (alpha * dt * rf * K))) / ((double) K);
    dr = ((int)(0.5 + ( beta * dt * bf * K))) / ((double) K);

    bf = bf - db;
    rf = rf - dr;

    if (bf < 0.0)
      bf = 0.0;

    if (rf < 0.0)
      rf = 0.0;

    i = i + 1;
  }

  return;
}

void simDBattle (const unsigned long int b0, const unsigned long int r0,
		 panj::PRNG* rng, const double alpha, const double beta,
		 unsigned long int& bf, unsigned long int& rf) {
  bf = b0;
  rf = r0;
  double sb = 0.0;
  double sr = 0.0;
  double pb = 0.0;
  double p = 0.0;

  while ((bf > 0) && (rf > 0)) {
    sb = alpha*bf;
    sr = beta*rf;
    pb = sb/(sb+sr);
    p = rng->uniform(0.0, 1.0);

    if (p < pb) {
      // blue hits first
      rf--;
    }
    else {
      // red hits first
      bf--;
    }
  }

  return;
}

void finalState (const double b0, const double r0,
		 const double alpha, const double beta,
		  double& bf, double& rf) {
  const double b02 = b0 * b0;
  const double sB = alpha * b02;
  const double r02 = r0 * r0;
  const double sR =  beta * r02;

  if (sB > sR) {
    bf = sqrt(b02 - ((beta * r02) / alpha));
    rf = 0.0;
  }

  else if (sR > sB) {
    rf = sqrt(r02 - ((alpha * b02) / beta));
    bf = 0.0;
  }

  else {
    // exactly evenly matched
    bf = 0.0;
    rf = 0.0;
  }

  return;
}


double probBwin(const double b0, const double r0, const double alpha, const double beta) {
  const double sB = alpha * b0 * b0;
  const double sR =  beta * r0 * r0;

  return sB / ( sB + sR );
}

double genQ(panj::PRNG* rng) {
  const double p = rng->uniform(0.0001, 0.9999);
  const double q = sqrt(p / (1.0 - p));
  return q;
}

double medianLargerQ(const double a) {
  const double q = sqrt( 1.0 + 2.0 * (a * a));
  return q;
}


double medianSmallerQ(const double a) {
  const double q = a / sqrt(2.0 + (a * a));
  return q;
}



// ------------------------------------------
// if alpha = beta, and ua = ub,
// then numerical search shows that
// blueUtil is maximized by b0/r0 around 2.7047
//

double blueUtil(double b0, double r0,
		double alpha, double beta,
		double ua, double ub) {
  const double sb = alpha*b0*b0;
  const double sr = beta*r0*r0;

  // case that Blue wins
  const double pb = sb / (sb + sr);
  const double q2B = 1.0 + (2*sr)/sb;
  const double bfB = sqrt(b0*b0 - (beta*r0*r0)/(alpha*q2B));

  //  const double rfB = 0.0;


  // case that Red wins
  const double pr = sr / (sr + sb);
  const double q2R = 1.0 + (2*sb)/sr;
  const double rfR = sqrt(r0*r0 - (alpha*b0*b0)/(beta*q2R));

  //  const double bfR = 0.0;


  // expected values
  const double bf = pb * bfB; // same as pb*bfB + pr*bfR
  const double rf = pr * rfR; // same as pr*rfR + pb*rfB
  const double db = b0 - bf;
  const double dr = r0 - rf;

  const double ut = (ua * dr) - (ub * db);

  return ut;
}

// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
