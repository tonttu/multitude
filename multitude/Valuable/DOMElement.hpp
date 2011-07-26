/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef VALUABLE_DOM_ELEMENT_HPP
#define VALUABLE_DOM_ELEMENT_HPP

#include <Valuable/Export.hpp>

#include <string>
#include <list>

namespace Valuable
{

  /// An element in DOMDocument
  class VALUABLE_API DOMElement
  {
  public:
    /// A list of DOM elements
    /// @todo should be rename to ElementList
    typedef std::list<DOMElement> NodeList;

    DOMElement();
    /// Constructs a copy
    DOMElement(const DOMElement &);
    ~DOMElement();

    /// Copies an element
    DOMElement & operator = (const DOMElement &);

    /// Checks if this is a null element
    bool isNull() const;

    /// Gets the tag name of the element
    std::string getTagName() const;

    /// Appends another element as a child
    void appendChild(DOMElement element);
    /// Sets an attribute for the element
    void setAttribute(const char * name, const char * value);

    /// Checks if this element has an attribute of the given name
    bool hasAttribute(const char * name) const;
    /// Returns the value of an attribute
    std::string getAttribute(const char * name) const;

    /// Gets the text content of this element
    /// @return The text content of this element
    std::string getTextContent() const;
    /// @copydoc getTextContent
    std::wstring getTextContentW() const;

    /// Sets the text content of this element
    void setTextContent(const std::string & content);
    /// Sets the text content of this element
    void setTextContent(const std::wstring & content);

    /// Gets a list of child elements
    NodeList getChildNodes() const;
    /// Gets a list of child elements with the given name
    NodeList selectChildNodes(const char * tagname) const;
    /// Gets a child element with the given name
    DOMElement getChildNode(const char * tagname);

    /// Dumps this elements into the given file
    void dumpInfo(FILE *, int recursion = 0);

  private:
    struct Wrapped;

    DOMElement(Wrapped * x);

    Wrapped * m_wrapped;

    friend class DOMDocument;
  };

}

#endif
