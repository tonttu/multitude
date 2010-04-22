#ifndef XMLARCHIVE_HPP
#define XMLARCHIVE_HPP

#include "Archive.hpp"

namespace Valuable
{
  class XMLArchiveElement : public ArchiveElement
  {
  public:
    XMLArchiveElement(DOMElement element) : m_element(element) {}

    DOMElement * xml();
    DOMElement & element() { return m_element; }

    void add(ArchiveElement & element) {
      m_element.appendChild(*element.xml());
    }

    void add(const char * name, const char * value) { m_element.setAttribute(name, value); }

    std::string get(const char * name) const { return m_element.getAttribute(name); }
    std::string get() const { return m_element.getTextContent(); }

    void set(const std::string & s) { m_element.setTextContent(s); }
    void set(const std::wstring & s) { m_element.setTextContent(s); }
    std::wstring getW() { return m_element.getTextContentW(); }

    std::string name() const { return m_element.getTagName(); }
    bool isNull() const { return m_element.isNull(); }

  protected:
    DOMElement m_element;
  };

  /**
   * This is a DOMDocument wrapper.
   */
  class XMLArchive : public Archive
  {
  public:
    XMLArchive() : m_document(DOMDocument::createDocument()), m_emptyElement(DOMElement()) {}
    DOMDocument * xml();
    ArchiveElement & createElement(const char * name)
    {
      m_elements.push_back(XMLArchiveElement(m_document->createElement(name)));
      return m_elements.back();
    }

    DOMDocument * doc() { return m_document.ptr(); }
    ArchiveElement & emptyElement() { return m_emptyElement; }
    ArchiveElement & root() {
      m_elements.push_back(XMLArchiveElement(m_document->getDocumentElement()));
      return m_elements.back();
    }

    void add(ArchiveElement & element) { m_document->appendChild(*element.xml()); }
    bool writeToFile(const char * file) { return m_document->writeToFile(file); }
    bool writeToMem(std::vector<char> & buffer) { return m_document->writeToMem(buffer); }
    bool readFromFile(const char * filename) { return m_document->readFromFile(filename); }

  protected:
    std::list<XMLArchiveElement> m_elements;
    Radiant::RefPtr<DOMDocument> m_document;
    XMLArchiveElement m_emptyElement;
  };
}

#endif // XMLARCHIVE_HPP
