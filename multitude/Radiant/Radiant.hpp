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

#ifndef RADIANT_RADIANT_HPP
#define RADIANT_RADIANT_HPP

#include <type_traits>

/** Radiant library is a collection of C++ utility classes.

    Radiant is a collection of C++ classes geared at wrapping
    platform-dependent programming features (threads, mutexes,
    sockets, etc.). Radiant also includes a collection of utilities
    for handling some vary basic string/file manipulation that is
    missing from C/C++ standard libraries.

    \b Copyright: The Radiant library has been developed by Helsinki
    Institute for Information Technology (HIIT, 2006-2008) and
    MultiTouch Oy (2007-2011).

    Radiant is released under the GNU Lesser General Public License
    (LGPL), version 2.1.

    @author Tommi Ilmonen, Esa Nuuros, Jarmo Hiipakka, Juha Laitinen,
    Jari Kleimola, George Whale
*/
namespace Radiant {
  /// @todo what is proper place for these?
  template<typename Y>
  typename std::enable_if<std::is_arithmetic<Y>::value, Y>::type createNull()
  {
    return Y(0);
  }

  template<typename Y>
  typename std::enable_if<!std::is_arithmetic<Y>::value, Y>::type createNull()
  {
    return Y::null();
  }
}

#define debugRadiant(...) (Radiant::trace("Radiant", Radiant::DEBUG, __VA_ARGS__))

#endif
