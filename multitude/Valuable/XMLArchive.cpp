#include "XMLArchive.hpp"

namespace Valuable
{
  XMLArchiveElement XMLArchiveElement::s_emptyElement = DOMElement();

  XMLArchiveElement::XMLIterator::XMLIterator(XMLArchiveElement & parent)
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

  XMLArchiveElement::XMLIterator::operator const void * ()
  {
    return m_valid ? this : 0;
  }

  ArchiveElement & XMLArchiveElement::XMLIterator::operator * ()
  {
    if (m_valid) {
      m_elements.push_back(XMLArchiveElement(*m_it));
      return m_elements.back();
    }
    return XMLArchiveElement::s_emptyElement;
  }

  ArchiveElement * XMLArchiveElement::XMLIterator::operator -> ()
  {
    if (m_valid) {
      m_elements.push_back(XMLArchiveElement(*m_it));
      return &m_elements.back();
    }
    return &XMLArchiveElement::s_emptyElement;
  }

  ArchiveElement::Iterator & XMLArchiveElement::XMLIterator::operator ++ ()
  {
    if (m_valid) m_valid = ++m_it != m_nodes.end();
    return *this;
  }

  ArchiveElement::Iterator & XMLArchiveElement::XMLIterator::operator ++ (int)
  {
    if (m_valid) {
      m_iterators.push_back(*this);
      m_valid = ++m_it != m_nodes.end();
      return m_iterators.back();
    }
    return *this;
  }

  bool XMLArchiveElement::XMLIterator::operator == (const Iterator & other)
  {
    const XMLArchiveElement::XMLIterator * it = dynamic_cast<const XMLArchiveElement::XMLIterator *>(&other);
    return it && it->m_it == m_it;
  }

  bool XMLArchiveElement::XMLIterator::operator != (const Iterator & other)
  {
    return !(*this == other);
  }

  ///////////////////////////////////////////////////////////////////////////

  XMLArchiveElement::XMLArchiveElement(DOMElement element)
    : m_element(element)
  {}

  XMLArchiveElement::~XMLArchiveElement()
  {}

  void XMLArchiveElement::add(ArchiveElement & element)
  {
    m_element.appendChild(*element.xml());
  }

  ArchiveElement::Iterator & XMLArchiveElement::children()
  {
    m_iterators.push_back(XMLIterator(*this));
    return m_iterators.back();
  }

  void XMLArchiveElement::add(const char * name, const char * value)
  {
    m_element.setAttribute(name, value);
  }

  std::string XMLArchiveElement::get(const char * name) const
  {
    return m_element.getAttribute(name);
  }

  void XMLArchiveElement::set(const std::string & s)
  {
    m_element.setTextContent(s);
  }

  void XMLArchiveElement::set(const std::wstring & s)
  {
    m_element.setTextContent(s);
  }

  std::string XMLArchiveElement::get() const
  {
    return m_element.getTextContent();
  }

  std::wstring XMLArchiveElement::getW() const
  {
    return m_element.getTextContentW();
  }

  std::string XMLArchiveElement::name() const
  {
    return m_element.getTagName();
  }

  bool XMLArchiveElement::isNull() const
  {
    return m_element.isNull();
  }

  DOMElement * XMLArchiveElement::xml()
  {
    return &m_element;
  }

  ///////////////////////////////////////////////////////////////////////////

  XMLArchive::XMLArchive(Options options)
    : Archive(options),
    m_document(DOMDocument::createDocument())
  {}

  XMLArchive::~XMLArchive()
  {}

  ArchiveElement & XMLArchive::createElement(const char * name)
  {
    m_elements.push_back(XMLArchiveElement(m_document->createElement(name)));
    return m_elements.back();
  }

  XMLArchiveElement & XMLArchive::createElement(const DOMElement & element)
  {
    m_elements.push_back(XMLArchiveElement(element));
    return m_elements.back();
  }

  ArchiveElement & XMLArchive::emptyElement()
  {
    return XMLArchiveElement::s_emptyElement;
  }

  ArchiveElement & XMLArchive::root()
  {
    m_elements.push_back(XMLArchiveElement(m_document->getDocumentElement()));
    return m_elements.back();
  }

  void XMLArchive::setRoot(ArchiveElement & element)
  {
    m_document->appendChild(*element.xml());
  }

  bool XMLArchive::writeToFile(const char * file)
  {
    return m_document->writeToFile(file);
  }

  bool XMLArchive::writeToMem(std::vector<char> & buffer)
  {
    return m_document->writeToMem(buffer);
  }

  bool XMLArchive::readFromFile(const char * filename)
  {
    return m_document->readFromFile(filename);
  }

  DOMDocument * XMLArchive::xml()
  {
    return m_document.get();
  }
}
