// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: banner, the legacy RNG
// include/type becomes abzar.h / panj::PRNG.
// ------------------------------------------

#ifndef LANCH_H
#define LANCH_H

#include <stdio.h>
#include <math.h>
#include <assert.h>

// ------------------------------------------

#include "aaa.h"
#include "abzar.h"
// #include "fsm.h"

// #include "big.h"
// #include "big2.h"

//#include <stl.h>
//#include <stl_list.h>
// #include <list>

// #include "mat.h"

// #include "ml.h"

// ------------------------------------------

// this performs a quick little numerical simulation,
// to step through dt of battle in K steps
//
void simCBattle (const double b0, const double r0,
		 const double alpha, const double beta,
		 const double dt, const int K,
		 double& bf, double& rf);

// this performs a quick little discrete simulation,
// stepping through the battle to conclusion.
// useful for mini-monte-carlo
//
void simDBattle (const unsigned long int b0, const unsigned long int r0,
		 panj::PRNG* rng, const double alpha, const double beta,
		 unsigned long int& bf, unsigned long int& rf);

// this uses classic Lanchester square law
// to estimate a final state.
void finalState (const double b0, const double r0,
		 const double alpha, const double beta,
		 double& bf, double& rf);

double probBwin(const double b0, const double r0,
		const double alpha, const double beta);

double genQ(panj::PRNG* rng);

double medianLargerQ(const double a);

double medianSmallerQ(const double a);

double blueUtil(double b0, double r0,
		double alpha, double beta,
		double ua, double ub);

// ------------------------------------------

#endif

// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
