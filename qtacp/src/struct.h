// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: banner, includes,
// evector becomes std::vector.
// ------------------------------------------

#ifndef STRUCTS_H
#define STRUCTS_H

// ------------------------------------------

#include "frwrdec.h"
#include "aaa.h"
#include "des.h"
#include "tgrid.h"
#include "tdv.h"

#include <vector>

ostream& operator << (ostream& s, Alignment a);

// notice that V used "Red", "Green", and "Blue", so we have to append "Side" to each

const char* alignmentString(Alignment);




// these are event-records to be put in a ACPSimEvent's data slot
struct MissileLaunchRecord {
  //  ResUnit* target;
  AAA::GVector launchPosition;
};



// this is a start at a more natural and simple representation
// of corridors.
// if boxes = (bx1, bx2, bx3, bx4, ....)
// then we know:
// (A) all boxes are well-formed
// (B) the baseline (D->A) of bx(i+1) intersects
// the line from the center of bx(i) to the center of bx(i+1)

class TempCorridor {
public:
  TempCorridor();
  virtual ~TempCorridor();

  friend ostream& operator << (ostream& s, TempCorridor& tc);
  virtual ostream& streamout(ostream& s);


  unsigned int size();

  std::vector<Box*> *boxes;
  std::vector<float> *sTimes; // time to get into the box
  std::vector<float> *eTimes; // time to leave it (if not last)

protected:

private:
};




#endif
// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
