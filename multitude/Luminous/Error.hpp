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

#include <iostream>

namespace Luminous
{
  /// @todo remove?
  const char * glInternalFormatToString(GLint format);
  const char * glFormatToString(GLenum format);

  std::string glErrorToString();

}

#endif
