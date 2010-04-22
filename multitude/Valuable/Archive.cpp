#include "Archive.hpp"

namespace Valuable
{
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

  Archive::~Archive()
  {}

  DOMDocument * Archive::xml()
  {
    return 0;
  }
}
