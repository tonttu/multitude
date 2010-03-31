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

#include "DOMDocument.hpp"
#include "DOMElement.hpp"

#include <Radiant/FileUtils.hpp>
#include <Radiant/Trace.hpp>

#include <QDomDocument>
#include <QFile>

#include <assert.h>

namespace Valuable
{
  using namespace Radiant;

  struct DOMDocument::Wrapped {
    Wrapped() : x("mtdoc")
    {}

    QDomDocument x;
  };

  struct DOMElement::Wrapped {
    QDomElement x;
  };

  DOMDocument::DOMDocument(DOMDocument::Wrapped * doc)
    : m_wrapped(doc)
  {}

  DOMDocument::DOMDocument()
    : m_wrapped(new Wrapped)
  {}

  DOMDocument::~DOMDocument()
  {
    delete m_wrapped;
  }

  void DOMDocument::appendChild(DOMElement element)
  {
    if(element.isNull()) return;

    m_wrapped->x.appendChild(element.m_wrapped->x);

    /*

    QDomElement e = m_wrapped->x.documentElement();
    if(e.isNull()) {
      e = m_wrapped->x.createElement("root");
      doc.appendChild(e);
    }
    
    e.appendChild(element.m_wrapped->x);
    */
  }

  DOMDocument * DOMDocument::createDocument()
  {
    return new DOMDocument();
  }

  DOMElement DOMDocument::createElement(const char * name)
  {
    QDomElement de = m_wrapped->x.createElement(name);
    DOMElement r;
    r.m_wrapped->x = de;
    return r;
  }

  DOMElement DOMDocument::createElement(const std::string & name)
  {
    return createElement(name.c_str());
  }


  bool DOMDocument::writeToFile(const char * filename)
  {
    return Radiant::FileUtils::writeTextFile
      (filename, m_wrapped->x.toByteArray().data());
  }

  bool DOMDocument::writeToMem(std::vector<char> & buffer)
  {
    QDomDocument & qdoc = m_wrapped->x;
    std::string xml = qdoc.toString().toStdString();

    buffer.resize(xml.size());

    memcpy( & buffer[0], xml.c_str(), xml.size());
    return true;
  }

  bool DOMDocument::readFromFile(const char * filename, bool /*validate*/)
  {
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly))
      return false;

    QString errstr;
    int errline = 0;

    if(!m_wrapped->x.setContent( & file, & errstr, & errline)) {
      file.close();
      error("DOMDocument::readFromFile # Cannot read file %s, line %d: %s",
            filename, errline, errstr.toStdString().c_str());
      return false;
    }

    file.close();
    return true;
  }
   
  DOMElement DOMDocument::getDocumentElement()
  {
    DOMElement r;
    r.m_wrapped->x = m_wrapped->x.documentElement();
    return r;
  }

}
