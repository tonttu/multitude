/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_DOM_DOCUMENT_HPP
#define VALUABLE_DOM_DOCUMENT_HPP

#include <memory>

#include <Valuable/Export.hpp>

#include <QString>
#include <vector>

namespace Valuable
{
  class DOMElement;

  /// Represents a DOM document
  class VALUABLE_API DOMDocument
  {
  public:
    ~DOMDocument();

    /// Creates a new DOM document
    static DOMDocument * createDocument();

    /// Creates a new DOM element with the given name
    /// @param name Tag name of the DOM element
    /// @return The created element
    DOMElement createElement(const QString & name);

    /// Appends an element to the document
    void appendChild(DOMElement element);

    /// Writes the document to a file
    bool writeToFile(const QString & filename);
    /// Writes the document to memory
    bool writeToMem(QByteArray & buf);

    /// Parse a document from a file.
    /// @param filename name of the file to read from
    /// @param validate if set to true, the XML must validate (it must have a
    /// schema)
    /// @return true if there were no errors
    bool readFromFile(const QString & filename, bool validate = false);

    bool readFromMem(const QByteArray & buffer);

    /// Returns the main document element
    DOMElement getDocumentElement();

  private:
    struct Wrapped;

    DOMDocument(Wrapped * doc);
    DOMDocument();

    Wrapped * m_wrapped;
  };

}

#endif
