// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: evector becomes
// std::vector; dead includes dropped.
// ------------------------------------------


#ifndef TTHREAD_HEADER
#define TTHREAD_HEADER

// ------------------------------------------------------

#include "frwrdec.h"
#include "struct.h"

#include "aaa.h"
#include "des.h"
#include "fsm.h"
#include "unit.h"

#include <vector>

// ------------------------------------------------------
class Box;
class Unit;
class Corridor;
class Strike_Seq; 
// ------------------------------------------------------

// notice that createTempCorridor copies bx1, so
// you can delete it w/o harming the corridor
TempCorridor*
createTempCorridor(Box *bx1, float st, float et);

TempCorridor*
convertCorridor(Corridor *corr);

// notice that extendTempCorridor copies bx1, so
// you can delete it w/o harming the corridor
void
extendTempCorridor(TempCorridor *corr, Box *bx1, double st, double et);

// original boxes (from a TempCorridor!) are bx1 and bx2
// it returns the regularized version rbx1 and rbx2.
// that is, close approximations to bx1 and bx2, such
// that both rbx1 and rbx2 perpendicular to line between
// their centers (which do not move)
void 
regularizeBoxPair (Box *bx1, Box *bx2, Box *&rbx1, Box *&rbx2);

void
printTempCorridor(TempCorridor *corr);

// shapeFactor near 1 makes it shaped mostly like the end box
// posFactor near 0 makes it placed near the start box
Box* compromiseBox(Box* sBox, Box* eBox, float shapeFactor, float posFactor);


// ------------------------------------------------------
// "tactical threads" are one possible generalization of movement 
// plans (Corridors) and strike sequences. 
//
// the key thing is that we can not only generate missions
// from either one but we can quickly check if two threads
// clash with each other (for constraint satisfaction)

// because we can not have virtual operators, we
// must do this funny thing with streamout()

class TThread {
public:
  TThread();
  virtual ~TThread();
    
  Box* bounding_box;
  float start_time;
  float end_time;
  void initialize();
  // delay impementing these until we use CCS
//   virtual TLogical clashes(TThread *t1)=0;
//   virtual TLogical clashes_with_corridor(Corridor* r)=0;
//   virtual TLogical clashes_with_strike_seq(Strike_Seq* b)=0;
  // this does the substantive check - when needed
//   virtual TLogical inner_clashes(TThread *t1);


  friend ostream& operator << (ostream& s, TThread& rt);
  virtual ostream& streamout(ostream& s)=0; 

  virtual void addBox(Box*, float, float);

  std::vector<Box*>* boxes;
  std::vector<float>* start_times;
  std::vector<float>* end_times;

protected:

  int current_n;
  int last_n_updated;

private:

};


AAA::Logical clash(TThread *t1, TThread *t2);


// ------------------------------------------------------


class Strike_Seq : public TThread
{
public:
    Strike_Seq();
    virtual ~Strike_Seq();

    virtual AAA::Logical clashes(TThread *t1);
    virtual AAA::Logical clashes_with_corridor(Corridor* r);
    virtual AAA::Logical clashes_with_strike_seq(Strike_Seq* b);

    friend ostream& operator << (ostream& s, Strike_Seq& rt);
    virtual ostream& streamout(ostream& s); 

protected:

private:

};

// ------------------------------------------------------
// Units of all echelons move by proceeding from
// box to box along a corridor.
// For res units, the box essentially defines the position
// tolerance to determine when the res unit has "arrived".
// For composite units, the box defines the area into which
// it must get to count as "arrived", and is quite useful
// in laying out formations

class Corridor : public TThread
{
public:
  Corridor();
  Corridor(Box*, float);
  virtual ~Corridor();
  void extend(Box *bx, float et);  // put a new, disjoint area at the end.

  // delay implementing these until we use CCS
//     virtual Logical clashes(TThread *t1);
//     virtual Logical clashes_with_corridor(Corridor* r);
//     virtual Logical clashes_with_strike_seq(Strike_Seq* b);

    std::vector<Box*> *area_boxes;
//     AAA::evector<Box*>* move_boxes;    //   data of type Box* , boxes inherited from TThread

    friend ostream& operator << (ostream& s, Corridor& crdr); 
    virtual ostream& streamout(ostream& s); 

    AAA::Logical check_integrity();

protected:

private:
  void initialize();
};

// ------------------------------------------------------

// to move a composite unit down a corridor,
// we need to cut it into boxes (hopefully)

class Formation
{
public:
  Formation();
  virtual ~Formation();

  // build a template
  virtual void add_point(AAA::GVector*);

  // build a set of units
  virtual void add_unit(Unit*);

  virtual void match_units();

  virtual std::vector<AAA::GVector*>* get_assigned_positions();
  virtual std::vector<Unit*>* get_assigned_units();

  friend ostream& operator << (ostream& s, Formation& f);

protected:

private:

  // slide and scale template
  virtual void move_template_onto_units();

  // assign each unit to that position
  // to which it is closest
  virtual void inner_match_units();

  std::vector<AAA::GVector*>* points;
  std::vector<Unit*>* units;

  std::vector<AAA::GVector*>* moved_points;
  std::vector<Unit*>* matched_units;
};



// ------------------------------------------------------

#endif

// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
