#include "XMLArchive.hpp"

namespace Valuable
{
  DOMElement * XMLArchiveElement::xml()
  {
    return &m_element;
  }

  DOMDocument * XMLArchive::xml()
  {
    return m_document.ptr();
  }
}
