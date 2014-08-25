/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_DOM_ELEMENT_HPP
#define VALUABLE_DOM_ELEMENT_HPP

#include <Valuable/Export.hpp>

#include <QString>
#include <list>

class QDomElement;

namespace Valuable
{

  /// An element in DOMDocument
  /// @todo this class and other DOM classes could be removed and replaced with
  ///       some helper functions that work directly with pure Qt classes
  class VALUABLE_API DOMElement
  {
  public:
    /// A list of DOM elements
    /// @todo should be rename to ElementList
    typedef std::list<DOMElement> NodeList;

    DOMElement();
    /// Constructs a copy
    DOMElement(const DOMElement &);
    DOMElement(DOMElement &&);
    DOMElement& operator=(DOMElement &&);
    ~DOMElement();

    /// Copies an element
    DOMElement & operator = (const DOMElement &);

    /// Checks if this is a null element
    bool isNull() const;

    /// Gets the tag name of the element
    QString getTagName() const;
    void setTagName(const QString & name);

    /// Appends another element as a child
    void appendChild(const DOMElement & element);
    /// Sets an attribute for the element
    void setAttribute(const QString & name, const QString & value);

    /// Checks if this element has an attribute of the given name
    bool hasAttribute(const QString & name) const;
    /// Returns the value of an attribute
    QString getAttribute(const QString & name) const;

    /// Gets the text content of this element
    /// @return The text content of this element
    QString getTextContent() const;

    /// Sets the text content of this element
    void setTextContent(const QString & content);

    /// Gets a list of child elements
    NodeList getChildNodes() const;
    /// Gets a list of child elements with the given name
    NodeList selectChildNodes(const QString & tagname) const;
    /// Gets a child element with the given name
    DOMElement getChildNode(const QString & tagname);

    /// Dumps this elements into the given file
    void dumpInfo(FILE *, int recursion = 0) const;

    /// @cond

    const QDomElement & qDomElement() const;

    /// @endcond
  private:
    struct Wrapped;

    DOMElement(Wrapped * x);

    Wrapped * m_wrapped;

    friend class DOMDocument;
  };

}

#endif
