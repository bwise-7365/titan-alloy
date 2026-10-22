// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: include list, evector -> std::vector.
// ------------------------------------------------------
// (A) this is copied from MACP, and is not fully compatible with yACP.
// it is slowly being converted to at least compilability, and eventually
// it will be folded in to make a workable option.
// ------------------------------------------------------
// key functions:
// void OrderCUnitAcrossBox::order_opposed_move
// void OrderCUnitAcrossBox::order_right_hook
// void OrderCUnitAcrossBox::order_left_hook
// ------------------------------------------------------

#ifndef CU_OPPOSED_MOVE_CC
#define CU_OPPOSED_MOVE_CC

// ------------------------------------------------------

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

using AAA::FSM;
using AAA::State;
using AAA::Action;
using AAA::Predicate;
using AAA::Conjunction;
using AAA::AllActions;

// ------------------------------------------------------

//  This needs all to be somewhat re-worked so as to use
//  the "process" FSM's within each state, and to use CCS
//  to define the tactical threads and FSM's to be used!
//

// ------------------------------------------------------

void
OrderCUnitAcrossBox::order_opposed_move(float opp_strength, GVector enemy_cg)
{
  float friendly_strength;
  Box *left_bx, *rght_bx;

  friendly_strength = unit->currentStrength();
  cout << "Apparent force ratio is " <<friendly_strength;
  cout <<":"<<opp_strength<<endl;
  cout << "Enemy CG is at "<<enemy_cg<<endl;

  left_bx = new Box((mv_bx->get_A() + mv_bx->get_D())/2,
		    (mv_bx->get_B() + mv_bx->get_C())/2,
		    mv_bx->get_C(), 
		    mv_bx->get_D());
  assert (NULL != left_bx);
  rght_bx = new Box(mv_bx->get_A(),
		    mv_bx->get_B(), 
		    (mv_bx->get_B() + mv_bx->get_C())/2,
		    (mv_bx->get_A() + mv_bx->get_D())/2);
  assert (NULL != rght_bx);

  if (left_bx->insideP(enemy_cg) == LTrue)
    {
      cout << "Indicated SOM: fix left, flank right"<<endl;
      this->order_right_hook(opp_strength, enemy_cg);
      //      this->order_left_hook(opp_strength, enemy_cg);
    }
  else if (rght_bx->insideP(enemy_cg) == LTrue)
    {
      cout << "Indicated SOM: fix right, flank left"<<endl;
      this->order_left_hook(opp_strength, enemy_cg);
    }
  else
    {
      cout << "No clearly assailable flank" << endl;
      cout << "Indicated SOM: frontal assault." << endl;
      this->order_unopposed_move();
    }

  return;
}

// ------------------------------------------------------

void
OrderCUnitAcrossBox::order_right_hook(float, GVector enemy_cg)
{
  //  stubFN("order_right_hook", 1);
  cout << "calling OrderCUnitAcrossBox::order_right_hook " << endl << flush;

  unsigned int num_subs = 0;
  unsigned int num_fix = 0;
  unsigned int num_flank = 0;
  int i = 0;
  GVector p1, p2, p3, p4, p5, p6, p7;
  GVector p8, p9, p10, p11, p12;
  GVector tmp_pt, tpa, tpb, tpc, tpd, cp_goal;
  float box_depth, box_width, s_time;
  GVector lcl_y, lcl_x, diag;
  Box *f1, *g1, *f2, *g2, *f3, *g3, *f4, *g4;
  Box *ab0, *ab1, *ab2, *ab3, *ab4;
  float t1, t2, t3, t4;
  //  FSM *hls_process;
  FSM *new_fsm = NULL;
  Corridor *fix_corr, *flank_corr;
  CmndUnit *cp_fix, *cp_flank;
  State *s1, *s2, *s3;
  Predicate *tmp_tst;
  Conjunction *tst1;
  AllActions *act1;
  Action *tmp_act;
  vector<Unit*> *fix_force = NULL;
  vector<Unit*> *flank_force = NULL;
  //  Node* inode = NULL;
  //  Node* jnode = NULL;

  s1 = new State();
  s2 = new State();
  s3 = new State();
  assert (NULL != s1);
  assert (NULL != s2);
  assert (NULL != s3);

  s_time = theSim->clock();
  if (s_time > e_time)
    s_time = e_time;


  p1 = nearestPoint(enemy_cg, end_bx->get_A(), end_bx->get_D());
  p2 = nearestPoint(enemy_cg,  mv_bx->get_C(),  mv_bx->get_D());
  p3 = nearestPoint(enemy_cg,  mv_bx->get_A(),  mv_bx->get_B());
  
  p4 = (p2 * 3.0 + enemy_cg)/4.0;
  p5 = (enemy_cg * 5.0 + p3 * 3.0)/8.0;
  p6 = (enemy_cg * 4.0 + p3 * 4.0)/8.0;
  p7 = (enemy_cg * 1.0 + p3 * 7.0)/8.0;

  p8 = nearestPoint(p4, mv_bx->get_A(), mv_bx->get_D());
  p9 = nearestPoint(p5, mv_bx->get_A(), mv_bx->get_D());

  p10 = nearestPoint(p6, mv_bx->get_A(), mv_bx->get_D());
  p11 = nearestPoint(p7, mv_bx->get_A(), mv_bx->get_D());

  // note that because we will split up the force,
  // we make the boxes of the subunits half as deep
  box_depth = (dist(end_bx->get_D(), end_bx->get_C()) +
	       dist(end_bx->get_B(), end_bx->get_A()))/4.0;

  lcl_y = mv_bx->local_y();
  lcl_x = mv_bx->local_x();
  diag = (lcl_x - lcl_y) * 0.707;

  // ==============================================
  //  build corridors and arty boxes.
  // this is like IPB, which then goes into CCS
  // ==============================================

// ------------------------------------------------------

  cout << "Building the fixing corridor ..." << endl;

  // formation at end of Phase 1
  tpb = ((p9 * 2) + (p5 ))/3.0;
  tpa = tpb - (lcl_y * box_depth);
  tpc = ((p8 * 2) + (p4 ))/3.0;
  tpd = tpc - (lcl_y * box_depth);
  f1 = new Box(tpa, tpb, tpc, tpd);
  assert (NULL != f1);
  t1 = s_time + (e_time - s_time)*(2.0/3.0)*(1.0/3.0);

  fix_corr = new Corridor(f1, t1); // one area_box, zero move_boxes

  
  // formation at end of Phase 2
  tpb = (p9 + (p5 * 3.0))/4.0;
  tpa = tpb - (lcl_y * box_depth);
  tpc = (p8 + (p4 * 3.0))/4.0;
  tpd = tpc - (lcl_y * box_depth);
  f2 = new Box(tpa, tpb, tpc, tpd);
  t2 = s_time + (e_time - s_time)*(2.0/3.0)*(3.0/4.0);

  fix_corr->extend(f2, t2); // two area_box, one move_boxes

  
  // formation at end of Phase 3
  tpb = (p9 + (p5 * 11.0))/12.0;
  tpa = tpb - (lcl_y * box_depth);
  tpc = (p8 + (p4 * 11.0))/12.0;
  tpd = tpc - (lcl_y * box_depth);
  f3 = new Box(tpa, tpb, tpc, tpd);
  t3 = s_time + (e_time - s_time)*(2.0/3.0)*(11.0/12.0);

  fix_corr->extend(f3, t3); // three area_box, two move_boxes

  

  
  // formation at end of Phase 4
  tpb = p5;
  tpa = tpb - (lcl_y * box_depth);
  tpc = p4;
  tpd = tpc - (lcl_y * box_depth);
  f4 = new Box(tpa, tpb, tpc, tpd);
  t4 = s_time + (e_time - s_time)*(2.0/3.0);

  fix_corr->extend(f4, t4); // four area_box, three move_boxes

  cout << " done building fixing corridor" << endl;
  cout << "Fixing corridor is " << *fix_corr << endl << flush;

// ------------------------------------------------------
  cout << "Building flanking corridor ..." << endl;


  // formation at end of Phase 1
  tpb = ((p11 * 2) + (p7))/3.0;
  tpa = tpb - (lcl_y * box_depth);
  tpc = ((p10 * 2) + (p6 ))/3.0;
  tpd = tpc - (lcl_y * box_depth);
  g1 = new Box(tpa, tpb, tpc, tpd);
  cout << "built first box" << endl;
  flank_corr = new Corridor(g1, t1); // one area_box, zero move_boxes

  // formation at end of Phase 2
  tpb = (p11 + (p7*3))/4.0;
  tpa = tpb - (lcl_y * box_depth);
  tpc = (p10 + (p6*3))/4.0;
  tpd = tpc - (lcl_y * box_depth);
  g2 = new Box(tpa, tpb, tpc, tpd);
  flank_corr->extend(g2, t2); // two area_box, one move_boxes

  // formation at end of Phase 3 
  p12 = nearestPoint(p7, end_bx->get_A(), end_bx->get_D());
  p12 = ((p12 * 3) + p7)/4;

  // in the case of skewed boxes, these points can
  // get placed outside the move-corridor. So pull them in.
  tpb = p12;
  if (mv_bx->insideP(p12) == LFalse)
    tpb = nearestPoint(tpb, mv_bx->get_A(), mv_bx->get_B());
  tpa = tpb + (diag * box_depth);
  if (mv_bx->insideP(tpa) == LFalse)
    tpa = nearestPoint(tpa, mv_bx->get_A(), mv_bx->get_B());

  tpc = p6;
  tpd = tpc + (diag * box_depth);
  g3 = new Box(tpa, tpb, tpc, tpd);
  flank_corr->extend(g3, t3); // three area_box, two move_boxes

  box_width = (dist(tpa, tpd) + dist(tpb, tpc))/2.0;


  // note that the flanking force cuts all the
  // way across at 90-degree angle.
  // formation at end of Phase 4

  tpc = p4 + lcl_y * box_depth;
  tpd = tpc + lcl_x * box_depth;
  tpb = tpc + lcl_y * box_width;
  tpa = tpb + lcl_x * box_depth;
  g4 = new Box(tpa, tpb, tpc, tpd);
  flank_corr->extend(g4, t4); // four area_box, three move_boxes

  cout << " done building flanking corridor" << endl;
  cout << "Flanking corridor is " << *flank_corr << endl << flush;

  // I do not check this anymore, as
  // A: I do not use the tthread concept in yacp
  // B: It does not matter to the code if they do cross,
  // C: I could not fix a clash anyway
  //  cout << "Are they clashing? " << clash(fix_corr, flank_corr)<< endl;

 // ------------------------------------------------------
  //  cout << "Building arty boxes" << endl;
  // not now ...
  ab0 = NULL;
  ab1 = NULL;
  ab2 = NULL;
  ab3 = NULL;
  ab4 = NULL;

  // later, run CCS here to do integrated arty/maneuver planning


  // having defined a suitable scheme of maneuver,
  // allocate forces to it and set the PROCESS
  // for crossing this box to be the newly-constructed,
  // corresponding FSM
  // the new FSM should:
  // (a) be the "process" FSM of the current state.
  //  note that this will require us to write the next-higher-
  //  level FSM in end-to-start order
  // (b) spawn subordinate CP's for fixing and flanking forces
  //  as an initial setup action
  // (c) task those CP's to move along the designated corridors
  // (d) reabsorb those CP's when both are in their final states
  

  // having found non-clashing tactical threads, we can set the
  // higher level state's process to be an FSM which will:
  //
  // create subordinate CP's,
  // order them to execute the threads,
  // re-absorb the CP's, and
  // order all sub's to the end box.


  

  cout << "Creating temporary CP for fixing force" << endl;
  cp_fix = new CmndUnit(theSim,
			unit->side,
			unit->currentPos(), 
			GVector(0,0,0)); 
  // the 'new CmndUnit' records it in the sim database of units

  cout << "Creating temporary CP for flanking force" << endl;
  cp_flank = new CmndUnit(theSim,
			unit->side,
			unit->currentPos(), 
			GVector(0,0,0)); 
  // the 'new CmndUnit' records it in the sim database of units


  num_subs = unit->subordinates->size();
  num_fix = ((int) (0.5 + (num_subs * 0.4)));
  num_flank = num_subs - num_fix;
  assert (num_fix < num_subs);
  assert (num_flank < num_subs);

  assert (num_fix > 0);
  assert (num_flank > 0);

  assert (num_subs == (num_fix + num_flank));


  // Arbitrarily slap some units into fixing force.
  // We've already shown how to match units
  // onto a formation and select the best, and
  // similarly better matching could be done here,
  // but I do not do it - YET.

  for (i=0; i<num_fix; i++) {
    cp_fix->add_sub( (*(unit->subordinates))[i] );
  }


  fix_force = cp_fix->subordinates;

  for (i=0; i<num_flank; i++) {
    cp_flank->add_sub (     (*(unit->subordinates))[num_fix + i]  );
  }

  flank_force = cp_flank->subordinates;

  // verify that we did get not miss the last one

  cout << "Making MnvrCP " << cp_fix->getSimEntID() << " the left fixing CP" << endl;
  // what we would like:
  //  cp_fix->allowSubPlanning = LTrue;
  cp_fix->allowSubPlanning = LFalse;
  unit->spawn_sub_cp(cp_fix,   fix_force  );


  cout << "Making MnvrCP " << cp_fix->getSimEntID()<<" the right flanking CP" << endl;
  // what we would like:
  //  cp_flank->allowSubPlanning = LTrue;
  cp_flank->allowSubPlanning = LFalse;
  unit->spawn_sub_cp(cp_flank, flank_force);

  cout << "Creating Fixing corridor_fsm" << endl;
  new_fsm = cp_fix->makeCorridorFSM(convertCorridor(fix_corr));
  cp_fix->setFSM(new_fsm);

  cout << "Creating Flanking corridor_fsm" << endl;
  new_fsm = cp_flank->makeCorridorFSM(convertCorridor(flank_corr));
  cp_flank->setFSM(new_fsm);


  // create FSM machine which will test when
  // when both have moved all the way down, then
  // absorb those temporary CP's and order to final positions.

  tst1 = new Conjunction();
  act1 = new AllActions();

  // UnitInArea will make its own copy of the box
  tmp_tst = new UnitInArea(cp_fix, f4);
  tst1->addPred(tmp_tst);

  // UnitInArea will make its own copy of the box
  tmp_tst = new UnitInArea(cp_flank, g4);
  tst1->addPred(tmp_tst);

  tmp_act = new AbsorbSubCP(unit, cp_fix);
  act1->addAction(tmp_act);

  tmp_act = new AbsorbSubCP(unit, cp_flank);
  act1->addAction(tmp_act);

  tmp_act = new OrderCUnitAcrossBox(unit, NULL, end_bx, 
				    e_time, higher_level_state); 
  act1->addAction(tmp_act);

  cp_goal = (((end_bx->get_A() * 5) + end_bx->get_B()) +
	     ((end_bx->get_D() * 5) + end_bx->get_C()))/12;
 return;
}

// ------------------------------------------------------

void
OrderCUnitAcrossBox::order_left_hook(float, GVector enemy_cg)
{
  //  stubFN("order_left_hook", 1);
  cout << "calling STUB of OrderCUnitAcrossBox::order_left_hook" << endl << flush;
  assert (false); // do not call stubs
  return;
}

// ------------------------------------------------------

#endif


// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
