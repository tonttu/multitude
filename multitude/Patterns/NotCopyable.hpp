/* COPYRIGHT
 *
 * This file is part of Patterns.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Patterns.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef PATTERNS_NOTCOPYABLE_HPP
#define PATTERNS_NOTCOPYABLE_HPP

namespace Patterns
{
  /// Base class for classes that cannot be copied. By inheriting this
  /// class you can disable copying of your classes.
  class NotCopyable
  {
    protected:
      NotCopyable() {}
      ~NotCopyable() {}

    private:
      NotCopyable(const NotCopyable &);
      const NotCopyable & operator = (const NotCopyable &);
  };

}

#endif
