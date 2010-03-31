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

#ifndef RADIANT_EXPORT_HPP
#define RADIANT_EXPORT_HPP

#ifdef WIN32

#pragma warning( disable : 4275 )

// Import by default
#ifdef RADIANT_EXPORT
#define RADIANT_API __declspec(dllexport)
#else
#define RADIANT_API __declspec(dllimport)
#endif

#else
#define RADIANT_API
#endif

#endif
