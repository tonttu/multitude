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

#include <GL/glew.h>

// #include <Luminous/lumiglew.hpp>

/// Luminous library is a collection of C++ classes for computer graphics, using OpenGL.

/** \b Copyright: The Luminous library has been developed in Helsinki
    Institute for Information Technology (HIIT, 2006-2008) and
    MultiTouch Oy (2007-2008).
        
    Luminous is released under the GNU Lesser General Public License
    (LGPL), version 2.1.

    @author Tommi Ilmonen, Esa Nuuros
    
*/
namespace Luminous
{

  /// Initializes the Luminous library.
  /** In practice this function only initializes the GLEW and checks
      the capabilities of the underlying OpenGL implementation. If the
      OpenGL version is below 2.0, then a warnign message is
      issued. 
      
      @return true if all relevant resources were successfully
      initialized, false if something was left missing (for example
      too low OpenGL version).
  */
  LUMINOUS_API bool initLuminous();
  LUMINOUS_API void initDefaultImageCodecs();

}

#endif
