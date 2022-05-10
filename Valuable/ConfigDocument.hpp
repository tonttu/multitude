/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
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

  /// Text document used to configure various settings with key-value pairs.
  /// @todo Doc, rename getXyz to xyz
  class VALUABLE_API ConfigDocument
  {
  public:
    /// Creates an empty ConfigDocument object
    ConfigDocument(void);
    virtual ~ConfigDocument(void);

    /// Reads a configuration from a file
    bool readConfigFile(const char *fileName);
    /// Writes the configuration to a file
    void writeConfigFile(const char *fileName);
    /// Writes the configuration to a stream
    void writeConfig(std::ostream &);

    /// Returns the root configuration element
    ConfigElement &root() { return m_doc; }

    /// Gets the configuration element with the given name
    /// @param key name of the element
    /// @return pointer to the element or 0 if element is not found
    ConfigElement *getConfigElement(const QString & key);
    /// @copybrief getConfigElement
    ConfigElement *getConfigElement(const QString & key, const QString & value);
    /// Returns the configuration as string
    static QString getConfigText(ConfigElement e, int recursion = 0);

  private:

    void loadConfigElement(std::string str);
    static void trimSpaces( std::string & str);
    static void trimSpaces( QString & str);
    void loadConfigValue(std::string key,std::string val);
    ConfigElement *findConfigElement(ConfigElement &e,const QString & elementName,bool &found);
    ConfigElement *findConfigElement(ConfigElement &e,bool &found,const QString& key, const QString & value);

    bool getline(FILE * source, std::string & str);

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

  /// Converts from text element to XML element
  void VALUABLE_API convert(DOMDocument  & doc, DOMElement & to, const ConfigElement & from);
  /// Converts from XML element to text element
  void VALUABLE_API convert(ConfigElement & to, DOMElement from);

}

#endif
