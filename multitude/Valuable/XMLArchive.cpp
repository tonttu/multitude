#include "XMLArchive.hpp"

namespace Valuable
{
  XMLArchiveElement::XMLArchiveElement(DOMElement element)
    : m_element(element)
  {}

  XMLArchiveElement::~XMLArchiveElement()
  {}

  void XMLArchiveElement::add(ArchiveElement & element) {
    m_element.appendChild(*element.xml());
  }

  void XMLArchiveElement::add(const char * name, const char * value) {
    m_element.setAttribute(name, value);
  }

  std::string XMLArchiveElement::get(const char * name) const {
    return m_element.getAttribute(name);
  }

  void XMLArchiveElement::set(const std::string & s) {
    m_element.setTextContent(s);
  }

  void XMLArchiveElement::set(const std::wstring & s) {
    m_element.setTextContent(s);
  }

  std::string XMLArchiveElement::get() const {
    return m_element.getTextContent();
  }

  std::wstring XMLArchiveElement::getW() const {
    return m_element.getTextContentW();
  }

  std::string XMLArchiveElement::name() const {
    return m_element.getTagName();
  }

  bool XMLArchiveElement::isNull() const {
    return m_element.isNull();
  }

  DOMElement * XMLArchiveElement::xml()
  {
    return &m_element;
  }

  ///////////////////////////////////////////////////////////////////////////

  XMLArchive::XMLArchive()
    : m_document(DOMDocument::createDocument()),
    m_emptyElement(DOMElement())
  {}

  XMLArchive::~XMLArchive()
  {}

  ArchiveElement & XMLArchive::createElement(const char * name)
  {
    m_elements.push_back(XMLArchiveElement(m_document->createElement(name)));
    return m_elements.back();
  }

  ArchiveElement & XMLArchive::emptyElement() {
    return m_emptyElement;
  }

  ArchiveElement & XMLArchive::root() {
    m_elements.push_back(XMLArchiveElement(m_document->getDocumentElement()));
    return m_elements.back();
  }

  void XMLArchive::add(ArchiveElement & element) {
    m_document->appendChild(*element.xml());
  }

  bool XMLArchive::writeToFile(const char * file) {
    return m_document->writeToFile(file);
  }

  bool XMLArchive::writeToMem(std::vector<char> & buffer) {
    return m_document->writeToMem(buffer);
  }

  bool XMLArchive::readFromFile(const char * filename) {
    return m_document->readFromFile(filename);
  }

  DOMDocument * XMLArchive::xml()
  {
    return m_document.ptr();
  }
}
