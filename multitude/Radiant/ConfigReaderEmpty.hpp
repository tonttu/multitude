/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

/** This file is part of the Radiant library.*/

#ifndef CONFIG_READER_EMPTY_HPP
#define CONFIG_READER_EMPTY_HPP

#include <Radiant/Export.hpp>

/** @file 
    Empty declarations of the classes. Use this header to avoid loading the more
    complex stuff into all of your files.
*/

namespace Radiant {

  class Variant;
  template <class T> RADIANT_API class ChunkT;

  typedef ChunkT<Variant> Chunk;
  typedef ChunkT<Chunk>   Config;

}

#endif
