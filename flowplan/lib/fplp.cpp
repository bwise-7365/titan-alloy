// Copyright Ben Paul Wise. All Rights Reserved.

#include "flowplanner.h"
#include <glpk.h>


// With the data already loaded, generate the matrix for GLPK,
// and run the solver.
void FlowPlanner::runGLPK(bool verbose) {


    const string outputNameLP = outputBaseName + ".lp.txt";
    const string outputNameLog = outputBaseName + ".log.txt";

    FILE* outputLog = fopen(outputNameLog.c_str(), "w");


    if (verbose) {
        fprintf(outputLog, "Starting GLPK solver\n");
    }

    glp_prob *lp = glp_create_prob();
    glp_set_prob_name(lp,  "flow_cost_min");
    glp_set_obj_dir(lp, GLP_MIN);

    const int arraySize = 1+(nSrc*nDst);

    auto flowNames = vector<char*>(arraySize);

    { // create variable names
        int vNdx = 1;
        for (int i = 1; i <= nSrc; i++) {
            for (int j = 1; j <= nDst; j++) {
                flowNames[vNdx] = newChars(20);
                sprintf(flowNames[vNdx], "f_%02d_%02d", i, j);
                vNdx++;
            }
        }
    }

    glp_add_rows(lp, nSrc + nDst);
    { // add one row per constraint, with name of the slack variable
        int k = 1;
        for (int i = 1; i <= nSrc; i++) {
            char* wd = newChars(6);
            sprintf(wd, "wd_%02d", i);

            glp_set_row_name(lp, k, wd);
            glp_set_row_bnds(lp, k, GLP_UP, 0.0, src[i-1]);

            k++;
        }
        for (int j = 1; j <= nDst; j++) {
            char* rs = newChars(6);
            sprintf(rs, "rs_%02d", j);
            glp_set_row_name(lp, k, rs);
            glp_set_row_bnds(lp, k, GLP_LO, dst[j-1], dst[j-1]);
            k++;

        }
    }

    glp_add_cols(lp, nSrc*nDst);
    { // add one column per structural variable (decision variable?)
        int vNdx = 1;
        for (int i = 0; i < nSrc; i++) {
            for (int j = 0; j < nDst; j++) {
                glp_set_col_name(lp, vNdx, flowNames[vNdx]);
                glp_set_col_bnds(lp, vNdx, GLP_LO, 0.0, 0.0); // 0 ≤ flow_i_j < +∞
                glp_set_obj_coef(lp, vNdx, cost[i][j]); // in objective, z
                vNdx++;
            }
        }
    }


    // The number of rows is nSrc + nDst, one per constraint.
    // The number of columns is nSrc*nDst, one per structural variable.
    int coeffSize = 1 + (nSrc+nDst)*(nSrc*nDst);
    int* ia = new int[coeffSize];
    int* ja = new int[coeffSize];
    double* ar = new double[coeffSize];
    for (int i=0; i < coeffSize; i++) {
        ia[i] = 17;
        ja[i] = 42;
        ar[i] = 3.1416;
    }

    {
        // these should be +1, -1, or mostly zero
        int ndx = 1;
        int row = 1;

        for (int i = 1; i <= nSrc; i++) {
            // constraint on this source
            int j = 1;
            // j, from (m,n), give the column for flow_m_n
            for (int m = 1; m <= nSrc; m++) {
                for (int n = 1; n <= nDst; n++) {
                    double c = 0.0;
                    if (m == i) {
                        c = 1.0;
                    }
                    ia[ndx] = row;  ja[ndx] = j;  ar[ndx] = c;
                    //printf("%3d:  ar[%2d][%2d] = %.2f\n", ndx, ia[ndx], ja[ndx],  ar[ndx]);
                    j++;
                    ndx++;
                }
            }
            row++; // ready for next row, with next constraint
        }


        for (int i = 1; i <= nDst; i++) {
            // constraint on this destination
            int j = 1;
            // j, from (m,n), give the column for flow_m_n
            for (int m = 1; m <= nSrc; m++) {
                for (int n = 1; n <= nDst; n++) {
                    double c = 0.0;
                    if (n == i) {
                        c = 1.0;
                    }
                    ia[ndx] = row;  ja[ndx] = j;  ar[ndx] = c;
                    //printf("%3d:  ar[%2d][%2d] = %.2f\n", ndx, ia[ndx], ja[ndx],  ar[ndx]);
                    j++;
                    ndx++;
                }
            }
            row++; // ready for next row, with next constraint
        }
    }
    cout << flush;

    glp_load_matrix(lp, coeffSize-1, ia, ja, ar);

    // CPLEX LP format
    glp_write_lp(lp, NULL, outputNameLP.c_str());

    // NOTE: This is where it actually runs Simplex
    glp_simplex(lp, NULL);

    if (verbose) {
        double z = glp_get_obj_val(lp);
        fprintf(outputLog, "LP objective: %.4f\n", z);
    }


    double myCost = 0.0;
    vector<vector<double>> myFlow = vector<vector<double>>(nSrc);
    {
        myFlow.resize(nSrc);
        for (int i = 0; i < nSrc; i++) {
            myFlow[i] = vector<double>(nDst);
            myFlow[i].resize(nDst);
        }

        int k = 1;
        for (int i = 1; i <= nSrc; i++) {
            for (int j = 1; j <= nDst; j++) {
                double xij = glp_get_col_prim(lp, k);
                myFlow[i-1][j-1] = xij;
                flow[i-1][j-1] = xij; // copy GLPK's solution so it can be displayed
                k++;
                if (0.0 < xij) {
                    //printf("f(%2d, %2d) = %.2f\n", i, j, xij);
                    myCost = myCost + (cost[i-1][j-1] * xij);
                }
            }
        }
        if (verbose) {
            cout <<endl << flush;
            fprintf(outputLog, "Cost from GLPK: %.4f\n", myCost);
            cout << endl;
            if (nSrc*nDst <= 100*50) {
                fprintf(outputLog, "Flow from GLPK, row = src, col = dst\n");
                showMatrix(outputLog, myFlow);
            }
            cout << flush;
        }
    }

    fclose(outputLog);
    outputLog = nullptr;


    delete [] ia;
    ia = nullptr;
    delete [] ja;
    ja = nullptr;
    delete [] ar;
    ar = nullptr;
    glp_delete_prob(lp);
    lp = nullptr;

    for (int i = 0; i < nSrc*nDst; i++) {
        delete [] flowNames[i];
        flowNames[i] = nullptr;
    }
}

// Copyright Ben Paul Wise. All Rights Reserved.
