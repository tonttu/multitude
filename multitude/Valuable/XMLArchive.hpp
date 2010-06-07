#ifndef XMLARCHIVE_HPP
#define XMLARCHIVE_HPP

#include "Archive.hpp"
#include "Export.hpp"

namespace Valuable
{
  class XMLArchive;

  /**
   * Wrapper for DOMElement that implements the ArchiveElement interface.
   */
  class VALUABLE_API XMLArchiveElement : public ArchiveElement
  {
  public:
    /// Iterator for XMLArchiveElement children
    class XMLIterator : public Iterator
    {
    public:
      /// Constructs a new iterator
      XMLIterator(XMLArchiveElement & parent);
      /// Constructs a copy of an iterator
      XMLIterator(const XMLIterator & it);
/// @cond
      operator const void * ();
      ArchiveElement & operator * ();
      ArchiveElement * operator -> ();
      Iterator & operator ++ ();
      Iterator & operator ++ (int);
      bool operator == (const Iterator & other);
      bool operator != (const Iterator & other);
/// @endcond
    private:
      XMLArchiveElement & m_parent;
      DOMElement::NodeList m_nodes;
      DOMElement::NodeList::iterator m_it;
      std::list<XMLIterator> m_iterators;
      std::list<XMLArchiveElement> m_elements;
      bool m_valid;
    };

    /// Creates a new wrapper object for given DOMElement object
    XMLArchiveElement(DOMElement element);
    virtual ~XMLArchiveElement();

    void add(ArchiveElement & element);
    Iterator & children();

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

    /// @cond
    static XMLArchiveElement s_emptyElement;
    /// @endcond

  private:
    std::list<XMLIterator> m_iterators;
    DOMElement m_element;
  };

  /**
   * Wrapper for DOMDocument that implements the Archive interface.
   *
   * All ArchiveElements are instances of XMLArchiveElement, constructed in
   * createElement and owned by XMLArchive. They will be freed only when the
   * XMLArhive object is destroyed.
   */
  class VALUABLE_API XMLArchive : public Archive
  {
  public:
    /// Creates a new DOMDocument
    XMLArchive(Options options = DEFAULTS);
    /// Frees every ArchiveElement this Archive have ever created
    virtual ~XMLArchive();

    ArchiveElement & createElement(const char * name);
    /// Wraps the given DOMElement to XMLArchiveElement
    XMLArchiveElement & createElement(const DOMElement & element);

    ArchiveElement & emptyElement();
    ArchiveElement & root();

    void setRoot(ArchiveElement & element);

    bool writeToFile(const char * file);
    bool writeToMem(std::vector<char> & buffer);
    bool readFromFile(const char * filename);

    /// Returns m_document.ptr()
    DOMDocument * xml();

  private:
    std::list<XMLArchiveElement> m_elements;
    std::shared_ptr<DOMDocument> m_document;
  };
}

#endif // XMLARCHIVE_HPP
