/* COPYRIGHT
 *
 * This file is part of Resonant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Resonant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef RESONANT_EXPORT_HPP
#define RESONANT_EXPORT_HPP

#include "Radiant/Platform.hpp"

// Import by default
#ifdef RESONANT_EXPORT
#define RESONANT_API MULTI_DLLEXPORT
#else
#define RESONANT_API MULTI_DLLIMPORT
#endif

#endif
