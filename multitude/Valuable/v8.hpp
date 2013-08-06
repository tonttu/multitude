#if !defined (VALUABLE_V8_HPP)
#define VALUABLE_V8_HPP

#include "Export.hpp"

#if defined (RADIANT_MSVC)
#  pragma warning(push)
#  pragma warning(disable : 4100) // Disable "unreferenced formal parameter" 
#  include <v8.h>
#  pragma warning(pop) 
#else

# ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wunused-parameter"
# endif

#  include <v8.h>

#ifdef __clang__
# pragma clang diagnostic pop
#endif

#endif

#endif // VALUABLE_V8_HPP
