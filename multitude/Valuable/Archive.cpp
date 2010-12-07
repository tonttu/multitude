/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "Archive.hpp"

namespace Valuable
{

  SerializationOptions::SerializationOptions(Options options)
    : m_options(options)
  {}

  //////////////////////////////////////////////////////////////////////////

  ArchiveElement::Iterator::~Iterator()
  {}

  //////////////////////////////////////////////////////////////////////////

  ArchiveElement::~ArchiveElement()
  {}

  DOMElement * ArchiveElement::xml()
  {
    return 0;
  }

  //////////////////////////////////////////////////////////////////////////

  Archive::Archive(Options options)
    : SerializationOptions(options)
  {}

  Archive::~Archive()
  {}

  DOMDocument * Archive::xml()
  {
    return 0;
  }
}
