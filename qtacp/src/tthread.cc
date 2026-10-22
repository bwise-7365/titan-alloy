// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: evector becomes
// std::vector (value-returning pop_back split into back() +
// pop_back()); croak takes const char*.
// ------------------------------------------

#include "aaa.h"
#include "tthread.h"

#include <vector>

using std::vector;
using AAA::GVector;
using AAA::Logical;
using AAA::LTrue;
using AAA::LFalse;
using AAA::LUnknown;
using AAA::delta;

// ------------------------------------------------------

void
croak(const char* text, Logical continuableP) {
  cout << text << endl;
  if (LTrue != continuableP)
    assert (LTrue == LFalse);
  return;
}

// ------------------------------------------------------


ostream&
operator << (ostream& s, TThread& tt)
{
  tt.streamout(s);
  return s;
}

ostream&
operator << (ostream& s, Corridor& c)
{
  c.streamout(s);
  return s;
}

ostream& 
Corridor::streamout(ostream& s) {
  Box *bx = NULL;
  float et;
  s << "<<Corridor:" << endl;

  unsigned int i = 0;
  unsigned int n = area_boxes->size();

  for (i=0; i<n; i++) {
    bx = (*area_boxes)[i];
    et = (*end_times)[i];
    s << "  ( " << *bx << ", " << et << " )"  << endl;
  }

  s << " >>";
  return s;
}



ostream&
operator << (ostream& s, TempCorridor& tc)
{
  tc.streamout(s);
  return s;
}

ostream& 
TempCorridor::streamout(ostream& s) {
  Box *bx = NULL;
  float st = 0.0;
  float et = 0.0;
  s << "<<TempCorridor:" << endl;

  unsigned int i = 0;
  unsigned int n = boxes->size();

  for (i=0; i<n; i++) {
    bx = (*boxes)[i];
    st = (*sTimes)[i];
    et = (*eTimes)[i];
    s << "  ( " << *bx << ", " << st << ", " << et<< " )"  << endl;
  }

  s << " >>";
  return s;
}


// ------------------------------------------------------

ostream&
operator << (ostream& s, Strike_Seq& c)
{
  c.streamout(s);
  return s;
}

ostream& 
Strike_Seq::streamout(ostream& s){
  s << "STUB: Can not print Strike_Seq" << endl;
  return s;
}

// ------------------------------------------------------

TThread::TThread()
{
  initialize();
}

void
TThread::initialize() {
  start_time = 0;
  end_time = 0;
  bounding_box = NULL;

  boxes = new vector<Box*>(); // data of type Box*
  start_times =  new vector<float>(); // data of type float*
  end_times = new vector<float>(); // data of type float*
  assert (NULL != boxes);
  assert (NULL != start_times);
  assert (NULL != end_times);
  return;
}

TThread::~TThread() {

  Box* bx = NULL;
  //  float t = 0.0;

  while (boxes->size() > 0) {
    bx = boxes->back();
    boxes->pop_back();
    delete bx;
    bx = NULL;
  }
  delete boxes;
  boxes = NULL;


  while (start_times->size() > 0)
    start_times->pop_back();
  delete start_times;
  start_times = NULL;

  while (end_times->size() > 0)
    end_times->pop_back();
  delete end_times;
  end_times = NULL;
}

void
TThread::addBox(Box* b, float st, float et) {
  boxes->push_back( b);
  start_times->push_back( st);
  end_times->push_back( et);

  assert (boxes->size() == start_times->size());
  assert (boxes->size() == end_times->size());
  return;
}

// ------------------------------------------------------


Corridor::Corridor() : TThread()
{
  initialize();
}

void
Corridor::initialize() {
  area_boxes = new vector<Box*>(); //   data of type Box*
  //   move_boxes = new evector<Box*>(); //   data of type Box*
  assert (NULL != area_boxes);
  return;
}

Corridor::~Corridor() {
  Box* bx = NULL;
  if (NULL != area_boxes) {
    while (area_boxes->size() > 0) {
      bx = area_boxes->back();
      area_boxes->pop_back();
      delete bx;
      bx = NULL;
    }
    delete area_boxes;
    area_boxes = NULL;
  }


  if (NULL != boxes) {
    while (boxes->size() > 0) {
      bx = boxes->back();
      boxes->pop_back();
      delete bx;
      bx = NULL;
    }
    delete boxes;
    boxes = NULL;
  }

}


// note how the first Box must be oriented: A->B is
// the right edge, B->C is the far edge,
// C->D is the left edge, and D->A is the base edge.
//
Corridor::Corridor(Box* bx1, float et) : TThread()
{
  Box *bx2;

  initialize();
  
  bx2 = new Box(bx1->get_A(),bx1->get_B(),bx1->get_C(),bx1->get_D());
  assert (NULL != bx2);
  area_boxes->push_back( bx2);
  end_times->push_back( et);
  boxes->push_back(NULL);      // first element is meaningless!
  start_times->push_back( -1);  // first element is meaningless!
}

// extend needs to do a little extra checking to ensure it
// doesn't make an ill-formed move box when it gets weird input.
// this is more geometric reasoning than exception-handling, and
// I'd rather not rely on the ill-supported exception-handling of C++.


// the pattern being built up is the following:
// 
//  area_boxes  a0   a1   a2   a3   a4   a5
//       boxes nil  b01  b12  b23  b34  b45
//   end_times  t0   t1   t2   t3   t4   t5
// start_times  -1   t0   t1   t2   t3   t4
//
// we should add random rotation of labels,
// as a debugging measure to ensure we don't
// accidentally depend on it!

void 
Corridor::extend(Box *orig_area, float et)
{
  float eTime;
  float sTime;
  float tmpETime;
  Box *area, *bx1, *bx2;
  Logical fixed;
  GVector pa, pb, pc, pd; // proposed new points
  //  Node *bxND; // data to type Box*
  //  Node *eND; // data of type float*
  //  Node *sND; // data of type float*

  area = new Box(orig_area->get_A(), orig_area->get_B(), 
		 orig_area->get_C(), orig_area->get_D());
  assert (NULL != area);
  //   cout << "PID is " << getpid() << "%5 (used in rotation)" << endl;
  //   // Semi-random rotation
  //   for (i=0; i<(getpid()%5);i++)
  //     area->d_permute_ccw();
  unsigned int numBoxes = area_boxes->size();
  assert (numBoxes > 0);
  //   bxND = area_boxes->last;
  //   bx1 = ((Box*) bxND->data);
  bx1 = (*area_boxes)[numBoxes - 1];
  assert (NULL != bx1);
  // note that we add at the end!
  pa = bx1->get_B();
  pb = area->get_B();
  pc = area->get_C();
  pd = bx1->get_C();
  fixed = LTrue;

  //   cout << "Standardly constructed box is " << pa << "," << pb << ",";
  //   cout << pc << "," << pd <<endl;
  // shamelessly force it to be well-formed by construction!
  //   if (True==probe_box_integrity(pa, pb, pc, pd))
  //         cout << "The standard box is OK" << endl;
  //     else
  //         cout << "The standard box is ill-formed" << endl;
  
  //  cout << "Forcing to trapezoid" << endl;
  bx2 = nearest_trapezoid(pa, pb, pc, pd);
  //   cout << "Nearest trapezoid: " << *bx2 << endl;
  pa = bx2->get_A();
  pb = bx2->get_B();
  pc = bx2->get_C();
  pd = bx2->get_D();

  if (LTrue==probe_box_integrity(pa, pb, pc, pd))
    { // we are in the easy, standard case
      fixed = LTrue;
    }
  else // trouble looms
    {
      fixed = LFalse;
      cout << endl << "Standard construction of move-box gives ill-formed" << endl;

      cout << "Last box currently in corridor is " << *bx1 << endl;
      cout << "Trying to add box " << *area << endl;
      
      if (LTrue == box_intersect_p(bx1, area))
	{ // I can't fix this (but it may not be a problem!)
	  croak("Corridor::extend(Box*, float) area_boxes intersect", LFalse);
	}
      else
	{ // possibly fixable
	  croak("Corridor::extend(Box*, float) needs to patch move_box", LTrue);
	  // make some effort to reset pa, pb, pc, pd
	  if ((fixed == LFalse) && (sameSideP(pa,pc,  pb,pd) == LTrue))
	    // c pushed in. if angle from b to c to d is acute, this fails
	    {
	      pd = nearestPoint(pc, pa, pd);
	      fixed = LTrue; // I hope!
	    }
	  if ((fixed == LFalse) && (sameSideP(pb,pd,  pa,pc) == LTrue))
	    // b pushed in. if angle from c to b to a is acute, this fails
	    {
	      pa = nearestPoint(pb, pd, pa);
	      fixed = LTrue; // I hope!
	    }
	}
    }

  // if all efforts to patch things fail, it will croak building this box.
  bx2 = new Box(pa, pb, pc, pd);
  assert (NULL != bx2);


  // clean up the actual area_box used, compared to that given.
  //  area->set_A(nearestPoint(area->get_A(), pa, pb));
  //  area->set_D(nearestPoint(area->get_D(), pc, pd));
  //  area->check_integrity(); // just to be sure!

  area_boxes->push_back( area);
  boxes->push_back( bx2);

  // copy the last  time from the end of
  // end_times onto the end of start_times
  //   eND = end_times->last;
  //   eTime = ((float*) eND->data);
  eTime = (*end_times)[end_times->size() - 1];
  tmpETime = eTime;
  sTime = 0.0;
  sTime = tmpETime;
  start_times->push_back( sTime);

  // copy the given end time onto the end of end_times
  eTime = 0.0;
  eTime = et;
  end_times->push_back( eTime);

  //   if (end_times.item() < start_times.item())
  if (et < tmpETime)
    croak("Corridor::extend(Box*, float): end time after start time", LFalse);


  //  STUB: these control graphical display, 
  // so I commented this out for now
  //   if (enqueue_area_boxes)
  //       enqueue_arb_quadrilateral(area);
  //   if (enqueue_move_boxes)
  //       enqueue_arb_quadrilateral(bx2);


  assert (LTrue == check_integrity());
  return;
}




Logical
Corridor::check_integrity() {
  Logical rslt = LTrue;

  unsigned int i = 0;
  unsigned int n = area_boxes->size();
  Box *box;

  for (i=0; ((i<n) && (LTrue == rslt)); i++) {
    box = (*area_boxes)[i];
    if  ( NULL == box)
      rslt = LFalse;
    if( LTrue != box->check_integrity())
      rslt = LFalse;
  }

  return rslt;
}

// ------------------------------------------------------
TempCorridor*
convertCorridor(Corridor *corr) {
  TempCorridor *tCorr = NULL;
  float start = 0.0;
  float end = 0.0;

  float st = 0.0;
  float et = 0.0;
  //  Node *bxNd, *stNd, *etNd;
  Box *prevBox = NULL;
  Box *currBox = NULL;

  int boxesIntersectP = 0;
  unsigned int i = 0;
  unsigned int n = corr->area_boxes->size();

  //   for (bxNd = corr->area_boxes->first,
  // 	 stNd = corr->start_times->first,
  // 	 etNd = corr->end_times->first;
  //        NULL != bxNd;
  //        bxNd = corr->area_boxes->nextNode(bxNd),
  // 	 stNd = corr->start_times->nextNode(stNd),
  // 	 etNd = corr->end_times->nextNode(etNd)) {
  for (i=0; i<n; i++) {
    prevBox = currBox;
    //    currBox = ((Box*) bxNd->data);
    currBox = (*(corr->area_boxes))[i];
    if  (NULL != currBox) {
      //       start = ((float*) stNd->data);
      //       end = ((float*) etNd->data);
      start = (*(corr->start_times))[i];
      end = (*(corr->end_times))[i];
      st = start;
      et = end;

      assert (et >=  0);

      if (-1 == st) // stupid meaningless first value 
	st = et/(1e8);

      assert (st >= 0);

      if (NULL == tCorr) {
	tCorr = createTempCorridor (currBox, st, et);
	// 	cout << "Started corridor with " << *currBox << " " << st << " " << et << endl;
      }
      else {
	if (LTrue == box_intersect_p(currBox, prevBox)) {
	  boxesIntersectP = 1;
	  // DO modify the originals
	  // 	  currBox = new Box(currBox);
	  // 	  prevBox = new Box(prevBox);
	}
	else
	  boxesIntersectP = 0;

	while (boxesIntersectP > 0) {
	  if (true == ACPSim::tracePlanning) {
	    cout << "Contracted A " << boxesIntersectP;
	    cout << " times" << endl;
	    cout << "  prevBox: " << *prevBox<< endl << flush;
	    cout << "  currBox: " << *currBox << endl << flush;
	  }
	  if (box_intersect_p(prevBox, currBox) == LTrue ) {
	    cout << "Need to contract again"<<endl<<flush;
	    prevBox->d_expand(.80);
	    currBox->d_expand(.80);
	    boxesIntersectP++;
	  }
	  else {
	    cout << "No need to contract again"<<endl<<flush;
	    boxesIntersectP = 0;
	  }
	} // end while

	// 	cout << "Extended corridor with " << *currBox << " " << st << " " << et << endl;
	extendTempCorridor(tCorr, currBox, st, et);
      }
    } // end if (NULL != box)..
  } // end for loop
       
  return tCorr;
}


TempCorridor*
createTempCorridor(Box *bx1, float st, float et)
{
  TempCorridor *corr;
  float start = 0.0;
  float end = 0.0;
  assert (st >= 0);
  assert (et >= 0);
  
  corr = new TempCorridor();
  assert (NULL != corr);
  assert (NULL != corr->boxes);
  assert (NULL != corr->sTimes);
  assert (NULL != corr->eTimes);

  corr->boxes->push_back( new Box(bx1) );

  start = st;
  end = et;
  corr->sTimes->push_back( start);
  corr->eTimes->push_back( end);

  return corr;
}

// the new box, bx2, has to get rotated around
// so that its baseline D->A is on the ray
// from bx1's center to bx2:

//          A ---> B
//          |      |
// bx1 ---> |      |
//          |      |
//          D <--- C
//
// (in "x right, y down" coordinates)
// 
// as usual, D-->A is the 'back' of bx2,
// and B-->C is the 'front' of bx2.
//
// Note that we do not change bx1 in anyway, so there
// is no guarantee that bx1's "front" (the B-C segment)
// faces bx2. To require that, as well as that
// bx2's back face bx1, would be over-constrained
// and usually impossible to achieve.

void
extendTempCorridor(TempCorridor *corr, Box *bx2, double st2, double et2) {
  if (true == ACPSim::traceGeometry) {
    cout << "Trying to extend corridor"<<endl;
    cout << "  Corridor: " << *corr << endl;
    cout << "  Box: "<< *bx2 << endl;
    cout << "  st2: "<<st2 << endl;
    cout << "  et2: "<<et2 << endl;
    cout << flush;
  }
  Box *bx1;
  float st1, et1;
  GVector bx1Center, bx2Center;
  GVector a=GVector(0,0,0);
  GVector b=GVector(0,0,0);
  GVector c=GVector(0,0,0);
  GVector d=GVector(0,0,0);
  float area2 = 0;
  float area2b = 0;
  int rotatedP = 0;

  assert (NULL != bx2);
  assert (LTrue == bx2->check_integrity());
  unsigned int n = corr->boxes->size();
  assert (n == corr->sTimes->size());
  assert (n == corr->eTimes->size());

  // should be strict < 
  if (st2 >= et2) {
    printf("Found st2=%.2f but et2=%.3f \n",
	   st2, et2);
    assert (st2 < et2);
  }

  bx1= (*(corr->boxes))[ n - 1];
  assert (NULL != bx1);

  assert (LTrue == bx1->check_integrity());

  if (LFalse != box_intersect_p(bx1, bx2)) {
    cout << "Intersecting boxes: "<<endl;
    cout << "  Bx1: "<<*bx1<<endl;
    cout << "  Bx2: "<<*bx2<<endl;
    cout << flush;
  assert (LFalse == box_intersect_p(bx1, bx2));
  }
  bx1Center = bx1->center;
  bx2Center = bx2->center;

  // check that the geometry makes sense:
  // bx2 must be 'in front of' bx1
  //  assert (true == segIntersectP(bx1Center, bx2Center, bx1->get_B(), bx1->get_C()));
  // AS NOTED FAR ABOVE, the immediately above condition is
  // usually impossible to achieve

  st1 = (*(corr->sTimes)) [ n - 1 ];
  et1 = (*(corr->eTimes)) [ n - 1 ];

  // check that the times make sense
  // hack: temporary accomodation to convertCorridor
  assert (st1 <= et1);
  assert (et1 <= st2);  
  assert (st1 <= st2);  
  assert (et1 <= et2);  
  //   assert (st1 < et1);
  //   assert (et1 < st2);  
  //   assert (st2 < et2);  

  // make it verbose
  //   GVector::DebugGVectorctrs = 1;
  // rotate the box around so that D->A is actually the baseline
  if (true == ACPSim::traceGeometry) {
    cout<<endl << "Looking for a rotation"<<endl;
    cout << (*bx1) << endl;
    cout << (*bx2) << endl;
    cout<<flush;
  }

  area2 = bx2->area();
  if (true == segIntersectP(bx1Center, bx2Center, bx2->get_A(), bx2->get_B())) {
    a = bx2->get_B();
    b = bx2->get_C();
    c = bx2->get_D();
    d = bx2->get_A();
    rotatedP = 1;
    if (true == ACPSim::traceGeometry) {
      cout << "Rotated box2 +1"<<endl<<flush;
    }
  }

  else  if (true == segIntersectP(bx1Center, bx2Center, bx2->get_B(), bx2->get_C())) {
    a = bx2->get_C();
    b = bx2->get_D();
    c = bx2->get_A();
    d = bx2->get_B();
    rotatedP = 1;
    if (true == ACPSim::traceGeometry) {
    cout << "Rotated box2 +2"<<endl<<flush;
    }
  }

  else  if (true == segIntersectP(bx1Center, bx2Center, bx2->get_C(), bx2->get_D())) {
    a = bx2->get_D();
    b = bx2->get_A();
    c = bx2->get_B();
    d = bx2->get_C();
    rotatedP = 1;
    if (true == ACPSim::traceGeometry) {
      cout << "Rotated box2 +3"<<endl<<flush;
    }
  }
  else if (true == segIntersectP(bx1Center, bx2Center, bx2->get_D(), bx2->get_A())) {
    a = bx2->get_A();
    b = bx2->get_B();
    c = bx2->get_C();
    d = bx2->get_D();
    rotatedP = 1;
    if (true == ACPSim::traceGeometry) {
      cout << "Rotated box2 0"<<endl<<flush;
    }
  }

  else
    {
      cout << "Failed to find any rotation of box2"<<endl;
      cout << "  bx1 = " << *bx1 << endl;
      cout << "  bx1->center = " << bx1Center << endl;
      cout << "  bx2 = " << *bx2 << endl;
      cout << "  bx2->center = " << bx2Center << endl;
      cout <<flush;
      assert ( 1 == rotatedP);
    }
  
  // reduce verbosity
  //   GVector::DebugGVectorctrs = 0;
  bx2->set_A(a);
  bx2->set_B(b);
  bx2->set_C(c);
  bx2->set_D(d);

  // check that the rotation was done correctly
  bx2->set_area_and_perimeter();
  area2b = bx2->area();
  assert (delta (area2, area2b) < 1.0); 
  assert (true == segIntersectP(bx1Center, bx2Center, bx2->get_D(), bx2->get_A()));
  assert (LTrue == bx2->check_integrity());


  // test regularization!

     Box* rbx1 = NULL;
     Box* rbx2 = NULL;
     regularizeBoxPair(bx1, bx2, rbx1, rbx2);
     assert (NULL != rbx1);
     assert (NULL != rbx2);
     delete rbx1;
     rbx1 = NULL;
     delete rbx2;
     rbx2 = NULL;

  // actually do the extension
  corr->sTimes->push_back( st2);
  corr->eTimes->push_back( et2);
  corr->boxes->push_back( new Box (bx2) );
  return;
}

void 
regularizeBoxPair (Box *bx0, Box *bx2, Box *&rbx1, Box *&rbx2) {
  // because these are from a TempCorridor,
  // we can assume that the line from center1 to center2
  // passes through the baseline (D->A) of bx2.
    if (true == ACPSim::traceGeometry) {
      cout << "entering regularizeBoxPair"<<endl<<flush;
    }

  assert (NULL == rbx1);
  assert (NULL == rbx2);

  GVector bx0Center = bx0->center;
  GVector bx2Center = bx2->center;
  // note generally possible:
//   assert (true == segIntersectP(bx0Center, bx2Center, 
// 				 bx0->get_B(), bx0->get_C()));

  assert (true == segIntersectP(bx0Center, bx2Center, 
				 bx2->get_A(), bx2->get_D()));

  if (true == ACPSim::traceGeometry) {
    cout << "   ... front, back, and centers line up "<<endl<<flush;
  }

  Box *bx1 = NULL;

  GVector a = GVector(0,0,0);
  GVector b = GVector(0,0,0);
  GVector c = GVector(0,0,0);
  GVector d = GVector(0,0,0);
  int rotated0P = 0;
  float facing1, side1;
  float facing2, side2;
  GVector centerRay = bx2Center - bx0Center;
  GVector sideRay, frontRay;


  // now rotate bx0 (making bx1) so that the line from center1 to center2
  // passes through the line (B->C) of bx1.

  // try to rotate bx1 so the center-center line crosses B->C 
  if (true == segIntersectP(bx0Center, bx2Center, bx0->get_A(), bx0->get_B())) {
    a = bx0->get_D();
    b = bx0->get_A();
    c = bx0->get_B();
    d = bx0->get_C();
    rotated0P = 1;
    if (true == ACPSim::traceGeometry) {
      cout << "Rotated box1 +1"<<endl<<flush;
    }
  }

  else  if (true == segIntersectP(bx0Center, bx2Center, bx0->get_B(), bx0->get_C())) {
    a = bx0->get_A();
    b = bx0->get_B();
    c = bx0->get_C();
    d = bx0->get_D();
    rotated0P = 1;
    if (true == ACPSim::traceGeometry) {
    cout << "Rotated box1 +2"<<endl<<flush;
    }
  }

  else  if (true == segIntersectP(bx0Center, bx2Center, bx0->get_C(), bx0->get_D())) {
    a = bx0->get_B();
    b = bx0->get_C();
    c = bx0->get_D();
    d = bx0->get_A();
    rotated0P = 1;
    if (true == ACPSim::traceGeometry) {
    cout << "Rotated box1 +3"<<endl<<flush;
    }
  }

  else  if (true == segIntersectP(bx0Center, bx2Center, bx0->get_D(), bx0->get_A())) {
    a = bx0->get_C();
    b = bx0->get_D();
    c = bx0->get_A();
    d = bx0->get_B();
    rotated0P = 1;
    if (true == ACPSim::traceGeometry) {
    cout << "Rotated box1 0"<<endl<<flush;
    }
  }
  else
    {
      cout << "Failed to find any rotation of box1"<<endl;
      cout << "  bx0 = " << *bx0 << endl;
      cout << "  bx0->center = " << bx0Center << endl;
      cout << "  bx2 = " << *bx2 << endl;
      cout << "  bx2->center = " << bx2Center << endl;
      cout <<flush;
      assert (1 == rotated0P);
    }

  bx1 = new Box(a,b,c,d);
  assert (NULL != bx1);

  assert (true == segIntersectP(bx0Center, bx2Center,
				 bx1->get_B(), bx1->get_C()));

    if (true == ACPSim::traceGeometry) {
      cout << "Rotated boxes to suitable orientation";
      cout << endl << flush;
    }

  facing1 = ( dist(bx1->get_A(), bx1->get_D())
	      +
	      dist(bx1->get_C(), bx1->get_B())) / 2.0;

  side1 = ( dist(bx1->get_A(), bx1->get_B())
	    +
	    dist(bx1->get_C(), bx1->get_D())) / 2.0;


  facing2 = ( dist(bx2->get_A(), bx2->get_D())
	      +
	      dist(bx2->get_C(), bx2->get_B())) / 2.0;

  side2 = ( dist(bx2->get_A(), bx2->get_B())
	    +
	    dist(bx2->get_C(), bx2->get_D())) / 2.0;

  // build rbx1
  frontRay = centerRay;
  // forward ray is length of side edge
  frontRay.scale_to(side1 / 2.0);  


  sideRay = rotateCCW(centerRay);

  // sideward ray is length of facing edge
  sideRay.scale_to(facing1 / 2.0);

  a = (bx0Center - frontRay) - sideRay;
  b = (bx0Center + frontRay) - sideRay;
  c = (bx0Center + frontRay) + sideRay;
  d = (bx0Center - frontRay) + sideRay;

  rbx1 = new Box(a,b,c,d);
  assert (NULL != rbx1);
  assert (LTrue == rbx1->check_integrity());
  assert (true == segIntersectP(bx0Center, bx2Center,
				 rbx1->get_B(), rbx1->get_C()));


  // build rbx2
  frontRay = centerRay;
  // forward ray is length of side edge
  frontRay.scale_to(side2 / 2.0);  


  sideRay = rotateCCW(centerRay);

  // sideward ray is length of facing edge
  sideRay.scale_to(facing2 / 2.0);

  a = (bx2Center - frontRay) - sideRay;
  b = (bx2Center + frontRay) - sideRay;
  c = (bx2Center + frontRay) + sideRay;
  d = (bx2Center - frontRay) + sideRay;

  rbx2 = new Box(a,b,c,d);
  assert (NULL != rbx2);
  assert (LTrue == rbx2->check_integrity());
  assert (true == segIntersectP(bx0Center, bx2Center,
				 rbx2->get_D(), rbx2->get_A()));

  delete bx1;
  bx1 = NULL;

  if (true == ACPSim::traceGeometry) {
    cout << "exitting regularizeBoxPair" << endl;
  }
  return;
}

void
printTempCorridor(TempCorridor *corr)
{
  Box *bx1;
  //   Node *bxNode; // data of type Box*
  //   Node *stNode; // data of type float*
  //   Node *etNode; // data of type float*
  //   float st1, et1;
  float st, et;
  unsigned int i = 0;
  unsigned int n = corr->boxes->size();

  cout << "[ TempCorridor: "<<endl;
  //   for (bxNode = corr->boxes->first, 
  // 	 stNode = corr->sTimes->first,
  // 	 etNode = corr->eTimes->first;
  //        ((NULL != bxNode) && (NULL != stNode) && (NULL != etNode));
  //        bxNode = corr->boxes->nextNode(bxNode),
  // 	 stNode = corr->sTimes->nextNode(stNode),
  // 	 etNode = corr->eTimes->nextNode(etNode)) {
  //     bx1 = ((Box*) bxNode->data);
  //     st = ((float*) stNode->data);
  //     et = ((float*) etNode->data);
  for (i=0; i<n; i++) {
    bx1 = (*(corr->boxes))[i];
    st =  (*(corr->sTimes))[i];
    et =  (*(corr->eTimes))[i];
    cout << "( " << *bx1 << endl <<  "      " << st << "  " << et << "  )" <<endl;
  }
  cout << " ]"<<endl<<flush;
  return;
}


// ------------------------------------------------------

// shapeFactor near 1 makes it shaped mostly like the end box
// posFactor near 0 makes it placed near the start box
Box*
compromiseBox(Box* start, Box* end, float shapeFactor, float posFactor) {
  Box* rslt = NULL;

  assert (0 <= shapeFactor);
  assert (shapeFactor <= 1);
  assert (0 <= posFactor);
  assert (posFactor <= 1);

  start->set_area_and_perimeter();
  end->set_area_and_perimeter();
  GVector sCenter = start->center;
  GVector eCenter = end->center;

  // get the right shape
  GVector midA = (end->get_A() * shapeFactor)  +  (start->get_A() * (1.0 - shapeFactor));
  GVector midB = (end->get_B() * shapeFactor)  +  (start->get_B() * (1.0 - shapeFactor));
  GVector midC = (end->get_C() * shapeFactor)  +  (start->get_C() * (1.0 - shapeFactor));
  GVector midD = (end->get_D() * shapeFactor)  +  (start->get_D() * (1.0 - shapeFactor));

  GVector tmpCenter = ((midA + midB) + (midC + midD))/4.0;

  // get the right place
  GVector desiredCenter =  (end->center * posFactor) + (start->center * (1.0 - posFactor));

  GVector correction = desiredCenter - tmpCenter;

  rslt = new Box (midA + correction,
		  midB + correction,
		  midC + correction,
		  midD + correction);
  assert (NULL != rslt);

  return rslt;
}


// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
