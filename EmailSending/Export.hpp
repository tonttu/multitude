/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef EMAIL_EXPORT_HPP
#define EMAIL_EXPORT_HPP

#include "Radiant/Platform.hpp"

// Import by default
#ifdef EMAIL_EXPORT
#define EMAIL_API MULTI_DLLEXPORT
#else
#define EMAIL_API MULTI_DLLIMPORT
#endif

#define ALWAYS_EXPORT MULTI_DLLEXPORT

#endif