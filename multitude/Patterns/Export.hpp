/* COPYRIGHT
 *
 * This file is part of Patterns.
 *
 * Copyright: Helsinki University of Technology, MultiTouch Oy and others.
 *
 * See file "Patterns.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef PATTERNS_EXPORT_HPP
#define PATTERNS_EXPORT_HPP

#ifdef WIN32

// Import by default
#ifdef PATTERNS_EXPORT
#define PATTERNS_API __declspec(dllexport)
#else
#define PATTERNS_API __declspec(dllimport)
#endif

#else
#define PATTERNS_API
#endif  

#endif
