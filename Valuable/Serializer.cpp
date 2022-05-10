/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Serializer.hpp"

#include <QRegExp>

namespace Valuable
{
  namespace Serializer
  {
    QString tagName(const std::type_info & typeinfo)
    {
      QString demangled = Radiant::StringUtils::demangle(typeinfo);
      QString tmp;
      QRegExp tagr("<[^<>]*>");
      /// Need to iterate, since nested template parameters need multiple replaces
      do {
        tmp = demangled;
        demangled.replace(tagr, "");
      } while (tmp != demangled);

      demangled.replace(" ", "");
      demangled.replace(QRegExp("^.*::"), "");
      QRegExp az("[a-zA-Z:_][a-zA-Z0-9_:.-]*");
      if(az.indexIn(demangled) >= 0) return az.cap(0);
      return "value";
    }
  }
}
