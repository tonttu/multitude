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

#include "XMLArchive.hpp"

namespace Valuable
{

  SerializationOptions::SerializationOptions(unsigned int options)
    : m_options(options)
  {}

  //////////////////////////////////////////////////////////////////////////

  ArchiveElementImpl::~ArchiveElementImpl()
  {
  }

  //////////////////////////////////////////////////////////////////////////

  ArchiveIteratorImpl::~ArchiveIteratorImpl()
  {}

  //////////////////////////////////////////////////////////////////////////

  ArchiveIterator::ArchiveIterator(std::shared_ptr<ArchiveIteratorImpl> impl)
    : m_impl(impl)
  {
  }

  ArchiveIterator::operator bool () const
  {
    return m_impl.get() && m_impl->isValid();
  }

  ArchiveElement ArchiveIterator::operator * () const
  {
    if(!m_impl.get()) return ArchiveElement();
    return ArchiveElement(m_impl->get());
  }

  ArchiveIterator & ArchiveIterator::operator ++ ()
  {
    assert(m_impl);
    m_impl->next();
    return *this;
  }

  bool ArchiveIterator::operator == (const ArchiveIterator & other) const
  {
    if(!m_impl || !other.m_impl) return m_impl == other.m_impl;
    return *m_impl == *other.m_impl;
  }

  bool ArchiveIterator::operator != (const ArchiveIterator & other) const
  {
    return !(*this == other);
  }

  //////////////////////////////////////////////////////////////////////////

  ArchiveElement::ArchiveElement(std::shared_ptr<ArchiveElementImpl> impl)
    : m_impl(impl) {}

  void ArchiveElement::add(const ArchiveElement & element)
  {
    assert(m_impl);
    m_impl->add(*element.m_impl);
  }

  ArchiveIterator ArchiveElement::children() const
  {
    assert(m_impl);
    return m_impl->children();
  }

  void ArchiveElement::add(const QString & name, const QString & value)
  {
    assert(m_impl);
    m_impl->add(name, value);
  }

  QString ArchiveElement::get(const QString & name) const
  {
    assert(m_impl);
    return m_impl->get(name);
  }

  void ArchiveElement::set(const QString & s)
  {
    assert(m_impl);
    m_impl->set(s);
  }

  QString ArchiveElement::get() const
  {
    assert(m_impl);
    return m_impl->get();
  }

  QString ArchiveElement::name() const
  {
    assert(m_impl);
    return m_impl->name();
  }

  bool ArchiveElement::isNull() const
  {
    return m_impl == 0;
  }

  const DOMElement * ArchiveElement::xml() const
  {
    XMLArchiveElement * e = dynamic_cast<XMLArchiveElement*>(m_impl.get());
    if(e) return e->xml();
    return 0;
  }

  //////////////////////////////////////////////////////////////////////////

  Archive::Archive(unsigned int options)
    : SerializationOptions(options)
  {}

  Archive::~Archive()
  {}

  DOMDocument * Archive::xml()
  {
    return 0;
  }
}
