#pragma once

#include <Radiant/Platform.hpp>

#ifdef PDF_EXPORT
#define PDF_API MULTI_DLLEXPORT
#else
#define PDF_API MULTI_DLLIMPORT
#endif
