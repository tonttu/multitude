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

#include "Serializer.hpp"

#include <QRegExp>

namespace Valuable
{
  namespace Serializer
  {
    QString tagName(const std::type_info & typeinfo)
    {
      QString demangled = Radiant::StringUtils::demangle(typeinfo.name());
      demangled.replace(" ", "");
      demangled.replace(QRegExp("<[^>]*>"), "");
      demangled.replace(QRegExp("^.*::"), "");
      QRegExp az("[a-zA-Z:_][a-zA-Z0-9_:.-]*");
      if(az.indexIn(demangled) >= 0) return az.cap(0);
      return "value";
    }
  }
}
