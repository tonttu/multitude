/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
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

  DOMElement DOMDocument::createElement(const QString & name)
  {
    QDomElement de = m_wrapped->x.createElement(name);
    DOMElement r;
    r.m_wrapped->x = de;
    return r;
  }


  bool DOMDocument::writeToFile(const QString & filename)
  {
    return Radiant::FileUtils::writeTextFile
      (filename.toUtf8().data(), m_wrapped->x.toByteArray().data());
  }

  bool DOMDocument::writeToMem(QByteArray & buffer)
  {
    QDomDocument & qdoc = m_wrapped->x;
    buffer = qdoc.toByteArray();
    return true;
  }

  bool DOMDocument::readFromFile(const QString & filename, bool /*validate*/)
  {
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly))
      return false;

    QString errstr;
    int errline = 0;

    if(!m_wrapped->x.setContent( & file, & errstr, & errline)) {
      file.close();
      Radiant::error("DOMDocument::readFromFile # Cannot read file %s, line %d: %s",
            filename.toUtf8().data(), errline, errstr.toUtf8().data());
      return false;
    }

    file.close();
    return true;
  }

  bool DOMDocument::readFromMem(const QByteArray & buffer)
  {
    QString errstr;
    int errline = 0;

    if(!m_wrapped->x.setContent(buffer, &errstr, &errline)) {
      Radiant::error("DOMDocument::readFromMem # Cannot parse line %d: %s",
            errline, errstr.toUtf8().data());
      return false;
    }
    return true;
  }

  DOMElement DOMDocument::getDocumentElement()
  {
    DOMElement r;
    r.m_wrapped->x = m_wrapped->x.documentElement();
    return r;
  }

}
