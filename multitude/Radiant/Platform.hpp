/* COPYRIGHT
 */

#ifndef RADIANT_PLATFORM_HPP
#define RADIANT_PLATFORM_HPP

// C++11 check
#if __cplusplus > 199711L || defined(__GXX_EXPERIMENTAL_CXX0X__)
  #define RADIANT_CXX11 1
#endif

// Discover the architecture
#if defined(__amd64__) || defined(_M_X64)
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

//
// Detect LLVM/CLANG
//
#if defined (__clang__)
#   define RADIANT_CLANG 1
#   define DLLEXPORT __attribute__((visibility("default")))
#   define DLLIMPORT __attribute__((visibility("default")))

// Test if we have override
#   if !__has_feature(cxx_override_control)
#     define NO_OVERRIDE
#   endif
// 
// Detect GNU GCC/G++
//
#elif defined (__GNUC__)
#   define RADIANT_GNUC 1
#   define DLLEXPORT __attribute__((visibility("default")))
#   define DLLIMPORT __attribute__((visibility("default")))

//  Override is a GCC4.7 and up feature when compiling code as c++0x/c++11
#   if !defined(RADIANT_CXX11) || ((__GNUC__ == 4 && __GNUC_MINOR__ < 7) || __GNUC__ < 4)
#     define NO_OVERRIDE
#   endif

//
// Detect Microsoft Visual C++
//
#elif defined (_MSC_VER)
#   define RADIANT_MSVC 1
#   define DLLEXPORT __declspec(dllexport)
#   define DLLIMPORT __declspec(dllimport)
#endif

#if !defined(NO_OVERRIDE)
#  define OVERRIDE override
#else
#  define OVERRIDE
#endif

//
// Detect OSX
//
#if defined (__APPLE__)
// && defined (__MACH__)
#   define RADIANT_OSX 1
#   define RADIANT_UNIX 1

#if defined(__IPHONE_OS_VERSION_MIN_REQUIRED)
# define RADIANT_IOS 1
# define RADIANT_OS_MOBILE 1
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
#   ifndef _HAS_TR1
#     error "Compiler TR1 support was not found. Please install Visual Studio 2008 Service Pack 1 or use a newer compiler."
#   endif

// Grmblrgrmbl, weird windows CRT stuffs
#define snprintf _snprintf

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
