/* COPYRIGHT
 *
 * This file is part of Poetic.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Poetic.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef POETIC_EXPORT_HPP
#define POETIC_EXPORT_HPP

#include "Radiant/Platform.hpp"

// Import by default
#ifdef POETIC_EXPORT
#define POETIC_API MULTI_DLLEXPORT
#else
#define POETIC_API MULTI_DLLIMPORT
#endif

#endif
