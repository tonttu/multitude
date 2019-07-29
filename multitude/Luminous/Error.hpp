/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_ERROR_HPP
#define LUMINOUS_ERROR_HPP

#include <Luminous/Luminous.hpp>
#include <Luminous/Export.hpp>

#include <iostream>

#include <QString>

#define CHECK_GL_ERROR Luminous::glErrorToString()

namespace Luminous
{
  /// Converts OpenGL error into a human-readable string. This function gets
  /// the current error code from OpenGL and displays it using Radiant::error.
  /// @param msg message prefix
  /// @param line line number
  LUMINOUS_API bool glErrorToString(const QString & msg = __FILE__, int line = __LINE__);
}

#endif
