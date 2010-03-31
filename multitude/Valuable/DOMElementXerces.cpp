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

#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/util/XMLString.hpp>

#include <Radiant/Trace.hpp>

#include <stdio.h>

namespace Valuable
{
  struct DOMElement::Wrapped
  {
    xercesc::DOMElement * x;
  };

  inline xercesc::DOMElement * XELEM(DOMElement::Wrapped * x) { return reinterpret_cast <xercesc::DOMElement *> (x); }
  inline DOMElement::Wrapped * ELEM(xercesc::DOMElement * x) { return reinterpret_cast <DOMElement::Wrapped *> (x); }
  
  DOMElement::DOMElement(DOMElement::Wrapped * x)
  : m_wrapped(x)
  {}
  
  std::string DOMElement::getTagName() const
  {
    if(!m_wrapped)
      return std::string();

    char * name = xercesc::XMLString::transcode(XELEM(m_wrapped)->getTagName());
    std::string str(name);
    xercesc::XMLString::release(&name);

    return str;    
  }

  void DOMElement::appendChild(DOMElement element)
  {
    XELEM(m_wrapped)->appendChild(XELEM(element.m_wrapped));
  }

  void DOMElement::setAttribute(const char * name, const char * value)
  {
    if(!value || !name)
      return;

    if(!strlen(value))
      return;

    XMLCh * xName = xercesc::XMLString::transcode(name);
    XMLCh * xValue = xercesc::XMLString::transcode(value);

    XELEM(m_wrapped)->setAttribute(xName, xValue);

    xercesc::XMLString::release(&xName);
    xercesc::XMLString::release(&xValue);
  }

  DOMElement::NodeList DOMElement::getChildNodes() const
  {
    NodeList list; 

    if(!m_wrapped)
      return list;

    xercesc::DOMNodeList * xList = XELEM(m_wrapped)->getChildNodes();

    for(XMLSize_t i = 0; i < xList->getLength(); i++) {
      xercesc::DOMElement * xe = dynamic_cast<xercesc::DOMElement *> (xList->item(i));
      if(!xe) continue;

      list.push_back(DOMElement(ELEM(xe)));    
    }
    
    return list;
  }
  
  DOMElement::NodeList DOMElement::selectChildNodes(const char * tagname) const
  {
    
    NodeList list; 

    if(!m_wrapped)
      return list;

    xercesc::DOMNodeList * xList = XELEM(m_wrapped)->getChildNodes();

    for(XMLSize_t i = 0; i < xList->getLength(); i++) {
      xercesc::DOMElement * xe = dynamic_cast<xercesc::DOMElement *> (xList->item(i));
      if(!xe) continue;
      
      char * name = xercesc::XMLString::transcode(XELEM((Wrapped *) xe)->getTagName());

      if(strcmp(name, tagname) == 0)
	list.push_back(DOMElement(ELEM(xe)));

      xercesc::XMLString::release(&name);
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

    return DOMElement(0);
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
    if(isNull())
      return;

    XMLCh * xCont = xercesc::XMLString::transcode(s.c_str());
    XELEM(m_wrapped)->setTextContent(xCont);
    xercesc::XMLString::release(&xCont);
  }

  void DOMElement::setTextContent(const std::wstring & ws)
  {
    if(isNull())
      return;

    std::basic_string<XMLCh> xs(ws.begin(), ws.end());

    XELEM(m_wrapped)->setTextContent(xs.c_str());
  }

  std::string DOMElement::getTextContent() const
  {
    if(isNull())
      return std::string();

    const XMLCh * xContent = XELEM(m_wrapped)->getTextContent();
    char * content = xercesc::XMLString::transcode(xContent);

    std::string result(content);
    xercesc::XMLString::release(&content);

    return result;
  }

  std::wstring DOMElement::getTextContentW() const
  {
    if(isNull())
      return std::wstring();

    const XMLCh * xContent = XELEM(m_wrapped)->getTextContent();
    size_t len = xercesc::XMLString::stringLen(xContent);
      
    std::wstring result;
    result.resize(len);

    for(size_t i = 0; i < len; i++)
      result[i] = xContent[i];
   
    return result;    
  }

  bool DOMElement::hasAttribute(const char * name) const
  {
    if(isNull())
      return false;

    XMLCh * xName = xercesc::XMLString::transcode(name);
    bool r = XELEM(m_wrapped)->hasAttribute(xName);
    xercesc::XMLString::release(&xName);

    return r;
  }

  std::string DOMElement::getAttribute(const char * name) const
  {
    if(isNull())
      return std::string();

    XMLCh * xName = xercesc::XMLString::transcode(name);

    const XMLCh * xValue = XELEM(m_wrapped)->getAttribute(xName);
    char * value = xercesc::XMLString::transcode(xValue);

    std::string r;
    if(value) r = value;

    xercesc::XMLString::release(&value);
    xercesc::XMLString::release(&xName);

    return r;
  }

}
