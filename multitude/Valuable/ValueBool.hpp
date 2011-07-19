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

#ifndef VALUABLE_VALUE_BOOL_HPP
#define VALUABLE_VALUE_BOOL_HPP

#include "ValueObject.hpp"

#define VALUEMIT_STD_OP emitChange(); return *this;

namespace Valuable
{

  /// A value object for boolean.
  class VALUABLE_API ValueBool : public ValueObjectT<bool>
  {
  public:    
    /// @copydoc ValueObject::ValueObject(HasValues *, const std::string &, bool transit)
    /// @param value The value of this object
    ValueBool(HasValues * host, const std::string & name, bool value, bool transit = false);
    virtual ~ValueBool();

    const char * type() const { return "bool"; }
    bool deserialize(ArchiveElement & element);

    /// @cond
    virtual void processMessage(const char *, Radiant::BinaryData & data);
    /// @endcond

    /// Copies a value
    ValueBool & operator = (bool v) { m_value = v; VALUEMIT_STD_OP }

    /// Boolean values can be set as integers in CSS files
    bool set(int v);

    std::string asString(bool * const ok = 0) const;
  };

}

#undef VALUEMIT_STD_OP

#endif // VALUABLE_VALUE_BOOL_HPP
