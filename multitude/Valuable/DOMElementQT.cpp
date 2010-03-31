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

#include "DOMElement.hpp"

#include <QDomElement>

#include <Radiant/Trace.hpp>

#include <stdio.h>

#include <assert.h>

namespace Valuable
{
  using namespace Radiant;

  struct DOMElement::Wrapped
  {
    QDomElement x;
  };
  
  DOMElement::DOMElement()
      : m_wrapped(new Wrapped())
  {}

  DOMElement::DOMElement(Wrapped * )
  {
    assert("Not to be used with Qt" == 0);
  }

  DOMElement::DOMElement(const DOMElement & that)
      : m_wrapped(new Wrapped())
  {
    m_wrapped->x = that.m_wrapped->x;
  }

  DOMElement::~DOMElement()
  {
    delete m_wrapped;
  }

  DOMElement & DOMElement::operator = (const DOMElement & that)
  {
    m_wrapped->x = that.m_wrapped->x;
    return * this;
  }

  bool DOMElement::isNull() const
  {
    return m_wrapped->x.isNull();
  }

  std::string DOMElement::getTagName() const
  {
    if(!m_wrapped)
      return std::string();
    
    return m_wrapped->x.tagName().toStdString();
  }

  void DOMElement::appendChild(DOMElement element)
  {
    m_wrapped->x.appendChild(element.m_wrapped->x);
  }

  void DOMElement::setAttribute(const char * name, const char * value)
  {
    if(!value || !name || !m_wrapped)
      return;

    if(!strlen(value))
      return;

    m_wrapped->x.setAttribute(name, value);
  }

  DOMElement::NodeList DOMElement::getChildNodes() const
  {
    NodeList list; 

    if(!m_wrapped)
      return list;

    QDomNode n = m_wrapped->x.firstChild();

    while(!n.isNull()) {

      QDomElement de = n.toElement();

      if(!de.isNull()) {
        DOMElement de2;
        de2.m_wrapped->x = de;
        list.push_back(de2);
      }

      n = n.nextSibling();
    }
    
    return list;
  }
  
  DOMElement::NodeList DOMElement::selectChildNodes(const char * tagname) const
  {
    NodeList list; 

    if(!m_wrapped)
      return list;

    QDomNode n = m_wrapped->x.firstChild();

    while(!n.isNull()) {

      QDomElement de = n.toElement();

      if(!de.isNull() && de.tagName() == tagname) {
        DOMElement de2;
        de2.m_wrapped->x = de;
        list.push_back(de2);
      }

      n = n.nextSibling();
    }
    
    return list; 
  }

  DOMElement DOMElement::getChildNode(const char * tagname)
  {
    NodeList nodes = getChildNodes();
    
    for(NodeList::iterator it = nodes.begin(); it != nodes.end(); it++) {
      DOMElement e = *it;
      if(e.getTagName() == tagname)
        return e;
    }

    return DOMElement();
  }

  static void addSpace(FILE * f, int n)
  {
    for(int i = 0; i < n; i++) {
      fprintf(f, "  ");
    }
  }

  void DOMElement::dumpInfo(FILE * f, int recursion)
  {
    NodeList nodes = getChildNodes();

    addSpace(f, recursion);
    fprintf(f, "NODE <%s> (%d children, %d deep)",
            getTagName().c_str(), (int) nodes.size(), recursion);

    std::string str = getTextContent();
    if(str.size() > 0 && str.size() < 100) {
      fprintf(f, " TEXT = \"%s\"", str.c_str());
    }

    fprintf(f, "\n");

    int i = 1;

    fflush(f);

    for(NodeList::iterator it = nodes.begin(); it != nodes.end(); it++) {
      addSpace(f, recursion);
      fprintf(f, "Child %d/%d\n", i, (int) nodes.size());
      (*it).dumpInfo(f, recursion + 1);
      i++;
    }
  }

  void DOMElement::setTextContent(const std::string & s)
  {
      if(isNull()) {
          Radiant::error("DOMElement::setTextContent # can not set content of a null element");
          return;
      }
    
    QDomElement & qde = m_wrapped->x;
    qde.appendChild(qde.ownerDocument().createTextNode(s.c_str()));
  }

  void DOMElement::setTextContent(const std::wstring & ws)
  {
      if(isNull()) {
          Radiant::error("DOMElement::setTextContent # can not set content of a null element");
          return;
      }
    
    QString qs(QString::fromStdWString(ws));
    QDomElement & qde = m_wrapped->x;
    // info("WIDE Text content : %s : %d", qs.toStdString().c_str(), (int) ws.size());
    qde.appendChild(qde.ownerDocument().createTextNode(qs));
  }

  std::string DOMElement::getTextContent() const
  {
      if(isNull()) {
          Radiant::error("DOMElement::getTextContent # can not get content of a null element");
          return std::string();
      }

    return m_wrapped->x.text().toStdString();
  }

  std::wstring DOMElement::getTextContentW() const
  {
      if(isNull()) {
          Radiant::error("DOMElement::setTextContent # can not get content of a null element");
          return std::wstring();
      }

    return m_wrapped->x.text().toStdWString();
  }

  bool DOMElement::hasAttribute(const char * name) const
  {
    if(isNull())
      return false;

    return m_wrapped->x.hasAttribute(name);
  }

  std::string DOMElement::getAttribute(const char * name) const
  {
      if(isNull()) {
          Radiant::error("DOMElement::getAttribute # can not get attribute from a null element");
          return std::string();
      }

    return m_wrapped->x.attribute(name).toStdString();
  }

}
