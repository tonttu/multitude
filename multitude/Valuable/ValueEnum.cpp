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

#include <Valuable/ValueEnum.hpp>

#include <assert.h>

#include <string.h>

namespace Valuable
{

  AttributeEnum::AttributeEnum(Valuable::Node * host, const char * name,
                       const char ** enumnames, int current)
  : AttributeIntT<int32_t>(host, name, current),
    m_enumnames(enumnames)
  {
    assert(enumnames != 0);
  }

  AttributeEnum::~AttributeEnum()
  {}


  void AttributeEnum::processMessage(const char * , Radiant::BinaryData & data)
  {
    QString str;
    data.readString(str);

    const char * name;

    for(int i = 0; (name = m_enumnames[i]) != 0; i++) {
      if(str == QString::fromUtf8(name)) {
        set(i);
      }
    }
  }
}
