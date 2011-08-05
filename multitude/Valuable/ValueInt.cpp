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

#include "ValueInt.hpp"
#include "ValueIntImpl.hpp"

namespace Valuable
{


  template <>
  void AttributeIntT<int32_t>::processMessage(const char *, Radiant::BinaryData & data)
  {
    bool ok = true;
    int32_t v = data.readInt32( & ok);
    
    if(ok)
      *this = v;
  }
  
  template <>
  void AttributeIntT<uint32_t>::processMessage(const char *, Radiant::BinaryData & data)
  {
    bool ok = true;
    uint32_t v = uint32_t(data.readInt32( & ok));
    
    if(ok)
      *this = v;
  }

  template <>
  void AttributeIntT<int64_t>::processMessage(const char *, Radiant::BinaryData & data)
  {
    bool ok = true;
    int64_t v = data.readInt64( & ok);
    
    if(ok)
      *this = v;
  }
  
  template <>
  void AttributeIntT<uint64_t>::processMessage(const char *, Radiant::BinaryData & data)
  {
    bool ok = true;
    uint64_t v = uint64_t(data.readInt64( & ok));
    
    if(ok)
      *this = v;
  }

  template <>
  void AttributeIntT<Radiant::TimeStamp>::processMessage(const char *, Radiant::BinaryData & data)
  {
    bool ok = true;
    Radiant::TimeStamp v = data.readTimeStamp( & ok);
    
    if(ok)
      *this = v;
  }

  template class AttributeIntT<int32_t>;
  template class AttributeIntT<uint32_t>;
  template class AttributeIntT<int64_t>;
  template class AttributeIntT<uint64_t>;
  
  template class AttributeIntT<Radiant::TimeStamp>;

}
