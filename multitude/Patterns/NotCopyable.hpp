/* COPYRIGHT
 *
 * This file is part of Patterns.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef PATTERNS_NOTCOPYABLE_HPP
#define PATTERNS_NOTCOPYABLE_HPP

#include <Patterns/Export.hpp>

namespace Patterns
{
  /// Base class for classes that cannot be copied. By inheriting this
  /// class you can disable copying of your classes.
  class PATTERNS_API NotCopyable
  {
    protected:
      NotCopyable();
      ~NotCopyable();

    private:
      NotCopyable(const NotCopyable &);
      const NotCopyable & operator = (const NotCopyable &);
  };

}

#endif
