#if !defined (VALUABLE_V8_HPP)
#define VALUABLE_V8_HPP

#include "Export.hpp"

#if defined (RADIANT_MSVC)
#  pragma warning(push)
#  pragma warning(disable : 4100) // Disable "unreferenced formal parameter" 
#  include <v8.h>
#  pragma warning(pop) 
#else
#  include <v8.h>
#endif

#endif // VALUABLE_V8_HPP