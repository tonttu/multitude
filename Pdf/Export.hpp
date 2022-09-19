/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#pragma once

#include <Radiant/Platform.hpp>

#ifdef PDF_EXPORT
#define PDF_API MULTI_DLLEXPORT
#else
#define PDF_API MULTI_DLLIMPORT
#endif
