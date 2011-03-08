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

#include <Radiant/Mutex.hpp>

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

  /// Initialize Poetic. This function must be called before using any other functions in the library.
  bool initialize();
  
  /// Returns a handle to the freetype library
  FT_LibraryRec_ ** freetype();

  /// Returns the last freetype error
  int error();

  // Hack around thread-safety
  Radiant::MutexStatic & freetypeMutex();
}

#endif
