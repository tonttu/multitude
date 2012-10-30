#ifndef ATTRIBUTETIMESTAMP_HPP
#define ATTRIBUTETIMESTAMP_HPP

#include "Export.hpp"
#include "AttributeObject.hpp"

#include <Radiant/TimeStamp.hpp>

#define VO_TYPE_TIMESTAMP "timestamp"

namespace Valuable
{

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

    AttributeTimeStamp(Node * host, const QByteArray & name, Radiant::TimeStamp v, bool transit = false)
        : Base(host, name, v, transit)
    {}

    virtual QString asString(bool * const ok) const OVERRIDE
    {
      if(ok) *ok = true;

      const Radiant::TimeStamp & ts = value();
      return QString::number(ts.value());
    }

    virtual const char * type() const OVERRIDE { return VO_TYPE_TIMESTAMP; }

  };

}

#endif // ATTRIBUTETIMESTAMP_HPP
