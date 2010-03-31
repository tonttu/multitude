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
  void ValueIntT<int32_t>::processMessage(const char *, Radiant::BinaryData & data)
  {
    bool ok = true;
    int32_t v = data.readInt32( & ok);
    
    if(ok)
      *this = v;
  }
  
  template <>
  void ValueIntT<uint32_t>::processMessage(const char *, Radiant::BinaryData & data)
  {
    bool ok = true;
    uint32_t v = uint32_t(data.readInt32( & ok));
    
    if(ok)
      *this = v;
  }

  template <>
  void ValueIntT<int64_t>::processMessage(const char *, Radiant::BinaryData & data)
  {
    bool ok = true;
    int64_t v = data.readInt64( & ok);
    
    if(ok)
      *this = v;
  }
  
  template <>
  void ValueIntT<uint64_t>::processMessage(const char *, Radiant::BinaryData & data)
  {
    bool ok = true;
    uint64_t v = uint64_t(data.readInt64( & ok));
    
    if(ok)
      *this = v;
  }

  template <>
  void ValueIntT<Radiant::TimeStamp>::processMessage(const char *, Radiant::BinaryData & data)
  {
    bool ok = true;
    Radiant::TimeStamp v = data.readTimeStamp( & ok);
    
    if(ok)
      *this = v;
  }

  template class ValueIntT<int32_t>;
  template class ValueIntT<uint32_t>;
  template class ValueIntT<int64_t>;
  template class ValueIntT<uint64_t>;
  
  template class ValueIntT<Radiant::TimeStamp>;

}
