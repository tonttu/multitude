#ifndef PUNCTUAL_EXPORT_HPP
#define PUNCTUAL_EXPORT_HPP

#include "Radiant/Platform.hpp"

// Import by default
#ifdef PUNCTUAL_EXPORT
#define PUNCTUAL_API MULTI_DLLEXPORT
#else
#define PUNCTUAL_API MULTI_DLLIMPORT
#endif

#endif // EXPORT_HPP
