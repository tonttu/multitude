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

#include "ValueFloatImpl.hpp"

namespace Valuable
{

  template <>
  void ValueFloatT<float>::processMessage(const char *, Radiant::BinaryData & data)
  {
    bool ok = true;
    float v = data.readFloat32( & ok);
    
    if(ok)
      *this = v;
  }

  template <>
  void ValueFloatT<double>::processMessage(const char *, Radiant::BinaryData & data)
  {
    bool ok = true;
    double v = data.readFloat64( & ok);
    
    if(ok)
      *this = v;
  }

  template class ValueFloatT<float>;

}
