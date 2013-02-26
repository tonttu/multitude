/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef VALUABLE_XMLARCHIVE_HPP
#define VALUABLE_XMLARCHIVE_HPP

#include "Archive.hpp"
#include "Export.hpp"

namespace Valuable
{
  class XMLArchive;

  /**
   * Wrapper for DOMElement that implements the ArchiveElementImpl interface.
   * This and XMLArchive implement XML serialization.
   */
  class VALUABLE_API XMLArchiveElement : public ArchiveElementImpl
  {
  public:
    /// Iterator for XMLArchiveElement children
    class XMLIterator : public ArchiveIteratorImpl
    {
    public:
      /// Constructs a new iterator
      XMLIterator(const XMLArchiveElement & parent);
      /// Constructs a copy of an iterator
      XMLIterator(const XMLIterator & it);

      virtual std::shared_ptr<ArchiveElementImpl> get() const;
      virtual void next();
      virtual bool isValid() const;
      virtual bool operator == (const ArchiveIteratorImpl & other) const;
    private:
      const XMLArchiveElement & m_parent;
      DOMElement::NodeList m_nodes;
      DOMElement::NodeList::iterator m_it;
      bool m_valid;
    };

    /// Creates a new wrapper object for given DOMElement object
    /// @param element DOMElement to wrap
    XMLArchiveElement(DOMElement element);

    void add(ArchiveElementImpl & element);
    ArchiveIterator children() const;

    void add(const QString & name, const QString & value);
    QString get(const QString & name) const;

    void set(const QString & s);
    QString get() const;

    QString name() const;
    /// @todo add virtual/OVERRIDE to all functions that need it
    virtual void setName(const QString & name) OVERRIDE;

    /// Returns a pointer to the wrapped DOMElement
    /// @return The wrapped DOMElement
    const DOMElement * xml() const;

    /// Wraps the given DOMElement to XMLArchiveElement
    /// @param element DOMElement to wrap
    /// @return New Element with XMLArchiveElement implementation
    static ArchiveElement create(const DOMElement & element);

  private:
    DOMElement m_element;
  };

  /**
   * Wrapper for DOMDocument that implements the Archive interface.
   *
   * All ArchiveElements use XMLArchiveElement implementation and are constructed
   * in createElement.
   */
  class VALUABLE_API XMLArchive : public Archive
  {
  public:
    /// Creates a new DOMDocument
    /// @param options Bitmask of SerializationOptions::Options
    XMLArchive(unsigned int options = DEFAULTS);
    /// Deletes the DOMDocument
    virtual ~XMLArchive();

    virtual ArchiveElement createElement(const QString & name);

    ArchiveElement root() const;

    void setRoot(const ArchiveElement & element);

    bool writeToFile(const QString & file) const;
    bool writeToMem(QByteArray & buffer) const;
    bool readFromFile(const QString & filename);
    bool readFromMem(const QByteArray & buffer);

    /// Returns a pointer to wrapped DOMDocument
    /// @return The wrapped DOMDocument
    DOMDocument * xml();

  private:
    std::shared_ptr<DOMDocument> m_document;
  };
}

#endif // VALUABLE_XMLARCHIVE_HPP
