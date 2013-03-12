/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_VALUE_BOOL_HPP
#define VALUABLE_VALUE_BOOL_HPP

#include "AttributeObject.hpp"

namespace Valuable
{

  /// A value object for boolean.
  class VALUABLE_API AttributeBool : public AttributeT<bool>
  {
  public:
    using AttributeT<bool>::operator =;

    /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
    /// @param value The value of this object
    AttributeBool(Node * host, const QByteArray &name, bool value, bool transit = false);
    virtual ~AttributeBool();

    /// @cond
    virtual void processMessage(const QByteArray &, Radiant::BinaryData & data) OVERRIDE;
    /// @endcond

    /// Boolean values can be set as integers in CSS files
    virtual bool set(int v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE;
    virtual bool set(const StyleValue & v, Layer layer = USER) OVERRIDE;

    virtual int asInt(bool * const ok) const OVERRIDE;
    virtual QString asString(bool * const ok = 0) const OVERRIDE;
  };

}

#endif // VALUABLE_VALUE_BOOL_HPP
