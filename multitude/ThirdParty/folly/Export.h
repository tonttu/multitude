#pragma once

#if defined (__clang__)
#   define FOLLY_DLLEXPORT __attribute__((visibility("default")))
#   define FOLLY_DLLIMPORT __attribute__((visibility("default")))

#elif defined (__GNUC__)
#   define FOLLY_DLLEXPORT __attribute__((visibility("default")))
#   define FOLLY_DLLIMPORT __attribute__((visibility("default")))

#elif defined (__INTEL_COMPILER)
#	 if defined(_WIN32)
#   define FOLLY_DLLEXPORT __declspec(dllexport)
#   define FOLLY_DLLIMPORT __declspec(dllimport)
#  else
#   define FOLLY_DLLEXPORT __attribute__((visibility("default")))
#   define FOLLY_DLLIMPORT __attribute__((visibility("default")))
#  endif

#elif defined (_MSC_VER)
#   define FOLLY_DLLEXPORT __declspec(dllexport)
#   define FOLLY_DLLIMPORT __declspec(dllimport)
#endif

#ifdef FOLLY_EXPORT
#   define FOLLY_API FOLLY_DLLEXPORT
#else
#   define FOLLY_API FOLLY_DLLIMPORT
#endif
