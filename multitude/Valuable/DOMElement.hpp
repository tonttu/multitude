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

  /// Wrapper for xercesc::DOMElement or QDOMElement
  class VALUABLE_API DOMElement
  {
  public:
    struct Wrapped;

    DOMElement();
    DOMElement(const DOMElement &);
    ~DOMElement();

    DOMElement & operator = (const DOMElement &);

    bool isNull() const;

    std::string getTagName() const;

    void appendChild(DOMElement element);
    void setAttribute(const char * name, const char * value);

    bool hasAttribute(const char * name) const;
    std::string getAttribute(const char * name) const;

    std::string getTextContent() const;
    std::wstring getTextContentW() const;

    void setTextContent(const std::string & content);
    void setTextContent(const std::wstring & content);

    typedef std::list<DOMElement> NodeList;
    NodeList getChildNodes() const;
    NodeList selectChildNodes(const char * tagname) const;
    DOMElement getChildNode(const char * tagname);

    void dumpInfo(FILE *, int recursion = 0);

  private:
    DOMElement(Wrapped * x);

    Wrapped * m_wrapped;

    friend class DOMDocument;
  };

}

#endif
