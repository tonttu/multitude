/* COPYRIGHT
 *
 * This file is part of Screenplay.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Screenplay.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef SCREENPLAY_EXPORT_HPP
#define SCREENPLAY_EXPORT_HPP

#ifdef WIN32

// Import by default
#ifdef SCREENPLAY_EXPORT
#define SCREENPLAY_API __declspec(dllexport)
#else
#define SCREENPLAY_API __declspec(dllimport)
#endif

#else

#define SCREENPLAY_API

#endif  

#endif
