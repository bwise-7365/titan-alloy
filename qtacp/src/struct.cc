// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: banner, evector becomes
// std::vector (pop_back split into back/pop_back), const char*.
// ------------------------------------------

#include "struct.h"

#include <vector>

// ------------------------------------------------------


const char* alignmentString(Alignment a)
{
  const char* rslt = "Grey";
  switch (a) {
  case  BlueSide:
    rslt = "Blue";
    break;

  case   RedSide:
    rslt = "Red";
    break;

  case   GreenSide:
    rslt = "Green";
    break;

  case   OrangeSide:
    rslt = "Orange";
    break;

  case   PurpleSide:
    rslt = "Purple";
    break;

  case   WhiteSide:
    rslt = "White";
    break;

  case   GreySide :
    rslt = "Grey";
    break;

  case   BlackSide :
    rslt = "Black";
    break;

  case   GoldSide :
    rslt = "Gold";
    break;

  case   CrimsonSide :
    rslt = "Crimson";
    break;
  }
  return rslt;
}

ostream& operator << (ostream& s, Alignment a) {
  s << alignmentString(a);
  return s;
}



// ------------------------------------------------------
TempCorridor::TempCorridor() {
  boxes = new std::vector<Box*>(); // data of type Box*
  sTimes = new std::vector<float>(); // data of type float*
  eTimes = new std::vector<float>(); // data of type float*
  assert (NULL != boxes);
  assert (NULL != sTimes);
  assert (NULL != eTimes);

}

TempCorridor::~TempCorridor() {
  Box* bx = NULL;
  unsigned long int n = boxes->size();
  assert (sTimes->size() == n);
  assert (eTimes->size() == n);

  while (boxes->size() > 0) {
    bx = boxes->back();
    boxes->pop_back();
    delete bx;
    bx = NULL;
  }

  delete boxes;
  boxes = NULL;

  delete sTimes;
  sTimes = NULL;

  delete eTimes;
  eTimes = NULL;

}

unsigned int TempCorridor::size() {
  unsigned int n = boxes->size();
  assert (sTimes->size() == n);
  assert (eTimes->size() == n);
  return n;
}


// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
