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

  /** Stores information about changed ValueObjects. This can be used for
  notifying other processes about changes. */
  /// @todo this should propably be a singleton
  class VALUABLE_API ChangeMap
  {
    public:
      ChangeMap();
      virtual ~ChangeMap();

      /// Object was deleted
      static void addDelete(ValueObject * vo);
      /// Object was changed
      static void addChange(ValueObject * vo);

    protected:
      /// Inserts a change to the internal set of changes
      virtual void queueChange(ValueObject * vo);

      /// Holds an instance of the change map
      static ChangeMap * instance;

      /// Container for the change set
      typedef std::set<ValueObject *> container;
      /// Current change set
      container m_changes;
  };

}

#endif
