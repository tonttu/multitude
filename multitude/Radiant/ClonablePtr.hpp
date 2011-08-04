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

#ifndef RADIANT_CLONABLE_PTR_HPP
#define RADIANT_CLONABLE_PTR_HPP

#include "Export.hpp"

namespace Radiant {

  /** Template class for managing clonable objects.

      The template class T needs to implement function "clone", that
      produces a replica of the object.
   */
  template <typename T>
  class ClonablePtr
  {
  public:
    /// Constructs a new pointer
    ClonablePtr(T * obj = 0) : m_object(obj) {}

    /// Constructs a copy
    ClonablePtr(const ClonablePtr &that)
    {
      if(that.m_object)
        m_object = that.m_object->clone();
      else
        m_object = 0;
    }

    ~ClonablePtr() { delete m_object; }

    /// Copies a pointer
    ClonablePtr & operator = (const ClonablePtr &that)
    {
      delete m_object;

      if(that.m_object)
        m_object = that.m_object->clone();
      else
        m_object = 0;
      return *this;
    }

    /// Returns a raw pointer to the object
    T * ptr() { return m_object; }
    /// Returns a raw pointer to the object
    const T * ptr() const { return m_object; }

  private:
    T *m_object;
  };


}

#endif

