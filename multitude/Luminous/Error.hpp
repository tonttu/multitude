/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
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

#include <QString>

#define CHECK_GL_ERROR Luminous::glErrorToString()

namespace Luminous
{
  /// Converts OpenGL error into a human-readalbe string
  /// @param msg message prefix
  /// @param msg line number
  LUMINOUS_API void glErrorToString(const QString & msg = __FILE__, int line = __LINE__);
}

#endif
