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

#include "Attribute.hpp"

namespace Valuable
{

  /// A value object for boolean.
  class VALUABLE_API AttributeBool : public AttributeT<bool>
  {
  public:
    using AttributeT<bool>::operator =;

    /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
    /// @param value The value of this object
    AttributeBool(Node * host = nullptr, const QByteArray & name = QByteArray(), bool value = false, bool transit = false);
    virtual ~AttributeBool();

    /// @cond
    virtual void eventProcess(const QByteArray &, Radiant::BinaryData & data) OVERRIDE;
    /// @endcond

    /// Boolean values can be set as integers in CSS files
    virtual bool set(int v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE;
    virtual bool set(float v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE;
    virtual bool set(const StyleValue & v, Layer layer = USER) OVERRIDE;

    virtual float asFloat(bool * const ok = 0, Layer layer = LAYER_CURRENT) const OVERRIDE;
    virtual int asInt(bool * const ok, Layer layer) const OVERRIDE;
    virtual QString asString(bool * const ok, Layer layer) const OVERRIDE;
  };

}

#endif // VALUABLE_VALUE_BOOL_HPP
