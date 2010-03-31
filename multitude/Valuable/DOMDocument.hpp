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

#ifndef VALUABLE_DOM_DOCUMENT_HPP
#define VALUABLE_DOM_DOCUMENT_HPP

#include <Radiant/RefPtr.hpp>

#include <Valuable/Export.hpp>

#include <string>
#include <vector>

namespace Valuable
{
  class DOMElement;

  /// Wrapper for QDomDocument or xercesc::DOMDocument
  ///@todo Doc
  class VALUABLE_API DOMDocument
  {
  public:
    struct Wrapped;

    ~DOMDocument();

    static DOMDocument * createDocument();

    DOMElement createElement(const char * name);
    DOMElement createElement(const std::string & name);

    void appendChild(DOMElement element);

    bool writeToFile(const char * filename);
    bool writeToMem(std::vector<char> & buf);

    /// Parse a document from a file.
    /// @param filename name of the file to read from
    /// @param validate if set to true, the XML must validate (it must have a
    /// schema)
    /// @return true if there were no errors
    bool readFromFile(const char * filename, bool validate = false);

    DOMElement getDocumentElement();

  private:
    DOMDocument(Wrapped * doc);
    DOMDocument();

    Wrapped * m_wrapped;
  };

}

#endif
