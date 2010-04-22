#ifndef XMLARCHIVE_HPP
#define XMLARCHIVE_HPP

#include "Archive.hpp"

namespace Valuable
{
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

    DOMElement * xml();

  protected:
    DOMElement m_element;
  };

  /**
   * DOMDocument wrapper.
   */
  class XMLArchive : public Archive
  {
  public:
    XMLArchive();
    virtual ~XMLArchive();

    ArchiveElement & createElement(const char * name);

    ArchiveElement & emptyElement();
    ArchiveElement & root();

    void add(ArchiveElement & element);

    bool writeToFile(const char * file);
    bool writeToMem(std::vector<char> & buffer);
    bool readFromFile(const char * filename);

    DOMDocument * xml();

  protected:
    std::list<XMLArchiveElement> m_elements;
    Radiant::RefPtr<DOMDocument> m_document;
    XMLArchiveElement m_emptyElement;
  };
}

#endif // XMLARCHIVE_HPP
