#ifndef DEV_MACRO_H
#define DEV_MACRO_H

#if DEV == 1
    #include <cstdlib>   // For getenv function
    #define AYON_LOCAL_TEST_POINT "AYON_LOCAL_TEST_POINT"
    #define DEV_SWITCH(x, y)      std::getenv(x)
#else
    #define AYON_LOCAL_TEST_POINT ""
    #define DEV_SWITCH(x, y)      y
#endif

#endif   // DEV_MACRO_H
