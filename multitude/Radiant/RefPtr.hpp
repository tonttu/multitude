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

#ifndef RADIANT_REF_PTR_HPP
#define RADIANT_REF_PTR_HPP

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "Radiant/Platform.hpp"

#include <cstddef>

// try to detect c++0x
#if defined(RADIANT_CPP0X)
  #include <memory>
#else
  #if defined(__GNUC__) || defined(RADIANT_LINUX) || defined(RADIANT_OSX)
    #include <tr1/memory>
  #elif defined(RADIANT_WIN32) && defined(_HAS_TR1)
    #include <memory>
  #else
    #include <boost/tr1/memory.hpp>
  #endif
  namespace std
  {
    using tr1::shared_ptr;
  }
#endif


#endif // DOXYGEN_SHOULD_SKIP_THIS

#endif
