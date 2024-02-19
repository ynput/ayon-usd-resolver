#ifndef DEV_MACRO_H
#define DEV_MACRO_H

#include "AyonCppApiCrossPlatformMacros.h"
#if JTRACE == 1
    #include "Instrumentor.h"
    #define PerfTimer(x) InstrumentationTimer timer(x);
#else
    #define PerfTimer(x)
#endif

#endif   // DEV_MACRO_H
