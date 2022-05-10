/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
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

typedef signed __int64 llong;
typedef unsigned __int64 ullong;
typedef unsigned int uint;
typedef unsigned long ulong;

#endif	//WIN32

typedef unsigned char uchar;

#endif


