// Copyright Ben Paul Wise. All Rights Reserved.

#include "flowplanner.h"



char* newChars(int n) {
    char* s = new char[n];
    for (int i = 0; i < n; i++) {
        s[i] = 0;
    }
    return s;
}

string dateTimeString(time_point<system_clock> ft) {
    time_t fTime = system_clock::to_time_t(ft);
    //char* buff = newChars(30); // 25 should suffice
    //asctime_r(gmtime(&fTime), buff);  // EOL included
    char* buff = asctime(gmtime(&fTime));  // allocates own memory
    string s1 = string(buff);
    int nNdx = (int)(s1.find('\n'));
    string s2 = s1.substr(0, nNdx); // EOL removed
    s1.clear();
    return s2;
}

time_point<system_clock>  displayProgramStart(string appName, string appVersion) {
    time_point<system_clock> st;
    st = system_clock::now();
    string dts = dateTimeString(st);
    if (0 < appName.size()) {
        if (0 < appVersion.size()) {
            cout << "Software version: " << appName.c_str() << " " << " " << appVersion.c_str() << endl;
        }
        else {
            cout << "Software version: " << appName.c_str() << endl;
        }
    }
    cout << "Start time (UTC): " << dts.c_str() << endl;
    dts.clear();
    cout << flush;
    return st;
}

void displayProgramEnd(time_point<system_clock> st) {
    time_point<system_clock> ft;
    ft = system_clock::now();
    duration<double> eTime = ft - st;
    string dts = dateTimeString(ft);
    cout << "Finish time (UTC): " << dts.c_str() << endl;
    dts.clear();
    double duration = eTime.count();
    if (duration < 0.01) {
        printf("Elapsed time: %.6f seconds \n", eTime.count());
    }
    else if (duration < 10.0) {
        printf("Elapsed time: %.4f seconds \n", eTime.count());
    }
    else {
        printf("Elapsed time: %.2f seconds \n", eTime.count());
    }
    cout << flush;
    return;
}


uint64_t msRandom() {

    using std::chrono::microseconds;
    using std::chrono::duration_cast;

    microseconds ms = duration_cast<microseconds>(system_clock::now().time_since_epoch());
    uint64_t s2 = ms.count(); // microseconds since the Unix Epoch
     return s2;
}

tuple<vector<int>, vector<int>, int> balancedSD(const vector<double> &src, const vector<double> &dst) {
    int sSum = 0;
    vector<int> iSrc(src.size());
    for (int i=0; i<src.size(); i++) {
        iSrc[i] = static_cast<int>(src[i]);
        sSum += iSrc[i];
    }

    int dSum = 0;
    vector<int> iDst(dst.size());
    for (int i=0; i<dst.size(); i++) {
        iDst[i] = static_cast<int>(dst[i]);
        dSum += iDst[i];
    }

    while (sSum < dSum) {
        int iter = max_element(iDst.begin(), iDst.end())-iDst.begin();
        iDst[iter] = iDst[iter]-1;
        dSum = dSum -1;
    }

    while (dSum < sSum) {
        int iter = min_element(iDst.begin(), iDst.end())-iDst.begin();
        iDst[iter] = iDst[iter]+1;
        dSum = dSum +1;
    }

    tuple<vector<int>, vector<int>, int>  x = {iSrc, iDst, sSum};
    return x;
}

// Copyright Ben Paul Wise. All Rights Reserved.

