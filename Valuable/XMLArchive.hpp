/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
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

    virtual void add(ArchiveElementImpl & element) OVERRIDE;
    virtual ArchiveIterator children() const OVERRIDE;

    virtual void add(const QString & name, const QString & value) OVERRIDE;
    virtual QString get(const QString & name) const OVERRIDE;

    virtual void set(const QString & s) OVERRIDE;
    virtual QString get() const OVERRIDE;

    virtual QString name() const OVERRIDE;
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

    /// Serialize archive as XML to a file
    /// @param filename file to open
    /// @return true if writing was successful
    bool writeToFile(const QString & filename) const;
    /// Serialize archive as XML to a buffer
    /// @param buffer that will be overwritten with the new data
    /// @return true if writing was successful
    bool writeToMem(QByteArray & buffer) const;
    /// Read archive contents from a file
    /// @param filename file to open
    /// @returns true if reading was successful
    bool readFromFile(const QString & filename);
    /// Read archive contents from a buffer
    /// @param buffer data buffer to read the data from
    /// @returns true if reading was successful
    bool readFromMem(const QByteArray & buffer);

    /// Returns a pointer to wrapped DOMDocument
    /// @return The wrapped DOMDocument
    DOMDocument * xml();

    /// Clean string so that it is a valid XML element name
    static QString cleanElementName(QString name);

  private:
    std::shared_ptr<DOMDocument> m_document;
  };
}

#endif // VALUABLE_XMLARCHIVE_HPP
