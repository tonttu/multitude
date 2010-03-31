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

#include <Radiant/Trace.hpp>

#include <Valuable/DOMDocument.hpp>
#include <Valuable/DOMElement.hpp>
#include <Valuable/Valuable.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char ** argv)
{

  Valuable::initialize();
  
  for(int i = 1; i < argc; i++) {

    const char * file = argv[i];

    Radiant::info("Reading XML file %s", file);

    Valuable::DOMDocument * doc = Valuable::DOMDocument::createDocument();

    if(!doc->readFromFile(file)) {
      Radiant::error("Failed to read XML file %s", file);
    }
    else
      doc->getDocumentElement().dumpInfo(stdout);

    delete doc;
  }

  Valuable::terminate();

  return 0;
}

