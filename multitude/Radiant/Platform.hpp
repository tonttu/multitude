/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */
#ifndef RADIANT_PLATFORM_HPP
#define RADIANT_PLATFORM_HPP

// C++11 check
#if __cplusplus > 199711L || defined(__GXX_EXPERIMENTAL_CXX0X__)
  #define RADIANT_CXX11 1
#endif

//////////////////////////////////////////////////////////////////////////
// Discover the architecture
//////////////////////////////////////////////////////////////////////////
#if defined (__aarch64__)
#   define RADIANT_ARM64 1
#elif defined(__amd64__) || defined(_M_X64)
#   define RADIANT_AMD64 1
#elif defined (__i386__) || defined (_M_IX86)
#   define RADIANT_X86 1
#elif defined (__ia64__) || defined (_M_IA64)
#   define RADIANT_IA64 1
#endif

// Discover debug builds
#if defined NDEBUG
#   define RADIANT_RELEASE 1
#else
#   define RADIANT_DEBUG 1
#endif


//////////////////////////////////////////////////////////////////////////
// Discover the platform
//////////////////////////////////////////////////////////////////////////
//
// Detect OSX
//
#if defined (__APPLE__)
// && defined (__MACH__)
#   define RADIANT_OSX 1
#   define RADIANT_UNIX 1

#if defined(__IPHONE_OS_VERSION_MIN_REQUIRED)
# define RADIANT_IOS 1
#endif
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


//////////////////////////////////////////////////////////////////////////
// Discover the compiler
//////////////////////////////////////////////////////////////////////////
//
// Detect LLVM/CLANG
//
#if defined (__clang__)
#   define RADIANT_CLANG 1
#   define MULTI_DLLEXPORT __attribute__((visibility("default")))
#   define MULTI_DLLIMPORT __attribute__((visibility("default")))

// Test if we have override
#   if !__has_feature(cxx_override_control)
#     define MULTI_NO_OVERRIDE
#     define MULTI_NO_FINAL
#   endif

// Test if we have alignas
#   if !__has_feature(cxx_alignas)
#     define MULTI_NO_ALIGNAS
#   endif
// 
// Detect GNU GCC/G++
//
#elif defined (__GNUC__)
#   define RADIANT_GNUC 1
#   define MULTI_DLLEXPORT __attribute__((visibility("default")))
#   define MULTI_DLLIMPORT __attribute__((visibility("default")))

//  Override and final are GCC4.7 and up features when compiling code as c++0x/c++11
#   if !defined(RADIANT_CXX11) || ((__GNUC__ == 4 && __GNUC_MINOR__ < 7) || __GNUC__ < 4)
#     define MULTI_NO_OVERRIDE
#     define MULTI_NO_FINAL
#   endif

#   if (__GNUC__ == 4 && __GNUC_MINOR__ >= 7) || __GNUC__ >= 5
// Check if deleted constructors are available in the compiler
#     define RADIANT_DELETED_CONSTRUCTORS 1
#   endif

// Current g++ does not have alignas
#   define MULTI_NO_ALIGNAS

#   if !defined(RADIANT_CXX11)
#     define nullptr 0
#   endif

//
// Detect Intel C++
//
#elif defined (__INTEL_COMPILER)
#   define RADIANT_ICC 1

#   define MULTI_NO_FINAL
#   define MULTI_NO_ALIGNAS

#  if RADIANT_WINDOWS
#   define MULTI_DLLEXPORT __declspec(dllexport)
#   define MULTI_DLLIMPORT __declspec(dllimport)
#  else
#   define MULTI_DLLEXPORT __attribute__((visibility("default")))
#   define MULTI_DLLIMPORT __attribute__((visibility("default")))
#  endif

//
// Detect Microsoft Visual C++
//
#elif defined (_MSC_VER)
#   define RADIANT_MSVC 1

#   if _MSC_VER < 1600
#     error "Unsupported compiler: Must have Visual Studio 2010 or newer"
#   elif _MSC_VER == 1600
#     define RADIANT_MSVC10
#   elif _MSC_VER == 1700
#     define RADIANT_MSVC11
#   endif

// Current MSVC does not have alignas
#   define MULTI_NO_ALIGNAS

#   define MULTI_DLLEXPORT __declspec(dllexport)
#   define MULTI_DLLIMPORT __declspec(dllimport)

// warning C4251: class X needs to have dll-interface to be used by clients of class Y
#   pragma warning(disable:4251)
// Inconsistent DLL linking
#   pragma warning(error:4273)
// default template arguments are only allowed on a class template
#   pragma warning(disable:4519)
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
#if !defined (MULTI_NO_ALIGNAS)
#  define MULTI_ALIGNED(type, var, alignment) type alignas(alignment) var;
#else
#  if defined (RADIANT_MSVC)
#    define MULTI_ALIGNED(type, var, alignment) type __declspec(align(alignment)) var
#  elif defined (RADIANT_GNUC)
#    define MULTI_ALIGNED(type, var, alignment) type __attribute__((aligned(alignment)) var
#  else
#    define MULTI_ALIGNED(type, var, alignment) type var
#  endif
#endif
#undef MULTI_NO_ALIGNAS

#if !defined(MULTI_NO_OVERRIDE)
#  define OVERRIDE override
#else
#  define OVERRIDE
#endif
#undef MULTI_NO_OVERRIDE


#if !defined(MULTI_NO_FINAL)
#  ifdef RADIANT_MSVC10
#    define FINAL sealed
#  else
#    define FINAL final
#  endif
#else
#  define FINAL
#endif
#undef MULTI_NO_FINAL

//////////////////////////////////////////////////////////////////////////

#if defined (RADIANT_GNUC) || defined (RADIANT_CLANG)
#  define PUSH_IGNORE_DEPRECATION_WARNINGS _Pragma ("GCC diagnostic push") \
  _Pragma ("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#  define POP_IGNORE_DEPRECATION_WARNINGS _Pragma ("GCC diagnostic pop")
#elif defined (RADIANT_MSVC)
#  define PUSH_IGNORE_DEPRECATION_WARNINGS __pragma(warning(disable:4996))
#  define POP_IGNORE_DEPRECATION_WARNINGS __pragma(warning(default:4996))
#else
#  define PUSH_IGNORE_DEPRECATION_WARNINGS
#  define POP_IGNORE_DEPRECATION_WARNINGS
#endif

//////////////////////////////////////////////////////////////////////////

#ifdef __GNUC__
#define RADIANT_PRINTF_CHECK(STR_IDX, FIRST_TO_CHECK) \
  __attribute__ ((format (printf, (STR_IDX), (FIRST_TO_CHECK))))
#else
#define RADIANT_PRINTF_CHECK(STR_IDX, FIRST_TO_CHECK)
#endif

#endif
