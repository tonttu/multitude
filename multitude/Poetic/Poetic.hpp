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
#ifndef POETIC_FREETYPE_HPP
#define POETIC_FREETYPE_HPP

struct FT_LibraryRec_;

/// Poetic is an OpenGL font rendering library.

/** Poetic is a C++ OpenGL font rendering library loosely based on FTGL
    2.1.2. It was built to be used in both single-threaded and
    multi-threaded rendering.

    The main classes of the library are #Poetic::CPUFont and #Poetic::GPUFont. The are
    abstract base classes for text rendering.
    
    @author Esa Nuuros
*/
namespace Poetic
{

  bool initialize();
  
  FT_LibraryRec_ ** freetype();

  int error();
}

#endif
