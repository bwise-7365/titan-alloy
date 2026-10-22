// Copyright Ben Paul Wise. All Rights Reserved.
//
//
#ifndef FP_UTILS_H
#define FP_UTILS_H



#include <chrono>
#include <iostream>
#include <ostream>
#include <random>
#include <vector>

#include <tuple>

using std::cout;
using std::endl;
using std::flush;
using std::vector;
using std::pair;
using std::string;
using std::tuple;
using std::chrono::duration;
using std::chrono::time_point;
using std::chrono::system_clock;

namespace Utils {
    time_point<system_clock>  displayProgramStart(string appName = "", string appVersion = "");
    void displayProgramEnd(time_point<system_clock> st);

    uint64_t msRandom(); // microseconds since the Unix Epoch, NOT scrambled

    constexpr uint64_t dSeed = 0xFE69A87450C4301C; // still my favorite

    // make empty C-style strings
    char* newChars(int n);

    tuple<vector<int>, vector<int>, int> balancedSD(const vector<double> &src, const vector<double> &dst);

    void showMatrix(FILE* file, const int nRows, const int nCols, const vector<vector<double>> &m);
}

#endif // FP_UTILS_H

// Copyright Ben Paul Wise. All Rights Reserved.
