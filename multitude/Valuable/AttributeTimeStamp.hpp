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
  class AttributeTimeStamp : public AttributeT<Radiant::TimeStamp>
  {
    typedef AttributeT<Radiant::TimeStamp> Base;
  public:

    using Base::value;
    using Base::m_current;
    using Base::m_values;
    using Base::m_valueSet;
    using Base::operator =;

    AttributeTimeStamp() : Base() {}

    AttributeTimeStamp(Node * host, const QByteArray & name, Radiant::TimeStamp v = Radiant::TimeStamp(), bool transit = false)
        : Base(host, name, v, transit)
    {}

    virtual QString asString(bool * const ok, Layer layer) const OVERRIDE
    {
      if(ok) *ok = true;

      const Radiant::TimeStamp & ts = value(layer);
      return QString::number(ts.value());
    }

    virtual void eventProcess(const QByteArray & /*id*/, Radiant::BinaryData & data) OVERRIDE
    {
      bool ok = true;
      auto ts = data.readTimeStamp(&ok);
      if (ok)
        *this = ts;
    }

  };

}

#endif // ATTRIBUTETIMESTAMP_HPP
