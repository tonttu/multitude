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

#ifndef VALUABLE_VALUE_NUMERIC_HPP
#define VALUABLE_VALUE_NUMERIC_HPP

#include <Radiant/StringUtils.hpp>

#include <Valuable/Export.hpp>
#include <Valuable/ValueObject.hpp>

namespace Valuable
{
  /// A template base class for numeric values.
  /** The actual value classes are inherited from this template
      class. */
  template<class T>
  class AttributeNumeric : public AttributeT<T>
  {
    typedef AttributeT<T> Base;

  public:
      using Base::value;

      AttributeNumeric() : AttributeT<T>() {}
      /// @copydoc Attribute::Attribute(HasValues *, const QString &, bool transit)
      /// @param v The numeric value of this object.
      AttributeNumeric(HasValues * host, const QString & name, T v, bool transit = false)
      : Base(host, name, v, transit)
      {}

      /// Converts the numeric value to float
      float asFloat(bool * const ok = 0) const { if(ok) *ok = true; return static_cast<float> (value()); }
      /// Converts the numeric value to integer
      int asInt(bool * const ok = 0) const { if(ok) *ok = true; return static_cast<int> (value()); }
      /// Converts the numeric value to string
      QString asString(bool * const ok = 0) const { if(ok) *ok = true; return Radiant::StringUtils::stringify(value()); }
  };

}

#endif
