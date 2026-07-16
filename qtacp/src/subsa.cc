// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: banner, evector becomes
// std::vector (inListP becomes std::find).
// ------------------------------------------

#include "aaa.h"
#include "assert.h"
#include "subsa.h"

#include <vector>
#include <algorithm>

using std::vector;

// ----------------------------------

SubSA::SubSA(Unit* u) {
  assert (NULL != u);
  unit = u;

  friendly = new vector<ResUnit*>();
  enemy = new vector<ResUnit*>();
}



SubSA::~SubSA() {
  unit = NULL;

  delete friendly;
  friendly = NULL;

  delete enemy;
  enemy = NULL;
}


void SubSA::clearFriendly() {
  delete friendly;
  friendly = new vector<ResUnit*>();
  return;
}


void SubSA::clearEnemy() {
  delete enemy;
  enemy = new vector<ResUnit*>();
  return;
}


void SubSA::mergeFriendly(ResUnit* fu) {
  assert (NULL != fu);
  if (false == (friendly->end() != std::find(friendly->begin(), friendly->end(), fu)))
    friendly->push_back(fu);

  return;
}



void SubSA::mergeEnemy(ResUnit* eu) {
  assert (NULL != eu);
  if (false == (enemy->end() != std::find(enemy->begin(), enemy->end(), eu)))
    enemy->push_back(eu);

  return;
}




// ----------------------------------
SubSA1::SubSA1(Unit* u) : SubSA(u) {
  // no-op
}



SubSA1::~SubSA1() {
  // no-op
}



void SubSA1::update() {
  // no-op
  return;
}


// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
