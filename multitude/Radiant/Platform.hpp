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

#ifndef RADIANT_PLATFORM_HPP
#define RADIANT_PLATFORM_HPP

// C++0x check
#if __cplusplus > 199711L
  #define RADIANT_CPP0X 1
#endif

// Discover the architecture
#if defined(__amd64__) || defined(_M_X64)
#   define RADIANT_AMD64 1
#elif defined (__i386__) || defined (_M_IX86)
#   define RADIANT_X86 1
#elif defined (__ia64__) || defined (_M_IA64)
#   define RADIANT_IA64 1
#endif

// Discover the compiler
#if defined (__clang__)
#   define RADIANT_CLANG 1
#elif defined (__GNUC__)
#   define RADIANT_GNUC 1
#elif defined (_MSC_VER)
#   define RADIANT_MSVC 1
#endif

//
// Detect OSX
// 
#if defined (__APPLE__) && defined (__MACH__)
#   define RADIANT_OSX 1
#   define RADIANT_UNIX 1
//
// Detect Windows
// 
#elif defined(_WIN32)
#	include <yvals.h>
#   define RADIANT_WINDOWS 1
#   ifdef _WIN64
#     define RADIANT_WIN64 1
#   else
#     define RADIANT_WIN32 1
#   endif
#   ifndef _HAS_TR1
#     error "Compiler TR1 support was not found. Please install Visual Studio 2008 Service Pack 1 or use a newer compiler."
#   endif
//
// Detect linux
// 
#elif __linux__
#   define RADIANT_LINUX 1
#   define RADIANT_UNIX 1
#else
//
// Unsupported
// 
#   error "Unsupported platform!"
#endif

#endif
