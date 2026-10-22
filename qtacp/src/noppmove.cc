// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: include list, evector -> std::vector.
// ------------------------------------------

#include "aaa.h"
#include "frwrdec.h"
#include "struct.h"
#include "cunit.h"
#include "orders.h"
#include "tthread.h"

#include <vector>

extern ACPSim* theSim;

using AAA::Logical;
using AAA::LTrue;
using AAA::LFalse;
using AAA::LUnknown;

using AAA::GVector;
using std::vector;

// ------------------------------------------------------
//  This needs all to be somewhat re-worked so as to use
//  the "process" FSM's within each state, and to use CCS
//  to define the tactical threads and FSM's to be used!
//
// ------------------------------------------------------
//   this is one enormous function. breaking it up
//    would help.
// ------------------------------------------------------

// ------------------------------------------------------
// This orders a unit to move, unopposed, into a given
// box. It orders it first to an intermediate box between
// the current position and the given box, then into the
// given box.
// If the unit is already perfectly centered in the given
// box, it orders it to reform in place
// ------------------------------------------------------
void
OrderCUnitAcrossBox::order_unopposed_move() {
  Formation *form;
  Action *action, *planner;
  TempCorridor *corr;
  GVector tmp_a, tmp_b, tmp_c, tmp_d;
  GVector tmp2_a, tmp2_b, tmp2_c, tmp2_d;
  GVector center_pt, cp_goal;
  GVector* center = NULL;
  Unit* sub;
  Box *sub_bx, *sub_end_bx, *sub_mid_bx;
  vector<Box*>* sub_end_boxes;  // DLList<Box*>
  vector<Unit*> *matched_subs;//  DLList<Unit*>
  //   Node *sbND; // data of type Box*
  //  Node *sebND; // data of type Box*
  //  Node *suND, *msND; // data of type Unit*
  //   int stuff;

  //  RNG* rng = unit->sim->rng;
  int numLS = unit->numLiveSubs();
  float posFactor = .45;
  float shapeFactor = 0.55;
  float negligableHeight = 0.01; // 1 cm

  int boxesDeIntersectedP;

  unsigned int i = 0;
  unsigned int n = 0;

  if (0 == numLS) {
    cout <<"Unit " <<unit->getSimEntID() <<"has zero live subs, yet is planning";
    cout <<endl<<flush;
    assert (numLS > 0);
  }

  GVector center_end, center_strt, v, w;
  GVector displacement;
  float ge, fe, s_time, m_time;

  if (true == ACPSim::tracePlanning) {
    cout <<"Unit " <<unit->getSimEntID();
    cout <<" needs to construct a plan and command a task to " <<endl;

    if (mv_bx != NULL)
      cout <<"   move along " <<*mv_bx <<" to " <<*end_bx <<endl;
    else
      cout <<"   freely-move to " <<*end_bx <<endl;

    cout <<endl <<flush;
  }

  planner = NULL;

  assert (unit->subordinates->size() > 0);
  sub_end_boxes = unit->compute_formation(end_bx, StandardFormation);
  assert (sub_end_boxes->size() == unit->numLiveSubs());

  assert (sub_end_boxes->size() == unit->numLiveSubs());
  form = new Formation();
  assert (NULL != form);

  n = sub_end_boxes->size();
  for (i=0; i<n; i++) {
    sub_bx = (*sub_end_boxes)[i];
    center = new GVector(sub_bx->center.get(0),
			     sub_bx->center.get(1),
			     0.0);
    form->add_point(center);
  }

  if (true == ACPSim::tracePlanning) {
    cout <<"Trying to print a formation" <<endl;
    // show those ideal points at the end
    cout <<*form <<endl <<flush;
  }

  // dump all live subordinates into the formation
  n = unit->subordinates->size();
  for (i=0; i<n; i++) {
    sub = (*(unit->subordinates))[i];
    assert (sub != NULL);
    //    cout << "Trying to add unit "<<i<<" of "<<n<<" to formation"<<endl<<flush;
    if (LTrue == sub->aliveP)
      form->add_unit(sub);
  }

  //  cout << "Trying to match units to formation"<<endl<<flush;
  form->match_units();
  // at this point, all the units are matched up with the
  // final boxes they should go to. That is, the matched_units
  // in the formation are in one-to-one correspondance
  // with the boxes in sub_end_boxes!

  matched_subs = form->get_assigned_units();
  //  assert (matched_subs->size() == unit->subordinates->size());
  assert (matched_subs->size() == unit->numLiveSubs());

  // we can NOT delete the formation,
  // or else no planning will occur!


  // to demo the concept, all we need now is to get some
  // starting boxes for the subordinate units. Ideally,
  // I'd like to see every ground unit know its own box
  // at all times (either because it is in it, or can
  // interpolate between its last and next boxes along a corridor)
  // and travel along within the assigned mv_box.
  // But for now, we'll do the following kludge (and what will
  // we do for helicopters? use virtual fn's to avoid this code!)

  // this fails when the start point is so near the final box
  // that the "sub_box" or "sub_mid_bx" end up intersecting
  // the final box

  n = sub_end_boxes->size();
  for (i=0; i<n; i++) {
    sub_end_bx = (*sub_end_boxes)[i];
    sub = (*matched_subs)[i];

    // we want to order this sub to get into that box.
    if (true == ACPSim::tracePlanning) {
      cout <<"    -  -  -  -" <<endl;
      cout <<"Unit " <<unit->getSimEntID() <<" is planning-1 for subordinate ";
      cout <<sub->getSimEntID() <<" to get into box:" <<endl <<flush;
      cout <<(*sub_end_bx) <<endl <<flush;

      // XXXX just a place to hook a breakpoint
      cout <<flush;
    }


    tmp2_a = sub_end_bx->get_A();
    tmp2_b = sub_end_bx->get_B();
    tmp2_c = sub_end_bx->get_C();
    tmp2_d = sub_end_bx->get_D();
    center_end = ((tmp2_a + tmp2_b) + (tmp2_c + tmp2_d))/4.0;
    center_strt = sub->currentPos();
    center_strt.set(0.0, 2);

    // double check zero altitude
    cout << "Center end: " << center_end <<endl<<flush;
    cout << "Center start: " << center_strt <<endl<<flush;
    assert (negligableHeight > fabs(center_end.get(2)));
    assert (negligableHeight > fabs(center_strt.get(2)));

    // half the sides' average of the END box
    ge = (dist(tmp2_c , tmp2_d) + dist(tmp2_a , tmp2_b))/4.0; 

    // half the ends' average of the END box
    fe = (dist(tmp2_a , tmp2_d) + dist(tmp2_c , tmp2_b))/4.0; 

    // this is the vector from the start position to the
    // center of the desired end-box
    displacement = center_end - center_strt;

    // the basic problem is that its current box could be
    // almost exactly the end box. if it is within 1/10
    // of the average of fe and ge, consider it a reform-in-place
    if (norm(displacement) <= (fe+ge)/20.0) {

      if (true == ACPSim::tracePlanning) {
  	cout <<"Unit " <<unit->getSimEntID() <<" is reforming subordinate ";
  	cout <<sub->getSimEntID() <<" because"<<endl;
	cout << "he's already almost exactly at desired location";
	cout << "  displacement: "<<norm(displacement)<<endl;
	cout << "  edge-lengths: "<<2*fe<<", "<<2*ge<<endl;
	cout <<endl <<flush;

	cout <<"Creating new reform order" <<endl;

	// XXXX just a hook for placing breakpoints
	cout <<flush;
      }

      action = new OrderUnitReformInPlace(sub, sub_end_bx, e_time);
      assert (NULL != action);

      if (true == ACPSim::tracePlanning) 
	cout <<"Performing new order immediately" <<endl;

      action->perform();

    }
    else { // displacement is not zero

      // setup (v, w) as a local coordinate system,
      // for building neat intermediate boxes.

      v = displacement; // copy before alteration!

      v.scale_to(fe);

      // double check planarity
      if (0 != v.get(2)) {
	cout <<"v.get(2) = " <<v.get(2) <<endl <<flush;
	assert (0 == v.get(2));
      }

      
      // now v is half as long as the ends, pointing to the left
      v = rotateCCW(v);


      w = displacement; // copy before alteration!
      // double check planarity
      assert (0 == w.get(2));

      // now w is half as long as the sides, pointing toward the destination
      w.scale_to(ge);

      // these make the corners of the basic box, but centered at our
      // current location, NOT centered on the desired end location

      tmp_a = (center_strt - w) - v;
      tmp_b = (center_strt + w) - v;
      tmp_c = (center_strt + w) + v;
      tmp_d = (center_strt - w) + v;

      tmp_a.set(0.0, 2);
      tmp_b.set(0.0, 2);
      tmp_c.set(0.0, 2);
      tmp_d.set(0.0, 2);

      if (true == ACPSim::tracePlanning) {
	cout <<endl;
	cout <<"sub_end_bx: " <<*sub_end_bx <<endl;
	cout <<"center_strt: " <<center_strt <<endl;
	cout <<"center_end: " <<center_end <<endl;
	cout <<flush;
      }

      sub_bx = new Box(tmp_a, tmp_b, tmp_c, tmp_d);
      assert (NULL != sub_bx);

      if (true == ACPSim::tracePlanning) 
	cout <<"sub_bx 0: " <<*sub_bx <<endl;

      // clean it up
      sub_bx = sub_end_bx->rectangularize();

      if (true == ACPSim::tracePlanning) 
	cout <<"sub_bx 1: " <<*sub_bx <<endl;

      // shift onto the desired end position
      // this is not quite the displacement of above,
      // as the boxes might have been altered.
      displacement = center_strt - center_end;
      assert (norm(displacement) > 0.0);

      sub_bx->d_shift(displacement);

      if (true == ACPSim::tracePlanning) 
	cout <<"sub_bx 2: " <<*sub_bx <<endl;

      if (box_intersect_p(sub_bx, sub_end_bx) == LTrue) 	{

	if (true == ACPSim::tracePlanning) {
	  cout <<"Have an intersection for subordinate ";
	  cout <<sub->getSimEntID() <<" of " <<unit->getSimEntID() <<endl;
	  cout <<"Sub center is at " <<center_strt <<endl;
	  cout <<"End box is at " <<*sub_end_bx <<endl;
	  cout <<"Constructed box is at " <<*sub_bx <<endl;
	  cout <<"The starting center is " <<center_strt <<endl;
	  if (LTrue == sub_end_bx->insideP(center_strt))
	    cout <<"The starting center is inside the final box" <<endl;
	  cout <<endl <<flush;
	}

	// I think I put this here so that I would have a new
	// chunk of memory, and a new object to alter,
	// without fear of altering a box someone else was using.
	sub_end_bx = new Box(sub_end_bx->get_A(),
			     sub_end_bx->get_B(),
			     sub_end_bx->get_C(),
			     sub_end_bx->get_D());
	assert (NULL != sub_end_bx);
	sub_bx->d_expand(.80);
	sub_end_bx->d_expand(.90);
	boxesDeIntersectedP = 1;
	while (boxesDeIntersectedP > 0) {
	  if ((true == ACPSim::tracePlanning) || (boxesDeIntersectedP > 100)) {
	    cout <<"Contracted B " <<boxesDeIntersectedP;
	    cout <<" times" <<endl;
	    cout <<"sub_bx: " <<*sub_bx<<endl <<flush;
	    cout <<"sub_end_bx: " <<*sub_end_bx<<endl <<flush;
	  }
	  // this can get caught in a loop when sub_bx almost equals
	  // sub_end_bx. if so, catch it and crash.
	  // when it works right, the number of contractions is 
	  // in the 2 to 20 range. (.80)^^100 = 2e-10
	  assert (boxesDeIntersectedP < 100);

	  if (box_intersect_p(sub_bx, sub_end_bx) == LTrue ) {
	    sub_bx->d_expand(.80);
	    sub_end_bx->d_expand(.90);
	    boxesDeIntersectedP++;
	  }
	  else
	    boxesDeIntersectedP = 0;
	}
      }
      //      assert(box_intersect_p(sub_bx, sub_end_bx) == False);

      // now we want this "sub" to go to sub_bx then to sub_end_bx
      // just to prove we have a full-fledged corridor, we insert
      // a box in the middle as an intermediate objective.
      // this forces subordinates to plan at shorter space and time
      // scales than their superiors, but it should be tactical
      // - not purely geometric - in deciding on intermediate objectives.
      // In the case of attack, obvious intermediate objectives are
      // smaller clumps of enemy forces between you and your objective.
      

      // Can this end up being a box of zero area ??
      // I have not seen it yet.
      tmp_a = sub_bx->get_A();
      tmp_b = sub_bx->get_B();
      tmp_c = sub_bx->get_C();
      tmp_d = sub_bx->get_D();

      tmp_a.set(0.0, 2);
      tmp_b.set(0.0, 2);
      tmp_c.set(0.0, 2);
      tmp_d.set(0.0, 2);

      shapeFactor = 0.75; // shaped mostly like the sub_end_box
      posFactor = 0.45; // positioned slightly closer to the sub_bx

      sub_mid_bx = compromiseBox(sub_bx, 
				 new Box(tmp2_a, tmp2_b, tmp2_c, tmp2_d), 
				 shapeFactor, posFactor);

      s_time = theSim->clock();
      if (e_time <= s_time) {

	if (true == ACPSim::tracePlanning) {
	  cout <<"e_time = " <<e_time <<" <= s_time = "<<s_time;
	  cout <<" problem" <<endl;
	}
      }
      if (s_time > e_time)
	s_time = e_time;

      if (box_intersect_p(sub_mid_bx, sub_bx) == LTrue) {
	if (true == ACPSim::tracePlanning) {
	  cout <<"Mid box intersects start box." <<endl;
	  cout << "   mid box:   " << *sub_mid_bx << endl;
	  cout << "   start box: " << *sub_bx << endl;
	  cout << flush;
	}
      }
      if (box_intersect_p(sub_mid_bx, sub_end_bx) == LTrue) {
	if (true == ACPSim::tracePlanning) {
	  cout <<"Mid box intersects end box." <<endl;
	  cout << "   mid box: " << *sub_mid_bx << endl;
	  cout << "   end box: " << *sub_end_bx << endl;
	  cout << flush;
	}
      }
      if (box_intersect_p(sub_bx, sub_end_bx) == LTrue) {
	if (true == ACPSim::tracePlanning) {
	  cout <<"Start box intersects end box." <<endl;
	  cout << "   start box: " << *sub_bx << endl;
	  cout << "   end box:   " << *sub_end_bx << endl;
	  cout << flush;
	}
      }

      // Finally, do it right away.
      m_time = (s_time * ( 1.0 - posFactor)) + (e_time * posFactor);

      if (true == ACPSim::tracePlanning) 
	cout <<"Creating new corridor" <<endl;

      corr = createTempCorridor(sub_bx, s_time, s_time + (0.01*(m_time - s_time)));
      assert (NULL != corr);

      if (true == ACPSim::tracePlanning)
	cout <<"Extending1 new corridor" <<endl;

      extendTempCorridor(corr, sub_mid_bx, s_time + (0.95*(m_time - s_time)), m_time);

      if (true == ACPSim::tracePlanning) 
	cout <<"Extending2 new corridor" <<endl;

      extendTempCorridor(corr, sub_end_bx, m_time + (0.95*(e_time - m_time)), e_time);
      //       cout <<" Corridor produced was " <<*corr <<endl;

      if (true == ACPSim::tracePlanning) 
	cout <<"Creating new corridor order" <<endl;

      action = new OrderUnitAlongCorridor(sub, corr);
      assert (NULL != action);

      if (true == ACPSim::tracePlanning) 
	cout <<"Performing new order immediately" <<endl;

      action->perform();
    } // displacement > 0
  } // end of loop over subordinates

  cp_goal = (((end_bx->get_A() * 5) + end_bx->get_B()) +
	     ((end_bx->get_D() * 5) + end_bx->get_C()))/12;

  if (NULL != unit->cmnd_veh) {
    action = new OrderResUnitToPoint(unit->cmnd_veh, cp_goal, e_time);
    assert (NULL != action);
    action->perform();
  }
  // xxx delete action;

  return;
}


// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
