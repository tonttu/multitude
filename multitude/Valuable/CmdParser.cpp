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

#include <QStringList>

namespace Valuable
{
  QStringList CmdParser::parse(int argc, char * argv[],
                               Valuable::HasValues & opts)
  {
    QStringList list;

    std::shared_ptr<Valuable::DOMDocument> tmpDoc(Valuable::DOMDocument::createDocument());

    for(int i = 1; i < argc; i++) {
      QString arg = argv[i];
      QString name;

      if(arg.length() == 2 && arg[0] == '-') {
        name = arg[1];
      } else if(arg.length() > 2 && arg.startsWith("--")) {
        name = arg.mid(2);
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
          Valuable::DOMElement e = tmpDoc->createElement("tmp");
          e.setTextContent(argv[++i]);
          obj->deserializeXML(e);
        } else {
          list.push_back(arg);
          Radiant::error("Command line parameter %s is missing an argument", name.toUtf8().data());
        }
      } else {
        if(name.length() > 3 && name.startsWith("no-")) {
          Valuable::ValueBool * b = dynamic_cast<Valuable::ValueBool*>(
              opts.getValue(name.mid(3)));
          if(b) {
            *b = false;
            continue;
          }
        }
        list.push_back(arg);
        //Radiant::error("Unknown command line parameter %s", name.c_str());
      }
    }
    return list;
  }
}
