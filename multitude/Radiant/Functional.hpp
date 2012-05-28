#ifndef RADIANT_FUNCTIONAL_HPP
#define RADIANT_FUNCTIONAL_HPP

/// @cond

#include "Platform.hpp"

#include <cstddef>

// try to detect C++11
#if defined(RADIANT_CXX11) && !defined(__APPLE__)
  #include <functional>
#elif defined(__GCCXML__)
  #include <generator/gccxml_tr1.hpp>
#else
  #if defined(__GNUC__) || defined(RADIANT_LINUX) || defined(RADIANT_OSX)
    #include <tr1/functional>
  #elif defined(RADIANT_WINDOWS) && defined(_HAS_TR1)
    #include <functional>
  #else
    #include <boost/tr1/functional.hpp>
  #endif
  namespace std
  {
    using tr1::function;
    using tr1::bind;
    using tr1::result_of;
    using tr1::mem_fn;
    using tr1::ref;
    using tr1::cref;
  }

#endif

/// @endcond

#endif // RADIANT_FUNCTIONAL_HPP
