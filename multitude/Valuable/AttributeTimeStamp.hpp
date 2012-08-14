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

    AttributeTimeStamp(Node * host, const QString & name, Radiant::TimeStamp v, bool transit = false)
        : Base(host, name, v, transit)
    {}

    virtual bool deserialize(const ArchiveElement & element) OVERRIDE
    {
      bool ok = false;

      int64_t tmp = static_cast<Radiant::TimeStamp::type>(element.get().toLongLong(&ok));

      *this = Radiant::TimeStamp(tmp);

      return ok;
    }

    virtual QString asString(bool * ok) const OVERRIDE
    {
      if(ok) *ok = true;

      const Radiant::TimeStamp & ts = value();
      return QString::number(ts.value());
    }

    virtual const char * type() const OVERRIDE { return VO_TYPE_TIMESTAMP; }

  };

}

#endif // ATTRIBUTETIMESTAMP_HPP
