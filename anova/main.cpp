#include <iostream>
#include <random>


int main() {
    const int numCase = 4;
    const int numCntd = 3;
    const int numCong = 3;
    const int numHndr = 3;
    const int numRun = 100;

    auto w0 = new double[numCase];
    auto x0 = new double[numCong];
    auto y0 = new double[numCntd];
    auto z0 = new double[numHndr];
    double scale = 1.0;
    auto data  = new double[numCase][numCong][numCntd][numHndr][numRun];

    std::mt19937 mt{1173};
    for (int i = 0; i < numCase; i++) {
        w0[i] = std::uniform_real_distribution<double>(10.0, 30.0)(mt);
    }


    for (int i = 0; i < numCong; i++) {
        x0[i] = std::uniform_real_distribution<double>(-1.5, +1.5)(mt);
        x0[i] = scale * x0[i];
    }

    for (int i = 0; i < numCntd; i++) {
        y0[i] = std::uniform_real_distribution<double>(-2.0, +2.0)(mt);
        y0[i] = scale * y0[i];
    }

    for (int i = 0; i < numHndr; i++) {
        z0[i] = std::uniform_real_distribution<double>(-1.0, +1.0)(mt);
        z0[i] = scale * z0[i];
    }


    for (int c=0; c<numCase; c++) {
        for (int i=0; i<numCong; i++) {
            for (int j=0; j<numCntd; j++) {
                for (int k=0; k<numHndr; k++) {
                    for (int r=0; r<numRun; r++) {
                        double noise = std::uniform_real_distribution<double>(-1.0, +1.0)(mt);
                        data[c][i][j][k][r] = w0[c] + x0[i] + y0[j] + z0[k] + noise;
                    }
                }

            }
        }
    }

    double mu = 0.0;
    auto w = new double[numCase];
    auto x = new double[numCntd];
    auto y = new double[numCong];
    auto z = new double[numHndr];

    // get the mean
    for (int c=0; c<numCase; c++) {
        for (int i=0; i<numCong; i++) {
            for (int j=0; j<numCntd; j++) {
                for (int k=0; k<numHndr; k++) {
                    for (int r=0; r<numRun; r++) {
                        mu = mu + data[c][i][j][k][r];
                    }
                }
            }
        }
    }
    mu = mu / (numCase * numCntd * numCong * numHndr * numRun);
    printf("mu = %.3f\n", mu);


    auto dvtnFO  = new double[numCase][numCntd][numCong][numHndr][numRun];

    // Get first-order deviations from mean
    for (int c=0; c<numCase; c++) {
        for (int i=0; i<numCong; i++) {
            for (int j=0; j<numCntd; j++) {
                for (int k=0; k<numHndr; k++) {
                    for (int r=0; r<numRun; r++) {
                        dvtnFO[c][i][j][k][r] = data[c][i][j][k][r] - mu;
                    }
                }
            }
        }
    }


    for (int c=0; c<numCase; c++) {
        w[c] = 0.0;
        for (int i=0; i<numCong; i++) {
            for (int j=0; j<numCntd; j++) {
                for (int k=0; k<numHndr; k++) {
                    for (int r=0; r<numRun; r++) {
                        w[c] = w[c] + dvtnFO [c][i][j][k][r];
                    }
                }
            }
        }
        w[c] = w[c] / (numCong * numCntd * numHndr * numRun);
        printf("mu+w[%d] = %6.3f, w0[%d] = %6.3f \n", c, mu+w[c], c, w0[c]);
    }

    for (int i=0; i<numCong; i++) {
        x[i] = 0.0;
        for (int c=0; c<numCase; c++) {
            for (int j=0; j<numCntd; j++) {
                for (int k=0; k<numHndr; k++) {
                    for (int r=0; r<numRun; r++) {
                        x[i] = x[i] + dvtnFO[c][i][j][k][r];
                    }
                }
            }
        }
        x[i] = x[i] / (numCase * numCntd * numHndr * numRun);
        printf("x[%d] = %6.3f, x0[%d] = %6.3f \n", i, x[i], i, x0[i]);
    }

    for (int j=0; j<numCntd; j++) {
        y[j] = 0.0;
        for (int c=0; c<numCase; c++) {
            for (int i=0; i<numCong; i++) {
                for (int k=0; k<numHndr; k++) {
                    for (int r=0; r<numRun; r++) {
                        y[j] = y[j] + dvtnFO[c][i][j][k][r];
                    }
                }
            }
        }
        y[j] = y[j] / (numCase * numCong * numHndr * numRun);
        printf("y[%d] = %6.3f, y0[%d] = %6.3f \n", j, y[j], j, y0[j]);
    }

    for (int k=0; k<numHndr; k++) {
        z[k] = 0.0;
        for (int c=0; c<numCase; c++) {
            for (int i=0; i<numCong; i++) {
                for (int j=0; j<numCntd; j++) {
                    for (int r=0; r<numRun; r++) {
                        z[k] = z[k] + dvtnFO[c][i][j][k][r];
                    }
                }
            }
        }
        z[k] = z[k] / (numCase * numCong * numCntd * numRun);
        printf("z[%d] = %6.3f, z0[%d] = %6.3f \n", k, z[k], k, z0[k]);
    }



    auto dvtnSO  = new double[numCase][numCntd][numCong][numHndr][numRun];

    // Get second-order deviations
    for (int c=0; c<numCase; c++) {
        for (int i=0; i<numCong; i++) {
            for (int j=0; j<numCntd; j++) {
                for (int k=0; k<numHndr; k++) {
                    for (int r=0; r<numRun; r++) {
                        dvtnSO[c][i][j][k][r] = dvtnFO[c][i][j][k][r] - (w[c] + x[i] + y[j] + z[k]);
                    }
                }
            }
        }
    }


    return 0;
}

//  End of file.