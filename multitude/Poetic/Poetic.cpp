/* COPYRIGHT
 *
 * This file is part of Poetic.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Poetic.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */
#include "Poetic.hpp"
#include <cassert>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace Poetic
{

  static FT_Library * g_library = 0;
  static FT_Error g_error = 0;

  bool initialize()
  {
    if(g_library) 
      return true;

    g_library = new FT_Library;

    g_error = FT_Init_FreeType(g_library);
    if(g_error) {
      delete g_library;
      g_library = 0;
      return false;
    }

    return true;    
  }

  FT_Error error() 
  {
    return g_error;
  }

  FT_Library * freetype()
  {
    if(!g_library) {
      initialize();
    }

    return g_library;
  }

}
