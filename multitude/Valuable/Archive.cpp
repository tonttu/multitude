#include "Archive.hpp"

namespace Valuable
{
  //Archive::Archive() : m_impl(new XMLArchive) {} /// @todo get rid of this default constructor

  DOMElement * ArchiveElement::xml()
  {
//    XMLArchiveElement * e = dynamic_cast<XMLArchiveElement*>(m_impl.ptr());
    XMLArchiveElement * x = dynamic_cast<XMLArchiveElement*>(this);
    if (x) { return &x->element(); }
    return 0;
  }

  DOMDocument * Archive::xml()
  {
    XMLArchive * x = dynamic_cast<XMLArchive*>(this);
    if (x) { return x->doc(); }
    return 0;
  }
}
