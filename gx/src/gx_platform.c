// include platform specific implementations

#if defined(WIN32)
#include "platforms/gx_gui.Win.c"

#elif defined(__linux__)
#include "platforms/gx_gui.X11.c"

#else
#error UNSUPPORTED PLATFORM

#endif
