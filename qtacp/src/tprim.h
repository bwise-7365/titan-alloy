// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: evector becomes
// std::vector; dead includes dropped.
// ------------------------------------------


#ifndef TACTICAL_PRIMITIVES_H
#define TACTICAL_PRIMITIVES_H

// ------------------------------------------------------

#include "aaa.h"
#include "frwrdec.h"
#include "struct.h"

#include "des.h"
#include "tdv.h"
#include "unit.h"

#include "tthread.h"

#include <vector>

// ------------------------------------------------------

class Arty_Seq;
class UnitGroupingScript;
class AbsorbSubCP;

//------------------------------------------------------

// UnitGrouping is a set of units, for use in planning
//
// notice that this is quite different from the force-stuctures
// developed by force-package etc. Those define the "actual" 
// command relationships and linkages, while the "groupings"
// below are just planning tools, possiblly used to decide future 
// command relationships, but not relationships in themselves.
// hence, there is no duplication.

class UnitGrouping 
{
public:
  UnitGrouping();
  virtual ~UnitGrouping();
  virtual void addUnit(Unit*);

  // the UnitGrouping does not itself propose scripts. The
  // MnvrCP proposes several plans of how to use this group to
  // accomplish that action, but this syntax is necessitated
  // to achieve proper dispatching of virtual functions.
  //
  //  virtual evector<UnitGroupingScript*> propose_scripts(MnvrCP*, 
  //						      TacticalAction, 
  //						      TacticalData*)=0;

  virtual int num_mnvr_units()=0;
  virtual int num_arty_units()=0;

  virtual std::vector<Unit*> *get_my_units() { return my_units;};


protected:
  std::vector<Unit*> *my_units;

private:

};
// ------------------------------------------------------

#endif

// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
