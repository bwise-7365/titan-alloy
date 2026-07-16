//-------------------------------------------------
//  Copyright Ben Paul Wise. All Rights Reserved.
//-------------------------------------------------
// System-specific functions, for Windows-like systems.
// Vendored from pershing/aaa for the qtacp port; the lean-and-mean
// macros keep windows.h from defining min/max macros that break
// the standard library under MSVC.
//-------------------------------------------------

#include "aaa.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <process.h>     // get process ID stuff
#include <winsock2.h>    // gets time stuff

namespace AAA {


unsigned long int aaaTime() {
      SYSTEMTIME st;
      GetSystemTime(&st);
      unsigned long msTime = (1000 * st.wSecond) + st.wMilliseconds;
      return msTime;  // obviously milliseconds
	  }


unsigned long int aaaPID() {
     return _getpid();
	 }


}

//-------------------------------------------------
//  Copyright Ben Paul Wise. All Rights Reserved.
//-------------------------------------------------
