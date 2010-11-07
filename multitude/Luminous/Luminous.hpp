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

#ifndef LUMINOUS_LUMINOUS_HPP
#define LUMINOUS_LUMINOUS_HPP

#include <Luminous/Export.hpp>
#include <Radiant/Platform.hpp>

#ifdef RADIANT_OSX
// We do not use GLEW on OSX
#define MULTI_WITHOUT_GLEW 1
#endif

#ifdef MULTI_WITHOUT_GLEW
// This define is for glext.h
#ifdef RADIANT_OSX
#define GL_GLEXT_PROTOTYPES 1

#ifdef RADIANT_IOS
// #include <OpenGLES/EAGL.h>
#include <OpenGLES/ES2/gl.h>
// #include <OpenGLES/ES2/glext.h>
#define LUMINOUS_OPENGLES 1
#else
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <OpenGL/glu.h>
#endif

#else
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>
#endif
#else
#include <GL/glew.h>
#endif

#ifdef LUMINOUS_OPENGLES
# define LUMINOUS_IN_FULL_OPENGL(x)
#else
# define LUMINOUS_IN_FULL_OPENGL(x) x
#endif

/// Luminous is a library of C++ classes for computer graphics, using OpenGL.
/** Copyright: The Luminous library has been developed in Helsinki
    Institute for Information Technology (HIIT, 2006-2008) and
    MultiTouch Oy (2007-2008).

    Luminous is released under the GNU Lesser General Public License
    (LGPL), version 2.1.
*/
namespace Luminous
{

  /** Initializes the Luminous library.
      In practice this function only initializes the GLEW and checks
      the capabilities of the underlying OpenGL implementation. If the
      OpenGL version is below 2.0, then a warning message is
      issued.

      @param initOpenGL if set to false, glew will not be initialized

      @return true if all relevant resources were successfully
      initialized, false if something was left missing (for example
      too low OpenGL version).
  */
  LUMINOUS_API bool initLuminous(bool initOpenGL = true);

  /** Initializes the default image codecs.
  The image codecs are loaded as plugins that need to be loaded before they can
  be used. This functions does just that.
  */
  LUMINOUS_API void initDefaultImageCodecs();

}

#endif
