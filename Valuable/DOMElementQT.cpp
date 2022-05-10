/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "DOMElement.hpp"

#include <QDomElement>

#include <Radiant/Trace.hpp>

#include <stdio.h>

#include <assert.h>

namespace Valuable
{
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

  DOMElement::DOMElement(DOMElement && that)
    : m_wrapped(that.m_wrapped)
  {
    that.m_wrapped = nullptr;
  }

  DOMElement & DOMElement::operator=(DOMElement && that)
  {
    std::swap(m_wrapped, that.m_wrapped);
    return *this;
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

  QString DOMElement::getTagName() const
  {
    if(!m_wrapped)
      return QString();
    
    return m_wrapped->x.tagName();
  }

  void DOMElement::setTagName(const QString & name)
  {
    m_wrapped->x.setTagName(name);
  }

  void DOMElement::appendChild(const DOMElement & element)
  {
    m_wrapped->x.appendChild(element.m_wrapped->x);
  }

  void DOMElement::setAttribute(const QString & name, const QString & value)
  {
    if(!m_wrapped)
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
  
  DOMElement::NodeList DOMElement::selectChildNodes(const QString & tagname) const
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

  DOMElement DOMElement::getChildNode(const QString & tagname)
  {
    NodeList nodes = getChildNodes();
    
    for(NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
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

  void DOMElement::dumpInfo(FILE * f, int recursion) const
  {
    NodeList nodes = getChildNodes();

    addSpace(f, recursion);
    fprintf(f, "NODE <%s> (%d children, %d deep)",
            getTagName().toUtf8().data(), (int) nodes.size(), recursion);

    QString str = getTextContent();
    if(str.size() > 0 && str.size() < 100) {
      fprintf(f, " TEXT = \"%s\"", str.toUtf8().data());
    }
    else {
      QString tmp = str.left(100) + "...";
      fprintf(f, " TEXT = \"%s\"", tmp.toUtf8().data());
    }

    fprintf(f, "\n");

    int i = 1;

    fflush(f);

    for(NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
      addSpace(f, recursion);
      fprintf(f, "Child %d/%d\n", i, (int) nodes.size());
      (*it).dumpInfo(f, recursion + 1);
      i++;
    }
  }

  const QDomElement & DOMElement::qDomElement() const
  {
    return m_wrapped->x;
  }

  void DOMElement::setTextContent(const QString & s)
  {
      if(isNull()) {
          Radiant::error("DOMElement::setTextContent # can not set content of a null element");
          return;
      }
    
    QDomElement & qde = m_wrapped->x;

    for(QDomNode child = qde.firstChild(); !child.isNull(); child = child.nextSibling()) {
      if(child.isText()) {
        qde.removeChild(child);
        break;
      }
    }

    qde.appendChild(qde.ownerDocument().createTextNode(s));
  }

  QString DOMElement::getTextContent() const
  {
      if(isNull()) {
          Radiant::error("DOMElement::getTextContent # can not get content of a null element");
          return QString();
      }

      return m_wrapped->x.text();
  }

  bool DOMElement::hasAttribute(const QString & name) const
  {
    if(isNull())
      return false;

    return m_wrapped->x.hasAttribute(name);
  }

  QString DOMElement::getAttribute(const QString & name) const
  {
      if(isNull()) {
          Radiant::error("DOMElement::getAttribute # can not get attribute from a null element");
          return QString();
      }

    return m_wrapped->x.attribute(name);
  }

}
