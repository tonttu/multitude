/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef VALUABLE_EXPORT_HPP
#define VALUABLE_EXPORT_HPP

#include "Radiant/Platform.hpp"

// Import by default
#ifdef VALUABLE_EXPORT
#define VALUABLE_API DLLEXPORT
#else
#define VALUABLE_API DLLIMPORT
#endif

#endif
