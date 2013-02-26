/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef RADIANT_EXPORT_HPP
#define RADIANT_EXPORT_HPP

#include "Radiant/Platform.hpp"

#ifdef RADIANT_MSVC
#pragma warning( disable : 4275 )
#endif

// Import by default
#ifdef RADIANT_EXPORT
#define RADIANT_API MULTI_DLLEXPORT
#else
#define RADIANT_API MULTI_DLLIMPORT
#endif

#endif
