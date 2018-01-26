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
