/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "CmdParser.hpp"

#include "Valuable/DOMDocument.hpp"
#include "Valuable/DOMElement.hpp"
#include "Valuable/Node.hpp"
#include "Valuable/AttributeAlias.hpp"
#include "Valuable/AttributeBool.hpp"
#include "Valuable/AttributeStringList.hpp"

#include <QStringList>

#ifdef RADIANT_OSX
#include <crt_externs.h>
#endif

namespace Valuable
{
  QStringList CmdParser::parse(int & argc, char * argv[],
                               Valuable::Node & opts)
  {
    CmdParser parser;
    return parser.parseAndStore(argc, argv, opts);
  }

  QStringList CmdParser::parse(const QStringList & argv,
                               Valuable::Node & opts)
  {
    CmdParser parser;
    return parser.parseAndStore(argv, opts);
  }

  bool CmdParser::isParsed(const QString & name)
  {
    return m_parsedArgs.contains(name);
  }

  QStringList CmdParser::parseAndStore(int & argc, char * argv[],
                                       Valuable::Node & opts)
  {
    QStringList tmp;
    for(int i = 1; i < argc; ++i)
      tmp << QString::fromLocal8Bit(argv[i]);

    QStringList out = parseAndStore(tmp, opts);

    int argv_out = 1;
    for(int i = 1, j = 0; i < argc; ++i) {
      if(j < out.size() && argv[i] == out[j]) {
        argv[argv_out++] = argv[i];
        ++j;
      }
    }

    // Set removed arguments to null. If argv is created by Qt in WinMain, we
    // will get double-delete unless we clear these here. See
    // https://redmine.multitaction.com/issues/16597
    for (int i = argv_out; i < argc; ++i)
      argv[i] = nullptr;

    argc = argv_out;

#ifdef RADIANT_OSX
    /// If we are processing global command line arguments, also update the
    /// global argc value, see https://redmine.multitouch.fi/issues/11998
    if (argv == *_NSGetArgv()) {
      *_NSGetArgc() = argc;
    }
#endif

    return out;
  }

  QStringList CmdParser::parseAndStore(const QStringList & argv,
                                       Valuable::Node & opts)
  {
    QStringList list;

    std::shared_ptr<Valuable::DOMDocument> tmpDoc(Valuable::DOMDocument::createDocument());

    int argc = argv.size();
    for(int i = 0; i < argc; i++) {
      const QString & arg = argv[i];
      QString name;

      if(arg.length() == 2 && arg[0] == '-') {
        name = arg[1];
      } else if(arg.length() > 2 && arg.startsWith("--")) {
        name = arg.mid(2);
      } else {
        list.push_back(arg);
        continue;
      }

      Valuable::Attribute * obj = opts.attribute(name.toUtf8());
      if(obj) {
        // check if we have an alias for an AttributeBool
        Valuable::AttributeAlias * alias = dynamic_cast<Valuable::AttributeAlias*>(obj);
        if(alias)
          obj = alias->attribute();

        Valuable::AttributeBool * b = dynamic_cast<Valuable::AttributeBool*>(obj);
        Valuable::AttributeStringList * strlst = dynamic_cast<Valuable::AttributeStringList*>(obj);
        if (b) {
          *b = true;
          m_parsedArgs.insert(name);
        } else if (strlst && i < argc - 1) {
          QString arg = argv[++i];
          QStringList lst = arg.split(";", QString::SkipEmptyParts);
          for (int i = 0; i < lst.size()-1; ) {
            if (lst[i].endsWith('\\')) {
              lst[i][lst[i].size()-1] = ';';
              lst[i] += lst[i+1];
              lst.removeAt(i+1);
            } else ++i;
          }
          *strlst = lst;
        } else if (i < argc - 1) {
          Valuable::DOMElement e = tmpDoc->createElement("tmp");
          e.setTextContent(argv[++i]);
          obj->deserializeXML(e);
          m_parsedArgs.insert(name);
        } else {
          list.push_back(arg);
          Radiant::error("Command line parameter %s is missing an argument", name.toUtf8().data());
        }
      } else {
        if(name.length() > 3 && name.startsWith("no-")) {
          Valuable::AttributeBool * b = dynamic_cast<Valuable::AttributeBool*>(
              opts.attribute(name.mid(3).toUtf8()));
          if(b) {
            *b = false;
            m_parsedArgs.insert(name);
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
