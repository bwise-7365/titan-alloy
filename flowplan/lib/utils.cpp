// Copyright Ben Paul Wise. All Rights Reserved.

#include "flowplanner.h"

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

// Copyright Ben Paul Wise. All Rights Reserved.

