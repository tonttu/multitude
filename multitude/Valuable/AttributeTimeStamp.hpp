/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef ATTRIBUTETIMESTAMP_HPP
#define ATTRIBUTETIMESTAMP_HPP

#include "Export.hpp"
#include "Attribute.hpp"

#include <Radiant/TimeStamp.hpp>

namespace Valuable
{

  /// This class provides a Radiant::TimeStamp attribute.
  template <>
  class AttributeT<Radiant::TimeStamp> : public AttributeBaseT<Radiant::TimeStamp>
  {
    typedef AttributeBaseT<Radiant::TimeStamp> Base;
  public:

    using Base::value;
    using Base::operator =;

    AttributeT() : Base() {}

    AttributeT(Node * host, const QByteArray & name, Radiant::TimeStamp v = Radiant::TimeStamp())
        : Base(host, name, v)
    {}

    virtual QString asString(bool * const ok, Layer layer) const OVERRIDE
    {
      if(ok) *ok = true;

      const Radiant::TimeStamp & ts = value(layer);
      return QString::number(ts.value());
    }

    static inline Radiant::TimeStamp interpolate(Radiant::TimeStamp a, Radiant::TimeStamp b, float m)
    {
      return Radiant::TimeStamp(Nimble::Math::Roundf(a.value() * (1.0f - m) + b.value() * m));
    }

    virtual void eventProcess(const QByteArray & /*id*/, Radiant::BinaryData & data) OVERRIDE
    {
      bool ok = true;
      auto ts = data.readTimeStamp(&ok);
      if (ok)
        *this = ts;
    }

  };
  typedef AttributeT<Radiant::TimeStamp> AttributeTimeStamp;
}

#endif // ATTRIBUTETIMESTAMP_HPP
