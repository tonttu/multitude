/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: Helsinki University of Technology, MultiTouch Oy and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include <Radiant/FileUtils.hpp>
#include <Radiant/Trace.hpp>

#include <Valuable/DOMDocument.hpp>
#include <Valuable/DOMElement.hpp>
#include <Valuable/ConfigDocument.hpp>
#include <Valuable/Valuable.hpp>

#include <stdlib.h>
#include <string.h>

int main(int argc, char ** argv)
{

  Valuable::initialize();
  
  for(int i = 1; i < argc; i++) {

    const char * file = argv[i];

    Valuable::DOMDocument * xdoc = Valuable::DOMDocument::createDocument();

    std::string suffix(Radiant::FileUtils::suffixLowerCase(file));
      
    if(suffix == "xml" || suffix == "xhtml") {

      Radiant::info("Reading XML file %s", file);
      
      if(!xdoc->readFromFile(file)) {
	Radiant::error("Failed to read XML file %s", file);
      }
      else {
	Valuable::ConfigElement ce;
	
	Valuable::convert(ce, xdoc->getDocumentElement());
	
	printf(Valuable::ConfigDocument::getConfigText(ce).c_str());
      }
    }
    else {
      Valuable::ConfigDocument cdoc;
      cdoc.readConfigFile(file);

      printf(Valuable::ConfigDocument::getConfigText(cdoc.root()).c_str());

      Valuable::DOMElement e(xdoc->getDocumentElement());
      cdoc.root().setElementName("X");
      Valuable::convert(*xdoc, e, cdoc.root());
    }
    
    delete xdoc;
  }

  Valuable::terminate();

  return 0;
}

