#ifndef ARCHIVE_HPP
#define ARCHIVE_HPP

#include <Valuable/DOMDocument.hpp>
#include <Valuable/DOMElement.hpp>

#include <cassert>

namespace Valuable {

  class ArchiveElement
  {
  public:
    DOMElement * xml();

    virtual void add(ArchiveElement & element) = 0;
    virtual void add(const char * name, const char * value) = 0;
    virtual std::string get(const char * name) const = 0;
    virtual std::string get() const = 0;
    virtual void set(const std::string & s) = 0;
    virtual void set(const std::wstring & s) = 0;
    virtual std::wstring getW() = 0;
    virtual std::string name() const = 0;
    virtual bool isNull() const = 0;

  protected:
    virtual ~ArchiveElement() {}
  };

  class Archive
  {
  public:
    virtual ArchiveElement & createElement(const char * name) = 0;
    virtual ~Archive() {}

    DOMDocument * xml();

    virtual ArchiveElement & emptyElement() = 0;
    virtual ArchiveElement & root() = 0;

    virtual void add(ArchiveElement & element) = 0;
    virtual bool writeToFile(const char * file) = 0;
    virtual bool writeToMem(std::vector<char> & buffer) = 0;
    virtual bool readFromFile(const char * filename) = 0;
  };

  class XMLArchiveElement : public ArchiveElement
  {
  public:
    XMLArchiveElement(DOMElement element) : m_element(element) {}

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
/*    void setTextContent(const std::string & s) { m_element.setTextContent(s); }
    std::string getTextContent() const { return m_element.getTextContent(); }
    void setAttribute(const char * name, const char * value) { m_element.setAttribute(name, value); }*/
  };

  class XMLArchive : public Archive
  {
  public:
    XMLArchive() : m_document(DOMDocument::createDocument()), m_emptyElement(DOMElement()) {}
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

/*  typedef DOMDocument Archive;
typedef DOMElement ;*/
/*
class ArchiveElementImpl {
public:
  virtual std::string getTextContent() const = 0;
  virtual void setTextContent(const std::string & s) = 0;
  virtual void setAttribute(const char * name, const char * value) = 0;
};

class ArchiveImpl {
public:
  virtual ArchiveElementImpl * createElement(const char * name) = 0;
};

class ArchiveElement
{
public:
  ArchiveElement() { assert(false); }
  ArchiveElement(ArchiveElementImpl * impl) : m_impl(impl) {}

  void setTextContent(const std::string & s) { m_impl->setTextContent(s); }
  std::string getTextContent() const { return m_impl->getTextContent(); }
  void setAttribute(const char * name, const char * value) { m_impl->setAttribute(name, value); }

  DOMElement * xml();

private:
  Radiant::RefPtr<ArchiveElementImpl> m_impl;
};

class Archive
{
public:
  Archive();

  ArchiveElement createElement(const char * name) { return ArchiveElement(m_impl->createElement(name)); }

private:
  Radiant::RefPtr<ArchiveImpl> m_impl;
};

class XMLArchiveElement : public ArchiveElementImpl
{
public:
  XMLArchiveElement(DOMElement element) : m_element(element) {}

  DOMElement & element() { return m_element; }

protected:
  DOMElement m_element;
  void setTextContent(const std::string & s) { m_element.setTextContent(s); }
  std::string getTextContent() const { return m_element.getTextContent(); }
  void setAttribute(const char * name, const char * value) { m_element.setAttribute(name, value); }
};

class XMLArchive : public ArchiveImpl
{
public:
  XMLArchive() : m_document(DOMDocument::createDocument()) {}
  virtual ArchiveElementImpl * createElement(const char * name)
  {
    return new XMLArchiveElement(m_document->createElement(name));
  }

protected:
  Radiant::RefPtr<DOMDocument> m_document;
};
*/
}
#endif // ARCHIVE_HPP
