#pragma once
//
// PRIVATE HEADER. This is the only file in modcurses that names a curses
// header, and exactly one translation unit (curses_terminal.cpp) includes it.
// If a second file ever includes this, the firewall is broken and the macro
// leakage that plagued CPPurses (border, getch, ...) comes back.
//
// Every #define below is load-bearing and was diagnosed the hard way; see
// BUILD_NOTES section 2.
//
#if !defined(MODCURSES_CURSES_SHIM_OWNER)
#error "curses_shim.hpp is private to the backend TU (define MODCURSES_CURSES_SHIM_OWNER)"
#endif

#if defined(_WIN32)

// PDCurses ships getmouse(void); only with NCURSES_MOUSE_VERSION defined does
// curses.h map getmouse(x) -> nc_getmouse(MEVENT*), the ncurses-shaped call.
#ifndef NCURSES_MOUSE_VERSION
#define NCURSES_MOUSE_VERSION 2
#endif

// vcpkg builds the PDCurses DLL with WIDE=Y UTF8=Y, but curses.h hides the
// wide API (cchar_t, setcchar, wadd_wchnstr, wget_wch, ...) unless the
// CONSUMER asks for it. Never probe for wide support with #ifdef add_wchstr:
// on PDCurses it is a function, not a macro, and CPPurses' probe silently
// selected a broken fallback that rendered the entire UI black-on-black.
#ifndef PDC_WIDE
#define PDC_WIDE
#endif
#ifndef PDC_FORCE_UTF8
#define PDC_FORCE_UTF8
#endif

#include <curses.h>

#else  // POSIX: the ncursesw variant, selected by CMake via pkg-config

#include <ncurses.h>

#endif

// curses.h defines these as macros and they collide with std:: and with our
// own member names. Nothing downstream of this header may use them.
#ifdef border
#undef border
#endif
#ifdef erase
#undef erase
#endif
#ifdef clear
#undef clear
#endif
#ifdef move
#undef move
#endif
#ifdef refresh
#undef refresh
#endif
#ifdef timeout
#undef timeout
#endif
#ifdef instr
#undef instr
#endif
