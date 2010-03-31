/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef VALUABLE_CHANGEMAP_HPP
#define VALUABLE_CHANGEMAP_HPP

#include <Valuable/Export.hpp>

#include <set>

namespace Valuable
{

  class ValueObject;

  /** Stores information about changed ValueObjects. */
  class VALUABLE_API ChangeMap 
  {
    public:
      ChangeMap();
      virtual ~ChangeMap();

      static void addDelete(ValueObject * vo);
      static void addChange(ValueObject * vo);

    protected:
      virtual void queueChange(ValueObject * vo);

      static ChangeMap * instance;

      typedef std::set<ValueObject *> container;
      container m_changes;
  };


}

#endif
