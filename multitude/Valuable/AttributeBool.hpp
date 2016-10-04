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
  template <>
  class VALUABLE_API AttributeT<bool> : public AttributeBaseT<bool>
  {
  public:
    using AttributeBaseT<bool>::operator =;

    /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
    /// @param value The value of this object
    AttributeT(Node * host = nullptr, const QByteArray &name = QByteArray(), bool value = false);
    virtual ~AttributeT();

    /// @cond
    virtual void eventProcess(const QByteArray &, Radiant::BinaryData & data) OVERRIDE;
    /// @endcond

    /// Boolean values can be set as integers in CSS files
    virtual bool set(int v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE;
    virtual bool set(float v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE;
    virtual bool set(const StyleValue & v, Layer layer = USER) OVERRIDE;

    virtual float asFloat(bool * const ok = 0, Layer layer = CURRENT_VALUE) const OVERRIDE;
    virtual int asInt(bool * const ok, Layer layer) const OVERRIDE;
    virtual QString asString(bool * const ok, Layer layer) const OVERRIDE;

    static inline bool interpolate(bool a, bool b, float m)
    {
      return m >= 0.5f ? b : a;
    }
  };
  typedef AttributeT<bool> AttributeBool;
}

#endif // VALUABLE_VALUE_BOOL_HPP
