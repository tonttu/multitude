/* COPYRIGHT
 *
 * This file is part of Nimble.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Nimble.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef NIMBLE_EXPORT_HPP
#define NIMBLE_EXPORT_HPP

#include "Radiant/Platform.hpp"

#ifdef RADIANT_MSVC
#pragma warning( disable : 4275 )
#endif

// Import by default
#ifdef NIMBLE_EXPORT
#define NIMBLE_API MULTI_DLLEXPORT
#else
#define NIMBLE_API MULTI_DLLIMPORT
#endif

#endif
