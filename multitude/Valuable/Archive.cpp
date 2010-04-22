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
