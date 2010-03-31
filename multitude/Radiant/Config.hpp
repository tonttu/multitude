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

#ifndef RADIANT_CONFIG_H
#define RADIANT_CONFIG_H

#include <Radiant/Endian.hpp>

/// @todo remove this file

// Version number:
#define RADIANT_VERSION_MAJOR 0
#define RADIANT_VERSION_MINOR 2
#define RADIANT_VERSION_SUBMINOR 10

// Is our RADIANT sample float or double:
#define RADIANT_SAMPLE_IS_FLOAT 1

#if RADIANT_SAMPLE_IS_FLOAT == 1
typedef float RADIANT_Sample;
#define RADIANT_ASF_SAMPLE RADIANT_ASF_FLOAT32

#else 
typedef double RADIANT_Sample;
#define RADIANT_ASF_SAMPLE RADIANT_ASF_FLOAT64

#endif

#define RADIANT_HAVE_PTHREAD 1
// #define RADIANT_HAVE_PWCIOCTL_H @HAVE_PWCIOCTL_H@

#endif
