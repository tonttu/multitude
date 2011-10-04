/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_CONTEXTVARIABLE_HPP
#define LUMINOUS_CONTEXTVARIABLE_HPP

#include "Collectable.hpp"
#include "RenderContext.hpp"

namespace Luminous {

  class RenderContext;

  /// Template class for accessing per-context graphics resources
  /** The purpose of this class is to simplify the management of OpenGL resources,
      for threaded applications. */
  template <class T>
  class ContextVariableT : public Luminous::Collectable
  {
  public:
    ContextVariableT() {}
    virtual ~ContextVariableT() {}
    /// Gets a reference to the OpenGL resource
    /** Before calling this function you should have a valid OpenGL context, with
        the right RenderContext main object set for this thread.

        Since this function
        gets a direct pointer to the RenderContext object, it is slightly faster than
        the function without this argument.

        @return Returns a reference to the OpenGL resource.
    */
    inline T & ref()
    {
      GLRESOURCE_ENSURE3(T, obj, this);
      return *obj;
    }



    /** @copydoc ref
        @param rs Pointer to the OpenGL resource container

        */
    inline T & ref(RenderContext * rs)
    {
      if(!rs) {
        return ref();
      }
      GLRESOURCE_ENSURE(T, obj, this, rs);
      return *obj;
    }

  };

} // namespace

#endif // CONTEXTVARIABLE_HPP
