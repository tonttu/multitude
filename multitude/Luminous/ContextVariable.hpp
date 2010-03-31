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

#include <Luminous/Collectable.hpp>

namespace Luminous {

  class GLResources;

  /// Template class for accessing per-context graphics resources
  /** The purpose of this class is to simplify the management of OpenGL resources,
      for threaded applications. */
  template <class T>
      class LUMINOUS_API ContextVariableT : public Luminous::Collectable
  {
  public:
    ContextVariableT() {}
    virtual ~ContextVariableT();
    /// Gets a reference to the OpenGL resource
    /** Before calling this function you should have a valid OpenGL context, with
        the right GLResources main object set for this thread. */
    T & ref();

    /// Gets a reference to the OpenGL resource
    /** Before calling this function you should have a valid OpenGL context, with
        the right GLResources main object set for this thread.

        Since this function
        gets a direct pointer to the GLResources object, it is slightly faster than
        the function without this argument.
    */
    T & ref(GLResources * rs);

  };

} // namespace

#endif // CONTEXTVARIABLE_HPP
