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

#ifndef ERROR_HPP
#define ERROR_HPP

#include <Luminous/Luminous.hpp>
#include <Luminous/Export.hpp>

#include <iostream>

#define CHECK_GL_ERROR Luminous::glErrorToString()

namespace Luminous
{
  /// Converts OpenGL texture internal format enum into human-readable string
  const char * glInternalFormatToString(GLint format);
  /// Converts OpenGL texture format enum into human-readable string
  const char * glFormatToString(GLenum format);

  /// Converts OpenGL error into a human-readalbe string
  LUMINOUS_API void glErrorToString(const std::string & msg = __FILE__, int line = __LINE__);


}

#endif
