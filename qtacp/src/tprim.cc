// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. Changes: evector becomes
// std::vector. Not compiled in the build; this file is an older
// variant of the classes (value members, addLast/item list API,
// True/False Logicals) and needs more work before it can be added.
// ------------------------------------------


#ifndef TACTICAL_PRIMITIVES_CC
#define TACTICAL_PRIMITIVES_CC

// ------------------------------------------------------


#include "aaa.h"
#include "frwrdec.h"
#include "struct.h"


#include "tprim.h"
#include "acpsim.h"

#include <vector>

// ------------------------------------------------------


void enqueue_arb_quadrilateral(Box*);
int enqueue_area_boxes;
int enqueue_move_boxes;

//------------------------------------------------------
TThread::TThread()
{
  bounding_box = NULL;
  start_time = 0;
  end_time = 0;

  current_n = 0;
  last_n_updated =0;
}

TThread::~TThread()
{
}



void
TThread::addBox(Box *bx, float st, float et)
{
    boxes.addLast(bx);
    start_times.addLast(st);
    end_times.addLast(et);
    //    current_n++;
}

Logical
TThread::inner_clashes(TThread *t1)
{
  Logical result;
  result = False;
  Box *bx1, *bx2;
  float st1, st2;
  float et1, et2;

  // loop through my boxes
  for (boxes.resetFirst(), 
	 end_times.resetFirst(), 
	 start_times.resetFirst();
       (
	//	(False == result) &&
	(boxes.isEnd()==False) &&
	(end_times.isEnd()==False) &&
	(start_times.isEnd()==False));
       boxes.next(), 
	 end_times.next(), 
	 start_times.next())
    {
      bx1 = boxes.item();
      st1 = start_times.item();
      et1 = end_times.item();
      if (bx1 != NULL)
	// loop through other's boxes
	for (t1->boxes.resetFirst(), 
	       t1->end_times.resetFirst(), 
	       t1->start_times.resetFirst();
	     (
	      //	      (False == result) &&
	      (t1->boxes.isEnd()==False) &&
	      (t1->end_times.isEnd()==False) &&
	      (t1->start_times.isEnd()==False));
	     t1->boxes.next(), 
	       t1->end_times.next(), 
	       t1->start_times.next())
	  {
	    bx2 = t1->boxes.item();
	    st2 = t1->start_times.item();
	    et2 = t1->end_times.item();
	    if (bx2 != NULL)
	      {
		if ((et1 < st2) ||
		    (et2 < st1) ||
		    (box_intersect_p(bx1, bx2) == False))
		  {
		    // no clash
		  }
		else
		  {
		    // clash
		    result = True;
		  }
	      }
	  }
    }
  return result;
}


ostream&
operator << (ostream& s, TThread& tt)
{
  tt.streamout(s);
  return s;
}


// ------------------------------------------------------


Arty_Seq::Arty_Seq() : TThread()
{
    boxes = std::vector<Box*>();
    start_times = std::vector<float>();
    end_times = std::vector<float>();
}
 
Arty_Seq::~Arty_Seq()
{
}


ostream&
Arty_Seq::streamout(ostream& s)
{
    s << "<< Arty_Seq";
    for (boxes.resetFirst(), 
	   end_times.resetFirst(),
	   start_times.resetFirst();
	 (boxes.isEnd() == False) && 
	   (end_times.isEnd() == False) && 
	   (start_times.isEnd() == False);
	 boxes.next(), end_times.next(), start_times.next())
      if (boxes.item() != NULL)
	{
	  s << " (" << *(boxes.item()) << ", " << start_times.item();
	  s << "," << end_times.item() << ") >>";
	}
      else
	{
	  s << " (" << "NULL" << ", " << start_times.item() << ",";
	  s << end_times.item()<< ") >>";
	}

    return s; 
}

// ------------------------------------------------------

Logical 
clash(TThread *t1, TThread *t2)
{
    return t1->clashes(t2);
}

// ------------------------------------------------------


Logical
Arty_Seq::clashes(TThread *tt)
{
  return tt->clashes_with_arty_seq(this);
}



Logical
Arty_Seq::clashes_with_corridor(Corridor* c)
{
  return this->inner_clashes(c);
}


Logical
Arty_Seq::clashes_with_arty_seq(Arty_Seq*)
{
  return False;
}



// ------------------------------------------------------
// q_list is the template,
// p_list is the actual situation
std::vector<GVector*>
slide_and_scale_points(std::vector<GVector*> q_list, std::vector<GVector*> p_list, float &rms)
{
    std::vector<GVector*> r_list;


    GVector *qi, *pi, all_q, all_p, *ri, all_r, slide;
    unsigned n;
    float scale, qp, q_all_p, qq, q_all_q, dx, dy, difference;

    r_list = std::vector<GVector*>();
    all_q = GVector(0,0);
    all_p = GVector(0,0);
    all_r = GVector(0,0);
    qp = 0;
    q_all_p = 0;
    qq = 0;
    q_all_q = 0;
    difference = 0;

    n = q_list.population();
    if (n != p_list.population())
      {
	warnUser ("Slide_and_scale tried to compare two point-sets\n of differing size.\nResults are meaningless and unpredictable.",
	  1);
      }

    for (q_list.resetFirst(), p_list.resetFirst();
	 ((q_list.isEnd() == False) && (p_list.isEnd() == False));
	 q_list.next(), p_list.next())
      {
	  qi = q_list.item();
	  pi = p_list.item();
	  all_q = all_q + *qi;
	  all_p = all_p + *pi;
      }


    for (q_list.resetFirst(), p_list.resetFirst();
	 ((q_list.isEnd() == False) && (p_list.isEnd() == False));
	 q_list.next(), p_list.next())
      {
	  qi = q_list.item();
	  pi = p_list.item();

	  qp      =    qp   + (*qi) * (*pi);
	  q_all_p = q_all_p + (*qi) * all_p;
	  qq      =    qq   + (*qi) * (*qi);
	  q_all_q = q_all_q + (*qi) * all_q;
      }

    scale = ((n * qp) - q_all_p) / ((n * qq) - q_all_q);

    for (q_list.resetFirst(), p_list.resetFirst();
	 ((q_list.isEnd() == False) && (p_list.isEnd() == False));
	 q_list.next(), p_list.next())
      {
	  qi = q_list.item();
	  pi = p_list.item();
	  ri = new GVector(pi->x - (scale * qi->x), pi->y - (scale * qi->y));
	  all_r = all_r + (*ri);
      }

    slide = all_r / n;

    for(q_list.resetFirst(), p_list.resetFirst();
	 ((q_list.isEnd() == False) && (p_list.isEnd() == False));
	 q_list.next(), p_list.next())
       
      {
	  qi = q_list.item();
	  pi = p_list.item();
	  ri = new GVector(slide.x + (scale * qi->x), slide.y + (scale * qi->y));
	  dx = pi->x - ri->x;
	  dy = pi->y - ri->y;
	  difference = difference + (dx*dx + dy*dy);
	  r_list.addLast(ri);
      }

    rms = sqrt(difference/n);

    return r_list;
}


// ------------------------------------------------------

Formation::Formation()
{
}

Formation::~Formation()
{
}

ostream& 
operator << (ostream& s, Formation& f)
{
  s << "<Formation ";
  for (f.points.resetFirst();
       f.points.isEnd()==False;
       f.points.next())
    {
      s << *(f.points.item()) << " ";
    }
  s << ">";
  return s;
}


void
Formation::add_point(GVector *p)
{
  GVector *my_private_point;
  my_private_point = new GVector();
  my_private_point->x = p->x;
  my_private_point->y = p->y;
  points.addLast(my_private_point);
}

void
Formation::add_unit(Unit* u)
{
  units.addLast(u);
}


void
Formation::match_units()
{
  this->move_template_onto_units();
  this->inner_match_units();
}

// the method of doing this is simple:
// compute the mean and stdv of the
// units' x coordinates, and transform
// the template's x coordinates to have
// the same mean and stdv.
// repeat for y.
//
// at the end, the matched_points are in the 
// same order as the original points, so
// the one-to-one mapping is clear

void
Formation::move_template_onto_units()
{
  float n;
  float mean_ux, stdv_ux, mean_uy, stdv_uy;
  float mean_fx, stdv_fx, mean_fy, stdv_fy;
  float norm_x, norm_y;
  GVector mean_u, mean_f, *pt, *pt_ptr, *new_pt;
  std::vector<GVector*> unit_ctrs;
  Unit *u;

  n = (float)(units.population());
  unit_ctrs = std::vector<GVector*>();
  for (units.resetFirst(); units.isEnd()== False; units.next())
    {
      u = units.item();
      pt = u->get_current_loc();
      unit_ctrs.addLast(pt);
    }

  // calculate statistics for units' points
  mean_u = mean_pt(unit_ctrs);
  mean_ux = mean_u.x;
  mean_uy = mean_u.y;
  stdv_ux = x_stdv(unit_ctrs);
  stdv_uy = y_stdv(unit_ctrs);

  for (unit_ctrs.resetFirst();
       unit_ctrs.isEnd() == False;
       unit_ctrs.next())
    {
    // xxx delete (unit_ctrs.item());
    }
  
  // calculate statistics for formations' points
  mean_f = mean_pt(points);
  mean_fx = mean_f.x;
  mean_fy = mean_f.y;
  stdv_fx = x_stdv(points);
  stdv_fy = y_stdv(points);


  moved_points = std::vector<GVector*>();
   for (points.resetFirst(); points.isEnd()== False; points.next())
    {
      pt_ptr = points.item();
      norm_x = (pt_ptr->x - mean_fx)/stdv_fx;  // normalized x coord
      norm_y = (pt_ptr->y - mean_fy)/stdv_fy;  // normalized y coord

      new_pt = new GVector(norm_x * stdv_ux + mean_ux,
		       norm_y * stdv_uy + mean_uy);
      moved_points.addLast(new_pt);
    }
   return;
}

// From before, the matched_points are in the 
// same order as the original points. At the
// end of inner_match_units, the units are
// now also matched up one-to-one with both
// the original points and the moved points, so
// the one-to-one mapping is clear.
//
// note, this is a greedy algorithm.

void
Formation::inner_match_units()
{
  GVector *pt, *u_pos;
  Unit *u, *u_closest;
  float dstnc, dstnc_closest;

  u_closest = NULL;
  matched_units = std::vector<Unit*>();

  for (moved_points.resetFirst(); 
       moved_points.isEnd()== False; 
       moved_points.next())
    {
      pt = moved_points.item();
      dstnc_closest = -1;  // an impossible value
      for (units.resetFirst(); units.isEnd()== False; units.next())
	{
	  u = units.item();
	  if (matched_units.contains(u) == False)
	    {
		u_pos = u->get_current_loc();
		dstnc = dist(*u_pos, *pt);
		// xxx delete u_pos;
		if ((dstnc_closest < 0) || (dstnc < dstnc_closest))
		  {
		      dstnc_closest = dstnc;
		      u_closest = u;
		  }
	    }
	}
      // now "u_closest" is the closest unassigned unit to "pt"
      matched_units.addLast(u_closest);
    }
  // now, the units in "matched_units" are 1-to-1 
  // matched with "moved_points"
  return;
}


std::vector<GVector*> 
Formation::get_assigned_positions()
{
  return moved_points;
}



std::vector<GVector*> 
Formation::get_original_positions()
{
  return points;
}



std::vector<Unit*> 
Formation::get_assigned_units()
{
  return matched_units;
}

// ------------------------------------------------------

Corridor::Corridor() : TThread()
{
    area_boxes = std::vector<Box*>();
    end_times = std::vector<float>();
    start_times = std::vector<float>();
    boxes = std::vector<Box*>();
}


Corridor::~Corridor()
{
}

// note how the first Box must be oriented: A->B is
// the right edge, B->C is the far edge,
// C->D is the left edge, and D->A is the base edge.
//
Corridor::Corridor(Box* bx1, float et) : TThread()
{
  Box *bx2;
  area_boxes = std::vector<Box*>();
  boxes = std::vector<Box*>();
  end_times = std::vector<float>();
  start_times = std::vector<float>();
  
  bx2 = new Box(bx1->get_A(),bx1->get_B(),bx1->get_C(),bx1->get_D());
  area_boxes.addLast(bx2);
  end_times.addLast(et);
  boxes.addLast(NULL);      // first element is meaningless!
  start_times.addLast(-1);  // first element is meaningless!
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
// we should add random permutation of labels,
// as a debugging measure to ensure we don't
// accidentally depend on it!

void 
Corridor::extend(Box *orig_area, float et)
{
  int i;
  Box *area, *bx1, *bx2;
  Logical fixed;
  GVector pa, pb, pc, pd; // proposed new points

  area = new Box(orig_area->get_A(), orig_area->get_B(), 
		 orig_area->get_C(), orig_area->get_D());
//   cout << "PID is " << getpid() << "%5 (used in rotation)" << endl;
//   // Semi-random rotation
//   for (i=0; i<(getpid()%5);i++)
//     area->d_permute_ccw();
  bx1 = area_boxes.item(); // assumes we are at the end!
  pa = bx1->get_B();
  pb = area->get_B();
  pc = area->get_C();
  pd = bx1->get_C();
  fixed = True;

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

  if (True==probe_box_integrity(pa, pb, pc, pd))
    { // we are in the easy, standard case
      fixed = True;
    }
  else // trouble looms
    {
      fixed = False;
      cout << endl << "Standard construction of move-box gives ill-formed" << endl;

      cout << "Last box currently in corridor is " << *bx1 << endl;
      cout << "Trying to add box " << *area << endl;
      
      if (True == box_intersect_p(bx1, area))
	{ // I can't fix this (but it may not be a problem!)
	  croak("Corridor::extend(Box*, float) area_boxes intersect", False);
	}
      else
	{ // possibly fixable
	  croak("Corridor::extend(Box*, float) needs to patch move_box", True);
	  // make some effort to reset pa, pb, pc, pd
	  if ((fixed == False) && (same_side_p(pa,pc,  pb,pd) == True))
	    // c pushed in. if angle from b to c to d is acute, this fails
	    {
	      pd = nearest_point(pc, pa, pd);
	      fixed = True; // I hope!
	    }
	  if ((fixed == False) && (same_side_p(pb,pd,  pa,pc) == True))
	    // b pushed in. if angle from c to b to a is acute, this fails
	    {
	      pa = nearest_point(pb, pd, pa);
	      fixed = True; // I hope!
	    }
	}
    }

  // if all efforts to patch things fail, it will croak building this box.
  bx2 = new Box(pa, pb, pc, pd);


  // clean up the actual area_box used, compared to that given.
  //  area->set_A(nearest_point(area->get_A(), pa, pb));
  //  area->set_D(nearest_point(area->get_D(), pc, pd));
  //  area->check_integrity(); // just to be sure!

  area_boxes.addLast(area);
  boxes.addLast(bx2);
  start_times.addLast(end_times.item());
  end_times.addLast(et);
  //    current_n++;

  if (end_times.item() < start_times.item())
    croak("Corridor::extend(Box*, float): end time after start time", False);

  if (enqueue_area_boxes)
      enqueue_arb_quadrilateral(area);
  if (enqueue_move_boxes)
      enqueue_arb_quadrilateral(bx2);
  return;
}

ostream& 
Corridor::streamout(ostream& s)
{
  Box *bx;
  float et;
  s << "<<Corridor:";

  for (area_boxes.resetFirst(), end_times.resetFirst();
       (area_boxes.isEnd() == False) && (end_times.isEnd() == False);
       area_boxes.next(), end_times.next())
    {
      bx = area_boxes.item();
      et = end_times.item();
      s << "  ( " << *bx << ", " << et << " )" ;
    }

  s << " >>";
  return s;
}

// ------------------------------------------------------


Logical
Corridor::clashes(TThread* tt)
{
  return tt->clashes_with_corridor(this);
}



Logical
Corridor::clashes_with_corridor(Corridor* c)
{
  return this->inner_clashes(c);
}


Logical
Corridor::clashes_with_arty_seq(Arty_Seq* as)
{
  return as->clashes_with_corridor(this);
}



// ------------------------------------------------------


UnitNearPoint::UnitNearPoint() : Predicate()
{
}

UnitNearPoint::~UnitNearPoint()
{
}

UnitNearPoint::UnitNearPoint(Unit *mu, GVector pnt, float d) : Predicate()
{
    force = mu;
    point = pnt;
    critical_dist = d;

    //    cout << "Built a test if unit " << force->name;
    //    cout << " is within " << critical_dist;
    //    cout << " of the point " << point << endl;
}

Logical
UnitNearPoint::eval()
{
    Logical rslt;
    GVector *f_pos = force->get_current_loc();
    if (dist(*(f_pos), point) <= critical_dist)
      rslt = True;
    else
      rslt = False;

    cout << "Tested if unit " << force->unit_id << " at point ";
    cout << *(f_pos) << " is within ";
    cout << critical_dist << " of the point " << point << endl;
    cout << "Result was " << rslt << endl;
    // xxx delete f_pos;
    return rslt;
}

// ------------------------------------------------------

UnitInArea::UnitInArea() : Predicate()
{
}


UnitInArea::~UnitInArea()
{
}


UnitInArea::UnitInArea(Unit* u, Box* b) : Predicate()
{
  force = u;
  area = b;
}

Logical
UnitInArea::eval()
{
  return force->inAreaP(area);
}

// ------------------------------------------------------

EnemyInArea::EnemyInArea() : Predicate()
{
}


EnemyInArea::~EnemyInArea()
{
}


EnemyInArea::EnemyInArea(Unit* u, Box* b, float ms) : Predicate()
{
  unit = u;
  my_side = unit->side;     // look for units which oppose this side
  area = b;                 // look in this area
  assert (ms > 0);
  assert (unit != NULL);
  min_size = ms;            // return True if opponent bigger than this found
}

Logical
EnemyInArea::eval()
{ 
  GVector mean_u;
  float opp_strength;
  Logical result = False;

  opp_strength = 0.0;

//   cout << "Scanning for opponents of side " <<my_side;
//   cout << " in area" << endl;

  unit->enemiesInArea(area , opp_strength, mean_u);
//   cout << "Enemy strength in box is " << opp_strength;
//   cout << " while threshold is " << min_size << endl;

  if (opp_strength >= min_size)
    {
//       cout << "Enemy CG is at " << mean_u << endl;
      result = True;
    }
  return result; 
}

// ------------------------------------------------------

TimePassed::TimePassed()
{
}

TimePassed::~TimePassed()
{
}

TimePassed::TimePassed(float tm, BPW_DES *rs)
{
    time = tm;
    reference_sim = rs;

    //    cout << "Built a test if time " << time << " has passed" << endl;
}

Logical
TimePassed::eval()
{
    Logical rslt;
    if (reference_sim->clock() >= time)
      rslt = True;
    else
      rslt = False;

    cout << "At time " << reference_sim->clock();
    cout << ", tested if time " << time << " has passed" << endl;
    cout << "Result was " << rslt << endl;
    return rslt;
}


// ------------------------------------------------------

OrderResUnitToPoint::OrderResUnitToPoint() : Action()
{
  unit = NULL;
  point = GVector(0,0);
  arrival_deadline = 0;
}

OrderResUnitToPoint::~OrderResUnitToPoint()
{
}

OrderResUnitToPoint::OrderResUnitToPoint(ResUnit* ru, GVector p, float t)
{
    unit = ru;
    point = p;
    arrival_deadline = t;
}


void
OrderResUnitToPoint::apply()
{
    // do not set his objective.
    unit->set_dsrd_loc(point);

    cout << "Ordering unit " << unit->unit_id;
    cout << " to go to point " << point;
    cout << " (objective remains " << unit->objective << ")" << endl;

// resunit should directly go for this point!
//    unit->receive_move_order(point, arrival_deadline);
}

// ------------------------------------------------------

OrderResUnitEngageEnemyInArea::OrderResUnitEngageEnemyInArea() : Action()
{
  unit = NULL;
  area = NULL;
}

OrderResUnitEngageEnemyInArea::~OrderResUnitEngageEnemyInArea()
{
}

OrderResUnitEngageEnemyInArea::OrderResUnitEngageEnemyInArea(ResUnit* ru, 
							     Box* a) : Action()
{
  unit = ru;
  area = a;
}

void
OrderResUnitEngageEnemyInArea::apply()
{
  float perceived_strength;
  GVector enemy_cg;
  unit->enemiesInArea(area, perceived_strength, enemy_cg);
  assert(perceived_strength > 0);  // it better be!
  unit->set_dsrd_loc(enemy_cg);

  cout << "Ordering unit " << unit->unit_id;
  cout << " to attack enemy_cg at " << enemy_cg;
  cout << " (objective remains " << unit->objective << ")" << endl;

  return;
}



// ------------------------------------------------------

OrderUnitToArea::OrderUnitToArea() : Action()
{
  unit = NULL;
  area = NULL;
  arrival_deadline = 0;
}

OrderUnitToArea::OrderUnitToArea(Unit *u, Box *b, float t) : Action()
{
  unit = u;
  area = b;
  arrival_deadline = t;
}

void
OrderUnitToArea::apply()
{
    // do not set his objective.
    cout << "Ordering unit " << unit->unit_id;
    cout << " to go to area " << area << "(not really)";
    cout << " (objective remains " << unit->objective << ")" << endl;


}

OrderUnitToArea::~OrderUnitToArea()
{
}
// ------------------------------------------------------

OrderUnitAlongCorridor::OrderUnitAlongCorridor() : Action()
{
}


OrderUnitAlongCorridor::~OrderUnitAlongCorridor()
{
}


OrderUnitAlongCorridor::OrderUnitAlongCorridor(Unit *u, Corridor *c)
  : Action()
{
  unit = u;
  corridor = c;
}

void
OrderUnitAlongCorridor::apply()
{
  FSM *new_fsm;
  
  new_fsm = unit->makeCorridorFSM(corridor);

  // xxx delete corridor; // all done with it
  corridor = NULL;

  unit->setFSM(new_fsm);
  return;
}

// ------------------------------------------------------
OrderUnitDefendAreas::OrderUnitDefendAreas() : Action()
{
}

OrderUnitDefendAreas::~OrderUnitDefendAreas()
{
}

OrderUnitDefendAreas::OrderUnitDefendAreas(Unit* u, float t,
					   Box* wait_area, 
					   Box* response_area) : Action()
{
  unit = u;
  waitArea = wait_area;
  responseArea = response_area;
  theta = t;
}

void
OrderUnitDefendAreas::apply()
{
  FSM *new_fsm;
  new_fsm = unit->makeDefendAreasFSM(waitArea,responseArea,theta);
  unit->setFSM(new_fsm);
  return;
}

// ------------------------------------------------------


SpawnSubCP::SpawnSubCP() : Action()
{
}

SpawnSubCP::~SpawnSubCP()
{
}

SpawnSubCP::SpawnSubCP(CmndUnit *pcu, 
		       std::vector<Unit*> utr,
		       CmndUnit *tcu) : Action()
{
  parent_unit = pcu;
  units_to_reorg = utr;
  tmp_cu = tcu;
}


void
SpawnSubCP::apply()
{
  parent_unit->spawn_sub_cp(tmp_cu, units_to_reorg);
  return;
}

// ------------------------------------------------------

AbsorbSubCP::AbsorbSubCP() : Action()
{
}


AbsorbSubCP::~AbsorbSubCP()
{
}

AbsorbSubCP::AbsorbSubCP(CmndUnit *pu, CmndUnit *su) : Action()
{
  parent_unit = pu;
  sub_unit = su;
}

void
AbsorbSubCP::apply()
{
  parent_unit->absorb_sub_cp(sub_unit);
  return;
}

// ------------------------------------------------------



UnitGrouping::UnitGrouping()
{
  my_units = std::vector<Unit*>();
}


UnitGrouping::~UnitGrouping()
{
}

void
UnitGrouping::addUnit(Unit *unt)
{
  my_units.addLast(unt);
}


//------------------------------------------------------

UnitGroupingScript::UnitGroupingScript()
{
  my_t_threads = std::vector<TThread*>();
}

UnitGroupingScript::~UnitGroupingScript()
{
}

void
UnitGroupingScript::addTThread(TThread *thrd)
{
  my_t_threads.addLast(thrd);
}

std::vector<TThread*>
UnitGroupingScript::get_my_t_threads()
{
  std::vector<TThread*> rslt;
  TThread* tth;
  rslt = std::vector<TThread*>();
  for (my_t_threads.resetFirst();
       my_t_threads.isEnd() == False;
       my_t_threads.next())
    {
      tth = my_t_threads.item();
      rslt.addLast(tth);
    }
  return rslt;
}

Logical 
clash(UnitGroupingScript *ugs1, UnitGroupingScript *ugs2)
{
  Logical rslt = False;
  TThread *tt1, *tt2;
  
  for (ugs1->my_t_threads.resetFirst();
       (ugs1->my_t_threads.isEnd() == False)&&(rslt==False);
       ugs1->my_t_threads.next())
    {
      tt1 = ugs1->my_t_threads.item();
      for (ugs2->my_t_threads.resetFirst();
	   (ugs2->my_t_threads.isEnd() == False)&&(rslt==False);
	   ugs2->my_t_threads.next())
	{
	  tt2 = ugs2->my_t_threads.item();
	  if (clash(tt1, tt2) == True)
	    {
	      rslt = True;
	    }
	}
    }
  return rslt;
}

// ------------------------------------------------------

#endif

// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
