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

#include <Radiant/Trace.hpp>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>
#include <xercesc/dom/DOMLocator.hpp>

#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <xercesc/util/RuntimeException.hpp>
#include <xercesc/util/XMLUniDefs.hpp>

namespace Valuable
{
  struct DOMDocument::Wrapped {
      xercesc::DOMDocument * x;
  };

  inline xercesc::DOMDocument * XDOC(DOMDocument::Wrapped * x) { return reinterpret_cast<xercesc::DOMDocument *> (x); }
  inline xercesc::DOMElement * XELEM(DOMElement::Wrapped * x) { return reinterpret_cast<xercesc::DOMElement *> (x); }

  inline DOMDocument::Wrapped * DOC(xercesc::DOMDocument * x) { return reinterpret_cast<DOMDocument::Wrapped *> (x); }
  inline DOMElement::Wrapped * ELEM(xercesc::DOMElement * x) { return reinterpret_cast<DOMElement::Wrapped *> (x); }

  DOMDocument::DOMDocument(DOMDocument::Wrapped * doc)
    : m_wrapped(doc)
  {}

  DOMDocument::~DOMDocument()
  {
    if(m_wrapped)
      XDOC(m_wrapped)->release();
  }

  void DOMDocument::appendChild(DOMElement element)
  {
    if(element.isNull()) return;

    assert(m_wrapped != 0);

    XDOC(m_wrapped)->appendChild(XELEM(element.m_wrapped));
  }

  DOMDocument * DOMDocument::createDocument()
  {
    const XMLCh LS[] =
      { xercesc::chLatin_L, xercesc::chLatin_S, xercesc::chNull };

    xercesc::DOMImplementation * impl =
      xercesc::DOMImplementationRegistry::getDOMImplementation(LS);

    // Create a document & writer
    xercesc::DOMDocument * doc = impl->createDocument();
    return new DOMDocument(DOC(doc));
  }

  DOMElement DOMDocument::createElement(const char * name)
  {
    XMLCh * xName = xercesc::XMLString::transcode(name);
    xercesc::DOMElement * elem = 0;

    try {
      elem = XDOC(m_wrapped)->createElement(xName);
    } catch(const xercesc::DOMException & e) {
      char * msg = xercesc::XMLString::transcode(e.getMessage());
      Radiant::error(
"DOMDocument::createElement # %s", msg);
      xercesc::XMLString::release(&msg);
    }

    xercesc::XMLString::release(&xName);

    return DOMElement(ELEM(elem));
  }

  DOMElement DOMDocument::createElement(const std::string & name)
  {
    return createElement(name.c_str());
  }

  bool writeDom(DOMDocument::Wrapped * doc, xercesc::XMLFormatTarget & target)
  {
     // Get implementation of the Load-Store (LS) interface
    const XMLCh LS[] = {xercesc::chLatin_L, xercesc::chLatin_S, xercesc::chNull};
    xercesc::DOMImplementation * impl = xercesc::DOMImplementationRegistry::getDOMImplementation(LS);

    // Create a document & writer
    xercesc::DOMWriter * writer = ((xercesc::DOMImplementationLS*)impl)->createDOMWriter();

    bool result = true;

    try {
      // Output pretty XML
      writer->setFeature(xercesc::XMLUni::fgDOMWRTFormatPrettyPrint, true);
      writer->writeNode(&target, *XDOC(doc));

      // Flush the target just to be sure all contents are written
      target.flush();
    } catch(const xercesc::XMLException & e)  {
      char * msg = xercesc::XMLString::transcode(e.getMessage());
      Radiant::error(
"DOMDocument::save # %s", msg);
      xercesc::XMLString::release(&msg);
      result = false;
    } catch(const xercesc::DOMException & e) {
      char * msg = xercesc::XMLString::transcode(e.msg);
      Radiant::error(
"DOMDocument::save # %s", msg);
      xercesc::XMLString::release(&msg);
      result = false;
    }

    // Cleanup
    writer->release();

    return result;
  }

  bool DOMDocument::writeToFile(const char * filename)
  {
    try {
      xercesc::LocalFileFormatTarget target(filename);
      return writeDom(m_wrapped, target);
    } catch(...) {
      return false;
    }
  }

  bool DOMDocument::writeToMem(std::vector<char> & buffer)
  {
    xercesc::MemBufFormatTarget target;
    if(!writeDom(m_wrapped, target)) {
      buffer.clear();
      return false;
    }

    size_t bytes = target.getLen();
    buffer.resize(bytes);

    memcpy(&buffer[0], target.getRawBuffer(), bytes);
    return true;
  }

  class ErrorHandler : public xercesc::DOMErrorHandler
  {
    public:
      ErrorHandler() {}

      // return false = stop processing, return true = keep processing
      virtual bool handleError(const xercesc::DOMError & e) {

        char * uri = xercesc::XMLString::transcode(e.getLocation()->getURI());
        int line = e.getLocation()->getLineNumber();
        char * msg = xercesc::XMLString::transcode(e.getMessage());

        Radiant::error(
"[XML] %s:%d: %s", uri, line, msg);

        xercesc::XMLString::release(&uri);
        xercesc::XMLString::release(&msg);

        return true;
      }
  };

  bool DOMDocument::readFromFile(const char * filename, bool validate)
  {
    // Get implementation of the Load-Store (LS) interface
    const XMLCh LS[] = {xercesc::chLatin_L, xercesc::chLatin_S, xercesc::chNull};
    xercesc::DOMImplementation * impl = xercesc::DOMImplementationRegistry::getDOMImplementation(LS);
	
    // Create a parser
    xercesc::DOMBuilder * parser = ((xercesc::DOMImplementationLS*)impl)->createDOMBuilder
      (xercesc::DOMImplementationLS::MODE_SYNCHRONOUS, 0);

  if(validate) {
	parser->setFeature(xercesc::XMLUni::fgDOMNamespaces, true);
    parser->setFeature(xercesc::XMLUni::fgXercesSchema, true);
    parser->setFeature(xercesc::XMLUni::fgXercesSchemaFullChecking, true);
    parser->setFeature(xercesc::XMLUni::fgDOMValidation, true);
    parser->setFeature(xercesc::XMLUni::fgDOMDatatypeNormalization, true);    
  }

  ErrorHandler * handler = new ErrorHandler;
  parser->setErrorHandler(handler);

	xercesc::DOMDocument * parsed = 0;

	try {
		parsed = parser->parseURI(filename);
	} catch(xercesc::RuntimeException e) {
		char * msg = xercesc::XMLString::transcode(e.getMessage());
		
		Radiant::error(
"DOMDocument::readFromFile # %s", msg);
		parser->release();
		xercesc::XMLString::release(&msg);
		return false;
	}
   
    // Clone the document because the parsed
    if(m_wrapped)
      XDOC(m_wrapped)->release();

    if(parsed)
      m_wrapped = DOC((xercesc::DOMDocument *) parsed->cloneNode((parsed == 0) ? false: true));
    else
      m_wrapped = 0;

    parser->release();
    delete handler;

    return (m_wrapped != 0);
  }
   
  DOMElement DOMDocument::getDocumentElement()
  {
    xercesc::DOMElement * de = XDOC(m_wrapped)->getDocumentElement();
    return DOMElement(ELEM(de));
  }

}
