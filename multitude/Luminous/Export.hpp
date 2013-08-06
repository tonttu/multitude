/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_EXPORT_HPP
#define LUMINOUS_EXPORT_HPP

#include "Radiant/Platform.hpp"

// Import by default
#ifdef LUMINOUS_EXPORT
#define LUMINOUS_API MULTI_DLLEXPORT
#else
#define LUMINOUS_API MULTI_DLLIMPORT
#endif

#endif
