/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef VALUABLE_CHANGEMAP_HPP
#define VALUABLE_CHANGEMAP_HPP

#if 0
#include <Valuable/Export.hpp>

#include <set>

namespace Valuable
{

  class Attribute;

  /** Stores information about changed Attributes. This can be used for
  notifying other processes about changes. */
  /// @todo this should propably be a singleton
  class VALUABLE_API ChangeMap
  {
    public:
      ChangeMap();
      virtual ~ChangeMap();

      /// Object was deleted
      static void addDelete(Attribute * vo);
      /// Object was changed
      static void addChange(Attribute * vo);

    protected:
      /// Inserts a change to the internal set of changes
      virtual void queueChange(Attribute * vo);

      /// Holds an instance of the change map
      static ChangeMap * instance;

      /// Container for the change set
      typedef std::set<Attribute *> container;
      /// Current change set
      container m_changes;
  };

}
#endif

#endif
