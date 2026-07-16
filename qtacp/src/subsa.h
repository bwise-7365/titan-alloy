// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: banner, evector becomes
// std::vector.
// ------------------------------------------
// NOTE WELL: Situation-Awareness !=Subordinate-Situation-Awareness,
// by a long shot. A unit's full situation awareness is merged out of
// little SSA's that individual units have gotten.
//
// in this very simple model of Subordinate-Situation-Awareness,
// the only friendly in the list is the unit itself.
// the full situation awareness is a copy of the merged picture from higher up
// ------------------------------------------
#ifndef SUBORDINATE_SA_H
#define SUBORDINATE_SA_H

// ------------------------------------------

#include "frwrdec.h"
#include "struct.h"
#include "acpsim.h"
#include "unit.h"
#include "cunit.h"
#include "runit.h"

#include <vector>


// ------------------------------------------
// notice that the lists of known friendly and enemy
// ResUnits are not necessarily complete, and may
// include both live and dead entities.

class SubSA {
 public:
  SubSA(Unit* u);
  virtual ~SubSA();

  void clearFriendly();
  void clearEnemy();

  void mergeFriendly(ResUnit* fu);
  void mergeEnemy(ResUnit* eu);

  virtual void update()=0;

  Unit* unit;
  std::vector<ResUnit*> *friendly;
  std::vector<ResUnit*> *enemy;

 protected:

 private:

};


// first attempt at a situation awareness structure
// resunits have only themselves as known friendly
// resunits have only those they directly see as known enemy
//
// CmndUnits have the union of their subordinates.
//
// Thus, how high up the hierarchy I can reach to grab data
// determines how comprehensive a CROP I can work with
class SubSA1 : public SubSA {
 public:
  SubSA1(Unit* u);
  ~SubSA1();

  void update();

 protected:

 private:

};

// ------------------------------------------

#endif

// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
