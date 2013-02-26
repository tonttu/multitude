/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "Platform.hpp"

#ifndef RADIANT_WINDOWS

#include "SHMDuplexPipe.hpp"

namespace Radiant
{
  
  SHMDuplexPipe::SHMDuplexPipe(const key_t writeKey, const uint32_t writeSize,
			     const key_t readKey,  const uint32_t readSize)
    : m_out(writeKey, writeSize),
      m_in(readKey, readSize)
  {}

  SHMDuplexPipe::~SHMDuplexPipe()
  {}
}

#endif
