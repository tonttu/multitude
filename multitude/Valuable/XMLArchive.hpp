#ifndef XMLARCHIVE_HPP
#define XMLARCHIVE_HPP

#include "Archive.hpp"

namespace Valuable
{
  /**
   * Wrapper for DOMElement that implements the ArchiveElement interface.
   */
  class XMLArchiveElement : public ArchiveElement
  {
  public:
    XMLArchiveElement(DOMElement element);
    virtual ~XMLArchiveElement();

    void add(ArchiveElement & element);

    void add(const char * name, const char * value);
    std::string get(const char * name) const;

    void set(const std::string & s);
    void set(const std::wstring & s);
    std::string get() const;
    std::wstring getW() const;

    std::string name() const;
    bool isNull() const;

    /// Returns a pointer to m_element
    DOMElement * xml();

  protected:
    DOMElement m_element;
  };

  /**
   * Wrapper for DOMDocument that implements the Archive interface.
   *
   * All ArchiveElements are instances of XMLArchiveElement, constructed in
   * createElement and owned by XMLArchive. They will be freed only when the
   * XMLArhive object is destroyed.
   */
  class XMLArchive : public Archive
  {
  public:
    /// Creates a new DOMDocument
    XMLArchive();
    /// Frees every ArchiveElement this Archive have ever created
    virtual ~XMLArchive();

    ArchiveElement & createElement(const char * name);

    ArchiveElement & emptyElement();
    ArchiveElement & root();

    void add(ArchiveElement & element);

    bool writeToFile(const char * file);
    bool writeToMem(std::vector<char> & buffer);
    bool readFromFile(const char * filename);

    /// Returns m_document.ptr()
    DOMDocument * xml();

  protected:
    std::list<XMLArchiveElement> m_elements;
    Radiant::RefPtr<DOMDocument> m_document;
    XMLArchiveElement m_emptyElement;
  };
}

#endif // XMLARCHIVE_HPP
