#ifndef PLATFORM_H
#define PLATFORM_H

#include <Luminous/MultiHead.hpp>

/* The platform specific C API to handle window creation

*/

// Called first, before any window creations
int poshPlatformInit();

// Called last, usually from atexit()
int poshPlatformTerminate();

// Create a window
// Can't use MultiHead::Window here because it won't work with Objective C
int poshWindowCreate(const Luminous::MultiHead::Window & wc);

#endif // PLATFORM_HPP
