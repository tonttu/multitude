/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef RADIANT_TYPES_HPP
#define RADIANT_TYPES_HPP

#ifndef WIN32

#include <sys/types.h>

typedef long long llong;
typedef unsigned long long ullong;

typedef unsigned int uint;
typedef unsigned long ulong;

#else

//#include <windows.h>

// #include <unixstuff.h>
typedef signed __int64 llong;
typedef unsigned __int64 ullong;
typedef unsigned int uint;
typedef unsigned long ulong;

#endif	//WIN32

typedef unsigned char uchar;

#endif


