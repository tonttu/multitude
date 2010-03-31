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

#ifndef VALUABLE_CONFIG_DOCUMENT_HPP
#define VALUABLE_CONFIG_DOCUMENT_HPP

#include <Valuable/ConfigElement.hpp>
#include <Valuable/DOMDocument.hpp>
#include <Valuable/DOMElement.hpp>
#include <Valuable/Export.hpp>

namespace Valuable
{
  ///@todo Doc, rename getXyz to xyz
  class VALUABLE_API ConfigDocument
  {
  public:
    ConfigDocument(void);
    virtual ~ConfigDocument(void);

    void readConfigFile(const char *fileName);
    void writeConfigFile(const char *fileName);
    void writeConfig(std::ostream &);

    ConfigElement &root() { return m_doc; }
    ConfigElement *getConfigElement(std::string elementName);
    ConfigElement *getConfigElement(std::string key,std::string value);

    static std::string getConfigText(ConfigElement e, int recursion = 0);

  private:

    void loadConfigElement(std::string str);
    static void trimSpaces( std::string & str);
    void loadConfigValue(std::string key,std::string val);
    ConfigElement *findConfigElement(ConfigElement &e,std::string elementName,bool &found);
    ConfigElement *findConfigElement(ConfigElement &e,bool &found,std::string key,std::string value);

    enum ParseFlags
    {
      ELEMENT_START,
      ELEMENT_END,
      ATTRIBUTE,
      NOT_VALID
    };

    ParseFlags parseLine(std::string line);

    ConfigElement m_doc;
  };

  void VALUABLE_API convert(DOMDocument  & doc, DOMElement & to, const ConfigElement & from);
  void VALUABLE_API convert(ConfigElement & to, DOMElement from);

}

#endif
