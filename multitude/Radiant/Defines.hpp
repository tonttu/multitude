/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_DEFINES_HPP
#define RADIANT_DEFINES_HPP

#if defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ < 5) || __GNUC__ < 4)
#define MULTI_ATTR_DEPRECATED(description, f) f __attribute__ ((deprecated))
#elif defined(__GNUC__) || defined(__clang__)
#define MULTI_ATTR_DEPRECATED(description, f) f __attribute__ ((deprecated(description)))
#elif defined(_MSC_VER)
#define MULTI_ATTR_DEPRECATED(description, f) __declspec(deprecated(description)) f
#else
#define MULTI_ATTR_DEPRECATED(description, f) f
#endif


#endif // DEFINES_HPP
