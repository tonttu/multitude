/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "XMLArchive.hpp"

#include <QRegExp>

namespace Valuable
{
  XMLArchiveElement::XMLIterator::XMLIterator(const XMLArchiveElement & parent)
    : m_parent(parent),
    m_nodes(parent.xml()->getChildNodes()),
    m_it(m_nodes.begin()),
    m_valid(m_it != m_nodes.end())
  {}

  XMLArchiveElement::XMLIterator::XMLIterator(const XMLIterator & it)
    : m_parent(it.m_parent),
    m_nodes(it.m_nodes),
    m_it(m_nodes.begin()),
    m_valid(it.m_valid)
  {
    if (m_valid) {
      DOMElement::NodeList::const_iterator i = it.m_nodes.begin();
      while (i != it.m_it) ++i, ++m_it;
    }
  }

  bool XMLArchiveElement::XMLIterator::isValid() const
  {
    return m_valid;
  }

  std::shared_ptr<ArchiveElementImpl> XMLArchiveElement::XMLIterator::get() const
  {
    if(!m_valid) return std::shared_ptr<ArchiveElementImpl>();
    return std::shared_ptr<ArchiveElementImpl>(new XMLArchiveElement(*m_it));
  }

  void XMLArchiveElement::XMLIterator::next()
  {
    if (m_valid) m_valid = ++m_it != m_nodes.end();
  }

  bool XMLArchiveElement::XMLIterator::operator == (const ArchiveIteratorImpl & other) const
  {
    const XMLArchiveElement::XMLIterator * it = dynamic_cast<const XMLArchiveElement::XMLIterator *>(&other);
    return it && it->m_it == m_it;
  }

  ///////////////////////////////////////////////////////////////////////////

  XMLArchiveElement::XMLArchiveElement(DOMElement element)
    : m_element(element)
  {}

  void XMLArchiveElement::add(ArchiveElementImpl & element)
  {
    XMLArchiveElement * e = dynamic_cast<XMLArchiveElement*>(&element);
    if(e) m_element.appendChild(*e->xml());
  }

  ArchiveElement::Iterator XMLArchiveElement::children() const
  {
    return ArchiveElement::Iterator(std::shared_ptr<ArchiveIteratorImpl>(
                                      new XMLIterator(*this)));
  }

  void XMLArchiveElement::add(const QString & name, const QString & value)
  {
    m_element.setAttribute(name, value);
  }

  QString XMLArchiveElement::get(const QString & name) const
  {
    return m_element.getAttribute(name);
  }

  void XMLArchiveElement::set(const QString & s)
  {
    m_element.setTextContent(s);
  }

  QString XMLArchiveElement::get() const
  {
    return m_element.getTextContent();
  }

  QString XMLArchiveElement::name() const
  {
    return m_element.getTagName();
  }

  void XMLArchiveElement::setName(const QString & name)
  {
    m_element.setTagName(name);
  }

  const DOMElement * XMLArchiveElement::xml() const
  {
    return &m_element;
  }

  ArchiveElement XMLArchiveElement::create(const DOMElement & element)
  {
    return std::shared_ptr<ArchiveElementImpl>(new XMLArchiveElement(element));
  }

  ///////////////////////////////////////////////////////////////////////////

  XMLArchive::XMLArchive(unsigned int options)
    : Archive(options),
    m_document(DOMDocument::createDocument())
  {}

  XMLArchive::~XMLArchive()
  {}

  ArchiveElement XMLArchive::createElement(const QString & name)
  {
    return std::shared_ptr<ArchiveElementImpl>(
          new XMLArchiveElement(m_document->createElement(name)));
  }

  ArchiveElement XMLArchive::root() const
  {
    return XMLArchiveElement::create(m_document->getDocumentElement());
  }

  void XMLArchive::setRoot(const ArchiveElement & element)
  {
    assert(element.xml());
    m_document->appendChild(*element.xml());
  }

  bool XMLArchive::writeToFile(const QString & file) const
  {
    return m_document->writeToFile(file);
  }

  bool XMLArchive::writeToMem(QByteArray & buffer) const
  {
    return m_document->writeToMem(buffer);
  }

  bool XMLArchive::readFromFile(const QString & filename)
  {
    return m_document->readFromFile(filename);
  }

  bool XMLArchive::readFromMem(const QByteArray & buffer)
  {
    return m_document->readFromMem(buffer);
  }

  DOMDocument * XMLArchive::xml()
  {
    return m_document.get();
  }

  QString XMLArchive::cleanElementName(QString name)
  {
    if (name.isEmpty())
      return "_";

    QRegExp startChar("^[A-Za-z:_]");
    if (startChar.indexIn(name) != 0)
      name[0] = '_';

    QRegExp nonNameChar("[^A-Za-z0-9:_-.]");
    return name.replace(nonNameChar, "_");
  }
}
