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

#include "CmdParser.hpp"

#include "Valuable/DOMDocument.hpp"
#include "Valuable/DOMElement.hpp"
#include "Valuable/HasValues.hpp"
#include "Valuable/ValueBool.hpp"

namespace Valuable
{
  Radiant::StringUtils::StringList CmdParser::parse(int argc, char * argv[],
                                                    Valuable::HasValues & opts)
  {
    Radiant::StringUtils::StringList list;

    Radiant::RefPtr<Valuable::DOMDocument> tmpDoc(Valuable::DOMDocument::createDocument());
    Valuable::DOMElement e = tmpDoc->createElement("useless");

    for(int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      std::string name;

      if(arg.length() == 2 && arg[0] == '-') {
        name = arg[1];
      } else if(arg.length() > 2 && arg.substr(0, 2) == "--") {
        name = arg.substr(2);
      } else {
        list.push_back(arg);
        continue;
      }

      Valuable::ValueObject * obj = opts.getValue(name);
      if(obj) {
        Valuable::ValueBool * b = dynamic_cast<Valuable::ValueBool*>(obj);
        if(b) {
          *b = true;
        } else if (i < argc - 1) {
          e.setTextContent(argv[++i]);
          obj->deserializeXML(e);
        } else {
          list.push_back(arg);
          Radiant::error("Command line parameter %s is missing an argument", name.c_str());
        }
      } else {
        if(name.length() > 3 && name.substr(0, 3) == "no-") {
          Valuable::ValueBool * b = dynamic_cast<Valuable::ValueBool*>(
              opts.getValue(name.substr(3)));
          if(b) {
            *b = false;
            continue;
          }
        }
        list.push_back(arg);
        Radiant::error("Unknown command line parameter %s", name.c_str());
      }
    }
    return list;
  }
}
