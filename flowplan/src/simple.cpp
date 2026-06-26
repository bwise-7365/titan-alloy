//
// Created by bwise on 6/24/2026 from the GLPK 4.64 manual, section 1.3.1
//

/*
 *
 *   maximize
 * z = 10x1 + 6x2 + 4x3
 *   subject to
 *   x1 +  x2 +  x3 <= 100
 * 10x1 + 4x2 + 5x3 <= 600
 *  2x1 + 2x2 + 6x3 <= 300
 *
 *   where all variables are non-negative
 * x1 => 0; x2 => 0; x3 => 0
 *
 *
 *  GLPK phrases this as opt c*x, Ax = p, bounds on x and p.
*    maximize
*  z = 10x1 + 6x2 + 4x3
*    subject to
* p =   x1 +  x2 +  x3
* q = 10x1 + 4x2 + 5x3
* r =  2x1 + 2x2 + 6x3
*
* and bounds of variables
*
* −∞ < p ≤ 100 , 0 ≤ x1 < +∞
* −∞ < q ≤ 600 , 0 ≤ x2 < +∞
* −∞ < r ≤ 300 , 0 ≤ x3 < +∞
 */

#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <glpk.h>
#include <iostream>

using std::cout;
using std::endl;
using std::flush;

void run_v00() {
 cout << endl << endl;
 glp_prob *lp;
 int ia[1+1000], ja[1+1000];
 double ar[1+1000], z, x1, x2, x3;
 lp = glp_create_prob();
 glp_set_prob_name(lp, "sample");
 glp_set_obj_dir(lp, GLP_MAX);

 glp_add_rows(lp, 3);
 glp_set_row_name(lp, 1, "p");
 glp_set_row_bnds(lp, 1, GLP_UP, 0.0, 100.0);

 glp_set_row_name(lp, 2, "q");
 glp_set_row_bnds(lp, 2, GLP_UP, 0.0, 600.0);

 glp_set_row_name(lp, 3, "r");
 glp_set_row_bnds(lp, 3, GLP_UP, 0.0, 300.0);

 glp_add_cols(lp, 3);
 glp_set_col_name(lp, 1, "x1");
 glp_set_col_bnds(lp, 1, GLP_LO, 0.0, 0.0);
 glp_set_obj_coef(lp, 1, 10.0); // in objective, z

 glp_set_col_name(lp, 2, "x2");
 glp_set_col_bnds(lp, 2, GLP_LO, 0.0, 0.0);
 glp_set_obj_coef(lp, 2, 6.0);

 glp_set_col_name(lp, 3, "x3");
 glp_set_col_bnds(lp, 3, GLP_LO, 0.0, 0.0);
 glp_set_obj_coef(lp, 3, 4.0);

 ia[1] = 1, ja[1] = 1, ar[1] = 1.0; /* a[1,1] = 1 */
 ia[2] = 1, ja[2] = 2, ar[2] = 1.0; /* a[1,2] = 1 */
 ia[3] = 1, ja[3] = 3, ar[3] = 1.0; /* a[1,3] = 1 */
 ia[4] = 2, ja[4] = 1, ar[4] = 10.0; /* a[2,1] = 10 */
 ia[5] = 3, ja[5] = 1, ar[5] = 2.0; /* a[3,1] = 2 */
 ia[6] = 2, ja[6] = 2, ar[6] = 4.0; /* a[2,2] = 4 */
 ia[7] = 3, ja[7] = 2, ar[7] = 2.0; /* a[3,2] = 2 */
 ia[8] = 2, ja[8] = 3, ar[8] = 5.0; /* a[2,3] = 5 */
 ia[9] = 3, ja[9] = 3, ar[9] = 6.0; /* a[3,3] = 6 */
 glp_load_matrix(lp, 9, ia, ja, ar);

 // GLPK format
 glp_write_prob(lp, NULL, "simple_v00.glpk.txt");
 // CPLEX LP format
 glp_write_lp(lp, NULL, "simple_v00.lp.txt");

 glp_simplex(lp, NULL);

 z = glp_get_obj_val(lp);
 x1 = glp_get_col_prim(lp, 1);
 x2 = glp_get_col_prim(lp, 2);
 x3 = glp_get_col_prim(lp, 3);
 printf("\nz = %g; x1 = %g; x2 = %g; x3 = %g\n",
z, x1, x2, x3);
 glp_delete_prob(lp);
 return;
}

// less confusing order, which seems to work just as well.
// The manual says that ...
//   "GLP_MAX means maximization"
//   "GLP_UP means that the row has an upper bound"
//   "GLP_LO means that the column has an [sic] lower bound,"
//
//  Perhaps the abc[1+n] means 'n' actual values in a zero-based array,
// and GLPK ignores the 0-th elements?

void run_v01() {
 cout << endl << endl;
 glp_prob *lp;
 int ia[1+9], ja[1+9];
 double ar[1+9], z, x1, x2, x3;
 lp = glp_create_prob();
 glp_set_prob_name(lp, "sample");
 glp_set_obj_dir(lp, GLP_MAX);

 glp_add_rows(lp, 3);
 glp_set_row_name(lp, 1, "p");
 glp_set_row_bnds(lp, 1, GLP_UP, 0.0, 100.0);

 glp_set_row_name(lp, 2, "q");
 glp_set_row_bnds(lp, 2, GLP_UP, 0.0, 600.0);

 glp_set_row_name(lp, 3, "r");
 glp_set_row_bnds(lp, 3, GLP_UP, 0.0, 300.0);

 glp_add_cols(lp, 3);
 glp_set_col_name(lp, 1, "x1");
 glp_set_col_bnds(lp, 1, GLP_LO, 0.0, 0.0);
 glp_set_obj_coef(lp, 1, 10.0); // in objective, z

 glp_set_col_name(lp, 2, "x2");
 glp_set_col_bnds(lp, 2, GLP_LO, 0.0, 0.0);
 glp_set_obj_coef(lp, 2, 6.0);

 glp_set_col_name(lp, 3, "x3");
 glp_set_col_bnds(lp, 3, GLP_LO, 0.0, 0.0);
 glp_set_obj_coef(lp, 3, 4.0);

 ia[1] = 1, ja[1] = 1, ar[1] = 1.0;  /* a[1,1] =  1 */
 ia[2] = 1, ja[2] = 2, ar[2] = 1.0;  /* a[1,2] =  1 */
 ia[3] = 1, ja[3] = 3, ar[3] = 1.0;  /* a[1,3] =  1 */

 ia[4] = 2, ja[4] = 1, ar[4] = 10.0; /* a[2,1] = 10 */
 ia[5] = 2, ja[5] = 2, ar[5] = 4.0;  /* a[2,2] =  4 */
 ia[6] = 2, ja[6] = 3, ar[6] = 5.0;  /* a[2,3] =  5 */

 ia[7] = 3, ja[7] = 1, ar[7] = 2.0;  /* a[3,1] =  2 */
 ia[8] = 3, ja[8] = 2, ar[8] = 2.0;  /* a[3,2] =  2 */
 ia[9] = 3, ja[9] = 3, ar[9] = 6.0;  /* a[3,3] =  6 */

 glp_load_matrix(lp, 9, ia, ja, ar);

 // GLPK format
 glp_write_prob(lp, NULL, "simple_v01.glpk.txt");
 // CPLEX LP format
 glp_write_lp(lp, NULL, "simple_v01.lp.txt");

 glp_simplex(lp, NULL);

 z = glp_get_obj_val(lp);
 x1 = glp_get_col_prim(lp, 1);
 x2 = glp_get_col_prim(lp, 2);
 x3 = glp_get_col_prim(lp, 3);
 printf("\nz = %g; x1 = %g; x2 = %g; x3 = %g\n",
    z, x1, x2, x3);
 glp_delete_prob(lp);
 return;
}





int main() {
 run_v00(); // original from manual
 run_v01();
 return 0;
}
/* eof */
